local WorldMap = {}
local MAX_CANDIDATES = 2
local MAX_RETIRED_IDENTITIES = 256
local cached_entry = nil
local cached_data = {}
local candidates = {}
local retired_identities = {}
local retired_order = {}

-- During map loading, all cached UObjects are considered unsafe.
local suspended = false
local needs_rescan = true
local access_guard = function() return true end
local lifecycle_epoch = 0
local candidate_token = 1
local wake_hint = false
local perf_diagnostics = nil
local scan_results = 0
local admitted = 0
local rejected_stale = 0
local rejected_cap = 0

local function access_allowed(epoch, token)
    return not suspended
        and (epoch == nil or epoch == lifecycle_epoch)
        and (token == nil or token == candidate_token)
        and access_guard(lifecycle_epoch) == true
end

local function is_valid(object)
    if object == nil or not access_allowed() then
        return false
    end

    local ok, valid = pcall(function()
        return object:IsValid()
    end)

    return ok and valid == true
end

local function safe_get(object, name)
    if object == nil or not access_allowed() then
        return nil
    end

    local ok, value = pcall(function()
        return object[name]
    end)

    return ok and value or nil
end

local function safe_call(object, name, ...)
    if object == nil or not access_allowed() then
        return false, nil
    end

    local arguments = { ... }

    return pcall(function()
        local callable = object[name]
        if callable == nil then
            error("function not found: " .. tostring(name))
        end

        return callable(object, table.unpack(arguments))
    end)
end

local function full_name(object)
    if object == nil or not access_allowed() then
        return "<nil>"
    end

    local ok, name = pcall(function()
        return object:GetFullName()
    end)

    return ok and tostring(name) or "<unavailable>"
end

local function is_visible(object)
    if not is_valid(object) then
        return false
    end

    local ok, visible = safe_call(object, "IsVisible")
    return ok and visible == true
end

local function is_world_map_layer(layer)
    if not is_valid(layer) then
        return false
    end

    local name = full_name(layer)

    return string.find(name, "/Engine/Transient", 1, true) ~= nil
        and string.find(name, ".DPanelWorldMap_C.", 1, true) ~= nil
end

local function record_candidate_metrics(event)
    if perf_diagnostics ~= nil then
        perf_diagnostics.record_world_map_candidates({
            event = event,
            retained = #candidates,
            scan_results = scan_results,
            admitted = admitted,
            rejected_stale = rejected_stale,
            rejected_cap = rejected_cap,
            epoch = lifecycle_epoch,
            token = candidate_token,
        })
    end
end

local function retire_identity(identity)
    if identity == nil or identity == "<unavailable>"
        or retired_identities[identity]
    then
        return
    end
    retired_identities[identity] = true
    table.insert(retired_order, identity)
    if #retired_order > MAX_RETIRED_IDENTITIES then
        local removed = table.remove(retired_order, 1)
        retired_identities[removed] = nil
    end
end

local function entry_current(entry)
    return entry ~= nil
        and entry.epoch == lifecycle_epoch
        and entry.token == candidate_token
        and access_allowed(entry.epoch, entry.token)
end

local function remember_world_map_layer(layer, source, allow_retired)
    local capture_epoch = lifecycle_epoch
    local capture_token = candidate_token
    if not access_allowed(capture_epoch, capture_token)
        or not is_valid(layer)
        or not is_world_map_layer(layer)
    then
        return false
    end

    local identity = full_name(layer)
    if not allow_retired and retired_identities[identity] then
        rejected_stale = rejected_stale + 1
        return false
    end
    for index = #candidates, 1, -1 do
        local entry = candidates[index]
        if not entry_current(entry) then
            table.remove(candidates, index)
        elseif entry.object == layer or entry.identity == identity then
            return true
        end
    end
    if #candidates >= MAX_CANDIDATES then
        if source == "notify-current-epoch" then
            -- Closed map widgets may remain valid but hidden after UMG removes
            -- them from presentation. Prefer the newly created current-epoch
            -- widget over one such hidden entry so two historical wrappers
            -- cannot permanently block later map opens. This is event-driven
            -- and bounded by MAX_CANDIDATES; active visible entries are never
            -- displaced and no recurring UObject work is added.
            local hidden_index = nil
            for index = 1, #candidates do
                local entry = candidates[index]
                if entry_current(entry)
                    and not is_visible(entry.object)
                then
                    hidden_index = index
                    break
                end
            end
            if hidden_index ~= nil then
                table.remove(candidates, hidden_index)
            end
        end
        if #candidates >= MAX_CANDIDATES then
            rejected_cap = rejected_cap + 1
            return false
        end
    end
    table.insert(candidates, {
        object = layer,
        identity = identity,
        epoch = capture_epoch,
        token = capture_token,
        source = tostring(source or "unknown"),
    })
    admitted = admitted + 1
    if source == "notify-current-epoch" then
        wake_hint = true
    end
    return true
end

local function scan_world_map_layers()
    if not access_allowed() then
        return
    end

    local scan_ok, layers = pcall(function()
        return FindAllOf("DLayerMap_C")
    end)

    if scan_ok and layers ~= nil then
        scan_results = scan_results + #layers
        -- FindAllOf ordering is treated only as a newest-first hint. Scan
        -- wrappers remain transient. Visible candidates are preferred, then
        -- at most the small cap of unseen hidden candidates is admitted.
        for pass = 1, 2 do
            for index = #layers, 1, -1 do
                local layer = layers[index]
                local visible = is_visible(layer)
                if (pass == 1 and visible)
                    or (pass == 2 and not visible)
                then
                    remember_world_map_layer(layer, "bounded-resume-scan", false)
                end
            end
        end
    end

    needs_rescan = false
    record_candidate_metrics("scan")
end

local function find_world_map_layer()
    if not access_allowed() then
        return nil
    end

    if needs_rescan then
        scan_world_map_layers()
    end

    if entry_current(cached_entry)
        and is_visible(cached_entry.object)
    then
        return cached_entry
    end

    cached_entry = nil

    for index = #candidates, 1, -1 do
        local entry = candidates[index]

        if not entry_current(entry) then
            table.remove(candidates, index)
        elseif not is_valid(entry.object) then
            retire_identity(entry.identity)
            table.remove(candidates, index)
        elseif is_visible(entry.object) then
            cached_entry = entry
            return entry
        end
    end

    return nil
end

local function detect_map_id(layer)
    local image = safe_get(layer, "Map_0_0")
    local brush = safe_get(image, "Brush")
    local resource = safe_get(brush, "ResourceObject")
    local name = full_name(resource)

    if string.find(name, "_W2_", 1, true)
        or string.find(name, "world_02", 1, true)
    then
        return 200
    end

    if string.find(name, "_W1_", 1, true)
        or string.find(name, "world_01", 1, true)
    then
        return 100
    end

    return nil
end

local function get_map_data(map_id)
    if map_id == nil then
        return nil
    end

    if cached_data[map_id] ~= nil then
        return cached_data[map_id]
    end

    local ok, objects = pcall(function()
        return FindAllOf("DWorldMapData")
    end)

    if not ok or objects == nil then
        return nil
    end

    for _, object in ipairs(objects) do
        local info = safe_get(object, "WorldMapDataInfo")

        local values_ok, data = pcall(function()
            if info == nil or tonumber(info.MapID) ~= map_id then
                return nil
            end

            return {
                map_id = map_id,
                dimensions = tonumber(info.MapDimensions),
                ui_size = tonumber(info.WorldMapUISize),
            }
        end)

        if values_ok
            and data ~= nil
            and data.dimensions ~= nil
            and data.dimensions > 0.0
            and data.ui_size ~= nil
            and data.ui_size > 0.0
        then
            cached_data[map_id] = data
            return data
        end
    end

    return nil
end

function WorldMap.reset()
    -- Drop every UObject wrapper retained from the previous world.
    for _, entry in ipairs(candidates) do
        retire_identity(entry.identity)
    end
    cached_entry = nil
    cached_data = {}
    candidates = {}
    candidate_token = candidate_token + 1
    needs_rescan = true
    wake_hint = false
    record_candidate_metrics("reset")
end

function WorldMap.recover_session()
    -- A failed active read may be a transient UMG property gap rather than a
    -- confirmed map close. Drop every wrapper immediately, then request one
    -- bounded current-epoch rescan on the next 250 ms control sample. No
    -- UObject survives the failure and no active-loop enumeration is added.
    cached_entry = nil
    candidates = {}
    candidate_token = candidate_token + 1
    needs_rescan = true
    wake_hint = true
    record_candidate_metrics("recover-session")
end

function WorldMap.has_retained_candidates()
    -- Pure scalar hint: the control loop may cheaply test the visibility of
    -- at most two already-bounded candidates without enumerating UObjects.
    return not suspended and #candidates > 0
end

function WorldMap.consume_wake_hint()
    local hinted = wake_hint
    wake_hint = false
    return hinted
end

function WorldMap.set_suspended(value, epoch)
    local requested_epoch = epoch ~= nil
        and (tonumber(epoch) or lifecycle_epoch)
        or lifecycle_epoch
    local epoch_changed = requested_epoch ~= lifecycle_epoch
    if epoch_changed then
        lifecycle_epoch = requested_epoch
        WorldMap.reset()
    end
    local new_value = value == true

    if suspended == new_value then
        if not new_value then
            needs_rescan = true
        end
        return
    end

    suspended = new_value

    if suspended then
        WorldMap.reset()
    else
        -- The new world has finished loading. Search for newly created widgets.
        needs_rescan = true
    end
end

function WorldMap.initialize(
    log,
    is_current_generation,
    is_lifecycle_current,
    diagnostics
)
    perf_diagnostics = diagnostics
    access_guard = type(is_lifecycle_current) == "function"
        and is_lifecycle_current or access_guard
    suspended = true
    WorldMap.reset()

    local notify_ok, notify_error = pcall(function()
        NotifyOnNewObject(
            "/Script/DSClient.DLayerMap",
            function(created_object)
                if is_current_generation() and access_allowed() then
                    remember_world_map_layer(
                        created_object,
                        "notify-current-epoch",
                        true
                    )
                    record_candidate_metrics("notify")
                end
            end
        )
    end)

    if not notify_ok then
        log(
            "World-map creation observer failed: "
                .. tostring(notify_error)
        )
    end
end

function WorldMap.read_state()
    if not access_allowed() then
        return nil
    end

    local entry = find_world_map_layer()
    if not entry_current(entry) then
        return nil
    end
    local layer = entry.object
    if not is_valid(layer) then return nil end

    local visible_ok, visible = safe_call(layer, "IsVisible")
    if visible_ok and visible == false then
        cached_entry = nil
        return nil
    end

    local map_id = detect_map_id(layer)
    local data = get_map_data(map_id)

    if data == nil then
        return nil
    end

    local overlay = safe_get(layer, "MapOverlay")
    local slot = safe_get(layer, "MapOverlaySlot")

    if overlay == nil or slot == nil then
        return nil
    end

    local layout_ok, layout = pcall(
        StaticFindObject,
        "/Script/UMG.Default__WidgetLayoutLibrary"
    )

    if not layout_ok or layout == nil then
        return nil
    end

    local position_ok, position = safe_call(slot, "GetPosition")
    local viewport_ok, viewport_size = safe_call(
        layout,
        "GetViewportSize",
        layer
    )
    local viewport_scale_ok, viewport_scale = safe_call(
        layout,
        "GetViewportScale",
        layer
    )

    local transform = safe_get(overlay, "RenderTransform")
    local scale = safe_get(transform, "Scale")
    local player = safe_get(layer, "PlayerIconWidget")
    local player_transform = safe_get(player, "RenderTransform")
    local player_translation = safe_get(
        player_transform,
        "Translation"
    )

    if not position_ok
        or not viewport_ok
        or not viewport_scale_ok
        or position == nil
        or viewport_size == nil
        or viewport_scale == nil
        or scale == nil
        or player_translation == nil
    then
        return nil
    end

    local values_ok, state = pcall(function()
        return {
            map_id = data.map_id,
            dimensions = data.dimensions,
            ui_size = data.ui_size,
            left = tonumber(position.X),
            top = tonumber(position.Y),
            zoom = tonumber(scale.X),
            viewport_width = tonumber(viewport_size.X),
            viewport_height = tonumber(viewport_size.Y),
            viewport_scale = tonumber(viewport_scale),
            player_map_x = data.ui_size * 0.5
                + tonumber(player_translation.X),
            player_map_y = data.ui_size * 0.5
                + tonumber(player_translation.Y),
        }
    end)

    if not values_ok
        or state == nil
        or state.left == nil
        or state.top == nil
        or state.zoom == nil
        or state.zoom <= 0.0
        or state.viewport_width == nil
        or state.viewport_width <= 0.0
        or state.viewport_height == nil
        or state.viewport_height <= 0.0
        or state.viewport_scale == nil
        or state.viewport_scale <= 0.0
        or state.player_map_x == nil
        or state.player_map_y == nil
    then
        return nil
    end

    return state
end

function WorldMap.debug_state()
    return {
        retained = #candidates,
        cap = MAX_CANDIDATES,
        epoch = lifecycle_epoch,
        token = candidate_token,
        retired = #retired_order,
        needs_rescan = needs_rescan,
    }
end

return WorldMap
