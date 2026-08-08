local M = {}

local function clean(value, limit)
    local text = tostring(value == nil and "" or value):gsub("[\r\n\t]+", " ")
    limit = tonumber(limit) or 1000
    if #text > limit then return text:sub(1, limit) .. "..." end
    return text
end

local function valid(object)
    if object == nil then return false end
    local ok, result = pcall(function() return object:IsValid() end)
    return ok and result == true
end

local function full_name(object)
    if object == nil then return "nil" end
    local name = ""
    pcall(function() name = object:GetFullName() end)
    if name == "" then pcall(function() name = object:GetName() end) end
    return clean(name ~= "" and name or "<object>", 700)
end

local function class_name(object)
    if object == nil then return "nil" end
    local name = ""
    pcall(function()
        local cls = object:GetClass()
        if cls ~= nil then name = cls:GetFullName() end
    end)
    return clean(name ~= "" and name or "<unknown-class>", 500)
end

local function unwrap_scalar(value)
    if value == nil then return nil, "nil" end
    local kind = type(value)
    if kind == "string" or kind == "number" or kind == "boolean" then
        return value, kind
    end
    local ok, got = pcall(function() return value:get() end)
    if ok then
        local got_kind = type(got)
        if got_kind == "string" or got_kind == "number" or got_kind == "boolean" then
            return got, got_kind
        end
    end
    return nil, kind
end

local function read_property(object, name)
    local ok, value = pcall(function() return object[name] end)
    if not ok or value == nil then return nil end
    return value
end

local function call_noarg(object, method)
    local ok, value = pcall(function() return object[method](object) end)
    if not ok then ok, value = pcall(function() return object[method]() end) end
    if not ok then return nil end
    return value
end

local function container_count(container)
    if container == nil then return nil end
    if type(container) == "table" then return #container end
    local methods = { "Num", "Length", "Count", "GetNumItems" }
    for _, method in ipairs(methods) do
        local value = call_noarg(container, method)
        local scalar = unwrap_scalar(value)
        if type(scalar) == "number" then return scalar end
    end
    local ok, count = pcall(function() return #container end)
    if ok and type(count) == "number" then return count end
    return nil
end

local function container_item(container, index)
    if container == nil then return nil end
    if type(container) == "table" then
        return container[index] or container[index + 1]
    end
    local methods = { "Get", "GetItemAt", "GetItem", "At" }
    for _, method in ipairs(methods) do
        local ok, value = pcall(function() return container[method](container, index) end)
        if not ok then ok, value = pcall(function() return container[method](index) end) end
        if ok and value ~= nil then return value end
    end
    return nil
end

local scalar_names = {
    "ID", "Id", "Cid", "CID", "MissionId", "MissionID", "MissionCid", "MissionCID",
    "QuestId", "QuestID", "QuestCid", "QuestCID", "TargetId", "TargetID",
    "TargetCid", "TargetCID", "MonsterId", "MonsterID", "MonsterCid", "MonsterCID",
    "BossId", "BossID", "BossCid", "BossCID", "GroupId", "GroupID",
    "Type", "MissionType", "QuestType", "ContentType", "ChallengeType", "Category",
    "Cnt", "Count", "CurrentCnt", "CurrentCount", "Progress", "CurrentProgress",
    "TargetCnt", "TargetCount", "RequiredCnt", "RequiredCount", "MaxCount",
    "bComplete", "bCompleted", "IsComplete", "IsCompleted", "Completed",
    "bClear", "bCleared", "IsClear", "IsCleared", "Clear",
    "bFinish", "bFinished", "IsFinish", "IsFinished", "Finished",
    "State", "Status", "Result", "RewardState", "bRewarded", "IsRewarded",
    "Day", "ResetDay", "Week", "Index", "ItemIndex", "EntryIndex",
}

local container_names = {
    "MissionInfoMap", "MissionInfoList", "MissionInfos", "MissionList", "Missions",
    "DailyMissionMap", "DailyMissionList", "DailyMissions",
    "WeeklyMissionMap", "WeeklyMissionList", "WeeklyMissions",
    "MercenaryMissionInfo", "MercenaryMissionInfoMap", "MercenaryMissionInfoList",
    "MercenaryDailyMissionMap", "MercenaryDailyMissionList",
    "QuestMap", "QuestList", "QuestInfos", "QuestInfoMap",
    "CompletedMissionMap", "CompletedMissionList", "CompletedMissions",
    "CompleteMissionMap", "CompleteMissionList", "ClearedMissionMap", "ClearedMissionList",
    "FinishedMissionMap", "FinishedMissionList", "FinishedMissions",
    "MissionProgressMap", "MissionProgressList", "ProgressMap", "ProgressList",
    "MissionDataMap", "MissionDataList", "ContentDataMap", "ContentDataList",
    "Items", "ListItems", "ItemList", "DataList", "DataMap",
}

local object_names = {
    "MissionController", "ClientMissionController", "QuestSystem", "ClientQuestSystem",
    "MissionData", "MissionInfo", "QuestData", "QuestInfo", "ContentData",
    "DataProvider", "ViewModel", "Model", "EntryData", "ItemData", "ListItemObject",
}

local list_methods = {
    "GetListItems", "GetAllItems", "GetItems", "GetMissionList", "GetMissionInfos",
    "GetDailyMissionList", "GetWeeklyMissionList", "GetCompletedMissionList",
}

local function add_scalar(row, fp, key, value)
    local scalar, kind = unwrap_scalar(value)
    if scalar == nil then return false end
    row[key] = clean(scalar, 600)
    row[key .. "_kind"] = kind
    fp[#fp + 1] = key .. "=" .. clean(scalar, 300)
    return true
end

local function scan_record(ctx, event_name, object, prefix, depth)
    if object == nil then return "nil" end
    local row = {
        prefix = prefix,
        depth = depth,
        object = valid(object) and full_name(object) or "<non-uobject>",
        object_class = valid(object) and class_name(object) or type(object),
    }
    local fp = { row.object, row.object_class }
    local scalar_count = 0

    if valid(object) then
        for _, name in ipairs(scalar_names) do
            if add_scalar(row, fp, "prop_" .. name, read_property(object, name)) then
                scalar_count = scalar_count + 1
            end
        end
    end

    row.scalar_count = scalar_count
    ctx.log(event_name, row)
    table.sort(fp)
    return table.concat(fp, "|")
end

function M.new(options)
    options = options or {}
    local self = {
        controller_classes = options.controller_classes or {},
        list_view_owner_class = options.list_view_owner_class or "DLayerMercenaryGuideWeeklyMission_C",
        list_view_property = options.list_view_property or "DListView_Content",
        max_objects_per_class = math.max(1, tonumber(options.max_objects_per_class) or 16),
        max_container_items = math.max(16, tonumber(options.max_container_items) or 512),
        max_nested_depth = math.max(1, tonumber(options.max_nested_depth) or 2),
        scan_sequence = 0,
    }

    local function dump_container(ctx, owner, owner_label, source_name, container, all_fp, depth)
        local count = container_count(container)
        ctx.log("MISSION_CONTAINER_STATE", {
            scan_sequence = self.scan_sequence,
            phase = ctx.phase_name,
            owner = owner_label,
            owner_class = valid(owner) and class_name(owner) or type(owner),
            source = source_name,
            container_type = type(container),
            count = count == nil and "unknown" or count,
            depth = depth,
        })
        if count == nil then return end
        local limit = math.min(count, self.max_container_items)
        for index = 0, limit - 1 do
            local item = container_item(container, index)
            if item ~= nil then
                local prefix = owner_label .. "." .. source_name .. "[" .. tostring(index) .. "]"
                local fp = scan_record(ctx, "MISSION_CONTAINER_ITEM_STATE", item, prefix, depth)
                all_fp[#all_fp + 1] = prefix .. ":" .. fp
                if depth < self.max_nested_depth and valid(item) then
                    for _, nested_name in ipairs(container_names) do
                        local nested = read_property(item, nested_name)
                        if nested ~= nil then
                            dump_container(ctx, item, prefix, nested_name, nested, all_fp, depth + 1)
                        end
                    end
                end
            end
        end
    end

    local function detect_phase()
        if type(FindAllOf) ~= "function" then return "unknown" end
        local ok, layers = pcall(function() return FindAllOf(self.list_view_owner_class) end)
        if ok and type(layers) == "table" then
            for _, layer in ipairs(layers) do
                if valid(layer) then return "task_page_loaded" end
            end
        end
        return "world_interface"
    end

    function self:scan(ctx)
        if type(FindAllOf) ~= "function" then
            return {
                status = "unsupported",
                value_type = "global_mission_state_scan",
                value = "FindAllOf_unavailable",
                fingerprint = "global_mission:unavailable",
            }
        end

        self.scan_sequence = self.scan_sequence + 1
        ctx.phase_name = detect_phase()
        ctx.log("MISSION_GLOBAL_SCAN_BEGIN", {
            scan_sequence = self.scan_sequence,
            phase = ctx.phase_name,
            controller_classes = table.concat(self.controller_classes, ","),
        })

        local all_fp = { "phase=" .. ctx.phase_name }
        local object_total, container_total = 0, 0

        for _, class_label in ipairs(self.controller_classes) do
            local ok, objects = pcall(function() return FindAllOf(class_label) end)
            if ok and type(objects) == "table" then
                local seen = 0
                for index, object in ipairs(objects) do
                    if seen >= self.max_objects_per_class then break end
                    if valid(object) then
                        seen = seen + 1
                        object_total = object_total + 1
                        local owner_label = class_label .. "#" .. tostring(index)
                        local fp = scan_record(ctx, "MISSION_GLOBAL_OBJECT_STATE", object, owner_label, 0)
                        all_fp[#all_fp + 1] = owner_label .. ":" .. fp

                        for _, object_name in ipairs(object_names) do
                            local child = read_property(object, object_name)
                            if child ~= nil and valid(child) then
                                local child_label = owner_label .. "." .. object_name
                                local child_fp = scan_record(ctx, "MISSION_GLOBAL_CHILD_STATE", child, child_label, 1)
                                all_fp[#all_fp + 1] = child_label .. ":" .. child_fp
                            end
                        end

                        for _, source_name in ipairs(container_names) do
                            local container = read_property(object, source_name)
                            if container ~= nil then
                                container_total = container_total + 1
                                dump_container(ctx, object, owner_label, source_name, container, all_fp, 1)
                            end
                        end

                        for _, method in ipairs(list_methods) do
                            local container = call_noarg(object, method)
                            if container ~= nil then
                                container_total = container_total + 1
                                dump_container(ctx, object, owner_label, "method_" .. method, container, all_fp, 1)
                            end
                        end

                        if class_label == self.list_view_owner_class then
                            local list_view = read_property(object, self.list_view_property)
                            if list_view ~= nil and valid(list_view) then
                                local list_owner = owner_label .. "." .. self.list_view_property
                                local list_count = call_noarg(list_view, "GetNumItems")
                                local scalar = unwrap_scalar(list_count)
                                ctx.log("MISSION_LISTVIEW_STATE", {
                                    scan_sequence = self.scan_sequence,
                                    phase = ctx.phase_name,
                                    owner = owner_label,
                                    list_view = full_name(list_view),
                                    item_count = scalar or "unknown",
                                })
                                local items = call_noarg(list_view, "GetListItems")
                                if items ~= nil then
                                    dump_container(ctx, list_view, list_owner, "GetListItems", items, all_fp, 1)
                                end
                            end
                        end
                    end
                end
            else
                ctx.log("MISSION_GLOBAL_CLASS_SCAN_FAILED", {
                    scan_sequence = self.scan_sequence,
                    phase = ctx.phase_name,
                    class = class_label,
                    error = clean(objects, 1000),
                })
            end
        end

        table.sort(all_fp)
        local fingerprint = table.concat(all_fp, "||")
        if #fingerprint > 24000 then fingerprint = fingerprint:sub(1, 24000) end

        ctx.log("MISSION_GLOBAL_SCAN_COMPLETE", {
            scan_sequence = self.scan_sequence,
            phase = ctx.phase_name,
            object_total = object_total,
            container_total = container_total,
        })

        return {
            status = "readable",
            value_type = "global_mission_state_scan",
            value = "phase=" .. ctx.phase_name
                .. ",objects=" .. tostring(object_total)
                .. ",containers=" .. tostring(container_total),
            fingerprint = "global_mission:" .. fingerprint,
        }
    end

    function self:steps()
        return {
            {
                name = "scan_global_task_state_and_list_sources",
                kind = "custom",
                role = "persistent_local_task_completion",
                purpose = "compare_world_interface_task_page_and_post_completion_local_state",
                cadence = 1,
                run = function(ctx) return self:scan(ctx) end,
            },
        }
    end

    return self
end

return M
