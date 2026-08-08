local M = {}

function M.new(options)
    local self = {
        config = options.config,
        logger = options.logger,
        managers = options.managers,
        order = options.order,
        current_probe = "none",
        cycle = 1,
        elapsed_ms = 0,
        next_due = {},
        last_status_ms = 0,
        last_stall_warning = {},
        started = false,
    }
    local automatic = self.config.automatic
    local safety = self.config.safety or {}
    local save_monitor = self.config.external_save_monitor or {}
    local intervals = automatic.probe_intervals_ms or {}

    local function interval_for(id)
        return math.max(automatic.scheduler_tick_ms, tonumber(intervals[id]) or automatic.pass_pause_ms)
    end

    local function next_due_utc()
        local minimum = nil
        for _, id in ipairs(self.order) do
            local due = self.next_due[id]
            if due and (minimum == nil or due < minimum) then minimum = due end
        end
        local remaining = math.max(0, (minimum or self.elapsed_ms) - self.elapsed_ms)
        return os.date("!%Y-%m-%dT%H:%M:%SZ", os.time() + math.floor(remaining / 1000))
    end

    local function status_text(reason)
        local lines = {
            "DragonSwordWorldDataProbe status",
            "version=" .. tostring(self.config.version),
            "mode=independent_probe_intervals",
            "runner_started=" .. tostring(self.started),
            "cycle=" .. tostring(self.cycle),
            "current_probe=" .. tostring(self.current_probe),
            "next_due_utc=" .. next_due_utc(),
            "reason=" .. tostring(reason or "periodic"),
            "property_reads=" .. tostring(safety.property_reads == true),
            "exact_no_argument_method_calls=" .. tostring(safety.exact_no_argument_method_calls == true),
            "targeted_find_all_of_character=" .. tostring(safety.targeted_find_all_of_character == true),
            "passive_actor_lifecycle_hooks=" .. tostring(safety.passive_actor_lifecycle_hooks == true),
            "reflection=" .. tostring(safety.reflection == true),
            "mutate_game_state=" .. tostring(safety.mutate_game_state == true),
            "external_save_monitor=" .. tostring(save_monitor.enabled == true),
            "persistent_state_table=" .. tostring(save_monitor.table_name or "none"),
        }
        for _, id in ipairs(self.order) do
            local manager = self.managers[id]
            if manager then
                local fields = manager:status_fields()
                lines[#lines + 1] = table.concat({
                    "probe=" .. id,
                    "interval_ms=" .. tostring(interval_for(id)),
                    "pass=" .. tostring(fields.pass),
                    "next_index=" .. tostring(fields.next_index),
                    "step_count=" .. tostring(fields.step_count),
                    "pending=" .. tostring(fields.pending),
                    "pending_age_ms=" .. tostring(fields.pending_age_ms),
                    "active_phase=" .. tostring(fields.active_phase),
                    "pass_complete=" .. tostring(fields.pass_complete),
                }, " ")
            end
        end
        lines[#lines + 1] = "utc=" .. os.date("!%Y-%m-%dT%H:%M:%SZ")
        lines[#lines + 1] = ""
        return table.concat(lines, "\n")
    end

    function self:write_status(reason)
        self.logger:write_status(status_text(reason))
        self.last_status_ms = self.elapsed_ms
    end

    local function prepare_completed_pass(id, manager)
        if not manager:is_pass_complete() then return false end
        self.logger:write("PASS_COMPLETE", {
            mode = "automatic", probe = id, pass = manager.state.pass,
            cycle = self.cycle, step_count = #manager.steps,
        })
        manager:start_next_pass()
        self.next_due[id] = self.elapsed_ms + interval_for(id)
        return true
    end

    local function warn_if_stalled(id, manager)
        if not manager:is_pending() then return end
        local age = manager:pending_age_ms()
        local last = self.last_stall_warning[id] or 0
        if age >= automatic.stalled_pending_warning_ms
            and self.elapsed_ms - last >= automatic.stalled_pending_warning_ms
        then
            self.last_stall_warning[id] = self.elapsed_ms
            self.logger:error("PENDING_STEP_STALLED", {
                probe = id, pending_age_ms = age, active_phase = manager.active_phase,
                action = "runner_paused_without_queueing_another_native_call",
            })
        end
    end

    function self:tick()
        self.elapsed_ms = self.elapsed_ms + automatic.scheduler_tick_ms
        if self.elapsed_ms - self.last_status_ms >= automatic.status_refresh_ms then
            self:write_status("periodic")
        end

        local any_pending = false
        for _, id in ipairs(self.order) do
            local manager = self.managers[id]
            if manager then
                warn_if_stalled(id, manager)
                if manager:is_pending() then any_pending = true end
                if not manager:is_pending() then prepare_completed_pass(id, manager) end
            end
        end

        -- checkpoint.lua owns one shared pending record; never overlap probes.
        if any_pending then return end

        -- Queue at most one game-thread step per scheduler tick.
        for _, id in ipairs(self.order) do
            local manager = self.managers[id]
            if manager and not manager:is_pending()
                and not manager:is_pass_complete()
                and self.elapsed_ms >= (self.next_due[id] or 0)
            then
                self.current_probe = id
                local result = manager:queue_next()
                self.logger:write("AUTOMATIC_STEP_RESULT", {
                    probe = id, cycle = self.cycle, result = result,
                    interval_ms = interval_for(id),
                })
                self.next_due[id] = self.elapsed_ms + automatic.step_interval_ms
                self.cycle = self.cycle + 1
                break
            end
        end
    end

    function self:start()
        for index, id in ipairs(self.order) do
            self.next_due[id] = automatic.startup_delay_ms
                + ((index - 1) * automatic.scheduler_tick_ms)
        end
        self.started = true
        self.logger:write("AUTOMATIC_RUNNER_START", {
            startup_delay_ms = automatic.startup_delay_ms,
            step_interval_ms = automatic.step_interval_ms,
            scheduler_tick_ms = automatic.scheduler_tick_ms,
            probe_count = #self.order,
            scheduling = "independent_probe_intervals",
        })
        self:write_status("startup")
        local ok, err = xpcall(function()
            LoopAsync(automatic.scheduler_tick_ms, function()
                local tick_ok, tick_err = xpcall(function() self:tick() end, debug.traceback)
                if not tick_ok then
                    self.logger:error("AUTOMATIC_TICK_ERROR", { error = self.logger:trace(tick_err) })
                end
                return false
            end)
        end, debug.traceback)
        if not ok then
            self.logger:error("AUTOMATIC_RUNNER_START_FAILED", { error = self.logger:trace(err) })
            self.started = false
            self:write_status("start_failed")
            return false
        end
        return true
    end

    return self
end

return M
