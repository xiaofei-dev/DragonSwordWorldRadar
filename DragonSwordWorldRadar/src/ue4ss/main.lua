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
local show_assaults = config.show_assaults ~= false
local show_moles = config.show_moles ~= false
local show_world_status = config.show_world_status ~= false
local mole_completion = nil
local mole_visible_mask = 0
local world_time_available = false
local world_time_seconds = 0
local weather_available = false
local weather_state = 0
local weather_bt_state = 0
local auto_start_overlay = config.auto_start_overlay ~= false
local start_key = config.start_key or config.refresh_key or "F7"
local stop_key = config.stop_key or config.toggle_key or "F8"
local no_paint_key = config.no_paint_key or "F5"
local no_motion_key = config.no_motion_key or "F6"
if diagnostics ~= nil then
    diagnostics.configure({
        version = "0.4.0-dev74-minigamecatalog2",
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
local world_environment = nil
if show_world_status then
    local environment_ok, environment_module = pcall(require, "world_environment")
    if environment_ok and type(environment_module) == "table" then
        local initialized, initialize_error = pcall(
            environment_module.initialize,
            log,
            perf_diagnostics ~= nil and function(event, fields)
                perf_diagnostics.debug(event, nil, fields or {})
            end or nil,
            { stable_context_ticks = 8 }
        )
        if initialized then
            world_environment = environment_module
        else
            show_world_status = false
            log("World status disabled: isolated clock initialization failed: "
                .. tostring(initialize_error))
        end
    else
        show_world_status = false
        log("World status disabled: isolated clock module could not be loaded.")
    end
end
if show_moles then
    local catalog_ok, mole_catalog = pcall(require, "mole_catalog")
    local completion_ok, completion_module = pcall(require, "mole_completion")
    if catalog_ok and completion_ok and type(completion_module) == "table" then
        local call_ok, initialized, failure = pcall(
            completion_module.initialize,
            mole_catalog,
            log,
            perf_diagnostics
        )
        if call_ok and initialized == true then
            mole_completion = completion_module
        else
            show_moles = false
            log("Mole/Fly layer disabled: "
                .. tostring(failure or initialized))
        end
    else
        show_moles = false
        log("Mole/Fly layer disabled because its catalog or completion module could not be loaded.")
    end
end
local mod_directory = nil
local resolve_mod_directory
local overlay_start_requested = false
local world_epoch = 1
local transition_active = true
local transition_reason = "startup"
local activation_requested = false
local ACTIVATION_STABLE_SAMPLE_COUNT = 4
local activation_stable_samples = 0
local activation_probe_pending = false
local activation_probe_request_token = 0
local activation_probe_pending_token = nil
local activation_probe_pending_epoch = nil
local resume_probe_pending = false
local world_map_resume_pending = world_map_markers_enabled
local enter_world_transition

local function is_world_epoch_current(epoch)
    return not transition_active
        and epoch == world_epoch
        and is_current_generation()
end

-- World-map property access already runs inside a generation-checked game-thread
-- callback. Keep its per-property guard Lua-only so large-map sampling does not
-- call ModRef:GetSharedVariable dozens of times per frame.
local function is_lifecycle_epoch_current(epoch)
    return not transition_active and epoch == world_epoch
end
if world_map_markers_enabled then
    world_map.initialize(
        log,
        is_current_generation,
        is_lifecycle_epoch_current,
        perf_diagnostics
    )
    log("World-transition safety mode: reference 1.6.1 Pawn-loss cooldown retry enabled; recovery performs no World-identity probe, LoadMap hook, or HUD-visibility scan.")
    if diagnostics ~= nil then
        diagnostics.info("CONFIG", nil, {
            world_map_markers = world_map_markers_enabled,
            show_height = show_height,
            show_treasure_types = show_treasure_types,
            text_scale = text_scale,
            show_treasures = show_treasures,
            show_bosses = show_bosses,
            show_assaults = show_assaults,
            show_moles = show_moles,
            show_world_status = show_world_status,
            start_key = start_key,
            stop_key = stop_key,
            no_paint_key = no_paint_key,
            no_motion_key = no_motion_key,
        })
    end
end

local TOWN_RADAR_RADIUS = 12500.0
local FIELD_RADAR_RADIUS = 22500.0
local MINIMAP_SCALE_THRESHOLD = 2.7
local MINIMAP_SCALE_CHECK_INTERVAL_MS = 1000
-- Protocol-v6 Motion Bridge is the sole runtime IPC channel. Player UObject
-- sampling occurs only in the 250 ms control callback; the 50 ms loops publish
-- or transform the latest scalar sample without another player-root traversal.
-- Both compact and world-map modes reuse that 250 ms numeric player sample.
-- Only the visible world map transforms that scalar at 8 ms; compact bridge
-- presentation remains 50 ms and neither loop performs another player read.
local WORLD_MAP_ACTIVE_INTERVAL_MS = 8
local MAP_LOAD_RESUME_DELAY_MS = 3000
local MOD_SWITCH_CHECK_INTERVAL_MS = 5000
local MINIMAP_UPDATE_INTERVAL_MS = 250
local WORLD_MAP_INACTIVE_CHECK_INTERVAL_MS = 2000
local world_map_inactive_elapsed_ms =
    WORLD_MAP_INACTIVE_CHECK_INTERVAL_MS
local FAST_MOTION_INTERVAL_MS = 50
local MOTION_PROTOCOL_VERSION = 6
local FAST_MOTION_RECORD_SIZE = 768
local FAST_MOTION_HEARTBEAT_MS = 1000
local CONTROL_WATCHDOG_SAMPLE_MS = 1000
local CONTROL_WATCHDOG_STALE_SAMPLES = 5
local MAX_AUTOMATIC_RUNTIME_RESTARTS = 3
local MOTION_POSITION_EPSILON_FLOOR = 20.0
local MOTION_SCREEN_PIXEL_EPSILON = 0.5
local REFERENCE_RADAR_RADIUS_PIXELS = 170.0
local MOTION_Z_EPSILON = 10.0
local WORLD_MAP_PAN_EPSILON = 0.10
local WORLD_MAP_PLAYER_MAP_EPSILON = 0.10
local WORLD_MAP_ZOOM_EPSILON_RATIO = 0.0001
local WORLD_MAP_ID = 100

local overlay_catalogs_delegated = false
local radar_radius = FIELD_RADAR_RADIUS
local enabled = false
local diagnostic_mode = "normal"
local loop_started = false
local world_map_loop_started = false
local control_tick_serial = 0
local control_watchdog_observed_serial = 0
local control_watchdog_elapsed_ms = 0
local control_watchdog_stale_samples = 0
local runtime_restart_requested = false
local runtime_restart_reason = nil
local runtime_restart_in_progress = false
local automatic_runtime_restart_count = 0
local motion_loop_token = 0
local world_map_loop_token = 0
local update_pending = false
local update_request_token = 0
local update_pending_token = nil
local motion_path_a = nil
local motion_path_b = nil
local motion_sequence = 0
local last_written_motion = {
    initialized = false,
    is_enabled = false,
    mode = "disabled",
    diagnostic_mode = "normal",
    show_height = show_height,
    show_treasure_types = show_treasure_types,
    show_treasures = show_treasures,
    show_bosses = show_bosses,
    show_moles = show_moles,
    mole_mask = 0,
    show_world_status = show_world_status,
    world_time_available = false,
    world_time_seconds = 0,
    weather_available = false,
    weather_state = 0,
    weather_bt_state = 0,
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
local latest_player_sample_timestamp_ms = 0.0
local written_motion_sample = 0
local engine = nil
local minimap_layer = nil
local motion_loop_started = false
local motion_update_pending = false
local world_motion_update_pending = false
local world_motion_request_token = 0
local world_motion_pending_token = nil
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
local update_pending_epoch = nil
local motion_pending_epoch = nil
local world_motion_pending_epoch = nil
local clock_capture_task_pending = false
local clock_capture_task_token = nil
local clock_context_samples = 0
local clock_context_samples_consumed = 0
local f7_trace_active = false
local f7_trace_remaining = 0
local f7_trace_id = 0
local f7_trace_deadline_ms = 0.0

-- Diagnostic-only, bounded breadcrumbs. These file-only scalar records make
-- the last completed native boundary durable if UE4SS terminates inside the
-- next UObject operation. They perform no additional UObject access.
local function f7_trace(stage, ...)
    if not f7_trace_active or perf_diagnostics == nil then return end
    if f7_trace_remaining <= 0
        or os.clock() * 1000.0 > f7_trace_deadline_ms
    then
        f7_trace_active = false
        return
    end
    f7_trace_remaining = f7_trace_remaining - 1
    local fields = {}
    local field_count = select("#", ...)
    for index = 1, field_count, 2 do
        local key = select(index, ...)
        if key ~= nil then
            fields[key] = select(index + 1, ...)
        end
    end
    fields.trace_id = f7_trace_id
    fields.stage = stage
    fields.epoch = world_epoch
    fields.control_sequence = control_sequence
    perf_diagnostics.debug("F7_CRASH_TRACE", nil, fields)
end

local function begin_f7_trace()
    if perf_diagnostics == nil then
        f7_trace_active = false
        return
    end
    f7_trace_id = f7_trace_id + 1
    f7_trace_remaining = 220
    f7_trace_deadline_ms = os.clock() * 1000.0 + 6000.0
    f7_trace_active = true
    f7_trace("F7_ENTER", "budget", f7_trace_remaining)
end

-- Height and treasure-type flags decorate treasure markers; they are not
-- independent layers and must not activate the 50 ms producer by themselves.
local function has_radar_marker_layers()
    return show_treasures or show_bosses or show_moles or show_assaults
end

local function has_configured_visible_features()
    return has_radar_marker_layers() or show_world_status
end

local function ensure_layers_loaded()
    if not show_treasures
        and not show_bosses
        and not show_assaults
        and not show_moles
        and not show_world_status
    then
        return false
    end

    -- Immutable treasure, Boss, Assault, and Mole marker catalogs are loaded
    -- directly by the external Overlay from data/generated. Lua publishes
    -- only live UObject data through the compact motion/control bridge.
    if not overlay_catalogs_delegated then
        overlay_catalogs_delegated = true
        log("Treasure, world-boss, Assault, Mole/Fly, and world-status rendering delegated to one Overlay; Static Bridge disabled.")
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
            mod_directory = win64_path .. "\\ue4ss\\Mods\\DragonSwordWorldRadar"
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
    runtime_restart_requested = false
    runtime_restart_reason = nil
    activation_requested = false
    enter_world_transition("mods_txt_disabled")
    log("DragonSwordWorldRadar disabled because mods.txt is set to 0.")
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

local function motion_position_epsilon(radius)
    -- Compact marker positions are projected into a 170-pixel radius. Motion
    -- below half a rendered pixel cannot create a meaningful visual change,
    -- so avoid serializing and repainting it. Keep the accepted 20-unit floor
    -- for malformed or unusually small radius values.
    local effective_radius = math.max(
        1.0,
        tonumber(radius) or radar_radius or FIELD_RADAR_RADIUS
    )
    return math.max(
        MOTION_POSITION_EPSILON_FLOOR,
        effective_radius
            / REFERENCE_RADAR_RADIUS_PIXELS
            * MOTION_SCREEN_PIXEL_EPSILON
    )
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
    current_diagnostic_mode,
    player_x,
    player_y,
    player_z,
    radius,
    map_state
)
    if previous == nil or previous.initialized ~= true then
        return true
    end
    local published_show_height = is_enabled == true and show_height
    local published_show_treasure_types = is_enabled == true and show_treasure_types
    local published_show_treasures = is_enabled == true and show_treasures
    local published_show_bosses = is_enabled == true and show_bosses
    local published_show_moles = is_enabled == true and show_moles
    local published_show_world_status = is_enabled == true and show_world_status
    local published_mole_mask = published_show_moles and mole_visible_mask or 0
    local status_visible = published_show_world_status
        and tostring(mode or "radar") == "radar"
    if previous.is_enabled ~= (is_enabled == true)
        or previous.mode ~= tostring(mode or "radar")
        or previous.diagnostic_mode
            ~= tostring(current_diagnostic_mode or "normal")
        or previous.show_height ~= published_show_height
        or previous.show_treasure_types ~= published_show_treasure_types
        or previous.show_treasures ~= published_show_treasures
        or previous.show_bosses ~= published_show_bosses
        or previous.show_moles ~= published_show_moles
        or previous.mole_mask ~= published_mole_mask
        or previous.show_world_status ~= published_show_world_status
        or (status_visible
            and (previous.world_time_available ~= world_time_available
                or math.floor(previous.world_time_seconds / 60)
                    ~= math.floor(world_time_seconds / 60)
                or previous.weather_available ~= weather_available
                or previous.weather_state ~= weather_state
                or previous.weather_bt_state ~= weather_bt_state))
        or absolute_difference(previous.text_scale, text_scale) > 0.001
        or absolute_difference(previous.radius, radius) > 0.001
        or (previous.player_z ~= nil) ~= (player_z ~= nil)
    then
        return true
    end

    local delta_x = (tonumber(player_x) or 0.0) - previous.player_x
    local delta_y = (tonumber(player_y) or 0.0) - previous.player_y
    local position_epsilon = motion_position_epsilon(radius)
    if delta_x * delta_x + delta_y * delta_y
        > position_epsilon * position_epsilon
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
    current_diagnostic_mode,
    player_x,
    player_y,
    player_z,
    radius,
    map_state
)
    -- Mutate one retained snapshot rather than allocating a new table on every
    -- movement frame. At 50 ms this removes roughly twenty short-lived tables
    -- per second in radar mode and two tables per frame in world-map mode.
    last_written_motion.initialized = true
    last_written_motion.is_enabled = is_enabled == true
    last_written_motion.mode = tostring(mode or "radar")
    last_written_motion.diagnostic_mode =
        tostring(current_diagnostic_mode or "normal")
    last_written_motion.show_height = is_enabled == true and show_height
    last_written_motion.show_treasure_types = is_enabled == true and show_treasure_types
    last_written_motion.show_treasures = is_enabled == true and show_treasures
    last_written_motion.show_bosses = is_enabled == true and show_bosses
    last_written_motion.show_moles = is_enabled == true and show_moles
    last_written_motion.mole_mask = is_enabled == true and show_moles and mole_visible_mask or 0
    last_written_motion.show_world_status = is_enabled == true and show_world_status
    last_written_motion.world_time_available = is_enabled == true and world_time_available
    last_written_motion.world_time_seconds = is_enabled == true and world_time_seconds or 0
    last_written_motion.weather_available = is_enabled == true and weather_available
    last_written_motion.weather_state = weather_state
    last_written_motion.weather_bt_state = weather_bt_state
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
    local published_diagnostic_mode = is_enabled == true
        and diagnostic_mode or "normal"
    local diagnostic_mode_code = published_diagnostic_mode == "no_paint" and 1
        or published_diagnostic_mode == "no_motion" and 2
        or 0
    local published_show_height = is_enabled == true and show_height
    local published_show_treasure_types = is_enabled == true and show_treasure_types
    local published_show_treasures = is_enabled == true and show_treasures
    local published_show_bosses = is_enabled == true and show_bosses
    local published_show_moles = is_enabled == true and show_moles
    local published_mole_mask = published_show_moles and mole_visible_mask or 0
    local published_show_world_status = is_enabled == true and show_world_status
    local published_world_time_available = published_show_world_status and world_time_available
    local diagnostic_start = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    local heartbeat_due = last_written_motion.initialized ~= true
        or motion_heartbeat_elapsed_ms >= FAST_MOTION_HEARTBEAT_MS
    local write_cause = force and "lifecycle"
        or (last_written_motion.mole_mask ~= mole_visible_mask and "mole")
        or ((last_written_motion.world_time_available ~= world_time_available
            or math.floor(last_written_motion.world_time_seconds / 60)
                ~= math.floor(world_time_seconds / 60)) and "world_time")
        or (map_motion_changed(last_written_motion.map_state, map_state) and "world_map")
        or (heartbeat_due and "heartbeat")
        or "motion"
    local visual_change = motion_has_visual_change(
        last_written_motion,
        is_enabled,
        mode,
        published_diagnostic_mode,
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
                false,
                write_cause
            )
        end
        return false
    end

    local body
    if map_state == nil then
        -- Protocol v6 adds one strict diagnostic-mode enum to the existing
        -- epoch/timestamp scalar record. No UObject wrapper crosses IPC. The
        -- compact frame is followed by eleven zeroed world-map fields. This is
        -- the sole runtime IPC record; no JSON Static Bridge is required.
        body = string.format(
            "%d|%d|%d|%.3f|%s|%s|%d|%s|%s|%s|%s|%s|%.0f|%s|%s|%d|%s|%d|%d|%.3f|%.6f|%.6f|%.6f|%s|%.6f|0|0|0|0|0|0|0|0|0|0|0",
            MOTION_PROTOCOL_VERSION,
            generation,
            world_epoch,
            latest_player_sample_timestamp_ms,
            is_enabled and "1" or "0",
            mode or "radar",
            diagnostic_mode_code,
            published_show_height and "1" or "0",
            published_show_treasure_types and "1" or "0",
            published_show_treasures and "1" or "0",
            published_show_bosses and "1" or "0",
            published_show_moles and "1" or "0",
            published_mole_mask,
            published_show_world_status and "1" or "0",
            published_world_time_available and "1" or "0",
            published_world_time_available and world_time_seconds or 0,
            weather_available and "1" or "0",
            weather_state,
            weather_bt_state,
            text_scale,
            player_x or 0,
            player_y or 0,
            player_z or 0,
            player_z ~= nil and "1" or "0",
            radius or radar_radius
        )
    else
        body = string.format(
            "%d|%d|%d|%.3f|%s|%s|%d|%s|%s|%s|%s|%s|%.0f|%s|%s|%d|%s|%d|%d|%.3f|%.6f|%.6f|%.6f|%s|%.6f|%d|%.6f|%.6f|%.6f|%.6f|%.9f|%.6f|%.6f|%.9f|%.6f|%.6f",
            MOTION_PROTOCOL_VERSION,
            generation,
            world_epoch,
            latest_player_sample_timestamp_ms,
            is_enabled and "1" or "0",
            mode or "world",
            diagnostic_mode_code,
            published_show_height and "1" or "0",
            published_show_treasure_types and "1" or "0",
            published_show_treasures and "1" or "0",
            published_show_bosses and "1" or "0",
            published_show_moles and "1" or "0",
            published_mole_mask,
            published_show_world_status and "1" or "0",
            published_world_time_available and "1" or "0",
            published_world_time_available and world_time_seconds or 0,
            weather_available and "1" or "0",
            weather_state,
            weather_bt_state,
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
                false,
                write_cause
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
                false,
                write_cause
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
            published_diagnostic_mode,
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
            ok and result == true,
            write_cause
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
    minimap_layer = nil
end

local function record_loop_event(kind, event, token)
    if perf_diagnostics ~= nil then
        perf_diagnostics.record_loop_event(
            kind,
            event,
            token,
            motion_loop_started and 1 or 0,
            world_map_loop_started and 1 or 0
        )
    end
end

local function invalidate_motion_loops(reason)
    motion_loop_token = motion_loop_token + 1
    world_map_loop_token = world_map_loop_token + 1
    motion_loop_started = false
    world_map_loop_started = false
    record_loop_event("all", "invalidate-" .. tostring(reason), 0)
end

local function reset_control_watchdog()
    control_watchdog_observed_serial = control_tick_serial
    control_watchdog_elapsed_ms = 0
    control_watchdog_stale_samples = 0
end

local function request_runtime_restart(reason)
    if runtime_restart_requested or runtime_restart_in_progress then
        return
    end
    runtime_restart_requested = true
    runtime_restart_reason = tostring(reason or "runtime_error")
end

local function observe_control_watchdog(delta_ms)
    if not enabled or transition_active then
        reset_control_watchdog()
        return false
    end

    control_watchdog_elapsed_ms = math.min(
        CONTROL_WATCHDOG_SAMPLE_MS,
        control_watchdog_elapsed_ms + delta_ms
    )
    if control_watchdog_elapsed_ms < CONTROL_WATCHDOG_SAMPLE_MS then
        return false
    end
    control_watchdog_elapsed_ms = 0

    if control_tick_serial ~= control_watchdog_observed_serial then
        control_watchdog_observed_serial = control_tick_serial
        control_watchdog_stale_samples = 0
        return false
    end

    control_watchdog_stale_samples = math.min(
        CONTROL_WATCHDOG_STALE_SAMPLES,
        control_watchdog_stale_samples + 1
    )
    if control_watchdog_stale_samples < CONTROL_WATCHDOG_STALE_SAMPLES then
        return false
    end

    request_runtime_restart("control_watchdog_stalled")
    return true
end

local function purge_runtime_references()
    invalidate_motion_loops("reference-purge")
    reset_player_context()
    minimap_mode = nil
    minimap_scale_elapsed_ms = MINIMAP_SCALE_CHECK_INTERVAL_MS
    latest_motion_map_state = nil
    latest_motion_x = nil
    latest_motion_y = nil
    latest_motion_z = nil
    latest_player_sample_timestamp_ms = 0.0
    latest_motion_sample = 0
    written_motion_sample = 0
    world_map_was_active = false
    last_world_left = nil
    last_world_top = nil
    last_world_zoom = nil
    update_pending = false
    activation_stable_samples = 0
    activation_probe_pending = false
    activation_probe_request_token = activation_probe_request_token + 1
    activation_probe_pending_token = nil
    activation_probe_pending_epoch = nil
    motion_update_pending = false
    world_motion_update_pending = false
    update_request_token = update_request_token + 1
    update_pending_token = nil
    world_motion_request_token = world_motion_request_token + 1
    world_motion_pending_token = nil
    update_pending_epoch = nil
    motion_pending_epoch = nil
    world_motion_pending_epoch = nil
    queued_update_started_ms = nil
    clock_capture_task_pending = false
    clock_capture_task_token = nil
    clock_context_samples = 0
    clock_context_samples_consumed = 0
    if world_environment ~= nil then
        world_environment.context_lost()
    end
    resume_probe_pending = false
    world_map_resume_pending = world_map_markers_enabled
    if world_map_markers_enabled then
        world_map.set_suspended(true, world_epoch)
    end
    if mole_completion ~= nil then
        mole_completion.set_context_available(false)
        mole_completion.invalidate_runtime_handles()
    end
    mole_visible_mask = 0
    world_time_available = false
    world_time_seconds = 0
    weather_available = false
    weather_state = 0
    weather_bt_state = 0
    reset_control_watchdog()
end

enter_world_transition = function(reason)
    world_epoch = world_epoch + 1
    transition_active = true
    transition_reason = tostring(reason or "world_context_lost")
    if world_environment ~= nil
        and (transition_reason == "f8"
            or transition_reason == "mods_txt_disabled"
            or transition_reason == "runtime_error_restart")
    then
        world_environment.cancel()
    end
    enabled = false
    map_resume_delay_remaining_ms = (
        transition_reason == "f8"
        or transition_reason == "mods_txt_disabled"
        or transition_reason == "runtime_error_restart"
    ) and 0 or MAP_LOAD_RESUME_DELAY_MS
    purge_runtime_references()
    write_disabled_motion()
    if diagnostics ~= nil then
        diagnostics.info("WORLD_TRANSITION_ENTER", nil, {
            epoch = world_epoch,
            reason = transition_reason,
            cooldown_ms = map_resume_delay_remaining_ms,
        })
    end
    if perf_diagnostics ~= nil then
        perf_diagnostics.debug("WORLD_REFERENCE_PURGE", nil, {
            epoch = world_epoch,
            pending_callbacks_invalidated = true,
        })
    end
end

local get_player_location
local complete_post_cooldown_probe
local report_async_failure

local function begin_post_cooldown_probe(expected_epoch, source)
    if not transition_active
        or expected_epoch ~= world_epoch
        or not activation_requested
        or map_resume_delay_remaining_ms > 0
    then
        return false
    end

    -- Match the proven 1.6.1 lifecycle: ending the Lua-only cooldown merely
    -- permits one normal player-location sample. It must not inspect a Pawn,
    -- World, widget, or provider to decide whether loading has completed.
    transition_active = false
    enabled = false
    activation_stable_samples = 0
    resume_probe_pending = true
    if diagnostics ~= nil then
        diagnostics.info("WORLD_TRANSITION_PROBE", nil, {
            epoch = world_epoch,
            source = tostring(source or "cooldown"),
            uobject_reads = 0,
        })
    end
    return true
end

-- F7 activation is deliberately separate from the normal radar callback.
-- This stable function object reads only the fresh current-player scalar chain;
-- map widgets, the clock, encounter conditions, save state, Overlay activation,
-- and fast producers remain dormant until four consecutive samples succeed.
local function activation_game_thread_callback()
    local request_token = activation_probe_pending_token
    local scheduled_epoch = activation_probe_pending_epoch
    f7_trace(
        "ACTIVATION_CALLBACK_ENTER",
        "request_token",
        request_token,
        "scheduled_epoch",
        scheduled_epoch
    )
    local callback_ok, callback_error = pcall(function()
        if scheduled_epoch == nil
            or request_token == nil
            or not activation_requested
            or enabled
            or not is_world_epoch_current(scheduled_epoch)
            or activation_probe_pending_token ~= request_token
        then
            return
        end

        f7_trace(
            "ACTIVATION_PLAYER_LOCATION_BEFORE",
            "request_token",
            request_token
        )
        local player_x, player_y = get_player_location()
        f7_trace(
            "ACTIVATION_PLAYER_LOCATION_AFTER",
            "request_token",
            request_token,
            "available",
            player_x ~= nil and player_y ~= nil
        )
        if player_x == nil or player_y == nil then
            activation_stable_samples = 0
            return
        end

        activation_stable_samples = math.min(
            ACTIVATION_STABLE_SAMPLE_COUNT,
            activation_stable_samples + 1
        )
        if perf_diagnostics ~= nil then
            perf_diagnostics.debug("ACTIVATION_STABILITY_SAMPLE", nil, {
                epoch = world_epoch,
                samples = activation_stable_samples,
                required = ACTIVATION_STABLE_SAMPLE_COUNT,
                uobject_scope = "current_player_location_only",
            })
        end
        if activation_stable_samples < ACTIVATION_STABLE_SAMPLE_COUNT then
            return
        end

        enabled = true
        complete_post_cooldown_probe()
        if world_environment ~= nil then
            world_environment.arm()
            world_time_available = false
            world_time_seconds = 0
        end
        if diagnostics ~= nil then
            diagnostics.info("ACTIVATION_STABLE", nil, {
                epoch = world_epoch,
                samples = activation_stable_samples,
                required = ACTIVATION_STABLE_SAMPLE_COUNT,
                deferred_layers_released = true,
            })
        end
        log("DragonSwordWorldRadar enabled after four consecutive stable 250 ms player-location samples.")
    end)

    if activation_probe_pending_epoch == scheduled_epoch
        and activation_probe_pending_token == request_token
    then
        activation_probe_pending = false
        activation_probe_pending_epoch = nil
        activation_probe_pending_token = nil
    end
    if not callback_ok then
        activation_stable_samples = 0
        report_async_failure(
            "activation_probe_callback",
            "ACTIVATION_PROBE_FAILED",
            "Activation stability probe failed: ",
            callback_error,
            { epoch = world_epoch }
        )
    end
    f7_trace(
        "ACTIVATION_CALLBACK_EXIT",
        "request_token",
        request_token,
        "callback_ok",
        callback_ok,
        "samples",
        activation_stable_samples
    )
end

local function queue_activation_probe()
    if not is_current_generation()
        or not activation_requested
        or enabled
        or transition_active
        or map_resume_delay_remaining_ms > 0
        or activation_probe_pending
    then
        return false
    end

    activation_probe_pending = true
    activation_probe_request_token = activation_probe_request_token + 1
    local request_token = activation_probe_request_token
    activation_probe_pending_token = request_token
    local scheduled_epoch = world_epoch
    activation_probe_pending_epoch = scheduled_epoch
    local queue_ok, queue_error = pcall(function()
        f7_trace(
            "ACTIVATION_EXECUTE_QUEUE_BEFORE",
            "request_token",
            request_token,
            "scheduled_epoch",
            scheduled_epoch
        )
        ExecuteInGameThread(activation_game_thread_callback)
        f7_trace(
            "ACTIVATION_EXECUTE_QUEUE_AFTER",
            "request_token",
            request_token
        )
    end)
    if not queue_ok then
        if activation_probe_pending_token == request_token then
            activation_probe_pending = false
            activation_probe_pending_epoch = nil
            activation_probe_pending_token = nil
        end
        report_async_failure(
            "activation_probe_queue",
            "ACTIVATION_PROBE_QUEUE_FAILED",
            "Could not queue activation stability probe: ",
            queue_error,
            { epoch = world_epoch }
        )
        return false
    end
    return true
end

complete_post_cooldown_probe = function()
    if not resume_probe_pending then return end

    resume_probe_pending = false
    if world_map_markers_enabled and world_map_resume_pending then
        world_map.set_suspended(false, world_epoch)
        world_map_resume_pending = false
    end
    if diagnostics ~= nil then
        diagnostics.info("WORLD_TRANSITION_RESUME", nil, {
            epoch = world_epoch,
            proof = "player_location",
        })
    end
end

local function resolve_player_controller()
    -- Match the supplied 1.6.1 implementation: Engine survives normal world
    -- changes and is resolved once, then cleared only after context failure.
    if engine == nil then
        f7_trace("ENGINE_FIND_BEFORE")
        engine = FindFirstOf("Engine")
        f7_trace("ENGINE_FIND_AFTER", "present", engine ~= nil)
    end
    if engine == nil then return nil end
    f7_trace("ENGINE_VIEWPORT_BEFORE")
    local viewport = engine.GameViewport
    f7_trace("ENGINE_VIEWPORT_AFTER", "present", viewport ~= nil)
    if viewport == nil then return nil end
    f7_trace("VIEWPORT_GAMEINSTANCE_BEFORE")
    local game_instance = viewport.GameInstance
    f7_trace(
        "VIEWPORT_GAMEINSTANCE_AFTER",
        "present",
        game_instance ~= nil
    )
    if game_instance == nil then return nil end
    f7_trace("GAMEINSTANCE_LOCALPLAYERS_BEFORE")
    local local_players = game_instance.LocalPlayers
    f7_trace(
        "GAMEINSTANCE_LOCALPLAYERS_AFTER",
        "present",
        local_players ~= nil
    )
    if local_players == nil then return nil end
    f7_trace("LOCALPLAYERS_INDEX_BEFORE")
    local local_player = local_players[1]
    f7_trace(
        "LOCALPLAYERS_INDEX_AFTER",
        "present",
        local_player ~= nil
    )
    if local_player == nil then return nil end
    f7_trace("LOCALPLAYER_CONTROLLER_BEFORE")
    local controller = local_player.PlayerController
    f7_trace(
        "LOCALPLAYER_CONTROLLER_AFTER",
        "present",
        controller ~= nil
    )
    if controller == nil then return nil end
    return controller
end

get_player_location = function()
    -- Match the supplied 1.6.1 implementation exactly: retain Engine only,
    -- and resolve the current Controller -> Pawn chain for every sample.
    -- A Pawn wrapper is never retained across frames/world teardown.
    -- This property traversal is not a UObject-array scan.
    if transition_active then return nil, nil, nil end
    local callback_epoch = world_epoch
    local root_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    local root_ms, actor_ms = 0.0, 0.0
    local ok, player_x, player_y, player_z = pcall(function()
        if not is_world_epoch_current(callback_epoch) then
            return nil, nil, nil
        end
        local controller = resolve_player_controller()
        if controller == nil then return nil, nil, nil end
        f7_trace("CONTROLLER_PAWN_BEFORE")
        local current_pawn = controller.Pawn
        f7_trace(
            "CONTROLLER_PAWN_AFTER",
            "present",
            current_pawn ~= nil
        )
        if current_pawn == nil then return nil, nil, nil end

        if perf_diagnostics ~= nil then
            root_ms = perf_diagnostics.now_ms() - root_started_ms
        end
        local actor_started_ms = perf_diagnostics ~= nil
            and perf_diagnostics.now_ms() or 0.0
        f7_trace("PAWN_LOCATION_BEFORE")
        local location = current_pawn:K2_GetActorLocation()
        f7_trace("PAWN_LOCATION_AFTER", "present", location ~= nil)
        if perf_diagnostics ~= nil then
            actor_ms = perf_diagnostics.now_ms() - actor_started_ms
        end
        if location == nil then return nil, nil, nil end

        f7_trace("LOCATION_FIELDS_BEFORE")
        local x, y, z = tonumber(location.X), tonumber(location.Y), tonumber(location.Z)
        f7_trace(
            "LOCATION_FIELDS_AFTER",
            "x_ok",
            x ~= nil,
            "y_ok",
            y ~= nil,
            "z_ok",
            z ~= nil
        )
        return x, y, z
    end)
    if perf_diagnostics ~= nil then
        if root_ms <= 0.0 then
            root_ms = math.max(0.0, perf_diagnostics.now_ms() - root_started_ms)
        end
        perf_diagnostics.record_player_location(root_ms, actor_ms)
    end
    if not ok then
        enter_world_transition("player_access_failed")
        return nil, nil, nil
    end
    if player_x == nil or player_y == nil then
        enter_world_transition("player_root_incomplete")
        return nil, nil, nil
    end
    return player_x, player_y, player_z
end

local function is_valid_object(object)
    if object == nil then return false end
    local ok, valid = pcall(function()
        return object:IsValid()
    end)
    return ok and valid == true
end

local function read_minimap_scale()
    if transition_active then return nil end
    local diagnostic_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    local cache_validation_ms = 0.0
    local find_ms = 0.0
    local resolved_validation_ms = 0.0
    local layer_map_access_ms = 0.0
    local map_overlay_access_ms = 0.0
    local scale_access_ms = 0.0
    local find_called = false
    local cache_hit = false
    local find_result_present = false
    local outcome = "unknown"
    local function emit_minimap_scale_diagnostic(scale)
        if perf_diagnostics == nil then return end
        perf_diagnostics.debug("MINIMAP_SCALE_PERF", nil, {
            epoch = world_epoch,
            cache_hit = cache_hit,
            find_called = find_called,
            find_result_present = find_result_present,
            cache_validation_ms = cache_validation_ms,
            find_ms = find_ms,
            resolved_validation_ms = resolved_validation_ms,
            layer_map_access_ms = layer_map_access_ms,
            map_overlay_access_ms = map_overlay_access_ms,
            scale_access_ms = scale_access_ms,
            total_ms = perf_diagnostics.now_ms() - diagnostic_started_ms,
            outcome = outcome,
            scale = scale,
        })
    end
    -- Exact 1.6.1 policy: retain the current minimap layer, resolve it only
    -- when absent/invalid, and use it solely for the one-hertz radius sample.
    -- Minimap visibility is intentionally not queried.
    local phase_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    f7_trace("MINIMAP_CACHE_ISVALID_BEFORE")
    cache_hit = is_valid_object(minimap_layer)
    f7_trace("MINIMAP_CACHE_ISVALID_AFTER", "valid", cache_hit)
    if perf_diagnostics ~= nil then
        cache_validation_ms = perf_diagnostics.now_ms() - phase_started_ms
    end
    if not cache_hit then
        find_called = true
        phase_started_ms = perf_diagnostics ~= nil
            and perf_diagnostics.now_ms() or 0.0
        f7_trace("MINIMAP_FIND_BEFORE")
        minimap_layer = FindFirstOf("DLayerMiniMap")
        f7_trace(
            "MINIMAP_FIND_AFTER",
            "present",
            minimap_layer ~= nil
        )
        find_result_present = minimap_layer ~= nil
        if perf_diagnostics ~= nil then
            find_ms = perf_diagnostics.now_ms() - phase_started_ms
        end
    end
    phase_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    f7_trace("MINIMAP_RESOLVED_ISVALID_BEFORE")
    local resolved_valid = is_valid_object(minimap_layer)
    f7_trace(
        "MINIMAP_RESOLVED_ISVALID_AFTER",
        "valid",
        resolved_valid
    )
    if perf_diagnostics ~= nil then
        resolved_validation_ms = perf_diagnostics.now_ms() - phase_started_ms
    end
    if not resolved_valid then
        minimap_layer = nil
        outcome = find_result_present
            and "minimap_layer_invalid" or "minimap_layer_missing"
        emit_minimap_scale_diagnostic(nil)
        return nil
    end

    local ok, scale = pcall(function()
        local current_minimap_layer = minimap_layer

        phase_started_ms = perf_diagnostics ~= nil
            and perf_diagnostics.now_ms() or 0.0
        f7_trace("MINIMAP_LAYERMAP_BEFORE")
        local layer_map = current_minimap_layer.LayerMap
        f7_trace(
            "MINIMAP_LAYERMAP_AFTER",
            "present",
            layer_map ~= nil
        )
        if perf_diagnostics ~= nil then
            layer_map_access_ms = perf_diagnostics.now_ms() - phase_started_ms
        end
        -- LayerMap and MapOverlay are nested property wrappers, not retained
        -- top-level UObject cache roots. UE4SS can report IsValid=false for
        -- these usable wrappers, so lifetime safety comes from this pcall and
        -- the validated DLayerMiniMap root above. Missing or throwing property
        -- access still fails closed and clears the root cache below.
        if layer_map == nil then
            outcome = "layer_map_missing"
            return nil
        end

        phase_started_ms = perf_diagnostics ~= nil
            and perf_diagnostics.now_ms() or 0.0
        f7_trace("MINIMAP_OVERLAY_BEFORE")
        local map_overlay = layer_map.MapOverlay
        f7_trace(
            "MINIMAP_OVERLAY_AFTER",
            "present",
            map_overlay ~= nil
        )
        if perf_diagnostics ~= nil then
            map_overlay_access_ms = perf_diagnostics.now_ms() - phase_started_ms
        end
        if map_overlay == nil then
            outcome = "map_overlay_missing"
            return nil
        end

        phase_started_ms = perf_diagnostics ~= nil
            and perf_diagnostics.now_ms() or 0.0
        f7_trace("MINIMAP_SCALE_BEFORE")
        local result = tonumber(map_overlay.RenderTransform.Scale.X)
        f7_trace("MINIMAP_SCALE_AFTER", "available", result ~= nil)
        if perf_diagnostics ~= nil then
            scale_access_ms = perf_diagnostics.now_ms() - phase_started_ms
        end
        outcome = result ~= nil and "success" or "scale_unavailable"
        return result
    end)
    if not ok or scale == nil then
        if not ok then outcome = "property_access_failed" end
        minimap_layer = nil
        emit_minimap_scale_diagnostic(nil)
        return nil
    end
    emit_minimap_scale_diagnostic(scale)
    return scale
end

local function update_radar_radius(scale)
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
    f7_trace("UPDATE_STATE_ENTER")
    -- This 250 ms callback now performs only low-frequency UObject/control
    -- sampling. It never builds JSON, serializes marker catalogs, or writes a
    -- second bridge. The 50 ms motion loops publish the latest values.
    local update_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    if not enabled or not ensure_layers_loaded() then
        return
    end

    control_sequence = control_sequence + 1
    local player_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    f7_trace("PLAYER_LOCATION_CHAIN_BEFORE")
    local player_x, player_y, player_z = get_player_location()
    f7_trace(
        "PLAYER_LOCATION_CHAIN_AFTER",
        "available",
        player_x ~= nil and player_y ~= nil
    )
    local player_ms = perf_diagnostics ~= nil
        and (perf_diagnostics.now_ms() - player_started_ms) or 0.0
    if player_x == nil or player_y == nil then
        if mole_completion ~= nil then mole_completion.set_context_available(false) end
        world_time_available = false
        world_time_seconds = 0
        weather_available = false
        weather_state = 0
        weather_bt_state = 0
        reset_player_context()
        minimap_mode = nil
        world_map_was_active = false
        latest_motion_map_state = nil

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

    -- A valid location is the only recovery proof used by the stable 1.6.1
    -- policy. World-map/provider work remains suspended until this succeeds.
    f7_trace("POST_COOLDOWN_COMPLETE_BEFORE")
    complete_post_cooldown_probe()
    f7_trace("POST_COOLDOWN_COMPLETE_AFTER")
    clock_context_samples = clock_context_samples + 1
    -- os.clock is the established UE4SS elapsed source on this Windows
    -- runtime. It is used only as a monotonic sample-to-sample numeric stamp;
    -- Overlay arrival time bounds prediction age independently.
    latest_player_sample_timestamp_ms = os.clock() * 1000.0

    local map_state = nil
    local world_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    if world_map_markers_enabled then
        -- Entry is detected in this low-frequency control loop. Once active,
        -- the 50 ms world loop owns transform updates and this callback reuses
        -- its latest state instead of traversing the map widget twice.
        local wake_hint = world_map.consume_wake_hint()
        local should_check_world_map = world_map_was_active
            or wake_hint
            or world_map.has_retained_candidates()
            or world_map_inactive_elapsed_ms
                >= WORLD_MAP_INACTIVE_CHECK_INTERVAL_MS
        if world_map_was_active and latest_motion_map_state ~= nil then
            map_state = latest_motion_map_state
        elseif should_check_world_map then
            world_map_inactive_elapsed_ms = 0
            local world_read_started_ms = perf_diagnostics ~= nil
                and perf_diagnostics.now_ms() or 0.0
            f7_trace("WORLD_MAP_READ_BEFORE")
            map_state = world_map.read_state()
            f7_trace(
                "WORLD_MAP_READ_AFTER",
                "active",
                map_state ~= nil
            )
            if perf_diagnostics ~= nil then
                perf_diagnostics.record_world_map_read(
                    perf_diagnostics.now_ms() - world_read_started_ms,
                    map_state ~= nil
                )
            end
        else
            world_map_inactive_elapsed_ms = math.min(
                WORLD_MAP_INACTIVE_CHECK_INTERVAL_MS,
                world_map_inactive_elapsed_ms
                    + MINIMAP_UPDATE_INTERVAL_MS
            )
        end
    end
    local world_ms = perf_diagnostics ~= nil
        and (perf_diagnostics.now_ms() - world_started_ms) or 0.0

    if map_state == nil then
        minimap_scale_elapsed_ms = math.min(
            MINIMAP_SCALE_CHECK_INTERVAL_MS,
            minimap_scale_elapsed_ms + MINIMAP_UPDATE_INTERVAL_MS
        )
        if minimap_scale_elapsed_ms >= MINIMAP_SCALE_CHECK_INTERVAL_MS then
            minimap_scale_elapsed_ms = 0
            f7_trace("MINIMAP_SCALE_READ_BEFORE")
            local minimap_scale = read_minimap_scale()
            f7_trace(
                "MINIMAP_SCALE_READ_AFTER",
                "available",
                minimap_scale ~= nil
            )
            if minimap_scale ~= nil then
                update_radar_radius(minimap_scale)
            end
        end
    end

    local world_time_started_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    local world_time_ms = perf_diagnostics ~= nil
        and (perf_diagnostics.now_ms() - world_time_started_ms) or 0.0
    local mole_started_ms = perf_diagnostics ~= nil and perf_diagnostics.now_ms() or 0.0
    if mole_completion ~= nil and enabled then
        mole_completion.set_context_available(true)
        local refreshed, refresh_error = pcall(mole_completion.refresh)
        if refreshed then mole_visible_mask = mole_completion.visible_mask()
        elseif refresh_error ~= nil then
            mole_visible_mask = 0
            log("Mole completion refresh failed closed: " .. tostring(refresh_error))
        end
    elseif mole_completion ~= nil then
        mole_completion.set_context_available(false)
    end
    local mole_ms = perf_diagnostics ~= nil and (perf_diagnostics.now_ms() - mole_started_ms) or 0.0

    local mode
    if map_state ~= nil then
        world_map_inactive_elapsed_ms = 0
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
        if world_map_was_active then
            world_map.recover_session()
            world_map_inactive_elapsed_ms = 0
        end
        world_map_was_active = false
        latest_motion_map_state = nil
        last_world_left = nil
        last_world_top = nil
        last_world_zoom = nil
        mode = "radar"

        publish_motion_sample(
            mode,
            player_x,
            player_y,
            player_z,
            nil
        )
    end

    -- A configuration containing only world status still uses this enabled
    -- control callback, but it must not start a 50 ms marker-motion producer.
    -- Flush its minute/lifecycle changes directly at the bounded 250 ms rate.
    if not has_radar_marker_layers() then
        flush_latest_motion()
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
            world_time_ms = world_time_ms,
            mole_ms = mole_ms,
            write_ok = true,
            left = map_state ~= nil and map_state.left or nil,
            top = map_state ~= nil and map_state.top or nil,
            zoom = map_state ~= nil and map_state.zoom or nil,
        })
    end
    f7_trace("UPDATE_STATE_EXIT")
end

local ensure_world_map_loop_started
local ensure_motion_loop_started
local perform_runtime_restart

report_async_failure = function(
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
    request_runtime_restart("async_failure:" .. tostring(key))
end

local function queue_world_time_capture()
    if world_environment == nil
        or not enabled
        or transition_active
        or clock_capture_task_pending
        or not world_environment.capture_ready()
    then
        return false
    end

    f7_trace("CLOCK_MARK_QUEUED_BEFORE")
    local token = world_environment.mark_capture_queued()
    f7_trace("CLOCK_MARK_QUEUED_AFTER", "token", token)
    if token == nil then return false end
    clock_capture_task_pending = true
    clock_capture_task_token = token
    local queued_ms = perf_diagnostics ~= nil
        and perf_diagnostics.now_ms() or 0.0
    local queue_ok, queue_error = pcall(function()
        f7_trace("CLOCK_EXECUTE_QUEUE_BEFORE", "token", token)
        ExecuteInGameThread(function()
            f7_trace("CLOCK_CALLBACK_ENTER", "token", token)
            local started_ms = perf_diagnostics ~= nil
                and perf_diagnostics.now_ms() or 0.0
            f7_trace("CLOCK_CAPTURE_CALL_BEFORE", "token", token)
            local capture_ok, captured, capture_error = pcall(
                world_environment.capture_in_game_thread,
                token
            )
            f7_trace(
                "CLOCK_CAPTURE_CALL_AFTER",
                "token",
                token,
                "call_ok",
                capture_ok,
                "captured",
                captured == true
            )
            local owns_task = clock_capture_task_token == token
            if owns_task then
                clock_capture_task_pending = false
                clock_capture_task_token = nil
                if capture_ok and captured == true then
                    world_time_available = world_environment.time_available()
                    world_time_seconds = world_environment.time_seconds()
                    latest_motion_sample = latest_motion_sample + 1
                else
                    world_time_available = false
                    world_time_seconds = 0
                end
            end
            if perf_diagnostics ~= nil then
                local capture_failure = "none"
                if not capture_ok then
                    capture_failure = tostring(captured)
                elseif captured ~= true then
                    capture_failure = tostring(
                        capture_error or "capture failed"
                    )
                end
                perf_diagnostics.debug("WORLD_TIME_TASK_PERF", nil, {
                    token = token,
                    queue_delay_ms = started_ms - queued_ms,
                    task_ms = perf_diagnostics.now_ms() - started_ms,
                    outcome = capture_ok and captured == true
                        and "captured" or "failed",
                    failure = capture_failure,
                    stale_task = not owns_task,
                })
            end
            f7_trace("CLOCK_CALLBACK_EXIT", "token", token)
        end)
        f7_trace("CLOCK_EXECUTE_QUEUE_AFTER", "token", token)
    end)
    if not queue_ok then
        if clock_capture_task_token == token then
            clock_capture_task_pending = false
            clock_capture_task_token = nil
        end
        world_environment.capture_queue_failed(token, queue_error)
        world_time_available = false
        world_time_seconds = 0
        return false
    end
    return true
end

local function queue_world_motion_update(owner_loop_token)
    if not is_current_generation()
        or not enabled
        or not world_map_was_active
        or owner_loop_token ~= world_map_loop_token
        or world_motion_update_pending
        or map_resume_delay_remaining_ms > 0
    then
        return
    end

    world_motion_update_pending = true
    world_motion_request_token = world_motion_request_token + 1
    local request_token = world_motion_request_token
    world_motion_pending_token = request_token
    local scheduled_epoch = world_epoch
    world_motion_pending_epoch = scheduled_epoch
    local queue_ok, queue_error = pcall(function()
        ExecuteInGameThread(function()
            local callback_ok, callback_error = pcall(function()
                if is_world_epoch_current(scheduled_epoch)
                    and request_token == world_motion_pending_token
                    and owner_loop_token == world_map_loop_token
                    and enabled
                    and world_map_was_active
                    and map_resume_delay_remaining_ms <= 0
                then
                    local world_read_started_ms = perf_diagnostics ~= nil
                        and perf_diagnostics.now_ms() or 0.0
                    local map_state = world_map.read_state()
                    if perf_diagnostics ~= nil then
                        perf_diagnostics.record_world_map_read(
                            perf_diagnostics.now_ms() - world_read_started_ms,
                            map_state ~= nil
                        )
                    end
                    if latest_motion_x ~= nil
                        and latest_motion_y ~= nil
                        and map_state ~= nil
                    then
                        last_world_left = map_state.left or 0
                        last_world_top = map_state.top or 0
                        last_world_zoom = map_state.zoom or 0
                        publish_motion_sample(
                            "world",
                            latest_motion_x,
                            latest_motion_y,
                            latest_motion_z,
                            map_state
                        )
                    elseif map_state == nil then
                        -- Match the reference behavior: stop fullscreen output
                        -- and immediately resume reference-style current-Pawn
                        -- sampling for the compact radar.
                        world_map.recover_session()
                        world_map_was_active = false
                        latest_motion_map_state = nil
                        last_world_left = nil
                        last_world_top = nil
                        last_world_zoom = nil
                        if latest_motion_x ~= nil and latest_motion_y ~= nil then
                            publish_motion_sample(
                                "radar",
                                latest_motion_x,
                                latest_motion_y,
                                latest_motion_z,
                                nil
                            )
                        end
                        if owner_loop_token == world_map_loop_token then
                            world_map_loop_token = world_map_loop_token + 1
                            world_map_loop_started = false
                            record_loop_event(
                                "world",
                                "map-close-stop",
                                owner_loop_token
                            )
                        end
                        ensure_motion_loop_started()
                    end
                end
            end)
            -- Always release the gate. Otherwise one failed UObject read would
            -- permanently stop all later map samples for this session.
            if world_motion_pending_epoch == scheduled_epoch
                and world_motion_pending_token == request_token
            then
                world_motion_update_pending = false
                world_motion_pending_epoch = nil
                world_motion_pending_token = nil
            end
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
        if world_motion_pending_token == request_token then
            world_motion_update_pending = false
            world_motion_pending_epoch = nil
            world_motion_pending_token = nil
        end
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

ensure_motion_loop_started = function()
    if motion_loop_started then
        return
    end
    motion_loop_token = motion_loop_token + 1
    local owner_loop_token = motion_loop_token
    motion_loop_started = true
    record_loop_event("compact", "start", owner_loop_token)

    LoopAsync(FAST_MOTION_INTERVAL_MS, function()
        if owner_loop_token ~= motion_loop_token then
            record_loop_event("compact", "stale-exit", owner_loop_token)
            return true
        end
        if not is_current_generation() then
            motion_loop_started = false
            record_loop_event("compact", "generation-stop", owner_loop_token)
            return true
        end
        if not enabled
            or transition_active
        then
            motion_loop_started = false
            record_loop_event("compact", "disabled-stop", owner_loop_token)
            return true
        end
        if world_map_was_active then
            -- Stop the radar motion loop completely while the map is active.
            -- Idling it would still wake the UE4SS async scheduler unnecessarily.
            motion_loop_started = false
            record_loop_event("compact", "world-stop", owner_loop_token)
            return true
        end
        if runtime_restart_requested
            or observe_control_watchdog(FAST_MOTION_INTERVAL_MS)
        then
            perform_runtime_restart()
            return true
        end
        motion_heartbeat_elapsed_ms = math.min(
            FAST_MOTION_HEARTBEAT_MS,
            motion_heartbeat_elapsed_ms + FAST_MOTION_INTERVAL_MS
        )
        flush_latest_motion()
        return false
    end)
end

-- One stable Lua function object is registered with UE4SS for every compact
-- control sample. Per-request state is scalar-only and lives in module fields;
-- no request-specific closure is retained by the native game-thread queue.
local function radar_game_thread_callback()
    local request_token = update_pending_token
    local scheduled_epoch = update_pending_epoch
    f7_trace(
        "RADAR_CALLBACK_ENTER",
        "request_token",
        request_token,
        "scheduled_epoch",
        scheduled_epoch
    )
    local queue_delay_ms = perf_diagnostics ~= nil
        and queued_update_started_ms ~= nil
        and (perf_diagnostics.now_ms() - queued_update_started_ms)
        or 0.0
    local callback_ok, callback_error = pcall(function()
        if scheduled_epoch ~= nil
            and request_token ~= nil
            and is_world_epoch_current(scheduled_epoch)
            and update_pending_token == request_token
            and enabled
            and map_resume_delay_remaining_ms <= 0
        then
            f7_trace(
                "RADAR_UPDATE_CALL_BEFORE",
                "request_token",
                request_token
            )
            local update_ok, update_error = pcall(update_radar_state, queue_delay_ms)
            f7_trace(
                "RADAR_UPDATE_CALL_AFTER",
                "request_token",
                request_token,
                "call_ok",
                update_ok
            )
            if not update_ok then
                report_async_failure("update_radar_state", "UPDATE_FAILED", "Radar state update failed: ", update_error, {
                    queue_delay_ms = queue_delay_ms,
                    world_map_active = world_map_was_active,
                    state_sequence = control_sequence,
                })
            end
        end

        if scheduled_epoch ~= nil
            and request_token ~= nil
            and is_world_epoch_current(scheduled_epoch)
            and update_pending_token == request_token
            and enabled
            and has_radar_marker_layers()
        then
            if world_map_was_active then
                if motion_loop_started then
                    local stopped_token = motion_loop_token
                    motion_loop_token = motion_loop_token + 1
                    motion_loop_started = false
                    record_loop_event("compact", "world-transition-stop", stopped_token)
                end
                ensure_world_map_loop_started()
            else
                ensure_motion_loop_started()
            end
        end
    end)
    if update_pending_epoch == scheduled_epoch
        and update_pending_token == request_token
    then
        queued_update_started_ms = nil
        update_pending = false
        update_pending_epoch = nil
        update_pending_token = nil
    end
    if not callback_ok then
        report_async_failure("radar_update_callback", "UPDATE_CALLBACK_FAILED", "Radar update callback failed: ", callback_error, {
            queue_delay_ms = queue_delay_ms,
            world_map_active = world_map_was_active,
            state_sequence = control_sequence,
        })
    end
    f7_trace(
        "RADAR_CALLBACK_EXIT",
        "request_token",
        request_token,
        "callback_ok",
        callback_ok
    )
end

local function queue_radar_update()
    f7_trace("RADAR_QUEUE_ENTER")
    if perf_diagnostics ~= nil then perf_diagnostics.record_queue_request() end
    if not is_current_generation() or not enabled or transition_active
        or map_resume_delay_remaining_ms > 0
    then
        if perf_diagnostics ~= nil then perf_diagnostics.record_queue_skip("generation_or_disabled") end
        return
    end
    if update_pending then
        if perf_diagnostics ~= nil then perf_diagnostics.record_queue_skip("pending") end
        return
    end

    update_pending = true
    update_request_token = update_request_token + 1
    local request_token = update_request_token
    update_pending_token = request_token
    local scheduled_epoch = world_epoch
    update_pending_epoch = scheduled_epoch
    queued_update_started_ms = perf_diagnostics ~= nil and perf_diagnostics.now_ms() or nil
    local queue_ok, queue_error = pcall(function()
        f7_trace(
            "RADAR_EXECUTE_QUEUE_BEFORE",
            "request_token",
            request_token,
            "scheduled_epoch",
            scheduled_epoch
        )
        ExecuteInGameThread(radar_game_thread_callback)
        f7_trace(
            "RADAR_EXECUTE_QUEUE_AFTER",
            "request_token",
            request_token
        )
    end)
    if not queue_ok then
        if update_pending_token == request_token then
            queued_update_started_ms = nil
            update_pending = false
            update_pending_epoch = nil
            update_pending_token = nil
        end
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
    world_map_loop_token = world_map_loop_token + 1
    local owner_loop_token = world_map_loop_token
    world_map_loop_started = true
    record_loop_event("world", "start", owner_loop_token)

    LoopAsync(WORLD_MAP_ACTIVE_INTERVAL_MS, function()
        if owner_loop_token ~= world_map_loop_token then
            record_loop_event("world", "stale-exit", owner_loop_token)
            return true
        end
        if not is_current_generation() then
            world_map_loop_started = false
            record_loop_event("world", "generation-stop", owner_loop_token)
            return true
        end
        if not enabled or transition_active or not world_map_was_active then
            world_map_loop_started = false
            record_loop_event("world", "inactive-stop", owner_loop_token)
            return true
        end

        if runtime_restart_requested
            or observe_control_watchdog(WORLD_MAP_ACTIVE_INTERVAL_MS)
        then
            perform_runtime_restart()
            return true
        end

        if perf_diagnostics ~= nil then
            perf_diagnostics.record_world_map_producer_tick()
        end

        -- Flush only the compact transform from the previous sample, then
        -- queue UObject reads for the next sample. No JSON is built here.
        motion_heartbeat_elapsed_ms = math.min(
            FAST_MOTION_HEARTBEAT_MS,
            motion_heartbeat_elapsed_ms + WORLD_MAP_ACTIVE_INTERVAL_MS
        )
        flush_latest_motion()
        queue_world_motion_update(owner_loop_token)
        return false
    end)
end

local function ensure_loop_started()
    if loop_started then
        return
    end
    loop_started = true

    LoopAsync(MINIMAP_UPDATE_INTERVAL_MS, function()
        control_tick_serial = control_tick_serial + 1
        if not is_current_generation() then
            return true
        end

        if runtime_restart_requested then
            perform_runtime_restart()
            -- Keep the established control LoopAsync callback alive. Replacing
            -- it from inside its own callback can invalidate UE4SS's active Lua
            -- function reference and trigger a native assertion.
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

        if not enabled and not activation_requested then
            loop_started = false
            return true
        end

        if map_resume_delay_remaining_ms > 0 then
            map_resume_delay_remaining_ms = math.max(
                0,
                map_resume_delay_remaining_ms
                    - MINIMAP_UPDATE_INTERVAL_MS
            )

            return false
        end

        if transition_active then
            -- The cooldown has elapsed. Release exactly one normal control
            -- sample without touching any UObject here. A failed sample
            -- re-enters this full three-second cooldown through
            -- the normal location sampler, preventing the previous 4 Hz hot retry.
            if begin_post_cooldown_probe(world_epoch, "cooldown") then
                queue_activation_probe()
            end
            return false
        end

        if activation_requested and not enabled then
            queue_activation_probe()
            return false
        end

        if world_environment ~= nil then
            if clock_context_samples > clock_context_samples_consumed then
                clock_context_samples_consumed = clock_context_samples
                world_environment.observe_context(true)
            end
            local minute_changed = world_environment.advance()
            world_time_available = world_environment.time_available()
            world_time_seconds = world_environment.time_seconds()
            if minute_changed then
                latest_motion_sample = latest_motion_sample + 1
            end
            -- Isolate the only DGameSingleton access from the normal
            -- Pawn/map/Bridge control task. This tick queues the clock job;
            -- the normal control sample resumes on the next 250 ms tick.
            if queue_world_time_capture() then
                return false
            end
        end

        queue_radar_update()
        if world_map_was_active then
            ensure_world_map_loop_started()
        end
        return false
    end)
end

local function request_active_diagnostic_mode(requested_mode, key_name)
    if not is_current_generation() then
        return
    end
    if not is_mod_enabled_in_mods_file() then
        disable_for_mod_switch()
        log(tostring(key_name) .. " ignored because DragonSwordWorldRadar is 0 in mods.txt.")
        return
    end
    diagnostic_mode = requested_mode
    f7_trace("ACTIVE_MODE_START_OVERLAY_BEFORE", "key", key_name)
    start_overlay()
    f7_trace("ACTIVE_MODE_START_OVERLAY_AFTER", "key", key_name)
    if not has_configured_visible_features() then
        log(tostring(key_name) .. " ignored because all configured visible features are disabled.")
        return
    end
    if enabled and not transition_active then
        latest_motion_sample = latest_motion_sample + 1
        flush_latest_motion()
        log("DragonSwordWorldRadar diagnostic mode changed by "
            .. tostring(key_name) .. ": " .. tostring(requested_mode) .. ".")
        return
    end
    if ensure_layers_loaded() then
        activation_requested = true
        activation_stable_samples = 0
        f7_trace("ACTIVE_MODE_LOOP_START_BEFORE", "key", key_name)
        ensure_loop_started()
        f7_trace("ACTIVE_MODE_LOOP_START_AFTER", "key", key_name)
        if transition_active then
            if not begin_post_cooldown_probe(
                world_epoch,
                string.lower(tostring(key_name))
            ) then
                log(tostring(key_name)
                    .. " deferred safely until the current Pawn-loss cooldown completes.")
                return
            end
        end
        enabled = false
        log("DragonSwordWorldRadar " .. tostring(requested_mode)
            .. " activation pending; all feature work remains dormant until four consecutive stable 250 ms player-location samples succeed.")
        f7_trace("ACTIVE_MODE_ACTIVATION_QUEUE_BEFORE", "key", key_name)
        queue_activation_probe()
        f7_trace("ACTIVE_MODE_ACTIVATION_QUEUE_AFTER", "key", key_name)
    end
end

perform_runtime_restart = function()
    if not runtime_restart_requested or runtime_restart_in_progress then
        return false
    end

    local reason = tostring(runtime_restart_reason or "runtime_error")
    runtime_restart_requested = false
    runtime_restart_reason = nil
    runtime_restart_in_progress = true

    if automatic_runtime_restart_count >= MAX_AUTOMATIC_RUNTIME_RESTARTS then
        activation_requested = false
        diagnostic_mode = "normal"
        enter_world_transition("runtime_error_limit")
        loop_started = false
        runtime_restart_in_progress = false
        log("DragonSwordWorldRadar automatic recovery stopped after "
            .. tostring(MAX_AUTOMATIC_RUNTIME_RESTARTS)
            .. " confirmed runtime failures; the mod remains safely disabled. reason="
            .. reason)
        return true
    end

    automatic_runtime_restart_count = automatic_runtime_restart_count + 1
    activation_requested = false
    diagnostic_mode = "normal"
    enter_world_transition("runtime_error_restart")
    runtime_restart_in_progress = false
    log("DragonSwordWorldRadar confirmed a runtime error and is performing automatic F8->F7 recovery: reason="
        .. reason
        .. "; attempt="
        .. tostring(automatic_runtime_restart_count))
    request_active_diagnostic_mode("normal", "AUTO_RECOVERY")
    return true
end

if config.debug_logging == true then
    RegisterKeyBind(Key[no_paint_key], function()
        request_active_diagnostic_mode("no_paint", "F5")
    end)

    RegisterKeyBind(Key[no_motion_key], function()
        request_active_diagnostic_mode("no_motion", "F6")
    end)
end

RegisterKeyBind(Key[start_key], function()
    begin_f7_trace()
    request_active_diagnostic_mode("normal", "F7")
end)

RegisterKeyBind(Key[stop_key], function()
    if not is_current_generation() then
        return
    end
    runtime_restart_requested = false
    runtime_restart_reason = nil
    automatic_runtime_restart_count = 0
    activation_requested = false
    diagnostic_mode = "normal"
    enter_world_transition("f8")
    log("DragonSwordWorldRadar disabled by F8; all markers, world status, completion polling, and time sampling are stopped for FPS comparison.")
end)

reset_bridge_files()
start_overlay()
write_disabled_motion()
if config.debug_logging == true then
    log("Ready for debug compact-radar A/B: F5 keeps producer/bridge/control work active but suppresses Overlay painting; F6 freezes the rendered motion path and slows control-only bridge consumption to 250 ms; F7 enables configured marker, save-state, and world-status work with normal rendering; F8 disables all active mod work. Confirmed async failures or five stale one-second control-heartbeat observations perform bounded automatic F8-to-F7 recovery; temporary missing map state does not. The clock performs one isolated baseline attempt after stable context. The sole Protocol-v6 Motion Bridge carries a strict diagnostic-mode enum. Use log: runtime/logs/DragonSwordWorldRadar.Lua.Use.log. Debug log: runtime/logs/DragonSwordWorldRadar.Lua.Debug.log.")
else
    log("Ready for normal play: F7 enables configured marker, save-state, and world-status work; F8 disables all active mod work. Confirmed async failures or five stale one-second control-heartbeat observations perform bounded automatic F8-to-F7 recovery; temporary missing map state does not. F5/F6 diagnostic A/B keys are not registered while debug_logging is false. The clock performs one isolated baseline attempt after stable context. Use log: runtime/logs/DragonSwordWorldRadar.Lua.Use.log.")
end
