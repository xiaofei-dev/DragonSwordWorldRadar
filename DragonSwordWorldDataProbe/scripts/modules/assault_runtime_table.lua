local source = debug.getinfo(1, "S").source
local script_file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local module_dir = script_file and script_file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir = scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local report_dir = mod_dir .. "\\runtime\\reports"

local place_path = report_dir .. "\\assault-runtime-place.tsv"
local kind_path = report_dir .. "\\assault-runtime-kind.tsv"
local condition_path = report_dir .. "\\assault-runtime-condition.tsv"
local schema_path = report_dir .. "\\assault-runtime-schema.tsv"
local active_path = report_dir .. "\\assault-active-layer.tsv"
local summary_path = report_dir .. "\\assault-runtime-table-summary.tsv"
local access_path = report_dir .. "\\assault-runtime-access.tsv"
local functions_path = report_dir .. "\\assault-runtime-functions.tsv"

local table_object = nil
local table_full_name = ""
local place_rows = {}
local kind_rows = {}
local condition_rows = {}
local schema_rows = {}

local place_done = false
local kind_done = false
local place_attempts = 0
local kind_attempts = 0
local active_done = false

local function valid(obj)
    if obj == nil then return false end
    local ok, result = pcall(function() return obj:IsValid() end)
    return ok and result == true
end

local function ue_type(value)
    if value == nil then return "nil" end
    local ok, result = pcall(function() return value:type() end)
    if ok and result ~= nil then return tostring(result) end
    return type(value)
end

local function unwrap(value)
    if value == nil then return nil end
    local wrapped_type = ue_type(value)
    if wrapped_type == "RemoteUnrealParam" or wrapped_type == "LocalUnrealParam" then
        local ok, result = pcall(function() return value:get() end)
        if ok then return result end
    end
    return value
end

local function scalar(value)
    value = unwrap(value)
    if value == nil then return "" end

    local native_type = type(value)
    if native_type == "string" or native_type == "number" or native_type == "boolean" then
        return tostring(value)
    end

    local ok, result = pcall(function() return value:ToString() end)
    if ok and result ~= nil then return tostring(result) end

    local ok2, result2 = pcall(function() return tostring(value) end)
    if ok2 and result2 ~= nil then return result2 end
    return ""
end

local function clean(value)
    value = tostring(value or "")
    value = value:gsub("\t", " ")
    value = value:gsub("\r", " ")
    value = value:gsub("\n", " ")
    return value
end

local function field(obj, name)
    obj = unwrap(obj)
    if obj == nil then return nil end

    -- UObject officially exposes GetPropertyValue; UScriptStruct exposes __index.
    local ok_type, object_type = pcall(function() return obj:type() end)
    if ok_type and object_type ~= "UScriptStruct" then
        local ok_get, result = pcall(function() return obj:GetPropertyValue(name) end)
        if ok_get and result ~= nil then return unwrap(result) end
    end

    local ok, result = pcall(function() return obj[name] end)
    if ok then return unwrap(result) end
    return nil
end

local function write_tsv(path, headers, rows)
    local file = io.open(path, "w")
    if not file then return false end
    file:write(table.concat(headers, "\t"), "\n")
    for _, row in ipairs(rows) do
        local values = {}
        for _, h in ipairs(headers) do
            values[#values + 1] = clean(row[h])
        end
        file:write(table.concat(values, "\t"), "\n")
    end
    file:close()
    return true
end

local function add_schema(scope, property_name, property_type, struct_name, runtime_type, note)
    schema_rows[#schema_rows + 1] = {
        Scope = scope or "",
        PropertyName = property_name or "",
        PropertyType = property_type or "",
        StructName = struct_name or "",
        RuntimeType = runtime_type or "",
        Note = note or "",
    }
end

local function prop_name(prop)
    local result = ""
    pcall(function() result = prop:GetFName():ToString() end)
    return tostring(result or "")
end

local function prop_full_name(prop)
    local result = ""
    pcall(function() result = prop:GetFullName() end)
    return tostring(result or "")
end

local function prop_type(prop)
    local result = ""
    pcall(function() result = prop:GetClass():GetFullName() end)
    return tostring(result or "")
end

local function get_reflected_property(obj, name, scope)
    if not valid(obj) then return nil end

    local reflected = nil
    local ok, err = pcall(function()
        local reflection = obj:Reflection()
        if reflection ~= nil then
            reflected = reflection:GetProperty(name)
        end
    end)

    if not ok then
        add_schema(scope, name, "", "", "", "Reflection.GetProperty_failed:" .. clean(err))
    elseif reflected == nil then
        add_schema(scope, name, "", "", "", "Reflection.GetProperty_nil")
    else
        local struct_name = ""
        pcall(function()
            local s = reflected:GetStruct()
            if s ~= nil then struct_name = s:GetFullName() end
        end)
        add_schema(
            scope,
            name,
            prop_type(reflected),
            struct_name,
            ue_type(field(obj, name)),
            "top_level_property"
        )
    end

    return reflected
end

local function struct_members(struct_property, scope)
    local names = {}
    if struct_property == nil then return names end

    local ok, err = pcall(function()
        local meta = struct_property:GetStruct()
        if meta == nil then return end

        meta:ForEachProperty(function(member)
            local name = prop_name(member)
            names[#names + 1] = name

            local nested_struct = ""
            pcall(function()
                local s = member:GetStruct()
                if s ~= nil then nested_struct = s:GetFullName() end
            end)

            add_schema(
                scope,
                name,
                prop_type(member),
                nested_struct,
                "",
                prop_full_name(member)
            )
        end)
    end)

    if not ok then
        add_schema(scope, "", "", "", "", "ForEachProperty_failed:" .. clean(err))
    end

    return names
end

local function map_is_iterable(value)
    value = unwrap(value)
    if value == nil then return false end

    local ok = pcall(function()
        value:ForEach(function(_, _)
            return true
        end)
    end)
    return ok
end

local access_probe

local function find_map(container, struct_property, scope)
    container = unwrap(container)
    if container == nil then
        add_schema(scope, "", "", "", "nil", "container_nil")
        return nil, ""
    end

    if map_is_iterable(container) then
        add_schema(scope, "<self>", "TMap", "", ue_type(container), "container_is_map")
        return container, "<self>"
    end

    -- Explicitly exercise documented UScriptStruct member access and record the
    -- underlying mapping addresses before any fallback guessing.
    local exact_data = access_probe(scope, container, "Data")
    if map_is_iterable(exact_data) then
        add_schema(scope, "Data", "TMap", "", ue_type(exact_data), "selected_exact_Data")
        return exact_data, "Data"
    end

    local candidates, seen = {}, {}
    for _, name in ipairs(struct_members(struct_property, scope)) do
        if name ~= "" and not seen[name] then
            seen[name] = true
            candidates[#candidates + 1] = name
        end
    end
    for _, name in ipairs({"Data", "Map", "DataMap", "Rows", "Records", "Items"}) do
        if not seen[name] then
            seen[name] = true
            candidates[#candidates + 1] = name
        end
    end

    for _, name in ipairs(candidates) do
        local value = field(container, name)
        add_schema(scope, name, "", "", ue_type(value), "runtime_member_probe")
        if map_is_iterable(value) then
            add_schema(scope, name, "TMap", "", ue_type(value), "selected_map_member")
            return value, name
        end
    end

    return nil, ""
end

local function inner_map(wrap, scope)
    wrap = unwrap(wrap)
    if wrap == nil then return nil, "" end

    for _, name in ipairs({"Data", "Map", "DataMap", "Rows", "Records"}) do
        local value = field(wrap, name)
        if map_is_iterable(value) then
            add_schema(scope, name, "TMap", "", ue_type(value), "selected_inner_map")
            return value, name
        end
    end

    if map_is_iterable(wrap) then return wrap, "<self>" end
    return nil, ""
end

local function foreach_map(map, callback)
    map = unwrap(map)
    if map == nil then return false, "nil_map" end

    local ok, err = pcall(function()
        map:ForEach(function(key_param, value_param)
            callback(unwrap(key_param), unwrap(value_param))
        end)
    end)
    return ok, ok and "" or tostring(err)
end


local access_rows = {}

access_probe = function(scope, obj, property_name)
    local row = {
        Scope = scope or "",
        PropertyName = property_name or "",
        ObjectType = ue_type(obj),
        ValueType = "",
        BaseAddress = "",
        StructAddress = "",
        PropertyAddress = "",
        ReflectedProperty = "",
        ReflectedValuePtr = "",
        DirectIndex = "",
        GetPropertyValue = "",
    }

    local value = nil

    if obj ~= nil then
        pcall(function() row.BaseAddress = tostring(obj:GetBaseAddress()) end)
        pcall(function() row.StructAddress = tostring(obj:GetStructAddress()) end)
        pcall(function() row.PropertyAddress = tostring(obj:GetPropertyAddress()) end)

        local ok_index, index_value = pcall(function() return obj[property_name] end)
        if ok_index then
            value = index_value
            row.DirectIndex = ue_type(index_value)
        else
            row.DirectIndex = "error:" .. clean(index_value)
        end

        local ok_get, get_value = pcall(function() return obj:GetPropertyValue(property_name) end)
        if ok_get then
            if value == nil then value = get_value end
            row.GetPropertyValue = ue_type(get_value)
        else
            row.GetPropertyValue = "unsupported_or_error"
        end
    end

    local reflected = nil
    if obj ~= nil then
        pcall(function()
            local reflection = obj:Reflection()
            if reflection ~= nil then
                reflected = reflection:GetProperty(property_name)
            end
        end)
    end
    if reflected ~= nil then
        row.ReflectedProperty = prop_type(reflected)
        pcall(function()
            row.ReflectedValuePtr = tostring(reflected:ContainerPtrToValuePtr(obj, 0))
        end)
    end

    row.ValueType = ue_type(value)
    access_rows[#access_rows + 1] = row

    write_tsv(
        access_path,
        {
            "Scope","PropertyName","ObjectType","ValueType",
            "BaseAddress","StructAddress","PropertyAddress",
            "ReflectedProperty","ReflectedValuePtr","DirectIndex","GetPropertyValue"
        },
        access_rows
    )

    return unwrap(value)
end

local function dump_table_functions(obj)
    if not valid(obj) then return end

    local rows = {}
    pcall(function()
        local cls = obj:GetClass()
        if cls ~= nil then
            cls:ForEachFunction(function(fn)
                local name = ""
                local full = ""
                pcall(function() name = fn:GetFName():ToString() end)
                pcall(function() full = fn:GetFullName() end)
                rows[#rows + 1] = {Name=tostring(name or ""), FullName=tostring(full or "")}
            end)
        end
    end)

    table.sort(rows, function(a,b) return a.Name < b.Name end)
    write_tsv(functions_path, {"Name","FullName"}, rows)
end

local function choose_table(ctx)
    if valid(table_object) then return table_object end

    ctx.phase("find_DUnexpectedMissionTable")
    local objects = FindAllOf("DUnexpectedMissionTable")
    if type(objects) ~= "table" then return nil end

    local fallback = nil
    for _, obj in ipairs(objects) do
        if valid(obj) then
            local name = ""
            pcall(function() name = obj:GetFullName() end)
            if not name:find("Default__DUnexpectedMissionTable", 1, true) then
                if fallback == nil then fallback = obj end
                if name:find("GameDBTableManager_Singleton.DUnexpectedMissionTable", 1, true) then
                    table_object = obj
                    table_full_name = name
                    dump_table_functions(obj)
                    return obj
                end
            end
        end
    end

    table_object = fallback
    if valid(fallback) then
        pcall(function() table_full_name = fallback:GetFullName() end)
        dump_table_functions(fallback)
    end
    return table_object
end

local function place_record(row, outer_key, wrap_map_id, inner_key)
    return {
        OuterMapID = scalar(outer_key),
        WrapMapID = scalar(wrap_map_id),
        InnerKey = scalar(inner_key),
        ID = scalar(field(row, "ID")),
        MapID = scalar(field(row, "MapID")),
        UpdateTarget = scalar(field(row, "UpdateTarget")),
        MissionPlaceWeight = scalar(field(row, "MissionPlaceWeight")),
        MissionKindData = scalar(field(row, "MissionKindData")),
        MissionRange = scalar(field(row, "MissionRange")),
        SortOrder = scalar(field(row, "SortOrder")),
    }
end

local function kind_record(row, outer_key, wrap_group_id, inner_key)
    return {
        OuterGroupID = scalar(outer_key),
        WrapGroupID = scalar(wrap_group_id),
        InnerKey = scalar(inner_key),
        ID = scalar(field(row, "ID")),
        GroupID = scalar(field(row, "GroupID")),
        MissionType = scalar(field(row, "MissionType")),
        MissionValue1 = scalar(field(row, "MissionValue1")),
        MissionValue2 = scalar(field(row, "MissionValue2")),
        MissionValue3 = scalar(field(row, "MissionValue3")),
        MissionCount = scalar(field(row, "MissionCount")),
        MissionKindWeight = scalar(field(row, "MissionKindWeight")),
        MissionLimitTime = scalar(field(row, "MissionLimitTime")),
        MissionNoticeDescription = scalar(field(row, "MissionNoticeDescription")),
        AcceptConditionType = scalar(field(row, "AcceptConditionType")),
        AcceptConditionValue1 = scalar(field(row, "AcceptConditionValue1")),
        AcceptConditionValue2 = scalar(field(row, "AcceptConditionValue2")),
        AcceptConditionValue3 = scalar(field(row, "AcceptConditionValue3")),
        Reward_ID = scalar(field(row, "Reward_ID")),
        DataLayer = scalar(field(row, "DataLayer")),
        MissionAreaSize = scalar(field(row, "MissionAreaSize")),
        MapInfoImgName = scalar(field(row, "MapInfoImgName")),
    }
end

local function snapshot_place(ctx)
    place_attempts = place_attempts + 1
    if place_attempts > 3 and not place_done then
        return {status="complete", value_type="bounded_failure", value="runtime_map_unavailable_after_3_attempts", fingerprint="place:bounded"}
    end
    if place_done then
        return {status="complete", value_type="row_count", value=tostring(#place_rows), fingerprint="place:"..#place_rows}
    end

    local obj = choose_table(ctx)
    if not valid(obj) then
        return {status="unavailable", value_type="uobject", value="table_not_ready", fingerprint="place:no_table"}
    end

    ctx.phase("reflect_place_property")
    local prop = get_reflected_property(obj, "UnexpectedMissionPlaceDataMap", "PlaceTop")
    local container = field(obj, "UnexpectedMissionPlaceDataMap")
    local outer, member = find_map(container, prop, "PlaceTop")

    write_tsv(schema_path, {"Scope","PropertyName","PropertyType","StructName","RuntimeType","Note"}, schema_rows)

    if outer == nil then
        return {status="unavailable", value_type="tmap", value="place_outer_map_not_resolved", fingerprint="place:no_outer"}
    end

    ctx.phase("iterate_place")
    local rows = {}
    local ok, err = foreach_map(outer, function(outer_key, outer_value)
        if field(outer_value, "ID") ~= nil and field(outer_value, "MissionKindData") ~= nil then
            rows[#rows+1] = place_record(outer_value, outer_key, "", outer_key)
            return
        end

        local inner = inner_map(outer_value, "PlaceWrap")
        if inner == nil then return end
        local wrap_map_id = field(outer_value, "MapID")

        foreach_map(inner, function(inner_key, record)
            rows[#rows+1] = place_record(record, outer_key, wrap_map_id, inner_key)
        end)
    end)

    if not ok then
        return {status="unavailable", value_type="tmap", value="place_iteration_failed:"..clean(err), fingerprint="place:iteration_failed"}
    end

    table.sort(rows, function(a,b) return (tonumber(a.ID) or 0) < (tonumber(b.ID) or 0) end)
    place_rows = rows

    local wrote = write_tsv(
        place_path,
        {"OuterMapID","WrapMapID","InnerKey","ID","MapID","UpdateTarget","MissionPlaceWeight","MissionKindData","MissionRange","SortOrder"},
        rows
    )
    place_done = wrote and #rows > 0

    ctx.log("ASSAULT_PLACE_CAPTURE", {rows=#rows, outer_member=member, table_object=table_full_name})
    return {status=place_done and "ok" or "unavailable", value_type="row_count", value=tostring(#rows), fingerprint="place:"..#rows}
end

local function snapshot_kind(ctx)
    kind_attempts = kind_attempts + 1
    if kind_attempts > 3 and not kind_done then
        return {status="complete", value_type="bounded_failure", value="runtime_map_unavailable_after_3_attempts", fingerprint="kind:bounded"}
    end
    if kind_done then
        return {status="complete", value_type="row_count", value=tostring(#kind_rows), fingerprint="kind:"..#kind_rows}
    end

    local obj = choose_table(ctx)
    if not valid(obj) then
        return {status="unavailable", value_type="uobject", value="table_not_ready", fingerprint="kind:no_table"}
    end

    ctx.phase("reflect_kind_property")
    local prop = get_reflected_property(obj, "UnexpectedMissionKindDataMap", "KindTop")
    local container = field(obj, "UnexpectedMissionKindDataMap")
    local outer, member = find_map(container, prop, "KindTop")

    write_tsv(schema_path, {"Scope","PropertyName","PropertyType","StructName","RuntimeType","Note"}, schema_rows)

    if outer == nil then
        return {status="unavailable", value_type="tmap", value="kind_outer_map_not_resolved", fingerprint="kind:no_outer"}
    end

    ctx.phase("iterate_kind")
    local rows = {}
    local ok, err = foreach_map(outer, function(outer_key, outer_value)
        if field(outer_value, "ID") ~= nil and field(outer_value, "MissionType") ~= nil then
            rows[#rows+1] = kind_record(outer_value, outer_key, "", outer_key)
            return
        end

        local inner = inner_map(outer_value, "KindWrap")
        if inner == nil then return end
        local wrap_group_id = field(outer_value, "GroupID")

        foreach_map(inner, function(inner_key, record)
            rows[#rows+1] = kind_record(record, outer_key, wrap_group_id, inner_key)
        end)
    end)

    if not ok then
        return {status="unavailable", value_type="tmap", value="kind_iteration_failed:"..clean(err), fingerprint="kind:iteration_failed"}
    end

    table.sort(rows, function(a,b)
        local ga, gb = tonumber(a.GroupID) or 0, tonumber(b.GroupID) or 0
        if ga ~= gb then return ga < gb end
        return (tonumber(a.ID) or 0) < (tonumber(b.ID) or 0)
    end)

    kind_rows = rows
    local wrote = write_tsv(
        kind_path,
        {"OuterGroupID","WrapGroupID","InnerKey","ID","GroupID","MissionType","MissionValue1","MissionValue2","MissionValue3","MissionCount","MissionKindWeight","MissionLimitTime","MissionNoticeDescription","AcceptConditionType","AcceptConditionValue1","AcceptConditionValue2","AcceptConditionValue3","Reward_ID","DataLayer","MissionAreaSize","MapInfoImgName"},
        rows
    )
    kind_done = wrote and #rows > 0

    ctx.log("ASSAULT_KIND_CAPTURE", {rows=#rows, outer_member=member, table_object=table_full_name})
    return {status=kind_done and "ok" or "unavailable", value_type="row_count", value=tostring(#rows), fingerprint="kind:"..#rows}
end

local function condition_batch(batch_index, ctx)
    if not place_done or #place_rows == 0 then
        return {status="deferred", value_type="dependency", value="waiting_for_place", fingerprint="cond:"..batch_index..":deferred"}
    end

    ctx.phase("condition_handles")
    local world_context = FindFirstOf("DClientQuestSystem")
    local util = StaticFindObject("/Script/DS.Default__DETUtil")
    local fn = StaticFindObject("/Script/DS.DETUtil:CUnexpectedMissionInStandAlone")
    if not valid(world_context) or not valid(util) or not valid(fn) then
        return {status="unavailable", value_type="ufunction", value="condition_function_not_ready", fingerprint="cond:"..batch_index..":no_fn"}
    end

    local ids, seen = {}, {}
    for _, row in ipairs(place_rows) do
        local id = tonumber(row.ID)
        if id and not seen[id] then
            seen[id] = true
            ids[#ids+1] = id
        end
    end
    table.sort(ids)

    local first = (batch_index - 1) * 32 + 1
    if first > #ids then
        return {status="complete", value_type="row_count", value="0", fingerprint="cond:"..batch_index..":empty"}
    end
    local last = math.min(first + 31, #ids)

    ctx.phase("condition_batch_" .. tostring(batch_index))
    for index = first, last do
        local id = ids[index]
        local a = fn(util, world_context, id, false)
        local b = fn(util, world_context, id, true)
        if type(a) ~= "boolean" or type(b) ~= "boolean" then
            return {status="unavailable", value_type="return_type", value="non_boolean_at:"..tostring(id), fingerprint="cond:"..batch_index..":bad"}
        end

        condition_rows[#condition_rows+1] = {
            MissionID=tostring(id),
            ISAllFalse=tostring(a),
            ISAllTrue=tostring(b),
            Source="DETUtil.CUnexpectedMissionInStandAlone",
            Semantics="raw_uninterpreted",
        }
    end

    write_tsv(condition_path, {"MissionID","ISAllFalse","ISAllTrue","Source","Semantics"}, condition_rows)
    return {status="ok", value_type="row_count", value=tostring(last-first+1), fingerprint="cond:"..batch_index..":"..tostring(last-first+1)}
end

local function active_layer_snapshot(ctx)
    if active_done then
        return {status="complete", value_type="row_count", value="1", fingerprint="active:captured"}
    end

    ctx.phase("find_active_layer")
    local layers = FindAllOf("DLayerUnexpectedMission")
    if type(layers) ~= "table" or #layers == 0 then
        return {status="unavailable", value_type="uobject", value="no_active_layer", fingerprint="active:none"}
    end

    local layer = nil
    for _, candidate in ipairs(layers) do
        if valid(candidate) then layer = candidate break end
    end
    if not valid(layer) then
        return {status="unavailable", value_type="uobject", value="no_valid_active_layer", fingerprint="active:none"}
    end

    local pos = field(layer, "TargetPosition")
    local rows = {{
        FullName = scalar((function() local x=""; pcall(function() x=layer:GetFullName() end); return x end)()),
        RangeType = scalar(field(layer, "RangeType")),
        StartTime = scalar(field(layer, "StartTime")),
        TargetTimer = scalar(field(layer, "TargetTimer")),
        ActivateTimer = scalar(field(layer, "ActivateTimer")),
        ActivateDist = scalar(field(layer, "ActivateDist")),
        TargetRadius = scalar(field(layer, "TargetRadius")),
        TargetX = scalar(field(pos, "X")),
        TargetY = scalar(field(pos, "Y")),
        TargetZ = scalar(field(pos, "Z")),
    }}

    local wrote = write_tsv(
        active_path,
        {"FullName","RangeType","StartTime","TargetTimer","ActivateTimer","ActivateDist","TargetRadius","TargetX","TargetY","TargetZ"},
        rows
    )
    active_done = wrote
    return {status=wrote and "ok" or "unavailable", value_type="row_count", value=wrote and "1" or "0", fingerprint=wrote and "active:1" or "active:0"}
end

local steps = {
    {
        kind="custom", name="assault_place_table_snapshot", class="DUnexpectedMissionTable",
        role="assault_static_runtime_table", purpose="read_only_place_snapshot",
        cadence=1, once_per_session=true, run=snapshot_place,
    },
    {
        kind="custom", name="assault_kind_table_snapshot", class="DUnexpectedMissionTable",
        role="assault_static_runtime_table", purpose="read_only_kind_snapshot",
        cadence=1, once_per_session=true, run=snapshot_kind,
    },
}

for batch = 1, 8 do
    local batch_id = batch
    steps[#steps+1] = {
        kind="custom",
        name="assault_condition_batch_"..tostring(batch_id),
        class="<DETUtil>",
        role="assault_condition_probe",
        purpose="read_only_condition_batch",
        cadence=1,
        once_per_session=true,
        run=function(ctx) return condition_batch(batch_id, ctx) end,
    }
end

steps[#steps+1] = {
    kind="custom", name="assault_active_layer_snapshot", class="DLayerUnexpectedMission",
    role="assault_active_correlation", purpose="optional_active_event_snapshot",
    cadence=1, once_per_session=true, run=active_layer_snapshot,
}

return {
    id="assault_runtime_table",
    enabled=true,
    description="Assault-only runtime capture: authoritative Place/Kind, condition batches, and optional active-layer correlation.",
    steps=steps,
}
