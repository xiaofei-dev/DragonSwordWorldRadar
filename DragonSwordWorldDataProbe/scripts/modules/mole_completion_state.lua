local source = debug.getinfo(1, "S").source
local script_file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local module_dir = script_file and script_file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir = scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local report_path = mod_dir .. "\\runtime\\reports\\mole-completion-state.tsv"

local util, world_context, clear_fn = nil, nil, nil
local canary_done, canary_ok, bulk_done = false, false, false
local result_count = 0
local results = {}

local function valid(obj)
    if obj == nil then return false end
    local ok, result = pcall(function() return obj:IsValid() end)
    return ok and result == true
end

local function ensure_handles(ctx)
    if valid(util) and valid(world_context) and valid(clear_fn) then return true end

    ctx.phase("find_world_context")
    world_context = FindFirstOf("DClientQuestSystem")
    if not valid(world_context) then return false end

    ctx.phase("find_detutil_cdo")
    util = StaticFindObject("/Script/DS.Default__DETUtil")
    if not valid(util) then return false end

    ctx.phase("find_clear_function")
    clear_fn = StaticFindObject("/Script/DS.DETUtil:CIsClearMiniGameInStandAlone")
    if not valid(clear_fn) then return false end

    return true
end

local function query_id(id)
    local value = clear_fn(util, world_context, id)
    if type(value) ~= "boolean" then return nil end
    if results[id] == nil then result_count = result_count + 1 end
    results[id] = value
    return value
end

local function write_report()
    local file = io.open(report_path, "w")
    if not file then return false end

    file:write("MiniGameID\tCompleted\tSource\n")
    for id = 12001, 12040 do
        local value = results[id]
        if type(value) == "boolean" then
            file:write(
                tostring(id), "\t",
                value and "true" or "false", "\t",
                "DETUtil.CIsClearMiniGameInStandAlone\n"
            )
        end
    end

    file:close()
    return true
end

local function run_canary(ctx)
    if canary_done then
        return {
            status = canary_ok and "ok" or "failed",
            value_type = "safety_gate",
            value = canary_ok and "passed" or "failed",
            fingerprint = "mole_canary:" .. tostring(canary_ok),
        }
    end

    if not ensure_handles(ctx) then
        return {
            status = "unavailable",
            value_type = "handles",
            value = "completion_query_not_ready",
            fingerprint = "mole_canary:no_handles",
        }
    end

    ctx.phase("canary_12001")
    local value = query_id(12001)

    canary_done = true
    canary_ok = type(value) == "boolean"

    ctx.log("MOLE_CANARY_RESULT", {
        mini_game_id = 12001,
        completed = value,
        canary_ok = canary_ok,
        external_process = false,
    })

    return {
        status = canary_ok and "ok" or "failed",
        value_type = type(value),
        value = canary_ok and tostring(value) or "bad_return",
        fingerprint = "mole_canary:" .. tostring(canary_ok),
    }
end

local function run_bulk(ctx)
    if bulk_done then
        return {
            status = "complete",
            value_type = "row_count",
            value = tostring(result_count),
            fingerprint = "mole_bulk:" .. tostring(result_count),
        }
    end

    if not canary_ok then
        return {
            status = "deferred",
            value_type = "safety_gate",
            value = "waiting_for_canary",
            fingerprint = "mole_bulk:deferred",
        }
    end

    if not ensure_handles(ctx) then
        return {
            status = "unavailable",
            value_type = "handles",
            value = "completion_query_not_ready",
            fingerprint = "mole_bulk:no_handles",
        }
    end

    ctx.phase("bulk_12002_12040")
    local failed_id = nil

    for id = 12002, 12040 do
        local value = query_id(id)
        if type(value) ~= "boolean" then
            failed_id = id
            break
        end
    end

    local wrote = write_report()

    if failed_id == nil and result_count == 40 then
        bulk_done = true
        ctx.log("MOLE_COMPLETION_SCAN_COMPLETE", {
            row_count = result_count,
            report_written = wrote,
            external_process = false,
        })
    else
        ctx.log("MOLE_COMPLETION_SCAN_PARTIAL", {
            row_count = result_count,
            failed_id = failed_id,
            report_written = wrote,
            external_process = false,
        })
    end

    return {
        status = bulk_done and "ok" or "partial",
        value_type = "row_count",
        value = tostring(result_count),
        fingerprint = "mole_bulk:" .. tostring(result_count),
    }
end

return {
    id = "mole_completion_state",
    enabled = true,
    description = "Read-only Mole completion query: one canary then one bulk pass.",
    steps = {
        {
            kind = "custom",
            name = "mole_completion_canary_12001",
            class = "<DETUtil>",
            role = "mole_completion_state",
            purpose = "read_only_canary",
            cadence = 1,
            once_per_session = true,
            run = run_canary,
        },
        {
            kind = "custom",
            name = "mole_completion_bulk_12002_12040",
            class = "<DETUtil>",
            role = "mole_completion_state",
            purpose = "read_only_bulk_completion_query",
            cadence = 1,
            once_per_session = true,
            run = run_bulk,
        },
    },
}
