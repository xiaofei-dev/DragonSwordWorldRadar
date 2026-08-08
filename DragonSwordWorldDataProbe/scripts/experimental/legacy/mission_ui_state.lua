local M = {}

local function clean(value, limit)
    local text = tostring(value == nil and "" or value):gsub("[\r\n\t]+", " ")
    limit = tonumber(limit) or 800
    if #text > limit then return text:sub(1, limit) .. "..." end
    return text
end

local function valid(object)
    if object == nil then return false end
    local ok, result = pcall(function() return object:IsValid() end)
    return ok and result == true
end

local function unwrap(value)
    if value == nil then return nil, "nil" end
    local kind = type(value)
    if kind == "string" or kind == "number" or kind == "boolean" then return value, kind end
    local ok_get, got = pcall(function() return value:get() end)
    if ok_get then
        local got_kind = type(got)
        if got_kind == "string" or got_kind == "number" or got_kind == "boolean" then
            return got, got_kind
        end
    end
    return value, kind
end

local function full_name(object)
    if object == nil then return "nil" end
    local name = ""
    pcall(function() name = object:GetFullName() end)
    if name == "" then pcall(function() name = object:GetName() end) end
    return clean(name ~= "" and name or "<object>", 600)
end

local function class_name(object)
    if object == nil then return "nil" end
    local name = ""
    pcall(function()
        local class = object:GetClass()
        if class ~= nil then name = class:GetFullName() end
    end)
    return clean(name ~= "" and name or "<unknown-class>", 400)
end

local function describe(value)
    if value == nil then return nil, "nil" end
    local scalar, kind = unwrap(value)
    if kind == "string" or kind == "number" or kind == "boolean" then
        return clean(scalar, 500), kind
    end
    if valid(scalar) then return full_name(scalar), "uobject" end
    return "<" .. tostring(kind) .. ">", kind
end

local function read_property(object, property)
    local ok, value = pcall(function() return object[property] end)
    if not ok or value == nil then return nil end
    local text, kind = describe(value)
    return { text = text, kind = kind, raw = value }
end

local function call_noarg(object, method)
    local ok, value = pcall(function() return object[method](object) end)
    if not ok then
        ok, value = pcall(function() return object[method]() end)
    end
    if not ok then return nil end
    local text, kind = describe(value)
    return { text = text, kind = kind, raw = value }
end

local function read_child_method(object, child_name, method)
    local child = read_property(object, child_name)
    if child == nil or not valid(child.raw) then return nil end
    return call_noarg(child.raw, method)
end

local function add_value(row, fingerprint, key, result)
    if result == nil or result.text == nil then return false end
    row[key] = result.text
    row[key .. "_kind"] = result.kind
    fingerprint[#fingerprint + 1] = key .. "=" .. result.text
    return true
end

function M.new(options)
    options = options or {}
    local self = {
        entry_classes = options.entry_classes or {
            "DUWG_MercenaryGuideSpecialChallenge_C",
            "DUWG_MercenaryDaily_C",
            "DUWG_MercenaryGuideTainingTask_C",
        },
        content_classes = options.content_classes or {
            "DMercenaryGuideWeeklyMissionContentData",
        },
        max_objects = math.max(32, tonumber(options.max_objects) or 512),
    }

    local scalar_properties = {
        "Cid", "CID", "ID", "Id", "MissionId", "MissionID", "MissionCid", "MissionCID",
        "QuestId", "QuestID", "QuestCid", "QuestCID", "ContentId", "ContentID",
        "TargetId", "TargetID", "TargetCid", "TargetCID", "MonsterId", "MonsterID",
        "MonsterCid", "MonsterCID", "BossId", "BossID", "BossCid", "BossCID",
        "Type", "MissionType", "ContentType", "ChallengeType", "Category",
        "Cnt", "Count", "CurrentCnt", "CurrentCount", "Progress", "CurrentProgress",
        "TargetCnt", "TargetCount", "RequiredCnt", "RequiredCount", "MaxCount",
        "bComplete", "bCompleted", "IsComplete", "IsCompleted", "Completed",
        "bClear", "bCleared", "IsClear", "IsCleared", "Clear", "State", "Status",
        "bReward", "bRewarded", "Reward", "RewardState", "bReceived", "IsReceived",
        "Index", "ItemIndex", "EntryIndex", "SelectedIndex",
    }

    local object_properties = {
        "ListItemObject", "ItemObject", "EntryObject", "EntryData", "ItemData", "ContentData",
        "MissionData", "MissionInfo", "MissionInfoData", "QuestData", "ChallengeData",
        "WeeklyMissionData", "SpecialChallengeData", "MercenaryMissionData",
        "Data", "Info", "Model", "ViewModel",
    }

    local object_methods = {
        "GetListItemObject", "GetItemObject", "GetEntryObject", "GetEntryData", "GetItemData",
        "GetContentData", "GetMissionData", "GetMissionInfo", "GetQuestData",
    }

    local text_children = {
        "Text_Name", "Text_Title", "Text_Mission", "Text_Desc", "Text_Description",
        "Text_Progress", "Text_Count", "Text_Num", "Text_State", "Txt_Name", "Txt_Title",
    }

    local visibility_children = {
        "Image_Clear", "DimBox", "Image_Complete", "Image_Completed", "ClearBox",
        "CompleteBox", "CompletedBox", "LockBox", "RewardBox", "Btn_Go",
    }

    local switcher_children = {
        "WidgetSwitcherInfo", "WidgetSwitcherMonster", "WidgetSwitcher_BTN", "WidgetSwitcher_State",
    }

    local function scan_object(ctx, event_name, object, index, class_label)
        local row = {
            index = index,
            source_class = class_label,
            object = full_name(object),
            object_class = class_name(object),
        }
        local fp = { row.object }
        local evidence = 0

        for _, property in ipairs(scalar_properties) do
            if add_value(row, fp, "prop_" .. property, read_property(object, property)) then evidence = evidence + 1 end
        end

        for _, property in ipairs(object_properties) do
            local result = read_property(object, property)
            if add_value(row, fp, "bound_" .. property, result) then
                evidence = evidence + 1
                if result ~= nil and valid(result.raw) then
                    row["bound_" .. property .. "_class"] = class_name(result.raw)
                    fp[#fp + 1] = property .. "Class=" .. row["bound_" .. property .. "_class"]
                    for _, nested in ipairs(scalar_properties) do
                        if add_value(row, fp, "bound_" .. property .. "_prop_" .. nested,
                            read_property(result.raw, nested)) then evidence = evidence + 1 end
                    end
                end
            end
        end

        for _, method in ipairs(object_methods) do
            local result = call_noarg(object, method)
            if add_value(row, fp, "method_" .. method, result) then
                evidence = evidence + 1
                if result ~= nil and valid(result.raw) then
                    row["method_" .. method .. "_class"] = class_name(result.raw)
                    for _, nested in ipairs(scalar_properties) do
                        if add_value(row, fp, "method_" .. method .. "_prop_" .. nested,
                            read_property(result.raw, nested)) then evidence = evidence + 1 end
                    end
                end
            end
        end

        for _, child in ipairs(text_children) do
            if add_value(row, fp, "text_" .. child, read_child_method(object, child, "GetText")) then evidence = evidence + 1 end
        end
        for _, child in ipairs(visibility_children) do
            if add_value(row, fp, "visibility_" .. child, read_child_method(object, child, "GetVisibility")) then evidence = evidence + 1 end
        end
        for _, child in ipairs(switcher_children) do
            if add_value(row, fp, "switcher_" .. child, read_child_method(object, child, "GetActiveWidgetIndex")) then evidence = evidence + 1 end
        end

        row.evidence_count = evidence
        ctx.log(event_name, row)
        table.sort(fp)
        return table.concat(fp, "|")
    end

    function self:scan(ctx)
        if type(FindAllOf) ~= "function" then
            return { status = "unsupported", value_type = "mission_ui_scan", value = "FindAllOf_unavailable", fingerprint = "mission_ui:unavailable" }
        end
        local all_fp = {}
        local total, valid_total, evidence_total = 0, 0, 0

        for _, class_label in ipairs(self.entry_classes) do
            ctx.phase("find_all_" .. class_label)
            local ok, objects = pcall(function() return FindAllOf(class_label) end)
            if ok and type(objects) == "table" then
                total = total + #objects
                for index, object in ipairs(objects) do
                    if index > self.max_objects then break end
                    if valid(object) then
                        valid_total = valid_total + 1
                        local fp = scan_object(ctx, "MISSION_UI_ENTRY_STATE", object, index, class_label)
                        all_fp[#all_fp + 1] = class_label .. ":" .. fp
                        evidence_total = evidence_total + 1
                    end
                end
            else
                ctx.log("MISSION_UI_CLASS_SCAN_FAILED", { class = class_label, error = clean(objects, 800) })
            end
        end

        for _, class_label in ipairs(self.content_classes) do
            ctx.phase("find_all_" .. class_label)
            local ok, objects = pcall(function() return FindAllOf(class_label) end)
            if ok and type(objects) == "table" then
                total = total + #objects
                for index, object in ipairs(objects) do
                    if index > self.max_objects then break end
                    if valid(object) then
                        valid_total = valid_total + 1
                        local fp = scan_object(ctx, "MISSION_CONTENT_BOUND_STATE", object, index, class_label)
                        all_fp[#all_fp + 1] = class_label .. ":" .. fp
                        evidence_total = evidence_total + 1
                    end
                end
            else
                ctx.log("MISSION_CONTENT_CLASS_SCAN_FAILED", { class = class_label, error = clean(objects, 800) })
            end
        end

        table.sort(all_fp)
        local fingerprint = table.concat(all_fp, "||")
        if #fingerprint > 12000 then fingerprint = fingerprint:sub(1, 12000) end
        ctx.log("MISSION_UI_SCAN_COMPLETE", {
            classes = table.concat(self.entry_classes, ","),
            content_classes = table.concat(self.content_classes, ","),
            returned_total = total,
            valid_total = valid_total,
            recorded_total = evidence_total,
        })
        return {
            status = "readable",
            value_type = "mission_ui_scan",
            value = "returned=" .. tostring(total) .. ",valid=" .. tostring(valid_total) .. ",recorded=" .. tostring(evidence_total),
            fingerprint = "mission_ui:" .. fingerprint,
        }
    end

    function self:steps()
        return {
            {
                name = "scan_boss_assault_daily_ui_entries",
                kind = "custom",
                role = "global_daily_completion_ui",
                purpose = "map_each_boss_or_assault_entry_to_bound_ID_title_and_clear_visibility",
                cadence = 1,
                run = function(ctx) return self:scan(ctx) end,
            },
        }
    end

    return self
end

return M
