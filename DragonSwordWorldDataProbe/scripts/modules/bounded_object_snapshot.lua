local source = debug.getinfo(1, "S").source
local scripts_dir = source:sub(2):match("^(.*)[/\\]modules[/\\][^/\\]+$") or "."
local config = dofile(scripts_dir .. "\\config.lua")
local settings = config.module_settings and config.module_settings.bounded_object_snapshot or {}

local function valid(object)
    if object == nil then return false end
    local ok, value = pcall(function() return object:IsValid() end)
    return ok and value == true
end
local function scalar(value)
    local kind = type(value)
    if kind == "string" or kind == "number" or kind == "boolean" then return tostring(value) end
    if value ~= nil then
        local ok, got = pcall(function() return value:get() end)
        if ok then
            local got_kind = type(got)
            if got_kind == "string" or got_kind == "number" or got_kind == "boolean" then return tostring(got) end
        end
    end
    return nil
end
local function object_name(object)
    local value = ""
    pcall(function() value = object:GetFullName() end)
    if value == "" then pcall(function() value = object:GetName() end) end
    return tostring(value)
end

local steps = {}
if config.safety.find_all_of == true then
    for index, target in ipairs(settings.targets or {}) do
        if type(target) == "table" and type(target.class) == "string" and target.class ~= "" then
            steps[#steps + 1] = {
                name = target.name or ("bounded_snapshot_" .. tostring(index)),
                kind = "custom", role = "bounded_object_snapshot",
                purpose = target.purpose or "explicit_whitelist_bounded_FindAllOf",
                cadence = target.cadence or 1,
                run = function(ctx)
                    if type(FindAllOf) ~= "function" then
                        return { status="unsupported", value_type="FindAllOf", value="unavailable", fingerprint="findall:unavailable" }
                    end
                    ctx.phase("find_all_of_" .. target.class)
                    local ok, objects = pcall(function() return FindAllOf(target.class) end)
                    if not ok or type(objects) ~= "table" then
                        return { status="call_failed", value_type="FindAllOf", value=tostring(objects), fingerprint="findall:failed" }
                    end
                    local limit = math.max(1, tonumber(target.max_objects or settings.default_max_objects) or 64)
                    local fields = target.fields or settings.default_fields or {}
                    local samples, fingerprints = 0, {}
                    for i, object in ipairs(objects) do
                        if samples >= limit then break end
                        if valid(object) then
                            samples = samples + 1
                            local row = { class=target.class, index=i, object=object_name(object) }
                            local fp = { row.object }
                            for _, field in ipairs(fields) do
                                local ok_read, raw = pcall(function() return object[field] end)
                                if ok_read then
                                    local value = scalar(raw)
                                    if value ~= nil then row[field] = value; fp[#fp+1] = field .. "=" .. value end
                                end
                            end
                            ctx.log("GENERIC_OBJECT_SAMPLE", row)
                            table.sort(fp); fingerprints[#fingerprints+1] = table.concat(fp, "|")
                        end
                    end
                    table.sort(fingerprints)
                    return {
                        status="readable", value_type="bounded_object_snapshot",
                        value="returned=" .. tostring(#objects) .. ",sampled=" .. tostring(samples),
                        fingerprint=target.class .. ":" .. table.concat(fingerprints, "||")
                    }
                end,
            }
        end
    end
end
return { id="bounded_object_snapshot", enabled=#steps > 0, steps=steps }
