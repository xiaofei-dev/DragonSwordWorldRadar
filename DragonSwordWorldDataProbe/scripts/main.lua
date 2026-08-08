local source = debug.getinfo(1, "S").source
local script_file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local scripts_dir = script_file and script_file:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir = scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."

local function probe_enabled()
    local file = io.open(mod_dir .. "\\enabled.txt", "r")
    if not file then return false end
    local value = tostring(file:read("*a") or ""):match("^%s*([01])")
    file:close()
    return value == "1"
end

if not probe_enabled() then
    pcall(function() print("[DragonSwordWorldDataProbe] disabled by enabled.txt") end)
    return
end

local function load_safe(relative_path)
    return xpcall(function() return dofile(scripts_dir .. "\\" .. relative_path) end, debug.traceback)
end

local config_ok, config = load_safe("config.lua")
if not config_ok then
    pcall(function() print("[DragonSwordWorldDataProbe][FATAL] config load failed") end)
    return
end
local logger_ok, logger_module = load_safe("core\\logger.lua")
if not logger_ok then
    pcall(function() print("[DragonSwordWorldDataProbe][FATAL] logger load failed") end)
    return
end

local session = os.date("!%Y%m%d-%H%M%S")
local logger = logger_module.new({
    log_path = mod_dir .. "\\runtime\\logs\\DragonSwordWorldDataProbe.log",
    error_path = mod_dir .. "\\runtime\\logs\\DragonSwordWorldDataProbe-errors.log",
    status_path = mod_dir .. "\\runtime\\reports\\probe-status.txt",
    observation_path = mod_dir .. "\\runtime\\logs\\probe-observations.tsv",
    change_path = mod_dir .. "\\runtime\\logs\\probe-changes.log",
    static_catalog_path = mod_dir .. "\\runtime\\logs\\legacy-static-catalog.tsv",
    actor_sample_path = mod_dir .. "\\runtime\\logs\\generic-object-samples.tsv",
    lifecycle_path = mod_dir .. "\\runtime\\logs\\targeted-hook-events.tsv",
    session = session,
    max_len = config.max_log_value_length,
})

local function launch_external_monitor()
    if config.auto_start_external_monitor ~= true then return end
    local vbs = mod_dir .. "\\tools\\LaunchMonitor.vbs"
    local command = 'start "" /b wscript.exe "' .. vbs .. '" "' .. mod_dir .. '"'
    local ok, result = pcall(os.execute, command)
    logger:write("EXTERNAL_MONITOR_LAUNCH", {
        ok = ok,
        result = result or "nil",
        launcher = vbs,
        duplicate_protection = "named_mutex",
    })
end

if type(ExecuteWithDelay) == "function" then
    ExecuteWithDelay(2500, launch_external_monitor)
else
    launch_external_monitor()
end

local function require_module(relative_path, id)
    local ok, result = load_safe(relative_path)
    if not ok then
        logger:error("MODULE_LOAD_FAILED", {
            module = id or relative_path,
            path = relative_path,
            error = logger:trace(result),
        })
        return nil
    end
    return result
end

local checkpoint_module = require_module("core\\checkpoint.lua", "checkpoint")
local manager_module = require_module("core\\probe_manager.lua", "probe_manager")
local runner_module = require_module("core\\automatic_runner.lua", "automatic_runner")
local encoder_module = require_module("core\\value_encoder.lua", "value_encoder")
if not checkpoint_module or not manager_module or not runner_module or not encoder_module then
    logger:error("FATAL_INIT_FAILED", { stage = "core_modules" })
    return
end

local checkpoint = checkpoint_module.new({
    state_dir = mod_dir .. "\\runtime\\state",
    pending_path = mod_dir .. "\\runtime\\state\\pending-step.txt",
    quarantine_path = mod_dir .. "\\runtime\\state\\quarantine.txt",
    unsupported_path = mod_dir .. "\\runtime\\state\\unsupported-steps.txt",
    schema = config.state_schema,
})

local managers, loaded_order = {}, {}
for _, probe_id in ipairs(config.probe_order or {}) do
    local path = config.probe_modules and config.probe_modules[probe_id]
    if not path then
        logger:error("PROBE_PATH_MISSING", { probe = probe_id })
    else
        local probe = require_module(path, probe_id)
        if type(probe) ~= "table" or probe.id ~= probe_id or type(probe.steps) ~= "table" then
            logger:error("PROBE_VALIDATION_FAILED", { probe = probe_id, path = path })
        elseif probe.enabled == false then
            logger:write("PROBE_DISABLED", { probe = probe_id, path = path })
        else
            local manager_ok, manager = xpcall(function()
                return manager_module.new({
                    config = config,
                    logger = logger,
                    checkpoint = checkpoint,
                    encoder = encoder_module,
                    probe = probe,
                })
            end, debug.traceback)
            if not manager_ok then
                logger:error("PROBE_MANAGER_INIT_FAILED", {
                    probe = probe_id,
                    path = path,
                    error = logger:trace(manager),
                })
            else
                managers[probe_id] = manager
                loaded_order[#loaded_order + 1] = probe_id
                logger:write("PROBE_LOADED", { probe = probe_id, step_count = #manager.steps })
            end
        end
    end
end

if #loaded_order == 0 then
    logger:error("FATAL_INIT_FAILED", { stage = "no_valid_probes" })
    return
end

local pending = checkpoint:read_pending()
if pending then
    if tonumber(pending.schema) ~= tonumber(config.state_schema) then
        checkpoint:clear_pending()
        logger:write("STALE_PENDING_CLEARED", {
            pending_schema = pending.schema or "none",
            current_schema = config.state_schema,
            step_id = pending.step_id or "unknown",
        })
    else
        local manager = managers[pending.probe]
        if manager then
            local recovery_ok, recovery_err = xpcall(function() manager:apply_recovery(pending) end, debug.traceback)
            if not recovery_ok then
                logger:error("PROBE_RECOVERY_FAILED", {
                    probe = pending.probe or "unknown",
                    step_id = pending.step_id or "unknown",
                    error = logger:trace(recovery_err),
                })
            end
        else
            checkpoint:add_quarantine(pending.step_id)
            logger:error("UNFINISHED_STEP_QUARANTINED_WITHOUT_MANAGER", {
                step_id = pending.step_id,
                probe = pending.probe or "unknown",
            })
        end
        checkpoint:clear_pending()
    end
end

local runner = runner_module.new({
    config = config,
    logger = logger,
    managers = managers,
    order = loaded_order,
})

logger:write("SESSION_START", {
    version = config.version,
    state_schema = config.state_schema,
    mode = "modular_multi_backend_data_probe",
    loaded_runtime_modules = #loaded_order,
    default_profile = "static_first_safe",
    property_reads = config.safety.property_reads,
    find_all_of = config.safety.find_all_of,
    native_hooks = config.safety.native_hooks,
    passive_actor_lifecycle_hooks = config.safety.passive_actor_lifecycle_hooks,
    mutate_game_state = config.safety.mutate_game_state,
})

if config.automatic and config.automatic.enabled then
    local runner_ok, runner_err = xpcall(function() return runner:start() end, debug.traceback)
    if not runner_ok then
        logger:error("AUTOMATIC_RUNNER_FATAL_START_ERROR", { error = logger:trace(runner_err) })
    end
end
