local M = {}

local source = debug.getinfo(1, "S").source
local file = type(source) == "string" and source:sub(1, 1) == "@" and source:sub(2) or nil
local core_dir = file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir = core_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir = scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."

M.mod_dir = mod_dir
M.report_dir = mod_dir .. "\\runtime\\reports"

local shared = rawget(_G, "__DSWDP_TREASURE_CAPTURE_127")
if type(shared) ~= "table" then
    shared = {
        catalog = nil,
        by_id = {},
        grid = {},
        previous = {},
        hook_registry = {},
        event_count = 0,
    }
    rawset(_G, "__DSWDP_TREASURE_CAPTURE_127", shared)
end
M.shared = shared

local function unwrap(value)
    if value == nil then return nil end
    local ok_type, wrapped_type = pcall(function() return value:type() end)
    if ok_type and
        (wrapped_type == "RemoteUnrealParam" or wrapped_type == "LocalUnrealParam")
    then
        local ok_get, got = pcall(function() return value:get() end)
        if ok_get then return got end
    end
    return value
end
M.unwrap = unwrap

function M.valid(object)
    object = unwrap(object)
    if object == nil then return false end
    local ok, value = pcall(function() return object:IsValid() end)
    return ok and value == true
end

function M.field(object, name)
    object = unwrap(object)
    if object == nil then return nil end

    local ok, value = pcall(function() return object[name] end)
    if ok and value ~= nil then return unwrap(value) end

    local ok_get, got = pcall(function() return object:GetPropertyValue(name) end)
    if ok_get then return unwrap(got) end

    return nil
end

function M.scalar(value)
    value = unwrap(value)
    if value == nil then return "" end

    local kind = type(value)
    if kind == "string" or kind == "number" or kind == "boolean" then
        return tostring(value)
    end

    local ok_text, text = pcall(function() return value:ToString() end)
    if ok_text and text ~= nil then return tostring(text) end

    local ok_string, string_value = pcall(function() return tostring(value) end)
    if ok_string and string_value ~= nil then return string_value end

    return ""
end

function M.number(value)
    value = unwrap(value)
    if type(value) == "number" then return value end
    return tonumber(M.scalar(value))
end

function M.boolean(value)
    value = unwrap(value)
    if type(value) == "boolean" then return value end
    local text = string.lower(M.scalar(value))
    if text == "true" or text == "1" then return true end
    if text == "false" or text == "0" then return false end
    return nil
end

function M.full_name(object)
    object = unwrap(object)
    if object == nil then return "" end
    local value = ""
    pcall(function() value = object:GetFullName() end)
    if value == "" then pcall(function() value = object:GetName() end) end
    return tostring(value or "")
end

local function normalized(value)
    return string.lower(tostring(value or "")):gsub("[^a-z0-9]", "")
end

local function cell_key(x, y)
    local cell = 10000
    return tostring(math.floor(x / cell)) .. ":" .. tostring(math.floor(y / cell))
end

local function load_catalog()
    if shared.catalog ~= nil then return end

    local ok, catalog = pcall(function()
        return dofile(mod_dir .. "\\reference\\treasure\\treasures.lua")
    end)

    if not ok or type(catalog) ~= "table" then
        shared.catalog = {}
        return
    end

    shared.catalog = catalog
    for _, point in ipairs(catalog) do
        local id = tonumber(point.save_id)
        local x = tonumber(point.x)
        local y = tonumber(point.y)

        if id then shared.by_id[id] = point end
        if x and y then
            local key = cell_key(x, y)
            shared.grid[key] = shared.grid[key] or {}
            shared.grid[key][#shared.grid[key] + 1] = point
        end
    end
end
load_catalog()

function M.location(actor)
    actor = unwrap(actor)
    if not M.valid(actor) then return nil, nil, nil end

    local location = nil
    pcall(function() location = actor:K2_GetActorLocation() end)
    location = unwrap(location)

    if location == nil then
        pcall(function() location = actor:GetActorLocation() end)
        location = unwrap(location)
    end

    if location == nil then
        local root = M.field(actor, "RootComponent")
        location = M.field(root, "RelativeLocation")
    end

    if location == nil then return nil, nil, nil end

    return M.number(M.field(location, "X")),
           M.number(M.field(location, "Y")),
           M.number(M.field(location, "Z"))
end

local function nearby_points(x, y)
    local result = {}
    local cx = math.floor(x / 10000)
    local cy = math.floor(y / 10000)

    for dx = -1, 1 do
        for dy = -1, 1 do
            local key = tostring(cx + dx) .. ":" .. tostring(cy + dy)
            for _, point in ipairs(shared.grid[key] or {}) do
                result[#result + 1] = point
            end
        end
    end

    return result
end

function M.match_catalog(full_name, object_id, x, y, z)
    load_catalog()

    local numeric_id = tonumber(object_id)
    if numeric_id and shared.by_id[numeric_id] then
        local point = shared.by_id[numeric_id]
        return point, "object_id", 0, 1, nil
    end

    local name_norm = normalized(full_name)
    local exact_name = nil
    if name_norm ~= "" then
        for _, point in ipairs(shared.catalog or {}) do
            local uid_norm = normalized(point.uid_name)
            if #uid_norm >= 6 and name_norm:find(uid_norm, 1, true) then
                exact_name = point
                break
            end
        end
    end

    if exact_name then
        local distance = nil
        if x and y and z then
            local dx = x - tonumber(exact_name.x or 0)
            local dy = y - tonumber(exact_name.y or 0)
            local dz = z - tonumber(exact_name.z or 0)
            distance = math.sqrt(dx * dx + dy * dy + dz * dz)
        end
        return exact_name, "uid_name", distance, 1, nil
    end

    if not x or not y then return nil, "none", nil, 0, nil end

    local matches = {}
    for _, point in ipairs(nearby_points(x, y)) do
        local px = tonumber(point.x)
        local py = tonumber(point.y)
        local pz = tonumber(point.z) or 0

        if px and py then
            local dx = x - px
            local dy = y - py
            local dz = (z or pz) - pz
            local distance = math.sqrt(dx * dx + dy * dy + dz * dz)
            if distance <= 3000 then
                matches[#matches + 1] = {
                    point = point,
                    distance = distance,
                }
            end
        end
    end

    table.sort(matches, function(a, b) return a.distance < b.distance end)

    if #matches == 0 then return nil, "none", nil, 0, nil end

    local second = #matches >= 2 and matches[2].distance or nil
    local method = (#matches == 1 or (second and second - matches[1].distance >= 500))
        and "nearest_unique"
        or "nearest_ambiguous"

    return matches[1].point, method, matches[1].distance, #matches, second
end

function M.owner_actor(value)
    local object = unwrap(value)
    if not M.valid(object) then return nil end

    -- An ActorComponent context exposes GetOwner().
    local owner = nil
    pcall(function() owner = object:GetOwner() end)
    owner = unwrap(owner)
    if M.valid(owner) then return owner end

    return object
end

function M.snapshot(value)
    local context = unwrap(value)
    local actor = M.owner_actor(context)
    if not M.valid(actor) then return nil end

    local interact = M.field(actor, "InteractComponent")
    if not M.valid(interact) and M.valid(context) then
        local possible_type = M.field(context, "InteractTypeValue")
        if possible_type ~= nil then
            interact = context
        end
    end

    local x, y, z = M.location(actor)
    local object_id = M.number(M.field(actor, "ObjectID"))
    local ds_guid = M.scalar(M.field(actor, "DsGuid"))
    local full_name = M.full_name(actor)

    local point, match_method, distance, nearby_count, second_distance =
        M.match_catalog(full_name, object_id, x, y, z)

    local interact_type = M.scalar(M.field(interact, "InteractTypeValue"))
    local interact_type_number = tonumber(interact_type)
    local interact_type_lower = string.lower(interact_type)

    local save_and_load = M.boolean(M.field(actor, "bShouldSaveAndLoadState"))
    local name_lower = string.lower(full_name)

    local treasure_type =
        interact_type_number == 4
        or interact_type_lower:find("treasurebox", 1, true) ~= nil

    local name_treasure =
        name_lower:find("treasure", 1, true) ~= nil
        or name_lower:find("trasure", 1, true) ~= nil
        or name_lower:find("chest", 1, true) ~= nil

    local exact_match = match_method == "object_id" or match_method == "uid_name"
    local close_match = distance ~= nil and distance <= 800
    local is_treasure =
        treasure_type
        or exact_match
        or (name_treasure and distance ~= nil and distance <= 3000)
        or (save_and_load == true and close_match)

    local save_id = point and tonumber(point.save_id) or nil
    local uid_name = point and tostring(point.uid_name or "") or ""

    local row = {
        UTC = os.date("!%Y-%m-%dT%H:%M:%SZ"),
        Actor = full_name,
        Context = M.full_name(context),
        ObjectID = object_id or "",
        DsGuid = ds_guid,
        SaveAndLoad = save_and_load,
        IsDisposableProp = M.boolean(M.field(actor, "IsDisposableProp")),
        CurrentUseSkeletalMesh = M.scalar(M.field(actor, "CurrentUseSkeletalMesh")),
        AnimInfoTableID = M.scalar(M.field(actor, "AnimInfoTableID")),
        SpawnWorldTime = M.scalar(M.field(actor, "SpawnWorldTime")),
        X = x or "",
        Y = y or "",
        Z = z or "",
        InteractComponent = M.full_name(interact),
        InteractTypeValue = interact_type,
        InteractableValue = M.scalar(M.field(interact, "InteractableValue")),
        IsShowUI = M.boolean(M.field(interact, "IsShowUI")),
        PropIconID = M.scalar(M.field(interact, "PropIconID")),
        NPCExecuteCount = M.scalar(M.field(interact, "NPCExecuteCount")),
        SaveID = save_id or "",
        CatalogUIDName = uid_name,
        MatchMethod = match_method,
        MatchDistance = distance or "",
        NearbyCatalogCount = nearby_count or 0,
        SecondDistance = second_distance or "",
        TreasureCandidate = is_treasure,
    }

    row.Key = save_id and ("save:" .. tostring(save_id)) or ("actor:" .. full_name)
    row.Fingerprint = table.concat({
        tostring(row.ObjectID),
        tostring(row.DsGuid),
        tostring(row.SaveAndLoad),
        tostring(row.IsDisposableProp),
        tostring(row.CurrentUseSkeletalMesh),
        tostring(row.InteractTypeValue),
        tostring(row.InteractableValue),
        tostring(row.IsShowUI),
        tostring(row.NPCExecuteCount),
        tostring(row.X),
        tostring(row.Y),
        tostring(row.Z),
    }, "|")

    return row
end

M.headers = {
    "UTC","Event","HookPhase","HookPath","Key","Actor","Context",
    "ObjectID","DsGuid","SaveAndLoad","IsDisposableProp",
    "CurrentUseSkeletalMesh","AnimInfoTableID","SpawnWorldTime",
    "X","Y","Z","InteractComponent","InteractTypeValue",
    "InteractableValue","IsShowUI","PropIconID","NPCExecuteCount",
    "SaveID","CatalogUIDName","MatchMethod","MatchDistance",
    "NearbyCatalogCount","SecondDistance","TreasureCandidate","Fingerprint",
}

local function clean(value)
    value = tostring(value == nil and "" or value)
    value = value:gsub("\t", " ")
    value = value:gsub("\r", " ")
    value = value:gsub("\n", " ")
    return value
end

function M.write_rows(path, rows)
    local file = io.open(path, "w")
    if not file then return false end

    file:write(table.concat(M.headers, "\t"), "\n")
    for _, row in ipairs(rows) do
        local values = {}
        for _, header in ipairs(M.headers) do
            values[#values + 1] = clean(row[header])
        end
        file:write(table.concat(values, "\t"), "\n")
    end

    file:close()
    return true
end

function M.append_row(path, row)
    local exists = false
    local current = io.open(path, "r")
    if current then
        exists = true
        current:close()
    end

    local file = io.open(path, "a")
    if not file then return false end

    if not exists then
        file:write(table.concat(M.headers, "\t"), "\n")
    end

    local values = {}
    for _, header in ipairs(M.headers) do
        values[#values + 1] = clean(row[header])
    end
    file:write(table.concat(values, "\t"), "\n")
    file:close()
    return true
end

function M.append_event(row, event_name, hook_phase, hook_path)
    if not row then return false end

    row.Event = event_name or "hook"
    row.HookPhase = hook_phase or ""
    row.HookPath = hook_path or ""
    row.UTC = os.date("!%Y-%m-%dT%H:%M:%SZ")

    return M.append_row(
        M.report_dir .. "\\treasure-state-events.tsv",
        row
    )
end

return M
