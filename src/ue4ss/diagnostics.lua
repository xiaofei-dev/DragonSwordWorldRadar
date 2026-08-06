local Diagnostics = {}

local MOD = "[DragonSwordWorldRadar]"
local version = "unknown"
local generation = 0
local enabled = true
local verbose = false
local interval_seconds = 5
local log_path = nil
local configured = false
local line_sequence = 0
local last_perf_wall = os.time()
local current_mode = "unknown"
local throttles = {}

local counters = nil

local function reset_counters()
    counters = {
        updates = 0,
        world_updates = 0,
        radar_updates = 0,
        failed_updates = 0,
        queue_requests = 0,
        queue_pending_skips = 0,
        queue_generation_skips = 0,
        player_missing = 0,
        state_writes = 0,
        state_write_failures = 0,
        motion_samples = 0,
        motion_write_skips = 0,
        motion_writes = 0,
        motion_write_failures = 0,
        motion_write_total_ms = 0.0,
        motion_write_max_ms = 0.0,
        player_total_ms = 0.0,
        player_max_ms = 0.0,
        world_total_ms = 0.0,
        world_max_ms = 0.0,
        build_total_ms = 0.0,
        build_max_ms = 0.0,
        write_total_ms = 0.0,
        write_max_ms = 0.0,
        total_total_ms = 0.0,
        total_max_ms = 0.0,
        queue_total_ms = 0.0,
        queue_max_ms = 0.0,
        world_input_changed = 0,
        world_input_unchanged = 0,
        max_pan_delta = 0.0,
        max_zoom_delta_percent = 0.0,
        last_world_left = nil,
        last_world_top = nil,
        last_world_zoom = nil,
        last_state_sequence = 0,
    }
end
reset_counters()

local function resolve_log_path()
    if log_path ~= nil then
        return log_path
    end
    local source = debug.getinfo(1, "S").source
    if type(source) == "string" and string.sub(source, 1, 1) == "@" then
        local script_file = string.sub(source, 2)
        local scripts_directory = string.match(
            script_file,
            "^(.*)[/\\][^/\\]+$"
        )
        local mod_directory = scripts_directory
            and string.match(scripts_directory, "^(.*)[/\\][^/\\]+$")
        if mod_directory ~= nil then
            log_path = mod_directory .. "\\runtime\\logs\\DragonSwordWorldRadar.Lua.log"
        end
    end
    return log_path
end

local function token(value)
    if value == nil then
        return "null"
    end
    local value_type = type(value)
    if value_type == "boolean" then
        return value and "true" or "false"
    end
    if value_type == "number" then
        return string.format("%.3f", value)
    end
    local text = tostring(value)
    text = string.gsub(text, "\\", "\\\\")
    text = string.gsub(text, "\r", "\\r")
    text = string.gsub(text, "\n", "\\n")
    text = string.gsub(text, "\t", "\\t")
    if string.find(text, "[%s;=']") ~= nil then
        text = "'" .. string.gsub(text, "'", "''") .. "'"
    end
    return text
end

local function sorted_keys(fields)
    local keys = {}
    if type(fields) == "table" then
        for key, _ in pairs(fields) do
            table.insert(keys, tostring(key))
        end
        table.sort(keys)
    end
    return keys
end

local function append_line(line)
    if not enabled then
        return
    end
    local path = resolve_log_path()
    if path == nil then
        return
    end
    local file = io.open(path, "a")
    if file == nil then
        return
    end
    local ok = pcall(function()
        file:write(line)
        file:write("\n")
        file:flush()
    end)
    pcall(function() file:close() end)
    return ok
end

function Diagnostics.event(level, event, message, fields, file_only)
    line_sequence = line_sequence + 1
    local parts = {
        "[" .. os.date("!%Y-%m-%dT%H:%M:%SZ") .. "]",
        "[" .. tostring(level or "INFO") .. "]",
        "[generation=" .. tostring(generation)
            .. " seq=" .. string.format("%06d", line_sequence) .. "]",
        "[lua/" .. tostring(event or "MESSAGE") .. "]",
    }
    if message ~= nil and tostring(message) ~= "" then
        table.insert(parts, "message=" .. token(message))
    end
    for _, key in ipairs(sorted_keys(fields)) do
        table.insert(parts, key .. "=" .. token(fields[key]))
    end
    table.insert(parts, "monoMs=" .. token(os.clock() * 1000.0))
    local line = table.concat(parts, " ")
    append_line(line)
    if not file_only then
        print(string.format("%s %s", MOD, line))
    end
end

function Diagnostics.info(event, message, fields)
    Diagnostics.event("INFO", event, message, fields, false)
end

function Diagnostics.debug(event, message, fields)
    if verbose then
        Diagnostics.event("DEBUG", event, message, fields, false)
    end
end

function Diagnostics.warn(event, message, fields)
    Diagnostics.event("WARN", event, message, fields, false)
end

function Diagnostics.error(event, message, fields)
    Diagnostics.event("ERROR", event, message, fields, false)
end

function Diagnostics.error_rate_limited(key, seconds, event, message, fields)
    local now = os.time()
    local entry = throttles[key]
    if entry == nil then
        entry = { next_time = 0, suppressed = 0 }
        throttles[key] = entry
    end
    if now >= entry.next_time then
        fields = fields or {}
        if entry.suppressed > 0 then
            fields.suppressed_since_last = entry.suppressed
        end
        entry.next_time = now + (tonumber(seconds) or 10)
        entry.suppressed = 0
        Diagnostics.error(event, message, fields)
    else
        entry.suppressed = entry.suppressed + 1
    end
end

function Diagnostics.configure(options)
    options = options or {}
    version = tostring(options.version or version)
    generation = tonumber(options.generation) or generation
    enabled = options.enabled ~= false
    verbose = options.verbose == true
    interval_seconds = math.max(
        3,
        math.min(30, tonumber(options.interval_seconds) or 5)
    )
    local path = resolve_log_path()
    if path ~= nil and not configured then
        local open_mode = "w"
        local existing = io.open(path, "r")
        if existing ~= nil then
            existing:close()
            local archived = false
            local runtime_directory = string.match(
                path,
                "^(.*)[/\\][^/\\]+$"
            )
            if runtime_directory ~= nil then
                local archive_path = runtime_directory
                    .. "\\logs\\archive\\DragonSwordWorldRadar.Lua."
                    .. os.date("!%Y%m%d-%H%M%S")
                    .. ".g" .. tostring(generation) .. ".log"
                local call_ok, rename_ok = pcall(
                    os.rename,
                    path,
                    archive_path
                )
                archived = call_ok and rename_ok ~= nil
            end
            -- If rotation fails, append rather than destroying the previous log.
            if not archived then
                open_mode = "a"
            end
        end
        local file = io.open(path, open_mode)
        if file ~= nil then
            file:close()
        end
    end
    configured = true
    last_perf_wall = os.time()
    reset_counters()
    Diagnostics.info("SESSION_START", nil, {
        version = version,
        generation = generation,
        diagnostics_enabled = enabled,
        verbose = verbose,
        interval_seconds = interval_seconds,
        log_path = path,
    })
end

function Diagnostics.now_ms()
    return os.clock() * 1000.0
end

function Diagnostics.set_mode(mode, sequence)
    mode = tostring(mode or "unknown")
    if mode ~= current_mode then
        Diagnostics.info("MODE_CHANGE", nil, {
            previous = current_mode,
            current = mode,
            state_sequence = sequence,
        })
        current_mode = mode
    end
end

function Diagnostics.record_queue_request()
    counters.queue_requests = counters.queue_requests + 1
end

function Diagnostics.record_queue_skip(reason)
    if reason == "pending" then
        counters.queue_pending_skips = counters.queue_pending_skips + 1
    else
        counters.queue_generation_skips = counters.queue_generation_skips + 1
    end
end

function Diagnostics.record_motion_sample()
    counters.motion_samples = counters.motion_samples + 1
end

function Diagnostics.record_motion_skip()
    counters.motion_write_skips = counters.motion_write_skips + 1
end

function Diagnostics.record_motion_write(duration_ms, ok)
    counters.motion_writes = counters.motion_writes + 1
    if not ok then
        counters.motion_write_failures = counters.motion_write_failures + 1
    end
    if duration_ms ~= nil then
        duration_ms = tonumber(duration_ms) or 0.0
        counters.motion_write_total_ms =
            counters.motion_write_total_ms + duration_ms
        counters.motion_write_max_ms = math.max(
            counters.motion_write_max_ms,
            duration_ms
        )
    end
end

local function add_metric(prefix, value)
    value = tonumber(value) or 0.0
    counters[prefix .. "_total_ms"] = counters[prefix .. "_total_ms"] + value
    counters[prefix .. "_max_ms"] = math.max(counters[prefix .. "_max_ms"], value)
end

function Diagnostics.record_update(metrics)
    metrics = metrics or {}
    counters.updates = counters.updates + 1
    counters.last_state_sequence = tonumber(metrics.state_sequence)
        or counters.last_state_sequence
    local mode = tostring(metrics.mode or "unknown")
    if mode == "world" then
        counters.world_updates = counters.world_updates + 1
    elseif mode == "radar" then
        counters.radar_updates = counters.radar_updates + 1
    end
    if metrics.player_missing then
        counters.player_missing = counters.player_missing + 1
    end
    if metrics.failed then
        counters.failed_updates = counters.failed_updates + 1
    end
    if metrics.write_ok == false then
        counters.state_write_failures = counters.state_write_failures + 1
    else
        counters.state_writes = counters.state_writes + 1
    end

    add_metric("player", metrics.player_ms)
    add_metric("world", metrics.world_ms)
    add_metric("build", metrics.build_ms)
    add_metric("write", metrics.write_ms)
    add_metric("total", metrics.total_ms)
    add_metric("queue", metrics.queue_delay_ms)

    if mode == "world" and metrics.left ~= nil
        and metrics.top ~= nil and metrics.zoom ~= nil
    then
        local left = tonumber(metrics.left)
        local top = tonumber(metrics.top)
        local zoom = tonumber(metrics.zoom)
        if counters.last_world_left ~= nil
            and left ~= nil and top ~= nil and zoom ~= nil
        then
            local dx = left - counters.last_world_left
            local dy = top - counters.last_world_top
            local pan_delta = math.sqrt(dx * dx + dy * dy)
            local zoom_delta_percent = 0.0
            if math.abs(counters.last_world_zoom or 0.0) > 0.000001 then
                zoom_delta_percent = math.abs(
                    (zoom - counters.last_world_zoom)
                        / counters.last_world_zoom
                ) * 100.0
            end
            if pan_delta > 0.01 or zoom_delta_percent > 0.001 then
                counters.world_input_changed = counters.world_input_changed + 1
            else
                counters.world_input_unchanged = counters.world_input_unchanged + 1
            end
            counters.max_pan_delta = math.max(
                counters.max_pan_delta,
                pan_delta
            )
            counters.max_zoom_delta_percent = math.max(
                counters.max_zoom_delta_percent,
                zoom_delta_percent
            )
        end
        counters.last_world_left = left
        counters.last_world_top = top
        counters.last_world_zoom = zoom
    end
    Diagnostics.set_mode(mode, metrics.state_sequence)
    Diagnostics.maybe_report(mode, metrics.state_sequence)
end

local function average(total, count)
    if count == nil or count <= 0 then
        return 0.0
    end
    return total / count
end

function Diagnostics.maybe_report(mode, state_sequence)
    if not enabled then
        return
    end
    local now = os.time()
    local elapsed = now - last_perf_wall
    if elapsed < interval_seconds then
        return
    end
    if elapsed <= 0 then
        elapsed = interval_seconds
    end
    local count = math.max(1, counters.updates)
    local world_input_samples = counters.world_input_changed
        + counters.world_input_unchanged
    local world_input_changed_percent = world_input_samples > 0
        and counters.world_input_changed * 100.0 / world_input_samples
        or 0.0
    local motion_sample_count = math.max(1, counters.motion_samples)
    local motion_skip_percent = counters.motion_write_skips
        * 100.0 / motion_sample_count
    Diagnostics.event("INFO", "LUA_PERF", nil, {
        window_seconds = elapsed,
        mode = mode,
        state_sequence = state_sequence,
        update_hz = counters.updates / elapsed,
        updates = counters.updates,
        world_updates = counters.world_updates,
        radar_updates = counters.radar_updates,
        failed_updates = counters.failed_updates,
        state_writes = counters.state_writes,
        state_write_failures = counters.state_write_failures,
        queue_requests = counters.queue_requests,
        pending_skips = counters.queue_pending_skips,
        generation_skips = counters.queue_generation_skips,
        player_missing = counters.player_missing,
        player_avg_ms = average(counters.player_total_ms, count),
        player_max_ms = counters.player_max_ms,
        world_read_avg_ms = average(counters.world_total_ms, count),
        world_read_max_ms = counters.world_max_ms,
        build_avg_ms = average(counters.build_total_ms, count),
        build_max_ms = counters.build_max_ms,
        write_avg_ms = average(counters.write_total_ms, count),
        write_max_ms = counters.write_max_ms,
        total_avg_ms = average(counters.total_total_ms, count),
        total_max_ms = counters.total_max_ms,
        queue_avg_ms = average(counters.queue_total_ms, count),
        queue_max_ms = counters.queue_max_ms,
        world_input_changed = counters.world_input_changed,
        world_input_unchanged = counters.world_input_unchanged,
        world_input_change_hz = counters.world_input_changed / elapsed,
        world_input_changed_percent = world_input_changed_percent,
        max_pan_delta = counters.max_pan_delta,
        max_zoom_delta_percent = counters.max_zoom_delta_percent,
        motion_samples = counters.motion_samples,
        motion_write_skips = counters.motion_write_skips,
        motion_skip_percent = motion_skip_percent,
        motion_write_hz = counters.motion_writes / elapsed,
        motion_writes = counters.motion_writes,
        motion_write_failures = counters.motion_write_failures,
        motion_write_avg_ms = average(
            counters.motion_write_total_ms,
            math.max(1, counters.motion_writes)
        ),
        motion_write_max_ms = counters.motion_write_max_ms,
    }, true)
    last_perf_wall = now
    reset_counters()
end

return Diagnostics
