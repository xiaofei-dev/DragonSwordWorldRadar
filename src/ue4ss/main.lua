local MOD = "[DragonSwordWorldRadar]"
local diagnostics = nil
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
        version = "0.4.0-dev9-performance1.1",
        generation = generation,
        enabled = config.diagnostic_logging ~= false,
        verbose = config.diagnostic_verbose == true,
        interval_seconds = tonumber(config.diagnostic_perf_interval_seconds) or 5,
    })
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
local MAX_RADAR_POINTS = 80
-- Full marker/configuration state stays low-frequency. Compact movement
-- frames use a separate double-buffered bridge. Both producer modes use a
-- 24 ms sampling cadence, then visual-delta filtering suppresses sub-pixel
-- jitter and idle writes before touching the filesystem.
local WORLD_MAP_ACTIVE_INTERVAL_MS = 24
local MAP_LOAD_RESUME_DELAY_MS = 3000
local MINIMAP_UPDATE_INTERVAL_MS = 250
local FAST_MOTION_INTERVAL_MS = 24
local FAST_MOTION_RECORD_SIZE = 768
local FAST_MOTION_HEARTBEAT_MS = 1000
local MOTION_POSITION_EPSILON = 20.0
local MOTION_Z_EPSILON = 10.0
local WORLD_MAP_PAN_EPSILON = 0.10
local WORLD_MAP_PLAYER_MAP_EPSILON = 0.10
local WORLD_MAP_ZOOM_EPSILON_RATIO = 0.0001
local WORLD_MAP_ID = 100
local TREASURE_GRID_CELL_SIZE = FIELD_RADAR_RADIUS

local world_treasures = nil
local treasure_grid = nil
local world_bosses = nil
local boss_tracker = nil
local radar_radius = FIELD_RADAR_RADIUS
local enabled = false
local loop_started = false
local world_map_loop_started = false
local update_pending = false
local state_path_a = nil
local state_path_b = nil
local motion_path_a = nil
local motion_path_b = nil
local motion_sequence = 0
local last_written_motion = nil
local last_motion_write_epoch = 0
local motion_write_error_logged = false
local latest_motion_x = nil
local latest_motion_y = nil
local latest_motion_z = nil
local latest_motion_mode = "radar"
local latest_motion_radius = FIELD_RADAR_RADIUS
local latest_motion_map_state = nil
local latest_motion_sample = 0
local written_motion_sample = 0
local write_error_logged = false
local engine = nil
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
local state_sequence = 0
local previous_state_write_ms = 0.0
local queued_update_started_ms = nil

local function ensure_treasures_loaded()
    if world_treasures ~= nil then
        return true
    end
    if not show_treasures then
        world_treasures = {}
        treasure_grid = {}
        return true
    end

    local ok_data, data = pcall(require, "treasures")
    if not ok_data or type(data) ~= "table" then
        log("treasures.lua could not be loaded: " .. tostring(data))
        return false
    end

    -- PosZ is generated locally and forwarded to the overlay for height
    -- diagnostics. UIDName remains in treasures.lua for overlay-side labels.
    world_treasures = {}
    treasure_grid = {}
    for _, treasure in ipairs(data) do
        local map_id = tonumber(string.sub(tostring(treasure.section), -3))
        local x = tonumber(treasure.x)
        local y = tonumber(treasure.y)
        local z = tonumber(treasure.z)
        local save_id = tonumber(treasure.save_id)
        if map_id == WORLD_MAP_ID
            and x ~= nil
            and y ~= nil
            and save_id ~= nil
        then
            local point = {
                save_id = save_id,
                x = x,
                y = y,
                z = z,
                has_z = z ~= nil,
            }
            table.insert(world_treasures, point)

            local cell_x = math.floor(x / TREASURE_GRID_CELL_SIZE)
            local cell_y = math.floor(y / TREASURE_GRID_CELL_SIZE)
            local cell_key = tostring(cell_x) .. ":" .. tostring(cell_y)
            local cell = treasure_grid[cell_key]
            if cell == nil then
                cell = {}
                treasure_grid[cell_key] = cell
            end
            table.insert(cell, point)
        end
    end

    log(string.format(
        "Loaded %d exact-CID world treasure locations.",
        #world_treasures
    ))
    return true
end

local function ensure_bosses_loaded()
    if world_bosses ~= nil and boss_tracker ~= nil then
        return true
    end
    if not show_bosses then
        world_bosses = {}
        return true
    end

    local ok_data, data = pcall(require, "bosses")
    if not ok_data or type(data) ~= "table" then
        log("bosses.lua could not be loaded: " .. tostring(data))
        return false
    end
    local ok_tracker, tracker = pcall(require, "boss_tracker")
    if not ok_tracker or type(tracker) ~= "table" then
        log("boss_tracker.lua could not be loaded: " .. tostring(tracker))
        return false
    end

    world_bosses = {}
    for _, boss in ipairs(data) do
        local boss_id = tonumber(boss.boss_id)
        local map_id = tonumber(boss.map_id)
        local x = tonumber(boss.x)
        local y = tonumber(boss.y)
        local z = tonumber(boss.z)
        if boss_id ~= nil and map_id ~= nil and x ~= nil and y ~= nil then
            table.insert(world_bosses, {
                boss_id = boss_id,
                map_id = map_id,
                x = x,
                y = y,
                z = z,
                uid_name = tostring(boss.uid_name or ""),
            })
        end
    end
    if #world_bosses ~= 9 then
        log(string.format(
            "World-boss dataset validation failed: expected 9, loaded %d.",
            #world_bosses
        ))
        world_bosses = nil
        return false
    end

    boss_tracker = tracker
    boss_tracker.initialize(
        world_bosses,
        log,
        resolve_mod_directory() .. "\\runtime\\boss_state.txt"
    )
    log("Loaded 9 world boss locations and initialized runtime state tracking.")
    return true
end

local function ensure_layers_loaded()
    if not show_treasures and not show_bosses then
        return false
    end
    return ensure_treasures_loaded() and ensure_bosses_loaded()
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

local function file_exists(path)
    local file = io.open(path, "rb")
    if file == nil then
        return false
    end
    file:close()
    return true
end

local function start_overlay()
    if overlay_start_requested or not auto_start_overlay then
        return
    end
    overlay_start_requested = true

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
    log("PowerShell overlay launch requested through hidden watcher: " .. request_path)
end

local function resolve_state_paths()
    if state_path_a ~= nil and state_path_b ~= nil then
        return state_path_a, state_path_b
    end

    local directory = resolve_mod_directory()
    if directory ~= nil then
        state_path_a = directory .. "\\runtime\\bridge\\radar_state_a.json"
        state_path_b = directory .. "\\runtime\\bridge\\radar_state_b.json"
        log("Static state bridge files: "
            .. state_path_a .. " / " .. state_path_b)
    end
    return state_path_a, state_path_b
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
    local static_a, static_b = resolve_state_paths()
    local motion_a, motion_b = resolve_motion_paths()
    local directory = resolve_mod_directory()
    local legacy_state = directory ~= nil
        and (directory .. "\\runtime\\bridge\\radar_state.json")
        or nil
    for _, path in ipairs({
        static_a,
        static_b,
        motion_a,
        motion_b,
        legacy_state,
    }) do
        if path ~= nil then
            pcall(os.remove, path)
        end
    end
    state_sequence = 0
    motion_sequence = 0
    last_written_motion = nil
    last_motion_write_epoch = 0
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
    if previous == nil then
        return true
    end
    if previous.is_enabled ~= (is_enabled == true)
        or previous.mode ~= tostring(mode or "radar")
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
    return {
        map_id = tonumber(map_state.map_id) or 0,
        dimensions = tonumber(map_state.dimensions) or 0.0,
        ui_size = tonumber(map_state.ui_size) or 0.0,
        left = tonumber(map_state.left) or 0.0,
        top = tonumber(map_state.top) or 0.0,
        zoom = tonumber(map_state.zoom) or 0.0,
        viewport_width = tonumber(map_state.viewport_width) or 0.0,
        viewport_height = tonumber(map_state.viewport_height) or 0.0,
        viewport_scale = tonumber(map_state.viewport_scale) or 0.0,
        player_map_x = tonumber(map_state.player_map_x) or 0.0,
        player_map_y = tonumber(map_state.player_map_y) or 0.0,
    }
end

local function remember_written_motion(
    is_enabled,
    mode,
    player_x,
    player_y,
    player_z,
    radius,
    map_state,
    written_at_epoch
)
    last_written_motion = {
        is_enabled = is_enabled == true,
        mode = tostring(mode or "radar"),
        player_x = tonumber(player_x) or 0.0,
        player_y = tonumber(player_y) or 0.0,
        player_z = player_z ~= nil and tonumber(player_z) or nil,
        radius = tonumber(radius) or radar_radius,
        map_state = copy_map_motion(map_state),
    }
    last_motion_write_epoch = written_at_epoch
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
    local diagnostic_start = diagnostics ~= nil
        and diagnostics.now_ms() or 0.0
    local heartbeat_epoch = os.time()
    local heartbeat_due = last_written_motion == nil
        or os.difftime(
            heartbeat_epoch,
            last_motion_write_epoch
        ) * 1000.0 >= FAST_MOTION_HEARTBEAT_MS
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
        if diagnostics ~= nil then
            diagnostics.record_motion_skip()
        end
        return true
    end

    local path_a, path_b = resolve_motion_paths()
    if path_a == nil or path_b == nil then
        if diagnostics ~= nil then
            diagnostics.record_motion_write(
                diagnostics.now_ms() - diagnostic_start,
                false
            )
        end
        return false
    end

    local map_values = map_state or {}
    local body = string.format(
        "%d|%s|%s|%.6f|%.6f|%.6f|%s|%.6f|%d|%.6f|%.6f|%.6f|%.6f|%.9f|%.6f|%.6f|%.9f|%.6f|%.6f",
        generation,
        is_enabled and "1" or "0",
        mode or "radar",
        player_x or 0,
        player_y or 0,
        player_z or 0,
        player_z ~= nil and "1" or "0",
        radius or radar_radius,
        tonumber(map_values.map_id) or 0,
        tonumber(map_values.dimensions) or 0,
        tonumber(map_values.ui_size) or 0,
        tonumber(map_values.left) or 0,
        tonumber(map_values.top) or 0,
        tonumber(map_values.zoom) or 0,
        tonumber(map_values.viewport_width) or 0,
        tonumber(map_values.viewport_height) or 0,
        tonumber(map_values.viewport_scale) or 0,
        tonumber(map_values.player_map_x) or 0,
        tonumber(map_values.player_map_y) or 0
    )

    motion_sequence = motion_sequence + 1
    local sequence_text = tostring(motion_sequence)
    local payload = sequence_text .. "|" .. body .. "|" .. sequence_text .. "\n"
    if #payload > FAST_MOTION_RECORD_SIZE then
        if not motion_write_error_logged then
            log("Fast motion record exceeded its fixed size.")
            motion_write_error_logged = true
        end
        if diagnostics ~= nil then
            diagnostics.record_motion_write(
                diagnostics.now_ms() - diagnostic_start,
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
        if diagnostics ~= nil then
            diagnostics.record_motion_write(
                diagnostics.now_ms() - diagnostic_start,
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
            map_state,
            heartbeat_epoch
        )
        if motion_write_error_logged then
            log("Fast motion bridge recovered.")
            motion_write_error_logged = false
        end
    end
    if diagnostics ~= nil then
        diagnostics.record_motion_write(
            diagnostics.now_ms() - diagnostic_start,
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

local function write_static_state(text)
    local path_a, path_b = resolve_state_paths()
    if path_a == nil or path_b == nil then
        return false, "state bridge paths unavailable"
    end
    local path = state_sequence % 2 == 0 and path_a or path_b
    local file, open_error = io.open(path, "w")
    if file == nil then
        return false, open_error
    end
    local ok, result = pcall(function()
        file:write(text)
        file:close()
        return true
    end)
    if not ok or result ~= true then
        pcall(file.close, file)
        return false, result
    end
    return true, nil
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
    if diagnostics ~= nil then
        diagnostics.record_motion_sample()
    end
end

local function write_disabled_state()
    state_sequence = state_sequence + 1
    local written, write_error = write_static_state(
        string.format(
            '{"stateSequence":%d,"producerGeneration":%d,"enabled":false,"mode":"disabled","points":[],"bosses":[]}',
            state_sequence,
            generation
        )
    )
    if not written and diagnostics ~= nil then
        diagnostics.error_rate_limited(
            "disabled_state_write",
            10,
            "DISABLED_STATE_WRITE_FAILED",
            tostring(write_error or "state path unavailable"),
            { state_sequence = state_sequence }
        )
    end
    write_fast_motion(false, "disabled", 0, 0, nil, radar_radius, nil, true)
    if diagnostics ~= nil then
        diagnostics.set_mode("disabled", state_sequence)
    end
    return written
end

local function get_player_location()
    -- A cached Pawn can remain a valid UObject after teleport, respawn,
    -- mounting, character replacement, or map transition. Validity alone is
    -- therefore insufficient: always resolve PlayerController.Pawn and make
    -- the cache follow the controller's current Pawn identity.
    local ok, player_x, player_y, player_z = pcall(function()
        if engine == nil then
            engine = FindFirstOf("Engine")
        end
        if engine == nil then
            return nil, nil, nil
        end

        local viewport = engine.GameViewport
        if viewport == nil then
            return nil, nil, nil
        end

        local game_instance = viewport.GameInstance
        if game_instance == nil then
            return nil, nil, nil
        end

        local local_players = game_instance.LocalPlayers
        if local_players == nil then
            return nil, nil, nil
        end

        local local_player = local_players[1]
        if local_player == nil then
            return nil, nil, nil
        end

        local controller = local_player.PlayerController
        if controller == nil then
            return nil, nil, nil
        end

        local current_pawn = controller.Pawn
        if current_pawn == nil then
            player_pawn = nil
            return nil, nil, nil
        end

        -- UE4SS can return a new Lua wrapper for the same UObject on each read.
        -- Comparing wrapper identity caused a false pawn-change event every frame.
        -- Always sample the current PlayerController.Pawn directly instead.
        player_pawn = current_pawn

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
        engine = nil
        player_pawn = nil
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

local function current_time_ms()
    if diagnostics ~= nil then
        return diagnostics.now_ms()
    end
    return os.clock() * 1000.0
end

local function build_boss_json(player_x, player_y, nearby_only, current_map_id)
    if not show_bosses or boss_tracker == nil then
        return "[]"
    end
    boss_tracker.update(player_x, player_y, current_time_ms(), current_map_id)
    local snapshot = boss_tracker.snapshot()
    local radius_squared = radar_radius * radar_radius
    local parts = { "[" }
    local written = 0
    for _, boss in ipairs(snapshot) do
        local dx = boss.x - player_x
        local dy = boss.y - player_y
        local is_nearby = dx * dx + dy * dy <= radius_squared
        if (not nearby_only or is_nearby) and boss.visible ~= false then
            if written > 0 then
                table.insert(parts, ",")
            end
            written = written + 1
            table.insert(parts, string.format(
                '{"bossId":%d,"mapId":%d,"x":%.3f,"y":%.3f,"z":%.3f,"hasZ":%s,"dx":%.3f,"dy":%.3f,"visible":true,"status":"%s"}',
                boss.boss_id,
                boss.map_id,
                boss.x,
                boss.y,
                boss.z or 0,
                boss.has_z and "true" or "false",
                dx,
                dy,
                tostring(boss.status or "unknown")
            ))
        end
    end
    table.insert(parts, "]")
    return table.concat(parts)
end

local function build_radar_json(player_x, player_y, player_z)
    local radius_squared = radar_radius * radar_radius
    local nearby = {}
    local center_cell_x =
        math.floor(player_x / TREASURE_GRID_CELL_SIZE)
    local center_cell_y =
        math.floor(player_y / TREASURE_GRID_CELL_SIZE)
    local cell_range =
        math.ceil(radar_radius / TREASURE_GRID_CELL_SIZE)

    for offset_x = -cell_range, cell_range do
        for offset_y = -cell_range, cell_range do
            local cell_key =
                tostring(center_cell_x + offset_x)
                .. ":"
                .. tostring(center_cell_y + offset_y)
            local cell = treasure_grid[cell_key]
            if cell ~= nil then
                for _, treasure in ipairs(cell) do
                    local delta_x = treasure.x - player_x
                    local delta_y = treasure.y - player_y
                    local planar_distance_squared =
                        delta_x * delta_x + delta_y * delta_y
                    local delta_z = 0
                    if treasure.has_z == true and player_z ~= nil then
                        local comparable_player_z = player_z - 150.0
                        delta_z = treasure.z - comparable_player_z
                    end
                    local distance_squared =
                        planar_distance_squared + delta_z * delta_z
                    if planar_distance_squared <= radius_squared then
                        table.insert(nearby, {
                            save_id = treasure.save_id,
                            x = treasure.x,
                            y = treasure.y,
                            z = treasure.z or 0,
                            has_z = treasure.has_z == true,
                            dx = delta_x,
                            dy = delta_y,
                            distance_squared = distance_squared,
                        })
                    end
                end
            end
        end
    end

    table.sort(nearby, function(left, right)
        return left.distance_squared < right.distance_squared
    end)

    local count = math.min(#nearby, MAX_RADAR_POINTS)
    local parts = {
        string.format(
            '{"enabled":true,"showHeight":%s,"showTreasureTypes":%s,"showTreasures":%s,"showBosses":%s,"textScale":%.3f,"playerX":%.3f,"playerY":%.3f,"playerZ":%.3f,"hasPlayerZ":%s,"mode":"radar","radius":%.3f,"points":[',
            show_height and "true" or "false",
            show_treasure_types and "true" or "false",
            show_treasures and "true" or "false",
            show_bosses and "true" or "false",
            text_scale,
            player_x,
            player_y,
            player_z or 0,
            player_z ~= nil and "true" or "false",
            radar_radius
        ),
    }
    for index = 1, count do
        local point = nearby[index]
        if index > 1 then
            table.insert(parts, ",")
        end
        table.insert(parts, string.format(
            '{"saveId":%d,"x":%.3f,"y":%.3f,"z":%.3f,"hasZ":%s,"dx":%.3f,"dy":%.3f}',
            point.save_id,
            point.x,
            point.y,
            point.z,
            point.has_z and "true" or "false",
            point.dx,
            point.dy
        ))
    end
    table.insert(parts, '],"bosses":')
    table.insert(parts, build_boss_json(player_x, player_y, true, nil))
    table.insert(parts, "}")
    return table.concat(parts)
end

local function build_world_map_json(map, player_x, player_y, player_z)
    return string.format(
        '{"enabled":true,"showHeight":%s,"showTreasureTypes":%s,"showTreasures":%s,"showBosses":%s,"textScale":%.3f,"playerX":%.3f,"playerY":%.3f,"playerZ":%.3f,"hasPlayerZ":%s,"mode":"world","worldMap":'
            .. '{"mapId":%d,"dimensions":%.3f,'
            .. '"uiSize":%.3f,"left":%.3f,"top":%.3f,'
            .. '"zoom":%.6f,"viewportWidth":%.3f,'
            .. '"viewportHeight":%.3f,"viewportScale":%.6f,'
            .. '"playerWorldX":%.3f,"playerWorldY":%.3f,'
            .. '"playerMapX":%.3f,"playerMapY":%.3f},'
            .. '"points":[],"bosses":%s}',
        show_height and "true" or "false",
        show_treasure_types and "true" or "false",
        show_treasures and "true" or "false",
        show_bosses and "true" or "false",
        text_scale,
        player_x,
        player_y,
        player_z or 0,
        player_z ~= nil and "true" or "false",
        map.map_id,
        map.dimensions,
        map.ui_size,
        map.left,
        map.top,
        map.zoom,
        map.viewport_width,
        map.viewport_height,
        map.viewport_scale,
        player_x,
        player_y,
        map.player_map_x,
        map.player_map_y,
        build_boss_json(player_x, player_y, false, map.map_id)
    )
end

local function inject_state_diagnostics(json, metrics)
    return string.format(
        '{"stateSequence":%d,"producerGeneration":%d,'
            .. '"producerQueueDelayMs":%.3f,'
            .. '"producerPlayerReadMs":%.3f,'
            .. '"producerWorldReadMs":%.3f,'
            .. '"producerBuildMs":%.3f,'
            .. '"producerPreviousWriteMs":%.3f,'
            .. '"producerBeforeWriteMs":%.3f,%s',
        metrics.state_sequence,
        generation,
        metrics.queue_delay_ms or 0.0,
        metrics.player_ms or 0.0,
        metrics.world_ms or 0.0,
        metrics.build_ms or 0.0,
        previous_state_write_ms or 0.0,
        metrics.before_write_ms or 0.0,
        string.sub(json, 2)
    )
end

local function update_radar_state(queue_delay_ms)
    local update_started_ms = diagnostics ~= nil
        and diagnostics.now_ms() or 0.0
    if not enabled or not ensure_layers_loaded() then
        return
    end

    local player_started_ms = diagnostics ~= nil
        and diagnostics.now_ms() or 0.0
    local player_x, player_y, player_z = get_player_location()
    local player_ms = diagnostics ~= nil
        and (diagnostics.now_ms() - player_started_ms) or 0.0
    if player_x == nil or player_y == nil then
        engine = nil
        player_pawn = nil
        minimap_layer = nil
        minimap_mode = nil
        world_map_was_active = false

        if world_map_markers_enabled then
            world_map.set_suspended(true)
            map_resume_delay_remaining_ms =
                MAP_LOAD_RESUME_DELAY_MS
        end

        local disabled_state_written = write_disabled_state()
        if diagnostics ~= nil then
            diagnostics.record_update({
                mode = "disabled",
                state_sequence = state_sequence,
                queue_delay_ms = queue_delay_ms or 0.0,
                player_ms = player_ms,
                world_ms = 0.0,
                build_ms = 0.0,
                write_ms = 0.0,
                total_ms = diagnostics.now_ms() - update_started_ms,
                player_missing = true,
                failed = true,
                write_ok = disabled_state_written,
            })
        end
        return
    end

    local map_state = nil
    local world_started_ms = diagnostics ~= nil
        and diagnostics.now_ms() or 0.0
    if world_map_markers_enabled then
        map_state = world_map.read_state()
    end
    local world_ms = diagnostics ~= nil
        and (diagnostics.now_ms() - world_started_ms) or 0.0

    local output
    local mode
    local build_started_ms = diagnostics ~= nil
        and diagnostics.now_ms() or 0.0
    if map_state ~= nil then
        world_map_was_active = true
        last_world_left = map_state.left or 0
        last_world_top = map_state.top or 0
        last_world_zoom = map_state.zoom or 0
        mode = "world"
        output = build_world_map_json(
            map_state,
            player_x,
            player_y,
            player_z
        )
        publish_motion_sample(
            "world",
            player_x,
            player_y,
            player_z,
            map_state
        )
    else
        world_map_was_active = false
        last_world_left = nil
        last_world_top = nil
        last_world_zoom = nil
        mode = "radar"

        minimap_scale_elapsed_ms =
            minimap_scale_elapsed_ms
                + MINIMAP_UPDATE_INTERVAL_MS

        if minimap_scale_elapsed_ms
            >= MINIMAP_SCALE_CHECK_INTERVAL_MS
        then
            minimap_scale_elapsed_ms = 0
            update_radar_radius()
        end

        output = build_radar_json(
            player_x,
            player_y,
            player_z
        )
        publish_motion_sample(
            "radar",
            player_x,
            player_y,
            player_z,
            nil
        )
    end
    local build_ms = diagnostics ~= nil
        and (diagnostics.now_ms() - build_started_ms) or 0.0

    state_sequence = state_sequence + 1
    local before_write_ms = diagnostics ~= nil
        and (diagnostics.now_ms() - update_started_ms) or 0.0
    output = inject_state_diagnostics(output, {
        state_sequence = state_sequence,
        queue_delay_ms = queue_delay_ms or 0.0,
        player_ms = player_ms,
        world_ms = world_ms,
        build_ms = build_ms,
        before_write_ms = before_write_ms,
    })

    local write_started_ms = diagnostics ~= nil
        and diagnostics.now_ms() or 0.0
    local written, write_error = write_static_state(output)
    local write_ms = diagnostics ~= nil
        and (diagnostics.now_ms() - write_started_ms) or 0.0
    previous_state_write_ms = write_ms
    if not written then
        if not write_error_logged then
            log("Could not write radar state: " .. tostring(write_error))
            write_error_logged = true
        end
    elseif write_error_logged then
        log("Radar state output recovered")
        write_error_logged = false
    end

    if diagnostics ~= nil then
        diagnostics.record_update({
            mode = mode,
            state_sequence = state_sequence,
            queue_delay_ms = queue_delay_ms or 0.0,
            player_ms = player_ms,
            world_ms = world_ms,
            build_ms = build_ms,
            write_ms = write_ms,
            total_ms = diagnostics.now_ms() - update_started_ms,
            write_ok = written,
            left = map_state ~= nil and map_state.left or nil,
            top = map_state ~= nil and map_state.top or nil,
            zoom = map_state ~= nil and map_state.zoom or nil,
        })
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
    ExecuteInGameThread(function()
        if is_current_generation()
            and enabled
            and world_map_was_active
            and map_resume_delay_remaining_ms <= 0
        then
            local player_x, player_y, player_z = get_player_location()
            local map_state = world_map.read_state()
            if player_x ~= nil and player_y ~= nil and map_state ~= nil then
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
                world_map_was_active = false
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
            end
        end
        world_motion_update_pending = false
    end)
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
    ExecuteInGameThread(function()
        if is_current_generation()
            and enabled
            and not world_map_was_active
            and map_resume_delay_remaining_ms <= 0
        then
            local player_x, player_y, player_z = get_player_location()
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
        motion_update_pending = false
    end)
end

local function ensure_motion_loop_started()
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
        flush_latest_motion()
        queue_motion_update()
        return false
    end)
end

local ensure_world_map_loop_started

local function queue_radar_update()
    if diagnostics ~= nil then
        diagnostics.record_queue_request()
    end
    if not is_current_generation() or not enabled
        or map_resume_delay_remaining_ms > 0
    then
        if diagnostics ~= nil then
            diagnostics.record_queue_skip("generation_or_disabled")
        end
        return
    end
    if update_pending then
        if diagnostics ~= nil then
            diagnostics.record_queue_skip("pending")
        end
        return
    end

    update_pending = true
    queued_update_started_ms = diagnostics ~= nil
        and diagnostics.now_ms() or nil
    ExecuteInGameThread(function()
        local queue_delay_ms = diagnostics ~= nil
            and queued_update_started_ms ~= nil
            and (diagnostics.now_ms() - queued_update_started_ms)
            or 0.0
        if is_current_generation()
            and map_resume_delay_remaining_ms <= 0
        then
            local ok, update_error = pcall(
                update_radar_state,
                queue_delay_ms
            )
            if not ok and diagnostics ~= nil then
                diagnostics.error_rate_limited(
                    "update_radar_state",
                    10,
                    "UPDATE_FAILED",
                    tostring(update_error),
                    {
                        queue_delay_ms = queue_delay_ms,
                        world_map_active = world_map_was_active,
                        state_sequence = state_sequence,
                    }
                )
            elseif not ok then
                log("Radar state update failed: " .. tostring(update_error))
            end
        end
        queued_update_started_ms = nil
        update_pending = false
        if world_map_was_active then
            ensure_world_map_loop_started()
        else
            -- Restart the fast radar bridge after the world map closes.
            ensure_motion_loop_started()
        end
    end)
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
    write_disabled_state()
    log("World radar disabled; all producer loops will stop.")
end)

reset_bridge_files()
start_overlay()
write_disabled_state()
log("Ready. F7 enables configured radar layers; F8 disables them. Static state uses 250 ms double buffering. Motion is sampled at 24 ms, written only on visual change or a 1 second heartbeat, and consumed by an adaptive Overlay timer. Diagnostics: runtime/logs/DragonSwordWorldRadar.Lua.log and Collect-Diagnostics.cmd.")
