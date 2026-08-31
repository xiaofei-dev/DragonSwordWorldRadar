local M = {}
local GAME_SECONDS_PER_REAL_SECOND = 60
local SECONDS_PER_DAY = 86400
local DEFAULT_STABLE_CONTEXT_TICKS = 8
local logger, debug_event = nil, nil
local required_stable_ticks = DEFAULT_STABLE_CONTEXT_TICKS
local activation_token = 0
local armed, attempted, pending, synchronized = false, false, false, false
local stable_context_ticks = 0
local baseline_game_seconds, baseline_wall_seconds, time_seconds = 0, 0, 0

local function emit(event, fields)
    if debug_event ~= nil then debug_event(event, fields or {}) end
end
local function safe_scalar(value)
    local kind = type(value)
    if kind == "number" or kind == "string" or kind == "boolean" then return value end
    return nil
end
local function read_field(object, name)
    local ok, value = pcall(function() return object[name] end)
    if not ok then return nil, false, value end
    return safe_scalar(value), true, nil
end
local function normalize_time(value)
    local number = tonumber(value)
    if number == nil or number ~= number or number == math.huge or number == -math.huge then return nil end
    number = math.floor(number % SECONDS_PER_DAY)
    return number
end

function M.initialize(log_function, debug_function, options)
    logger, debug_event = log_function, debug_function
    options = options or {}
    required_stable_ticks = math.max(1, math.floor(tonumber(options.stable_context_ticks) or DEFAULT_STABLE_CONTEXT_TICKS))
    M.cancel()
    return true
end
function M.arm()
    activation_token = activation_token + 1
    armed, attempted, pending, synchronized = true, false, false, false
    stable_context_ticks, baseline_game_seconds, baseline_wall_seconds, time_seconds = 0, 0, 0, 0
    emit("WORLD_TIME_ARMED", { token=activation_token, stable_context_ticks=required_stable_ticks, reads_planned=1, retry_reads=0 })
end
function M.cancel()
    activation_token = activation_token + 1
    armed, attempted, pending, synchronized = false, false, false, false
    stable_context_ticks, baseline_game_seconds, baseline_wall_seconds, time_seconds = 0, 0, 0, 0
end
function M.context_lost()
    if armed and not synchronized then
        activation_token = activation_token + 1
        -- A travel transition may cancel a job which has not begun native
        -- access. Only that cancelled request may be re-armed after the new
        -- context stabilizes. A completed failed read remains the sole read
        -- for this F7 activation and is never retried.
        if pending then attempted = false end
        pending, stable_context_ticks = false, 0
    end
end
function M.observe_context(available)
    if not armed or synchronized or attempted or pending then return end
    if available ~= true then stable_context_ticks = 0; return end
    stable_context_ticks = math.min(required_stable_ticks, stable_context_ticks + 1)
end
function M.capture_ready()
    return armed and not synchronized and not attempted and not pending and stable_context_ticks >= required_stable_ticks
end
function M.mark_capture_queued()
    if not M.capture_ready() then return nil end
    pending, attempted = true, true
    return activation_token
end
function M.capture_queue_failed(token, failure)
    if token ~= activation_token or not armed or not pending then return false end
    pending = false
    emit("WORLD_TIME_CAPTURE_FAILED", {
        token=token,
        attempted_reads=0,
        retry_reads=0,
        reason=tostring(failure or "queue failed"),
    })
    if logger ~= nil then
        logger("World clock baseline queue failed; hidden until the next F7: "
            .. tostring(failure or "unknown failure"))
    end
    return true
end
function M.capture_in_game_thread(token)
    if token ~= activation_token or not armed or not pending then return false, "cancelled" end
    emit("WORLD_TIME_CAPTURE_BEGIN", { token=token, attempted_reads=1, retry_reads=0 })
    emit("F7_CRASH_TRACE", { stage="CLOCK_SINGLETON_FIND_BEFORE", token=token })
    local found, singleton = pcall(FindFirstOf, "DGameSingleton")
    emit("F7_CRASH_TRACE", { stage="CLOCK_SINGLETON_FIND_AFTER", token=token, call_ok=found, present=singleton~=nil })
    if token ~= activation_token or not armed then singleton=nil; pending=false; return false,"cancelled" end
    if not found or singleton == nil then
        local failure = found and "DGameSingleton is not ready" or tostring(singleton)
        singleton=nil; pending=false
        emit("WORLD_TIME_CAPTURE_FAILED", { token=token, attempted_reads=1, retry_reads=0, reason=failure })
        if logger ~= nil then logger("World clock baseline unavailable; hidden until the next F7: "..failure) end
        return false,failure
    end
    emit("F7_CRASH_TRACE", { stage="CLOCK_SINGLETON_ISVALID_BEFORE", token=token })
    local valid_ok, valid = pcall(function()
        return singleton:IsValid()
    end)
    emit("F7_CRASH_TRACE", { stage="CLOCK_SINGLETON_ISVALID_AFTER", token=token, call_ok=valid_ok, valid=valid==true })
    if not valid_ok or valid ~= true then
        local failure = valid_ok and "DGameSingleton is invalid"
            or tostring(valid)
        singleton=nil; pending=false
        emit("WORLD_TIME_CAPTURE_FAILED", { token=token, attempted_reads=1, retry_reads=0, reason=failure })
        if logger ~= nil then logger("World clock baseline unavailable; hidden until the next F7: "..failure) end
        return false,failure
    end
    emit("F7_CRASH_TRACE", { stage="CLOCK_TIMEOFDAY_BEFORE", token=token })
    local value, read_ok, read_failure = read_field(singleton, "TimeOfDay")
    emit("F7_CRASH_TRACE", { stage="CLOCK_TIMEOFDAY_AFTER", token=token, call_ok=read_ok, available=value~=nil })
    singleton=nil
    local actual = read_ok and normalize_time(value) or nil
    if actual == nil then
        local failure = read_ok and "TimeOfDay is not ready" or tostring(read_failure)
        pending=false
        emit("WORLD_TIME_CAPTURE_FAILED", { token=token, attempted_reads=1, retry_reads=0, reason=failure })
        if logger ~= nil then logger("World clock baseline unavailable; hidden until the next F7: "..failure) end
        return false,failure
    end
    baseline_game_seconds = actual
    baseline_wall_seconds = os.time()
    time_seconds = actual
    synchronized, pending = true, false
    emit("WORLD_TIME_CAPTURED", { token=token, baseline_seconds=actual, attempted_reads=1, retry_reads=0, game_seconds_per_real_second=GAME_SECONDS_PER_REAL_SECOND })
    return true,nil
end
function M.advance()
    if not armed or not synchronized then return false end
    local previous_minute = math.floor(time_seconds/60)
    local elapsed_real_seconds = math.max(0, os.time() - baseline_wall_seconds)
    time_seconds = math.floor((baseline_game_seconds + elapsed_real_seconds*GAME_SECONDS_PER_REAL_SECOND)%SECONDS_PER_DAY)
    return previous_minute ~= math.floor(time_seconds/60)
end
function M.time_available() return armed and synchronized end
function M.time_seconds() return M.time_available() and time_seconds or 0 end
function M.capture_pending() return pending end
function M.capture_attempted() return attempted end
return M
