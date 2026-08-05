local M = {}

-- Runtime Character enumeration is intentionally disabled. In this game a
-- missing streamed actor does not prove that a field boss is dead, and global
-- Character scans cause visible frame-time spikes. Lua publishes the static
-- boss catalog; the Overlay applies tb_actor_respawn cooldown state.
local records = nil
local states = {}
local log = nil

function M.initialize(boss_records, logger, state_file)
    records = boss_records or {}
    log = logger
    states = {}
    for _, record in ipairs(records) do
        states[record.boss_id] = {
            visible = true,
            status = "save-cooldown-driven",
        }
    end

    -- Old actor-derived death records are no longer authoritative. Remove the
    -- file so an earlier false positive cannot hide a marker after upgrading.
    if state_file ~= nil then
        pcall(function() os.remove(state_file) end)
    end

    if log ~= nil then
        log(string.format(
            "World boss tracker initialized with %d static records; " ..
            "Overlay save-cooldown filtering enabled; Character discovery disabled",
            #records))
    end
end

function M.update(player_x, player_y, now_ms, current_map_id)
    -- Deliberately empty. The Overlay filters these static markers with
    -- tb_actor_respawn and the active RespawnCycle rule.
end

function M.snapshot()
    local result = {}
    if records == nil then return result end
    for _, record in ipairs(records) do
        local state = states[record.boss_id]
        table.insert(result, {
            boss_id = record.boss_id,
            map_id = record.map_id,
            x = record.x, y = record.y, z = record.z,
            has_z = record.z ~= nil,
            visible = state == nil or state.visible ~= false,
            status = state == nil and "save-cooldown-driven" or state.status,
            respawn_cycle_id = record.respawn_cycle_id,
            switch_week_id = record.switch_week_id,
        })
    end
    return result
end

return M
