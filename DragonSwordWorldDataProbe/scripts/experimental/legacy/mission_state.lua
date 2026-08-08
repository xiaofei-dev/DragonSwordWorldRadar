local M = {}

local GLOBAL_REGISTRY_KEY = "__DSWDP_MISSION_STATE_HOOK_V1"

local function clean(value, limit)
    local text = tostring(value == nil and "" or value):gsub("[\r\n\t]+", " ")
    limit = tonumber(limit) or 800
    if #text > limit then return text:sub(1, limit) .. "..." end
    return text
end

local function unwrap_scalar(value)
    if value == nil then return nil, "nil" end
    local kind = type(value)
    if kind == "number" or kind == "string" or kind == "boolean" then
        return value, kind
    end
    local unwrapped = value
    local ok = pcall(function() unwrapped = value:get() end)
    if ok then
        local unwrapped_kind = type(unwrapped)
        if unwrapped_kind == "number" or unwrapped_kind == "string" or unwrapped_kind == "boolean" then
            return unwrapped, unwrapped_kind
        end
    end
    return nil, kind
end

local function valid(object)
    if object == nil then return false end
    local ok, result = pcall(function() return object:IsValid() end)
    return ok and result == true
end

local function read_scalar(object, name)
    local ok, value = pcall(function() return object[name] end)
    if not ok then return nil end
    local scalar = unwrap_scalar(value)
    return scalar
end

local function object_name(object)
    local name = ""
    pcall(function() name = object:GetFullName() end)
    if name == "" then pcall(function() name = object:GetName() end) end
    return clean(name, 500)
end

function M.new(options)
    options = options or {}
    local registry = rawget(_G, GLOBAL_REGISTRY_KEY)
    if type(registry) ~= "table" then
        registry = { installed = false, target = nil }
        rawset(_G, GLOBAL_REGISTRY_KEY, registry)
    end

    local self = {
        logger = nil,
        registry = registry,
        hook_path = options.update_hook_path,
        content_class = options.content_data_class or "DMercenaryGuideWeeklyMissionContentData",
        max_content_objects = math.max(32, tonumber(options.max_content_objects) or 512),
        sequence = 0,
    }

    local function on_refresh(context, ...)
        local target = self.registry.target
        if target == nil or target.logger == nil then return end
        local args = { ... }
        local type_value, type_kind = unwrap_scalar(args[1])
        local cid_value, cid_kind = unwrap_scalar(args[2])
        local cnt_value, cnt_kind = unwrap_scalar(args[3])
        target.sequence = target.sequence + 1
        target.logger:write("MISSION_INFO_REFRESH", {
            sequence = target.sequence,
            mission_type = type_value or "nil",
            mission_type_kind = type_kind,
            cid = cid_value or "nil",
            cid_kind = cid_kind,
            cnt = cnt_value or "nil",
            cnt_kind = cnt_kind,
            argument_count = select("#", ...),
            interpretation = "candidate_daily_boss_or_assault_progress_update",
        })
    end

    function self:install_hook(ctx)
        self.logger = ctx.logger
        self.registry.target = self
        local safety = ctx.config and ctx.config.safety or {}
        if safety.targeted_mission_update_hook ~= true or safety.native_hooks ~= true then
            return {
                status = "unsupported", value_type = "mission_hook",
                value = "disabled_by_safety_policy",
                fingerprint = "mission_hook:disabled",
            }
        end
        if self.registry.installed == true then
            return {
                status = "readable", value_type = "mission_hook",
                value = "already_installed_process_session",
                fingerprint = "mission_hook:installed",
            }
        end
        if type(RegisterHook) ~= "function" then
            return {
                status = "unsupported", value_type = "mission_hook",
                value = "RegisterHook_unavailable",
                fingerprint = "mission_hook:unavailable",
            }
        end
        ctx.phase("register_targeted_mission_update_hook")
        local ok, pre_id, post_id = pcall(function()
            return RegisterHook(self.hook_path, on_refresh, function() end)
        end)
        if not ok then
            ctx.log("MISSION_UPDATE_HOOK_FAILED", {
                path = self.hook_path, error = clean(pre_id, 1000),
            })
            return {
                status = "call_failed", value_type = "mission_hook",
                value = clean(pre_id, 400),
                fingerprint = "mission_hook:failed:" .. clean(pre_id, 200),
            }
        end
        self.registry.installed = true
        self.registry.pre_id = pre_id
        self.registry.post_id = post_id
        ctx.log("MISSION_UPDATE_HOOK_REGISTERED", {
            path = self.hook_path,
            pre_id = pre_id or "nil",
            post_id = post_id or "nil",
        })
        return {
            status = "readable", value_type = "mission_hook",
            value = "registered",
            fingerprint = "mission_hook:installed",
        }
    end

    function self:scan_content_data(ctx)
        if type(FindAllOf) ~= "function" then
            return {
                status = "unsupported", value_type = "mission_content_scan",
                value = "FindAllOf_unavailable",
                fingerprint = "mission_content_scan:unavailable",
            }
        end
        ctx.phase("find_all_mission_content_data")
        local ok, objects = pcall(function() return FindAllOf(self.content_class) end)
        if not ok or type(objects) ~= "table" then
            return {
                status = "call_failed", value_type = "mission_content_scan",
                value = clean(objects, 400),
                fingerprint = "mission_content_scan:failed:" .. clean(objects, 200),
            }
        end

        local fields = {
            "Cid", "CID", "MissionCid", "MissionCID", "QuestCid", "QuestCID",
            "Type", "MissionType", "Cnt", "Count", "CurrentCnt", "CurrentCount",
            "TargetCnt", "TargetCount", "RequiredCnt", "RequiredCount",
            "bComplete", "bCompleted", "IsComplete", "IsCompleted",
            "bClear", "bCleared", "Clear", "Completed", "State", "Status"
        }
        local valid_count, scalar_rows = 0, 0
        local fingerprints = {}
        for index, object in ipairs(objects) do
            if index > self.max_content_objects then break end
            if valid(object) then
                valid_count = valid_count + 1
                local row = { object = object_name(object) }
                local has_scalar = false
                for _, field in ipairs(fields) do
                    local value = read_scalar(object, field)
                    if value ~= nil then
                        row[field] = value
                        has_scalar = true
                    end
                end
                if has_scalar then
                    scalar_rows = scalar_rows + 1
                    ctx.log("MISSION_CONTENT_STATE", row)
                    local key = tostring(row.Cid or row.CID or row.MissionCid or row.MissionCID
                        or row.QuestCid or row.QuestCID or row.object)
                    local cnt = tostring(row.Cnt or row.Count or row.CurrentCnt or row.CurrentCount or "")
                    local completed = tostring(row.bComplete or row.bCompleted or row.IsComplete
                        or row.IsCompleted or row.bClear or row.bCleared or row.Completed or "")
                    fingerprints[#fingerprints + 1] = key .. ":" .. cnt .. ":" .. completed
                end
            end
        end
        table.sort(fingerprints)
        local fingerprint = table.concat(fingerprints, "|")
        if #fingerprint > 3000 then fingerprint = fingerprint:sub(1, 3000) end
        ctx.log("MISSION_CONTENT_SCAN_COMPLETE", {
            returned = #objects,
            valid = valid_count,
            scalar_rows = scalar_rows,
            class = self.content_class,
        })
        return {
            status = "readable",
            value_type = "mission_content_scan",
            value = "returned=" .. tostring(#objects)
                .. ",valid=" .. tostring(valid_count)
                .. ",scalar_rows=" .. tostring(scalar_rows),
            fingerprint = "mission_content:" .. fingerprint,
        }
    end

    function self:steps()
        return {
            {
                name = "install_targeted_mission_progress_hook",
                kind = "custom",
                role = "global_daily_mission_progress",
                purpose = "capture_Refresh_Mercenary_MissionInfo_Type_Cid_Cnt",
                once_per_session = true,
                run = function(ctx) return self:install_hook(ctx) end,
            },
            {
                name = "scan_mission_content_state",
                kind = "custom",
                role = "global_daily_mission_state",
                purpose = "extract_Cid_count_and_completion_candidates_from_all_mission_content_objects",
                cadence = 1,
                run = function(ctx) return self:scan_content_data(ctx) end,
            },
        }
    end

    return self
end

return M
