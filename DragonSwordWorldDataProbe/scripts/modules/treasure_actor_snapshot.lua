local source = debug.getinfo(1, "S").source
local file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local module_dir = file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local common = dofile(scripts_dir .. "\\core\\treasure_state_common.lua")

local current_path = common.report_dir .. "\\treasure-actor-current.tsv"
local history_path = common.report_dir .. "\\treasure-actor-history.tsv"

local function run(ctx)
    if type(FindAllOf) ~= "function" then
        return {
            status = "unsupported",
            value_type = "FindAllOf",
            value = "FindAllOf_unavailable",
            fingerprint = "treasure_snapshot:no_findall",
        }
    end

    ctx.phase("find_all_DsAnimationProp")

    local ok, objects = pcall(function()
        return FindAllOf("DsAnimationProp")
    end)

    if not ok or type(objects) ~= "table" then
        return {
            status = "unavailable",
            value_type = "FindAllOf",
            value = tostring(objects),
            fingerprint = "treasure_snapshot:findall_failed",
        }
    end

    local rows = {}
    local current = {}
    local scanned = 0
    local limit = math.min(#objects, 512)

    for index = 1, limit do
        local object = objects[index]
        if common.valid(object) then
            scanned = scanned + 1
            local row = common.snapshot(object)
            if row and row.TreasureCandidate == true then
                row.Event = "current"
                row.HookPhase = ""
                row.HookPath = ""
                rows[#rows + 1] = row
                current[row.Key] = row
            end
        end
    end

    table.sort(rows, function(a, b)
        return tostring(a.Key) < tostring(b.Key)
    end)

    common.write_rows(current_path, rows)

    local changes = 0

    for key, row in pairs(current) do
        local previous = common.shared.previous[key]
        if previous == nil then
            row.Event = "appeared"
            common.append_row(history_path, row)
            changes = changes + 1
        elseif previous.Fingerprint ~= row.Fingerprint then
            row.Event = "state_changed"
            common.append_row(history_path, row)
            changes = changes + 1
        end
    end

    for key, previous in pairs(common.shared.previous) do
        if current[key] == nil then
            local row = {}
            for k, value in pairs(previous) do row[k] = value end
            row.UTC = os.date("!%Y-%m-%dT%H:%M:%SZ")
            row.Event = "disappeared_or_streamed"
            common.append_row(history_path, row)
            changes = changes + 1
        end
    end

    common.shared.previous = current

    local fingerprints = {}
    for _, row in ipairs(rows) do
        fingerprints[#fingerprints + 1] = row.Key .. "=" .. row.Fingerprint
    end
    table.sort(fingerprints)

    ctx.log("TREASURE_ACTOR_SNAPSHOT", {
        returned = #objects,
        scanned = scanned,
        treasure_candidates = #rows,
        changes = changes,
        targeted_class = "DsAnimationProp",
        global_actor_scan = false,
    })

    return {
        status = "ok",
        value_type = "candidate_count",
        value = tostring(#rows),
        fingerprint = "treasure_snapshot:" .. table.concat(fingerprints, "||"),
    }
end

return {
    id = "treasure_actor_snapshot",
    enabled = true,
    description = "Targeted DsAnimationProp snapshot matched to the 1693-point treasure catalog.",
    steps = {
        {
            kind = "custom",
            name = "treasure_animation_prop_snapshot",
            class = "DsAnimationProp",
            role = "treasure_state_capture",
            purpose = "targeted_class_snapshot",
            cadence = 1,
            run = run,
        },
    },
}
