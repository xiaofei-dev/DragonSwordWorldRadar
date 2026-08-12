local M = {}

-- Runtime completion queries are intentionally disabled. Repeated crash
-- samples showed that the native standalone-completion query can throw through
-- UE4SS after quest/world lifecycle transitions even when its UObject wrappers
-- report valid. Lua pcall cannot contain that native C++ exception. The static
-- 34-record catalog remains safe, so publish every generated Mole/Fly marker
-- until a future provider can prove lifecycle-safe completion state.
local FIRST_MOLE_ID = 11001
local LAST_MOLE_ID = 11034
local EXPECTED_FLY_RECORD_COUNT = 33
local EXPECTED_CATALOG_COUNT = 83

local visible_mask = 0
local initialized = false
local context_available = false

local function bit_value(mask_bit)
    return 2 ^ mask_bit
end

function M.initialize(mole_records, log_function, _performance_diagnostics)
    local ids = {}
    local bits = {}
    visible_mask = 0
    initialized = false
    context_available = false

    local catalog_count = 0
    for _, source in ipairs(mole_records or {}) do
        catalog_count = catalog_count + 1
        local id = tonumber(source.mini_game_id)
        local mask_bit = tonumber(source.mask_bit)
        if id == nil then
            error("Generated mini-game dataset contains an invalid record")
        end
        if id >= FIRST_MOLE_ID and id <= LAST_MOLE_ID and id ~= 11024 then
            if mask_bit == nil
            or mask_bit < 0
            or mask_bit >= EXPECTED_FLY_RECORD_COUNT
            or ids[id]
            or bits[mask_bit]
            then
                error("Generated Fly dataset contains an invalid or duplicate record")
            end
            ids[id] = true
            bits[mask_bit] = true
            visible_mask = visible_mask + bit_value(mask_bit)
        elseif not ((id >= 12001 and id <= 12040) or (id >= 13001 and id <= 13010)) then
            error("Generated mini-game dataset contains an unsupported ID")
        end
    end

    local count = 0
    for _ in pairs(ids) do count = count + 1 end
    if count ~= EXPECTED_FLY_RECORD_COUNT or catalog_count ~= EXPECTED_CATALOG_COUNT then
        error(string.format(
            "Generated mini-game dataset must contain exactly %d Fly records and %d total records; loaded %d Fly/%d total",
            EXPECTED_FLY_RECORD_COUNT,
            EXPECTED_CATALOG_COUNT,
            count,
            catalog_count
        ))
    end

    initialized = true
    if log_function ~= nil then
        log_function(
            "Mini-game safety mode active: publishing 33 Fly candidate bits and 50 static Mole/Wave candidates; "
                .. "the Overlay applies reward-save filtering and unsafe runtime completion queries are disabled."
        )
    end
    return true
end

function M.refresh()
    -- Deliberately performs no UObject lookup, function resolution, or query.
    return false
end

function M.visible_mask()
    if not initialized or not context_available then return 0 end
    return visible_mask
end

function M.snapshot_ready()
    return initialized
end

function M.unfinished_count()
    return initialized and EXPECTED_CATALOG_COUNT or 0
end

function M.set_context_available(available)
    context_available = initialized and available == true
end

function M.invalidate_runtime_handles()
    -- No runtime UObject handles exist in safety mode.
end

function M.count()
    return initialized and EXPECTED_CATALOG_COUNT or 0
end

return M
