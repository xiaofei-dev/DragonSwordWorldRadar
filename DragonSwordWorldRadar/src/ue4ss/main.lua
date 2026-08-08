local MOD = "[DragonSwordWorldRadar]"
local diagnostics = nil
local perf_diagnostics = nil
local diagnostics_ok, diagnostics_module = pcall(require, "diagnostics")
if diagnostics_ok and type(diagnostics_module) == "table" then
    diagnostics = diagnostics_module
end

local function log(message)
    if diagnostics ~= nil then
        diagnostics.info("MESSAGE", tostring(message), nil)
    else
        print(string.format("%s %s\n", MOD, tostring(message)))
    end
end

local GENERATION_KEY = "DragonSwordWorldRadar.Generation"
local previous_generation =
    tonumber(ModRef:GetSharedVariable(GENERATION_KEY)) or 0
local generation = previous_generation + 1
ModRef:SetSharedVariable(GENERATION_KEY, generation)

local function is_current_generation()
    local ok, current_generation = pcall(function()
        return ModRef:GetSharedVariable(GENERATION_KEY)
    end)
    return ok and tonumber(current_generation) == generation
end

local ok_config, config = pcall(require, "config")
if not ok_config then
    log("config.lua could not be loaded: " .. tostring(config))
    return
end

local ok_world_map, world_map = pcall(require, "world_map")
if not ok_world_map then
    log("world_map.lua could not be loaded: " .. tostring(world_map))
    return
end
local world_map_markers_enabled = config.world_map_markers ~= false
local show_height = config.show_height ~= false
local show_treasure_types = config.show_treasure_types ~= false
local text_scale = tonumber(config.text_scale) or 1.0
text_scale = math.max(0.5, math.min(2.0, text_scale))
local show_treasures = config.show_treasures ~= false
local show_bosses = config.show_bosses ~= false
local auto_start_overlay = config.auto_start_overlay ~= false
local start_key = config.start_key or config.refresh_key or "F7"
local stop_key = config.stop_key or config.toggle_key or "F8"
if diagnostics ~= nil then
    diagnostics.configure({
        version = "0.4.0-dev9-performance1.8-singlebridge1",
        generation = generation,
        use_enabled = config.use_logging ~= false
            and config.diagnostic_logging ~= false,
        debug_enabled = config.debug_logging == true
            or config.diagnostic_verbose == true,
        interval_seconds = tonumber(config.diagnostic_perf_interval_seconds) or 5,
    })
    if diagnostics.is_debug_enabled ~= nil
        and diagnostics.is_debug_enabled()
    then
        perf_diagnostics = diagnostics
    end
end
local mod_directory = nil
local resolve_mod_directory
local overlay_start_requested = false
if world_map_markers_enabled then
    world_map.initialize(log, is_current_generation)
    -- Avoid LoadMap hooks: this game is more stable when world changes are
    -- detected through temporary Pawn loss and delayed widget rescanning.
    log("World-map safety mode: Pawn-loss detection enabled.")
    if diagnostics ~= nil then
        diagnostics.info("CONFIG", nil, {
            world_map_markers = world_map_markers_enabled,
            show_height = show_height,
            show_treasure_types = show_treasure_types,
            text_scale = text_scale,
            show_treasures = show_treasures,
            show_bosses = show_bosses,
            start_key = start_key,
            stop_key = stop_key,
        })
    end
end

local TOWN_RADAR_RADIUS = 12500.0
local FIELD_RADAR_RADIUS = 22500.0
local MINIMAP_SCALE_THRESHOLD = 2.7
local MINIMAP_SCALE_CHECK_INTERVAL_MS = 1000
-- Protocol-v2 Motion Bridge is the sole runtime IPC channel. Both producer
-- modes use a 24 ms sampling cadence, while a 250 ms control callback refreshes
-- Pawn/map context and minimap radius without serializing marker catalogs.
local WORLD_MAP_ACTIVE_INTERVAL_MS = 24
local MAP_LOAD_RESUME_DELAY_MS = 3000
local MOD_SWITCH_CHECK_INTERVAL_MS = 5000
local MINIMAP_UPDATE_INTERVAL_MS = 250
local FAST_MOTION_INTERVAL_MS = 24
local MOTION_PROTOCOL_VERSION = 2
local FAST_MOTION_RECORD_SIZE = 768
local FAST_MOTION_HEARTBEAT_MS = 1000
local MOTION_POSITION_EPSILON = 20.0
local MOTION_Z_EPSILON = 10.0
local WORLD_MAP_PAN_EPSILON = 0.10
local WORLD_MAP_PLAYER_MAP_EPSILON = 0.10
local WORLD_MAP_ZOOM_EPSILON_RATIO = 0.0001
local WORLD_MAP_ID = 100

local overlay_catalogs_delegated = false
local radar_radius = FIELD_RADAR_RADIUS
local enabled = false
local loop_started = false
local world_map_loop_started = false
local update_pending = false
local motion_path_a = nil
local motion_path_b = nil
local motion_sequence = 0
local last_written_motion = {
    initialized = false,
    is_enabled = false,
    mode = "disabled",
    show_height = show_height,
    show_treasure_types = show_treasure_types,
    show_treasures = show_treasures,
    show_bosses = show_bosses,
    text_scale = text_scale,
    player_x = 0.0,
    player_y = 0.0,
    player_z = nil,
    radius = FIELD_RADAR_RADIUS,
    map_state = nil,
}
local last_written_map_motion = {
    map_id = 0,
    dimensions = 0.0,
    ui_size = 0.0,
    left = 0.0,
    top = 0.0,
    zoom = 0.0,
    viewport_width = 0.0,
    viewport_height = 0.0,
    viewport_scale = 0.0,
    player_map_x = 0.0,
    player_map_y = 0.0,
}
local motion_heartbeat_elapsed_ms = FAST_MOTION_HEARTBEAT_MS
local motion_write_error_logged = false
local latest_motion_x = nil
local latest_motion_y = nil
local latest_motion_z = nil
local latest_motion_mode = "radar"
local latest_motion_radius = FIELD_RADAR_RADIUS
local latest_motion_map_state = nil
local latest_motion_sample = 0
local written_motion_sample = 0
local engine = nil
local player_controller = nil
local player_pawn = nil
local motion_loop_started = false
local motion_update_pending = false
local world_motion_update_pending = false
local minimap_layer = nil
local minimap_scale_elapsed_ms = MINIMAP_SCALE_CHECK_INTERVAL_MS
local minimap_mode = nil
local world_map_was_active = false
local last_world_left = nil
local last_world_top = nil
local last_world_zoom = nil
local map_resume_delay_remaining_ms = 0
local mod_switch_elapsed_ms = 0
local control_sequence = 0
local queued_update_started_ms = nil

local function ensure_layers_loaded()
    if not show_treasures and not show_bosses then
        return false
    end

    -- Both immutable marker catalogs are loaded directly by the external
    -- Overlay from data/generated. Lua now publishes only live UObject data
    -- through the compact motion/control bridge.
    if not overlay_catalogs_delegated then
        overlay_catalogs_delegated = true
        log("Treasure and world-boss catalogs delegated to Overlay; Static Bridge disabled.")
    end
    return true
end

resolve_mod_directory = function()
    if mod_directory ~= nil then
        return mod_directory
    end

    local source = debug.getinfo(1, "S").source
    if type(source) == "string" and string.sub(source, 1, 1) == "@" then
        local script_file = string.sub(source, 2)
        local scripts_directory = string.match(
            script_file,
            "^(.*)[/\\][^/\\]+$"
        )
        mod_directory = scripts_directory
            and string.match(scripts_directory, "^(.*)[/\\][^/\\]+$")
    end

    if mod_directory == nil then
        local ok, directories = pcall(IterateGameDirectories)
        local win64 = ok and directories
            and directories.Game
            and directories.Game.Binaries
            and directories.Game.Binaries.Win64
        local win64_path = win64 and win64.__absolute_path
        if type(win64_path) == "string" then
            mod_directory = win64_path .. "\\Mods\\DragonSwordWorldRadar"
        end
    end
    return mod_directory
end

local write_disabled_motion

local function file_exists(path)
    local file = io.open(path, "rb")
    if file == nil then
        return false
    end
    file:close()
    return true
end

local function is_mod_enabled_in_mods_file()
    local directory = resolve_mod_directory()
    if directory == nil then
        return true
    end

    local mods_path = directory .. "\\..\\mods.txt"
    local file = io.open(mods_path, "rb")
    if file == nil then
        return true
    end

    local content = file:read("*a") or ""
    file:close()

    for line in string.gmatch(content, "[^\r\n]+") do
        local value = string.match(
            line,
            "^%s*DragonSwordWorldRadar%s*:%s*([01])"
        )
        if value ~= nil then
            return value == "1"
        end
    end

    return true
end

local function disable_for_mod_switch()
    if enabled then
        enabled = false
        latest_motion_sample = 0
        written_motion_sample = 0
        latest_motion_map_state = nil
        world_map_was_active = false
        write_disabled_motion()
        log("World radar disabled because mods.txt is set to 0.")
    end
    overlay_start_requested = false
end

local function start_overlay()
    if overlay_start_requested or not auto_start_overlay then
        return
    end

    local directory = resolve_mod_directory()
    if directory == nil then
        log("Could not resolve the DragonSwordWorldRadar directory; overlay request was not written.")
        return
    end

    local runtime_directory = directory .. "\\runtime"
    local request_path = runtime_directory .. "\\launch.request"
    local request = io.open(request_path, "wb")
    if request == nil then
        log("Could not write overlay launch request: " .. request_path)
        return
    end
    request:write(tostring(os.time()), "\n")
    request:close()
    -- The durable request is now available to an existing watcher. Mark the
    -- launch as requested only after this point so a transient path/write
    -- failure can be retried by the next activation call.
    overlay_start_requested = true

    local watcher_path = directory
        .. "\\host\\DragonSwordWorldRadar.Watcher.vbs"
    local command = 'cmd.exe /c start "" /b wscript.exe //B //NoLogo "'
        .. watcher_path
        .. '" "'
        .. directory
        .. '"'
    local call_ok, execute_result, execute_kind, execute_code =
        pcall(os.execute, command)
    local launch_ok = call_ok
    if launch_ok then
        if execute_result == nil or execute_result == false then
            launch_ok = false
        elseif type(execute_result) == "number"
            and execute_result ~= 0
        then
            launch_ok = false
        elseif type(execute_code) == "number"
            and execute_code ~= 0
        then
            launch_ok = false
        end
    end
    if not launch_ok then
        -- Keep the durable request, but allow F7/the next activation to retry
        -- launching the watcher. Its mutex makes duplicate successful starts
        -- harmless when the shell result is ambiguous.
        overlay_start_requested = false
        log("Could not launch the per-session overlay watcher: result="
            .. tostring(execute_result)
            .. "; kind=" .. tostring(execute_kind)
            .. "; code=" .. tostring(execute_code))
        return
    end
    log("Per-session overlay watcher launched: " .. request_path)
end

local function resolve_motion_paths()
    if motion_path_a ~= nil and motion_path_b ~= nil then
        return motion_path_a, motion_path_b
    end

    local directory = resolve_mod_directory()
    if directory ~= nil then
        motion_path_a = directory .. "\\runtime\\bridge\\radar_motion_a.dat"
        motion_path_b = directory .. "\\runtime\\bridge\\radar_motion_b.dat"
        log("Fast motion bridge files: "
            .. motion_path_a .. " / " .. motion_path_b)
    end
    return motion_path_a, motion_path_b
end

local function reset_bridge_files()
    local motion_a, motion_b = resolve_motion_paths()
    local directory = resolve_mod_directory()
    local bridge_directory = directory ~= nil
        and (directory .. "\\runtime\\bridge") or nil
    local paths = {
        motion_a,
        motion_b,
        -- Upgrade cleanup only. 1.8 never reads or writes these legacy
        -- Static Bridge files.
        bridge_directory ~= nil
            and (bridge_directory .. "\\radar_state_a.json") or nil,
        bridge_directory ~= nil
            and (bridge_directory .. "\\radar_state_b.json") or nil,
        bridge_directory ~= nil
            and (bridge_directory .. "\\radar_state.json") or nil,
    }
    for _, path in ipairs(paths) do
        if path ~= nil then
            pcall(os.remove, path)
        end
    end
    control_sequence = 0
    motion_sequence = 0
    last_written_motion.initialized = false
    last_written_motion.map_state = nil
    motion_heartbeat_elapsed_ms = FAST_MOTION_HEARTBEAT_MS
end

local function absolute_difference(left, right)
    return math.abs((tonumber(left) or 0.0) - (tonumber(right) or 0.0))
end

local function zoom_changed(left, right)
    left = tonumber(left) or 0.0
    right = tonumber(right) or 0.0
    local scale = math.max(math.abs(left), math.abs(right), 1.0)
    return math.abs(left - right)
        > scale * WORLD_MAP_ZOOM_EPSILON_RATIO
end

local function map_motion_changed(previous, current)
    local had_map = previous ~= nil
    local has_map = current ~= nil
    if had_map ~= has_map then
        return true
    end
    if not has_map then
        return false
    end

    return (tonumber(previous.map_id) or 0)
            ~= (tonumber(current.map_id) or 0)
        or absolute_difference(previous.dimensions, current.dimensions) > 0.001
        or absolute_difference(previous.ui_size, current.ui_size) > 0.001
        or absolute_difference(previous.left, current.left)
            > WORLD_MAP_PAN_EPSILON
        or absolute_difference(previous.top, current.top)
            > WORLD_MAP_PAN_EPSILON
        or zoom_changed(previous.zoom, current.zoom)
        or absolute_difference(
            previous.viewport_width,
            current.viewport_width
        ) > 0.001
        or absolute_difference(
            previous.viewport_height,
            current.viewport_height
        ) > 0.001
        or zoom_changed(
            previous.viewport_scale,
            current.viewport_scale
        )
        or absolute_difference(
            previous.player_map_x,
            current.player_map_x
        ) > WORLD_MAP_PLAYER_MAP_EPSILON
        or absolute_difference(
            previous.player_map_y,
            current.player_map_y
        ) > WORLD_MAP_PLAYER_MAP_EPSILON
end

local function motion_has_visual_change(
    previous,
    is_enabled,
    mode,
    player_x,
    player_y,
    player_z,
    radius,
    map_state
)
    if previous == nil or previous.initialized ~= true then
        return true
    end
    if previous.is_enabled ~= (is_enabled == true)
        or previous.mode ~= tostring(mode or "radar")
        or previous.show_height ~= show_height
        or previous.show_treasure_types ~= show_treasure_types
        or previous.show_treasures ~= show_treasures
        or previous.show_bosses ~= show_bosses
        or absolute_difference(previous.text_scale, text_scale) > 0.001
        or absolute_difference(previous.radius, radius) > 0.001
        or (previous.player_z ~= nil) ~= (player_z ~= nil)
    then
        return true
    end

    local delta_x = (tonumber(player_x) or 0.0) - previous.player_x
    local delta_y = (tonumber(player_y) or 0.0) - previous.player_y
    if delta_x * delta_x + delta_y * delta_y
        > MOTION_POSITION_EPSILON * MOTION_POSITION_EPSILON
    then
        return true
    end
    if player_z ~= nil
        and absolute_difference(previous.player_z, player_z)
            > MOTION_Z_EPSILON
    then
        return true
    end
    return map_motion_changed(previous.map_state, map_state)
end

local function copy_map_motion(map_state)
    if map_state == nil then
        return nil
    end

    local destination = last_written_map_motion
    destination.map_id = tonumber(map_state.map_id) or 0
    destination.dimensions = tonumber(map_state.dimensions) or 0.0
    destination.ui_size = tonumber(map_state.ui_size) or 0.0
    destination.left = tonumber(map_state.left) or 0.0
    destination.top = tonumber(map_state.top) or 0.0
    destination.zoom = tonumber(map_state.zoom) or 0.0
    destination.viewport_width =
        tonumber(map_state.viewport_width) or 0.0
    destination.viewport_height =
        tonumber(map_state.viewport_height) or 0.0
    destination.viewport_scale =
        tonumber(map_state.viewport_scale) or 0.0
    destination.player_map_x =
        tonumber(map_state.player_map_x) or 0.0
    destination.player_map_y =
        tonumber(map_state.player_map_y) or 0.0
    return destination
end

local function remember_written_motion(
    is_enabled,
    mode,
    player_x,
    player_y,
    player_z,
    radius,
    map_state
)
    -- Mutate one retained snapshot rather than allocating a new table on every
    -- movement frame. At 24 ms this removes roughly forty short-lived tables
    -- per second in radar mode and two tables per frame in world-map mode.
    last_written_motion.initialized = true
    last_written_motion.is_enabled = is_enabled == true
    last_written_motion.mode = tostring(mode or "radar")
    last_written_motion.show_height = show_height
    last_written_motion.show_treasure_types = show_treasure_types
    last_written_motion.show_treasures = show_treasures
    last_written_motion.show_bosses = show_bosses
    last_written_motion.text_scale = text_scale
    last_written_motion.player_x = tonumber(player_x) or 0.0
    last_written_motion.player_y = tonumber(player_y) or 0.0
    last_written_motion.player_z = player_z ~= nil
        and tonumber(player_z) or nil
    last_written_motion.radius = tonumber(radius) or radar_radius
    last_written_motion.map_state = copy_map_motion(map_state)
    motion_heartbeat_elapsed_ms = 0
end

local function write_fast_motion(
    is_enabled,
    mode,
    player_x,
    player_y,
    player_z,
    radius,
    map_state,
    force
)
    local diagnostic_start = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    local heartbeat_due = last_written_motion.initialized ~= true
        or motion_heartbeat_elapsed_ms >= FAST_MOTION_HEARTBEAT_MS
    local visual_change = motion_has_visual_change(
        last_written_motion,
        is_enabled,
        mode,
        player_x,
        player_y,
        player_z,
        radius,
        map_state
    )
    if not force and not visual_change and not heartbeat_due then
        if perf_diagnostics ~= nil then
            perf_diagnostics.record_motion_skip()
        end
        return true
    end

    local path_a, path_b = resolve_motion_paths()
    if path_a == nil or path_b == nil then
        if perf_diagnostics ~= nil then
            perf_diagnostics.record_motion_write(
                perf_diagnostics.now_ms() - diagnostic_start,
                false
            )
        end
        return false
    end

    local body
    if map_state == nil then
        -- Protocol v2 carries all session-static display controls in each
        -- compact frame, followed by eleven zeroed world-map fields. This is
        -- the sole runtime IPC record; no JSON Static Bridge is required.
        body = string.format(
            "%d|%d|%s|%s|%s|%s|%s|%s|%.3f|%.6f|%.6f|%.6f|%s|%.6f|0|0|0|0|0|0|0|0|0|0|0",
            MOTION_PROTOCOL_VERSION,
            generation,
            is_enabled and "1" or "0",
            mode or "radar",
            show_height and "1" or "0",
            show_treasure_types and "1" or "0",
            show_treasures and "1" or "0",
            show_bosses and "1" or "0",
            text_scale,
            player_x or 0,
            player_y or 0,
            player_z or 0,
            player_z ~= nil and "1" or "0",
            radius or radar_radius
        )
    else
        body = string.format(
            "%d|%d|%s|%s|%s|%s|%s|%s|%.3f|%.6f|%.6f|%.6f|%s|%.6f|%d|%.6f|%.6f|%.6f|%.6f|%.9f|%.6f|%.6f|%.9f|%.6f|%.6f",
            MOTION_PROTOCOL_VERSION,
            generation,
            is_enabled and "1" or "0",
            mode or "world",
            show_height and "1" or "0",
            show_treasure_types and "1" or "0",
            show_treasures and "1" or "0",
            show_bosses and "1" or "0",
            text_scale,
            player_x or 0,
            player_y or 0,
            player_z or 0,
            player_z ~= nil and "1" or "0",
            radius or radar_radius,
            tonumber(map_state.map_id) or 0,
            tonumber(map_state.dimensions) or 0,
            tonumber(map_state.ui_size) or 0,
            tonumber(map_state.left) or 0,
            tonumber(map_state.top) or 0,
            tonumber(map_state.zoom) or 0,
            tonumber(map_state.viewport_width) or 0,
            tonumber(map_state.viewport_height) or 0,
            tonumber(map_state.viewport_scale) or 0,
            tonumber(map_state.player_map_x) or 0,
            tonumber(map_state.player_map_y) or 0
        )
    end

    motion_sequence = motion_sequence + 1
    local sequence_text = tostring(motion_sequence)
    local payload = sequence_text .. "|" .. body .. "|" .. sequence_text .. "\n"
    if #payload > FAST_MOTION_RECORD_SIZE then
        if not motion_write_error_logged then
            log("Fast motion record exceeded its fixed size.")
            motion_write_error_logged = true
        end
        if perf_diagnostics ~= nil then
            perf_diagnostics.record_motion_write(
                perf_diagnostics.now_ms() - diagnostic_start,
                false
            )
        end
        return false
    end

    local path = motion_sequence % 2 == 0 and path_a or path_b
    local file, open_error = io.open(path, "w")
    if file == nil then
        if not motion_write_error_logged then
            log("Could not open fast motion bridge: " .. tostring(open_error))
            motion_write_error_logged = true
        end
        if perf_diagnostics ~= nil then
            perf_diagnostics.record_motion_write(
                perf_diagnostics.now_ms() - diagnostic_start,
                false
            )
        end
        return false
    end

    local ok, result = pcall(function()
        file:write(payload)
        file:close()
        return true
    end)
    if not ok or result ~= true then
        pcall(file.close, file)
        if not motion_write_error_logged then
            log("Could not write fast motion bridge: " .. tostring(result))
            motion_write_error_logged = true
        end
    else
        remember_written_motion(
            is_enabled,
            mode,
            player_x,
            player_y,
            player_z,
            radius,
            map_state
        )
        if motion_write_error_logged then
            log("Fast motion bridge recovered.")
            motion_write_error_logged = false
        end
    end
    if perf_diagnostics ~= nil then
        perf_diagnostics.record_motion_write(
            perf_diagnostics.now_ms() - diagnostic_start,
            ok and result == true
        )
    end
    return ok and result == true
end

local function flush_latest_motion()
    if latest_motion_sample == written_motion_sample then
        return
    end
    local sample = latest_motion_sample
    local written = write_fast_motion(
        true,
        latest_motion_mode,
        latest_motion_x,
        latest_motion_y,
        latest_motion_z,
        latest_motion_radius,
        latest_motion_map_state,
        false
    )
    -- A visual-delta suppression is reported as success, while a real bridge
    -- failure keeps the sample pending so the next loop retries it instead of
    -- silently dropping the newest player/map transform.
    if written then
        written_motion_sample = sample
    end
end

local function publish_motion_sample(
    mode,
    player_x,
    player_y,
    player_z,
    map_state
)
    latest_motion_x = player_x
    latest_motion_y = player_y
    latest_motion_z = player_z
    latest_motion_mode = mode
    latest_motion_radius = radar_radius
    latest_motion_map_state = map_state
    latest_motion_sample = latest_motion_sample + 1
    if perf_diagnostics ~= nil then
        perf_diagnostics.record_motion_sample()
    end
end

write_disabled_motion = function()
    local written = write_fast_motion(
        false,
        "disabled",
        0,
        0,
        nil,
        radar_radius,
        nil,
        true
    )
    if perf_diagnostics ~= nil then
        perf_diagnostics.set_mode("disabled", control_sequence)
    end
    return written
end

local function reset_player_context()
    engine = nil
    player_controller = nil
    player_pawn = nil
end

local function resolve_player_controller()
    if engine == nil then
        engine = FindFirstOf("Engine")
    end
    if engine == nil then
        return nil
    end

    local viewport = engine.GameViewport
    if viewport == nil then
        return nil
    end
    local game_instance = viewport.GameInstance
    if game_instance == nil then
        return nil
    end
    local local_players = game_instance.LocalPlayers
    if local_players == nil then
        return nil
    end
    local local_player = local_players[1]
    if local_player == nil then
        return nil
    end

    player_controller = local_player.PlayerController
    return player_controller
end

local function get_player_location(force_context_refresh)
    -- Stable 1.2 traversed Engine -> Viewport -> GameInstance -> LocalPlayer ->
    -- PlayerController -> Pawn on every 24 ms sample. Refresh that complete
    -- chain in the 250 ms control callback and reuse the current Pawn directly
    -- in the motion callback. A destroyed/stale wrapper is caught by pcall;
    -- the next 24 ms sample performs a complete recovery traversal.
    local ok, player_x, player_y, player_z = pcall(function()
        local current_pawn = player_pawn
        if force_context_refresh == true or current_pawn == nil then
            local controller = resolve_player_controller()
            if controller == nil then
                player_pawn = nil
                return nil, nil, nil
            end
            current_pawn = controller.Pawn
            player_pawn = current_pawn
        end
        if current_pawn == nil then
            player_pawn = nil
            return nil, nil, nil
        end

        local location = current_pawn:K2_GetActorLocation()
        if location == nil then
            player_pawn = nil
            return nil, nil, nil
        end

        return tonumber(location.X),
            tonumber(location.Y),
            tonumber(location.Z)
    end)
    if not ok then
        reset_player_context()
        return nil, nil, nil
    end
    return player_x, player_y, player_z
end

local function is_valid_object(object)
    if object == nil then
        return false
    end
    local ok, valid = pcall(function()
        return object:IsValid()
    end)
    return ok and valid == true
end

local function read_minimap_scale()
    if not is_valid_object(minimap_layer) then
        minimap_layer = FindFirstOf("DLayerMiniMap")
    end
    if not is_valid_object(minimap_layer) then
        minimap_layer = nil
        return nil
    end

    local ok, scale = pcall(function()
        local layer_map = minimap_layer.LayerMap
        if not is_valid_object(layer_map) then
            return nil
        end

        local map_overlay = layer_map.MapOverlay
        if not is_valid_object(map_overlay) then
            return nil
        end

        return tonumber(map_overlay.RenderTransform.Scale.X)
    end)
    if not ok or scale == nil then
        minimap_layer = nil
        return nil
    end
    return scale
end

local function update_radar_radius()
    local scale = read_minimap_scale()
    if scale == nil then
        return
    end

    local new_mode
    local new_radius
    if scale >= MINIMAP_SCALE_THRESHOLD then
        new_mode = "town"
        new_radius = TOWN_RADAR_RADIUS
    else
        new_mode = "field"
        new_radius = FIELD_RADAR_RADIUS
    end

    radar_radius = new_radius
    if minimap_mode ~= new_mode then
        minimap_mode = new_mode
        log(string.format(
            "Minimap scale %.3f detected; using %.0f m radar range.",
            scale,
            new_radius / 100.0
        ))
    end
end

local function update_radar_state(queue_delay_ms)
    -- This 250 ms callback now performs only low-frequency UObject/control
    -- sampling. It never builds JSON, serializes marker catalogs, or writes a
    -- second bridge. The 24 ms motion loops publish the latest values.
    local update_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    if not enabled or not ensure_layers_loaded() then
        return
    end

    control_sequence = control_sequence + 1
    local player_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    local player_x, player_y, player_z = get_player_location(true)
    local player_ms = perf_diagnostics ~= nil
        and (perf_diagnostics.now_ms() - player_started_ms) or 0.0
    if player_x == nil or player_y == nil then
        reset_player_context()
        minimap_layer = nil
        minimap_mode = nil
        world_map_was_active = false
        latest_motion_map_state = nil

        if world_map_markers_enabled then
            world_map.set_suspended(true)
            map_resume_delay_remaining_ms = MAP_LOAD_RESUME_DELAY_MS
        end

        local disabled_motion_written = write_disabled_motion()
        if perf_diagnostics ~= nil then
            perf_diagnostics.record_update({
                mode = "disabled",
                state_sequence = control_sequence,
                queue_delay_ms = queue_delay_ms or 0.0,
                player_ms = player_ms,
                world_ms = 0.0,
                build_ms = 0.0,
                write_ms = 0.0,
                total_ms = perf_diagnostics.now_ms() - update_started_ms,
                player_missing = true,
                failed = true,
                write_ok = disabled_motion_written,
            })
        end
        return
    end

    local map_state = nil
    local world_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    if world_map_markers_enabled then
        -- Entry is detected in this low-frequency control loop. Once active,
        -- the 24 ms world loop owns transform updates and this callback reuses
        -- its latest state instead of traversing the map widget twice.
        if world_map_was_active and latest_motion_map_state ~= nil then
            map_state = latest_motion_map_state
        else
            map_state = world_map.read_state()
        end
    end
    local world_ms = perf_diagnostics ~= nil
        and (perf_diagnostics.now_ms() - world_started_ms) or 0.0

    local mode
    if map_state ~= nil then
        world_map_was_active = true
        last_world_left = map_state.left or 0
        last_world_top = map_state.top or 0
        last_world_zoom = map_state.zoom or 0
        mode = "world"
        publish_motion_sample(
            mode,
            player_x,
            player_y,
            player_z,
            map_state
        )
    else
        world_map_was_active = false
        latest_motion_map_state = nil
        last_world_left = nil
        last_world_top = nil
        last_world_zoom = nil
        mode = "radar"

        minimap_scale_elapsed_ms =
            minimap_scale_elapsed_ms + MINIMAP_UPDATE_INTERVAL_MS
        if minimap_scale_elapsed_ms >= MINIMAP_SCALE_CHECK_INTERVAL_MS then
            minimap_scale_elapsed_ms = 0
            update_radar_radius()
        end

        publish_motion_sample(
            mode,
            player_x,
            player_y,
            player_z,
            nil
        )
    end

    if perf_diagnostics ~= nil then
        perf_diagnostics.record_update({
            mode = mode,
            state_sequence = control_sequence,
            queue_delay_ms = queue_delay_ms or 0.0,
            player_ms = player_ms,
            world_ms = world_ms,
            build_ms = 0.0,
            write_ms = 0.0,
            total_ms = perf_diagnostics.now_ms() - update_started_ms,
            write_ok = true,
            left = map_state ~= nil and map_state.left or nil,
            top = map_state ~= nil and map_state.top or nil,
            zoom = map_state ~= nil and map_state.zoom or nil,
        })
    end
end

local ensure_world_map_loop_started
local ensure_motion_loop_started

local function report_async_failure(
    key,
    event,
    fallback_prefix,
    failure,
    fields
)
    if diagnostics ~= nil then
        diagnostics.error_rate_limited(
            key,
            10,
            event,
            tostring(failure),
            fields or {}
        )
    else
        log(fallback_prefix .. tostring(failure))
    end
end

local function queue_world_motion_update()
    if not is_current_generation()
        or not enabled
        or not world_map_was_active
        or world_motion_update_pending
        or map_resume_delay_remaining_ms > 0
    then
        return
    end

    world_motion_update_pending = true
    local queue_ok, queue_error = pcall(function()
        ExecuteInGameThread(function()
            local callback_ok, callback_error = pcall(function()
                if is_current_generation()
                    and enabled
                    and world_map_was_active
                    and map_resume_delay_remaining_ms <= 0
                then
                    local player_x, player_y, player_z =
                        get_player_location()
                    local map_state = world_map.read_state()
                    if player_x ~= nil
                        and player_y ~= nil
                        and map_state ~= nil
                    then
                        last_world_left = map_state.left or 0
                        last_world_top = map_state.top or 0
                        last_world_zoom = map_state.zoom or 0
                        publish_motion_sample(
                            "world",
                            player_x,
                            player_y,
                            player_z,
                            map_state
                        )
                    elseif map_state == nil then
                        -- A closed map must stop the fullscreen producer
                        -- immediately. Do not retain a stale transform or wait
                        -- for missing samples.
                        world_map_was_active = false
                        latest_motion_map_state = nil
                        last_world_left = nil
                        last_world_top = nil
                        last_world_zoom = nil
                        if player_x ~= nil and player_y ~= nil then
                            publish_motion_sample(
                                "radar",
                                player_x,
                                player_y,
                                player_z,
                                nil
                            )
                        end
                        ensure_motion_loop_started()
                    end
                end
            end)
            -- Always release the gate. Otherwise one failed UObject read would
            -- permanently stop all later map samples for this session.
            world_motion_update_pending = false
            if not callback_ok then
                report_async_failure(
                    "world_motion_callback",
                    "WORLD_MOTION_UPDATE_FAILED",
                    "World-map motion update failed: ",
                    callback_error,
                    {
                        world_map_active = world_map_was_active,
                        state_sequence = control_sequence,
                    }
                )
            end
        end)
    end)
    if not queue_ok then
        world_motion_update_pending = false
        report_async_failure(
            "world_motion_queue",
            "WORLD_MOTION_QUEUE_FAILED",
            "Could not queue world-map motion update: ",
            queue_error,
            {
                world_map_active = world_map_was_active,
                state_sequence = control_sequence,
            }
        )
    end
end

local function queue_motion_update()
    if not is_current_generation()
        or not enabled
        or world_map_was_active
        or motion_update_pending
        or map_resume_delay_remaining_ms > 0
    then
        return
    end

    motion_update_pending = true
    local queue_ok, queue_error = pcall(function()
        ExecuteInGameThread(function()
            local callback_ok, callback_error = pcall(function()
                if is_current_generation()
                    and enabled
                    and not world_map_was_active
                    and map_resume_delay_remaining_ms <= 0
                then
                    local player_x, player_y, player_z =
                        get_player_location()
                    if player_x ~= nil and player_y ~= nil then
                        publish_motion_sample(
                            "radar",
                            player_x,
                            player_y,
                            player_z,
                            nil
                        )
                    end
                end
            end)
            -- Release the gate even when an engine read fails. The next 24 ms
            -- loop iteration can then recover without restarting the mod.
            motion_update_pending = false
            if not callback_ok then
                report_async_failure(
                    "radar_motion_callback",
                    "RADAR_MOTION_UPDATE_FAILED",
                    "Radar motion update failed: ",
                    callback_error,
                    {
                        world_map_active = world_map_was_active,
                        state_sequence = control_sequence,
                    }
                )
            end
        end)
    end)
    if not queue_ok then
        motion_update_pending = false
        report_async_failure(
            "radar_motion_queue",
            "RADAR_MOTION_QUEUE_FAILED",
            "Could not queue radar motion update: ",
            queue_error,
            {
                world_map_active = world_map_was_active,
                state_sequence = control_sequence,
            }
        )
    end
end

ensure_motion_loop_started = function()
    if motion_loop_started then
        return
    end
    motion_loop_started = true

    LoopAsync(FAST_MOTION_INTERVAL_MS, function()
        if not is_current_generation() then
            return true
        end
        if not enabled then
            motion_loop_started = false
            return true
        end
        if world_map_was_active then
            -- Stop the radar motion loop completely while the map is active.
            -- Idling it would still wake the UE4SS async scheduler unnecessarily.
            motion_loop_started = false
            return true
        end
        motion_heartbeat_elapsed_ms = math.min(
            FAST_MOTION_HEARTBEAT_MS,
            motion_heartbeat_elapsed_ms + FAST_MOTION_INTERVAL_MS
        )
        flush_latest_motion()
        queue_motion_update()
        return false
    end)
end

local function queue_radar_update()
    if perf_diagnostics ~= nil then
        perf_diagnostics.record_queue_request()
    end
    if not is_current_generation() or not enabled
        or map_resume_delay_remaining_ms > 0
    then
        if perf_diagnostics ~= nil then
            perf_diagnostics.record_queue_skip("generation_or_disabled")
        end
        return
    end
    if update_pending then
        if perf_diagnostics ~= nil then
            perf_diagnostics.record_queue_skip("pending")
        end
        return
    end

    update_pending = true
    queued_update_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or nil
    local queue_ok, queue_error = pcall(function()
        ExecuteInGameThread(function()
            local queue_delay_ms = perf_diagnostics ~= nil
                and queued_update_started_ms ~= nil
                and (perf_diagnostics.now_ms() - queued_update_started_ms)
                or 0.0
            local callback_ok, callback_error = pcall(function()
                if is_current_generation()
                    and map_resume_delay_remaining_ms <= 0
                then
                    local update_ok, update_error = pcall(
                        update_radar_state,
                        queue_delay_ms
                    )
                    if not update_ok then
                        report_async_failure(
                            "update_radar_state",
                            "UPDATE_FAILED",
                            "Radar state update failed: ",
                            update_error,
                            {
                                queue_delay_ms = queue_delay_ms,
                                world_map_active = world_map_was_active,
                                state_sequence = control_sequence,
                            }
                        )
                    end
                end

                if is_current_generation() and enabled then
                    if world_map_was_active then
                        ensure_world_map_loop_started()
                    else
                        -- Restart the fast radar bridge after the world map
                        -- closes.
                        ensure_motion_loop_started()
                    end
                end
            end)
            -- This cleanup is deliberately outside the protected body so an
            -- unexpected callback failure cannot permanently suppress updates.
            queued_update_started_ms = nil
            update_pending = false
            if not callback_ok then
                report_async_failure(
                    "radar_update_callback",
                    "UPDATE_CALLBACK_FAILED",
                    "Radar update callback failed: ",
                    callback_error,
                    {
                        queue_delay_ms = queue_delay_ms,
                        world_map_active = world_map_was_active,
                        state_sequence = control_sequence,
                    }
                )
            end
        end)
    end)
    if not queue_ok then
        queued_update_started_ms = nil
        update_pending = false
        report_async_failure(
            "radar_update_queue",
            "UPDATE_QUEUE_FAILED",
            "Could not queue radar state update: ",
            queue_error,
            {
                world_map_active = world_map_was_active,
                state_sequence = control_sequence,
            }
        )
    end
end

ensure_world_map_loop_started = function()
    if world_map_loop_started then
        return
    end
    world_map_loop_started = true

    LoopAsync(WORLD_MAP_ACTIVE_INTERVAL_MS, function()
        if not is_current_generation() then
            return true
        end
        if not enabled or not world_map_was_active then
            world_map_loop_started = false
            return true
        end

        -- Flush only the compact transform from the previous sample, then
        -- queue UObject reads for the next sample. No JSON is built here.
        motion_heartbeat_elapsed_ms = math.min(
            FAST_MOTION_HEARTBEAT_MS,
            motion_heartbeat_elapsed_ms + WORLD_MAP_ACTIVE_INTERVAL_MS
        )
        flush_latest_motion()
        queue_world_motion_update()
        return false
    end)
end

local function ensure_loop_started()
    if loop_started then
        return
    end
    loop_started = true

    LoopAsync(MINIMAP_UPDATE_INTERVAL_MS, function()
        if not is_current_generation() then
            return true
        end

        mod_switch_elapsed_ms =
            mod_switch_elapsed_ms + MINIMAP_UPDATE_INTERVAL_MS
        if mod_switch_elapsed_ms >= MOD_SWITCH_CHECK_INTERVAL_MS then
            mod_switch_elapsed_ms = 0
            if not is_mod_enabled_in_mods_file() then
                disable_for_mod_switch()
                loop_started = false
                return true
            end
        end

        if not enabled then
            loop_started = false
            return true
        end

        if map_resume_delay_remaining_ms > 0 then
            map_resume_delay_remaining_ms = math.max(
                0,
                map_resume_delay_remaining_ms
                    - MINIMAP_UPDATE_INTERVAL_MS
            )

            if map_resume_delay_remaining_ms == 0
                and world_map_markers_enabled
            then
                world_map.set_suspended(false)
            end

            return false
        end

        queue_radar_update()
        if world_map_was_active then
            ensure_world_map_loop_started()
        end
        return false
    end)
end

RegisterKeyBind(Key[start_key], function()
    if not is_current_generation() then
        return
    end
    if not is_mod_enabled_in_mods_file() then
        disable_for_mod_switch()
        log("F7 ignored because DragonSwordWorldRadar is 0 in mods.txt.")
        return
    end
    start_overlay()
    if not show_treasures and not show_bosses then
        log("All radar layers are disabled in config.lua.")
        return
    end
    if ensure_layers_loaded() then
        enabled = true
        ensure_loop_started()
        ensure_motion_loop_started()
        log("World radar enabled.")
        queue_motion_update()
        queue_radar_update()
    end
end)

RegisterKeyBind(Key[stop_key], function()
    if not is_current_generation() then
        return
    end
    enabled = false
    latest_motion_sample = 0
    written_motion_sample = 0
    latest_motion_map_state = nil
    world_map_was_active = false
    last_world_left = nil
    last_world_top = nil
    last_world_zoom = nil
    write_disabled_motion()
    log("World radar disabled; all producer loops will stop.")
end)

reset_bridge_files()
start_overlay()
write_disabled_motion()
log("Ready. F7 enables configured radar layers; F8 disables them. Protocol-v2 Motion Bridge is the sole runtime IPC channel. Low-frequency control sampling runs at 250 ms; radar motion is sampled at 24 ms without world-map probing, and world-map motion runs only while the map is open. Use log: runtime/logs/DragonSwordWorldRadar.Lua.Use.log. Debug log: runtime/logs/DragonSwordWorldRadar.Lua.Debug.log.")
