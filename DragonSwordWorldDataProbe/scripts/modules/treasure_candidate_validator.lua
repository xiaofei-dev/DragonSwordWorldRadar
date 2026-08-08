local source = debug.getinfo(1, "S").source
local script_file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local module_dir = script_file and script_file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir = scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local config = dofile(scripts_dir .. "\\treasure_candidate.lua")
local report_path = mod_dir .. "\\runtime\\reports\\treasure-candidate-validation.tsv"

local function valid(obj)
    if obj == nil then return false end
    local ok, result = pcall(function() return obj:IsValid() end)
    return ok and result == true
end

local function write_rows(rows)
    local file = io.open(report_path, "w")
    if not file then return false end
    file:write("SaveID\tGroundTruth\tResult\tMatch\tFunctionPath\n")
    for _, row in ipairs(rows) do
        file:write(
            tostring(row.id), "\t",
            tostring(row.truth), "\t",
            tostring(row.result), "\t",
            tostring(row.match), "\t",
            tostring(config.function_path), "\n"
        )
    end
    file:close()
    return true
end

local function run_validation(ctx)
    if config.enabled ~= true then
        return {
            status = "complete",
            value_type = "disabled",
            value = "candidate_not_selected",
            fingerprint = "treasure_validator:disabled",
        }
    end

    if config.function_path == ""
        or config.call_shape == ""
        or #config.known_opened_ids == 0
        or #config.known_unopened_ids == 0
    then
        return {
            status = "unsupported",
            value_type = "configuration",
            value = "both_opened_and_unopened_ground_truth_are_required",
            fingerprint = "treasure_validator:invalid_config",
        }
    end

    -- Refuse obviously mutating names even if the config is edited incorrectly.
    local lower = string.lower(config.function_path)
    local query_prefix =
        lower:find(":cis", 1, true)
        or lower:find(":is", 1, true)
        or lower:find(":has", 1, true)
        or lower:find(":can", 1, true)
        or lower:find(":check", 1, true)
        or lower:find(":get", 1, true)

    if not query_prefix then
        return {
            status = "unsupported",
            value_type = "safety",
            value = "candidate_name_is_not_query_shaped",
            fingerprint = "treasure_validator:unsafe_name",
        }
    end

    local owner = StaticFindObject(config.owner_cdo_path)
    local fn = StaticFindObject(config.function_path)
    local world = FindFirstOf("DClientQuestSystem")

    if not valid(owner) or not valid(fn) then
        return {
            status = "unavailable",
            value_type = "handles",
            value = "candidate_handles_not_ready",
            fingerprint = "treasure_validator:no_handles",
        }
    end

    if config.call_shape ~= "id_only" and not valid(world) then
        return {
            status = "unavailable",
            value_type = "world_context",
            value = "DClientQuestSystem_not_ready",
            fingerprint = "treasure_validator:no_world",
        }
    end

    local rows = {}

    local function call_id(id)
        if config.call_shape == "world_id" then
            return fn(owner, world, id)
        elseif config.call_shape == "id_only" then
            return fn(owner, id)
        elseif config.call_shape == "world_id_bool" then
            return fn(owner, world, id, config.extra_bool == true)
        end
        return nil
    end

    local function test(ids, truth)
        for _, id in ipairs(ids) do
            local result = call_id(id)
            if type(result) ~= "boolean" then
                return false, "non_boolean_return"
            end
            rows[#rows + 1] = {
                id = id,
                truth = truth,
                result = result,
                match = result == truth,
            }
        end
        return true, ""
    end

    local ok_opened, err_opened =
        test(config.known_opened_ids, config.expected.opened == true)
    if not ok_opened then
        return {
            status = "unsupported",
            value_type = "return_type",
            value = err_opened,
            fingerprint = "treasure_validator:bad_opened_return",
        }
    end

    local ok_unopened, err_unopened =
        test(config.known_unopened_ids, config.expected.unopened == true)
    if not ok_unopened then
        return {
            status = "unsupported",
            value_type = "return_type",
            value = err_unopened,
            fingerprint = "treasure_validator:bad_unopened_return",
        }
    end

    local matches = 0
    for _, row in ipairs(rows) do
        if row.match then matches = matches + 1 end
    end

    local wrote = write_rows(rows)
    local all_match = #rows > 0 and matches == #rows

    ctx.log("TREASURE_CANDIDATE_VALIDATION", {
        function_path = config.function_path,
        samples = #rows,
        matches = matches,
        all_match = all_match,
        report_written = wrote,
    })

    return {
        status = all_match and "ok" or "partial",
        value_type = "match_count",
        value = tostring(matches) .. "/" .. tostring(#rows),
        fingerprint = "treasure_validator:" .. tostring(matches) .. ":" .. tostring(#rows),
    }
end

return {
    id = "treasure_candidate_validator",
    enabled = config.enabled == true,
    description = "Exact-candidate validator; disabled until both opened and unopened ground truth are supplied.",
    steps = {
        {
            kind = "custom",
            name = "treasure_exact_candidate_validation",
            class = "<configured>",
            role = "treasure_candidate_validation",
            purpose = "ground_truth_bool_validation",
            cadence = 1,
            once_per_session = true,
            run = run_validation,
        },
    },
}
