local M = {}

local function join_path(path)
    if type(path) ~= "table" or #path == 0 then return "<root>" end
    return table.concat(path, ".")
end

local function make_step_id(probe_id, step, index)
    local parts = {
        probe_id,
        tostring(index),
        step.kind or "unknown",
        step.class or "unknown",
        join_path(step.path),
        step.method or "none",
        step.name or "unnamed",
    }
    return table.concat(parts, "|")
end

function M.new(options)
    local self = {
        config = options.config,
        logger = options.logger,
        checkpoint = options.checkpoint,
        encoder = options.encoder,
        probe = options.probe,
        pending = false,
        pending_since_ms = nil,
        state = nil,
        quarantine = nil,
        unsupported = nil,
        generation = 0,
        steps = {},
        signatures = {},
        active_phase = "none",
        once_completed = {},
        session_disabled = {},
        failure_counts = {},
    }

    local function now_ms() return os.time() * 1000 end
    local max_lua_errors = math.max(1, tonumber(
        self.config.safety and self.config.safety.max_lua_errors_per_step) or 3)

    for index, source in ipairs(self.probe.steps or {}) do
        local step = {}
        for key, value in pairs(source) do step[key] = value end
        step.kind = step.kind or "class"
        step.class_name = step.class or "<custom>"
        step.name = step.name or (step.kind .. "_" .. tostring(index))
        step.path = step.path or {}
        step.path_text = join_path(step.path)
        step.role = step.role or "unknown"
        step.purpose = step.purpose or "discovery"
        step.cadence = math.max(1, tonumber(step.cadence) or 1)
        step.id = make_step_id(self.probe.id, step, index)
        self.steps[#self.steps + 1] = step
    end

    self.state = self.checkpoint:load_state(self.probe.id)
    self.quarantine = self.checkpoint:load_quarantine()
    self.unsupported = self.checkpoint:load_unsupported()

    local function persist_state()
        local ok, err = self.checkpoint:save_state(self.state)
        if not ok then self.logger:error("STATE_WRITE_FAILED", { probe = self.probe.id, error = err or "unknown" }) end
        return ok
    end

    function self:apply_recovery(pending)
        if not pending or pending.probe ~= self.probe.id then return false end
        local id = pending.step_id
        if pending.phase == "queued" then
            self.logger:write("UNSTARTED_QUEUED_STEP_RECOVERED", {
                step_id = id, probe = pending.probe or "unknown",
                pass = pending.pass or "unknown", index = pending.index or "unknown",
                meaning = "previous_process_ended_before_game_thread_execution",
            })
            return true
        end
        local ok, err = self.checkpoint:add_quarantine(id)
        if ok then self.quarantine[id] = true end
        local pending_index = tonumber(pending.index) or 0
        if pending_index >= self.state.index then self.state.index = pending_index + 1; persist_state() end
        self.logger:error("UNFINISHED_NATIVE_STEP_QUARANTINED", {
            step_id = id, probe = pending.probe or "unknown", kind = pending.kind or "unknown",
            class = pending.class or "unknown", path = pending.path or "<root>",
            method = pending.method or "none", phase = pending.phase or "unknown",
            pass = pending.pass or "unknown", next_index = self.state.index,
            quarantine_write_ok = ok, quarantine_write_error = err or "none",
            meaning = "previous_process_ended_before_native_step_completion",
        })
        return true
    end

    function self:is_pending() return self.pending end
    function self:pending_age_ms()
        if not self.pending or not self.pending_since_ms then return 0 end
        return math.max(0, now_ms() - self.pending_since_ms)
    end
    function self:is_pass_complete() return self.state.index > #self.steps end
    function self:start_next_pass()
        if self.pending then return false, "step_pending" end
        self.state.pass = self.state.pass + 1
        self.state.index = 1
        persist_state()
        self.logger:write("NEW_PASS", { mode = "automatic", probe = self.probe.id, pass = self.state.pass, step_count = #self.steps })
        return true
    end
    function self:status_fields()
        return {
            pass = self.state.pass, next_index = self.state.index, step_count = #self.steps,
            pending = self.pending, pending_age_ms = self:pending_age_ms(), active_phase = self.active_phase,
            pass_complete = self:is_pass_complete(),
        }
    end

    local function advance_without_execution(step, reason)
        self.logger:write("STEP_SKIPPED", {
            step_id = step.id, step_name = step.name, probe = self.probe.id, kind = step.kind,
            class = step.class_name, path = step.path_text, method = step.method or "none",
            index = self.state.index, pass = self.state.pass, reason = reason,
        })
        self.state.index = self.state.index + 1
        persist_state()
    end

    local function get_next_step()
        while self.state.index <= #self.steps do
            local step = self.steps[self.state.index]
            step.index = self.state.index
            step.pass = self.state.pass
            if self.quarantine[step.id] then
                advance_without_execution(step, "quarantined")
            elseif self.unsupported[step.id] then
                advance_without_execution(step, "unsupported")
            elseif self.session_disabled[step.id] then
                advance_without_execution(step, "session_disabled")
            elseif step.once_per_session == true and self.once_completed[step.id] then
                advance_without_execution(step, "once_per_session_complete")
            elseif step.cadence > 1 and ((self.state.pass - 1) % step.cadence ~= 0) then
                advance_without_execution(step, "cadence")
            else
                return step
            end
        end
        return nil
    end

    local function pending_record(step, phase)
        return {
            step_id = step.id, step_name = step.name, probe = self.probe.id, kind = step.kind,
            class = step.class_name, path = step.path_text, method = step.method or "",
            purpose = step.purpose, phase = phase, index = step.index, pass = step.pass,
            utc = os.date("!%Y-%m-%dT%H:%M:%SZ"),
        }
    end

    local function set_phase(step, phase)
        local ok, err = self.checkpoint:write_pending(pending_record(step, phase))
        if not ok then
            self.logger:error("PENDING_PHASE_WRITE_FAILED", {
                step_id = step.id, step_name = step.name, probe = self.probe.id,
                class = step.class_name, path = step.path_text, method = step.method or "none",
                phase = phase, error = err or "unknown",
            })
            return false
        end
        self.active_phase = phase
        return true
    end

    local function finish_step(step, outcome, fields, mark_once_completed)
        self.checkpoint:clear_pending()
        if step.index >= self.state.index then self.state.index = step.index + 1 end
        persist_state()
        self.pending = false
        self.pending_since_ms = nil
        self.active_phase = "none"
        if mark_once_completed ~= false and step.once_per_session == true then
            self.once_completed[step.id] = true
        end
        fields = fields or {}
        fields.step_id = step.id
        fields.step_name = step.name
        fields.probe = self.probe.id
        fields.kind = step.kind
        fields.class = step.class_name
        fields.path = step.path_text
        fields.method = step.method or "none"
        fields.index = step.index
        fields.pass = step.pass
        fields.outcome = outcome
        self.logger:write("STEP_COMPLETE", fields)
    end

    local function record_observation(step, encoded)
        local previous = self.signatures[step.id]
        local changed = previous ~= nil and previous ~= encoded.fingerprint
        local first_seen = previous == nil
        self.signatures[step.id] = encoded.fingerprint
        local operation = step.kind
        local fields = {
            pass = step.pass, step_id = step.id, step_name = step.name, probe = self.probe.id,
            class = step.class_name, role = step.role, operation = operation, path = step.path_text,
            method = step.method or "none", purpose = step.purpose, status = encoded.status,
            value_type = encoded.value_type, value = encoded.value, fingerprint = encoded.fingerprint,
            changed = changed,
        }
        self.logger:observation(fields)
        if first_seen or changed then
            self.logger:change({
                pass = step.pass, step_id = step.id, step_name = step.name, probe = self.probe.id,
                class = step.class_name, role = step.role, operation = operation, path = step.path_text,
                method = step.method or "none", purpose = step.purpose, status = encoded.status,
                value_type = encoded.value_type, value = encoded.value,
                previous_fingerprint = previous or "none", fingerprint = encoded.fingerprint,
                first_seen = first_seen, changed = changed,
            })
        end
    end

    local function handle_step_lua_error(step, err)
        local failures = (self.failure_counts[step.id] or 0) + 1
        self.failure_counts[step.id] = failures
        local disabled = failures >= max_lua_errors
        if disabled then self.session_disabled[step.id] = true end
        self.logger:error(disabled and "STEP_LUA_ERROR_SESSION_DISABLED" or "STEP_LUA_ERROR_RETRY", {
            step_id = step.id, step_name = step.name, probe = self.probe.id, kind = step.kind,
            class = step.class_name, path = step.path_text, method = step.method or "none",
            phase = self.active_phase, failure_count = failures,
            failure_limit = max_lua_errors, retry_next_pass = not disabled,
            error = self.logger:trace(err),
        })
        finish_step(step, disabled and "lua_error_session_disabled" or "lua_error_retry", nil, false)
    end

    local function find_valid_object(step)
        if not set_phase(step, "find_first_of") then return nil, "checkpoint_failed" end
        self.logger:write("NATIVE_CALL_BEGIN", {
            operation = "FindFirstOf", step_id = step.id, step_name = step.name,
            probe = self.probe.id, class = step.class_name, path = step.path_text,
            method = step.method or "none", pass = step.pass,
        })
        local object = FindFirstOf(step.class_name)
        self.logger:write("NATIVE_CALL_RETURN", {
            operation = "FindFirstOf", step_id = step.id, step_name = step.name,
            probe = self.probe.id, class = step.class_name, returned_nil = object == nil,
        })
        if object == nil then return nil, "class_missing" end
        if not set_phase(step, "is_valid") then return nil, "checkpoint_failed" end
        local valid = object:IsValid()
        self.logger:write("NATIVE_CALL_RETURN", {
            operation = "UObject:IsValid", step_id = step.id, step_name = step.name,
            probe = self.probe.id, class = step.class_name, valid = valid,
        })
        if not valid then return nil, "invalid_object" end
        return object, "valid"
    end

    local function resolve_path(step, root)
        local current = root
        for index, segment in ipairs(step.path) do
            if current == nil then return nil, "nil_path" end
            if not set_phase(step, "path_read_" .. tostring(index) .. "_" .. segment) then
                return nil, "checkpoint_failed"
            end
            self.logger:write("NATIVE_CALL_BEGIN", {
                operation = "PropertyPathRead", step_id = step.id, step_name = step.name,
                class = step.class_name, path = step.path_text, path_index = index, segment = segment,
            })
            current = current[segment]
            self.logger:write("NATIVE_CALL_RETURN", {
                operation = "PropertyPathRead", step_id = step.id, step_name = step.name,
                class = step.class_name, path = step.path_text, path_index = index, segment = segment,
                returned_nil = current == nil, value_type = type(current),
            })
        end
        return current, "resolved"
    end

    local function custom_context(step)
        local context = {
            logger = self.logger,
            config = self.config,
            probe = self.probe,
            step = step,
            encoder = self.encoder,
        }
        function context.phase(name)
            if not set_phase(step, tostring(name or "custom")) then
                error("custom_checkpoint_failed:" .. tostring(name))
            end
            return true
        end
        function context.log(event, fields)
            fields = fields or {}
            fields.step_id = fields.step_id or step.id
            fields.step_name = fields.step_name or step.name
            fields.probe = fields.probe or self.probe.id
            fields.pass = fields.pass or step.pass
            self.logger:write(event, fields)
        end
        function context.actor_sample(fields)
            fields = fields or {}
            fields.step_name = fields.step_name or step.name
            self.logger:actor_sample(fields)
        end
        function context.lifecycle(fields)
            self.logger:lifecycle(fields or {})
        end
        function context.encode(value)
            return self.encoder.encode(value, {
                limit = self.config.max_log_value_length,
                max_items = self.config.max_table_summary_items,
            })
        end
        return context
    end

    local function normalize_custom_result(result)
        if type(result) == "table"
            and result.status ~= nil
            and result.value_type ~= nil
            and result.value ~= nil
            and result.fingerprint ~= nil
        then
            return {
                status = tostring(result.status),
                value_type = tostring(result.value_type),
                value = tostring(result.value),
                fingerprint = tostring(result.fingerprint),
            }
        end
        return self.encoder.encode(result, {
            limit = self.config.max_log_value_length,
            max_items = self.config.max_table_summary_items,
        })
    end

    local function execute_step(step)
        local safety = self.config.safety or {}
        if (step.kind == "property" or step.kind == "length")
            and safety.property_reads ~= true
        then
            local encoded = {
                status = "unsupported",
                value_type = "safety_policy",
                value = "property_reads_disabled",
                fingerprint = "safety:property_reads_disabled",
            }
            record_observation(step, encoded)
            self.session_disabled[step.id] = true
            finish_step(step, "safety_disabled", {
                status = encoded.status,
                value_type = encoded.value_type,
                session_disabled = true,
            })
            return
        end
        if step.kind == "method"
            and safety.exact_no_argument_method_calls ~= true
        then
            local encoded = {
                status = "unsupported",
                value_type = "safety_policy",
                value = "exact_no_argument_method_calls_disabled",
                fingerprint = "safety:method_calls_disabled",
            }
            record_observation(step, encoded)
            self.session_disabled[step.id] = true
            finish_step(step, "safety_disabled", {
                status = encoded.status,
                value_type = encoded.value_type,
                session_disabled = true,
            })
            return
        end

        if step.kind == "custom" then
            if type(step.run) ~= "function" then
                error("custom_step_missing_run:" .. tostring(step.name))
            end
            if not set_phase(step, "custom_entry") then
                finish_step(step, "checkpoint_failed")
                return
            end
            local result = step.run(custom_context(step))
            local encoded = normalize_custom_result(result)
            record_observation(step, encoded)
            self.failure_counts[step.id] = 0
            if encoded.status == "unsupported" then
                self.session_disabled[step.id] = true
            end
            local mark_once =
                encoded.status == "ok"
                or encoded.status == "complete"
            finish_step(step, "observation_recorded", {
                status = encoded.status, value_type = encoded.value_type,
                session_disabled = encoded.status == "unsupported",
            }, mark_once)
            return
        end

        local object, status = find_valid_object(step)
        if status ~= "valid" then
            if step.kind ~= "class" then
                record_observation(step, {
                    status = status,
                    value_type = "none",
                    value = status,
                    fingerprint = status,
                })
            end
            finish_step(step, status)
            return
        end
        if step.kind == "class" then finish_step(step, "class_found", { found = true, role = step.role }); return end

        local value, path_status = resolve_path(step, object)
        if path_status ~= "resolved" then finish_step(step, path_status); return end

        local encoded
        if step.kind == "property" then
            encoded = self.encoder.encode(value, {
                limit = self.config.max_log_value_length,
                max_items = self.config.max_table_summary_items,
            })
        elseif step.kind == "length" then
            if not set_phase(step, "container_length") then finish_step(step, "checkpoint_failed"); return end
            encoded = self.encoder.length(value)
        elseif step.kind == "method" then
            if value == nil then
                encoded = self.encoder.encode(nil, { limit = self.config.max_log_value_length })
            else
                if not set_phase(step, "method_lookup_" .. tostring(step.method)) then finish_step(step, "checkpoint_failed"); return end
                local method = value[step.method]
                if method == nil then
                    encoded = {
                        status = "unsupported",
                        value_type = "method",
                        value = "method_not_available:" .. tostring(step.method),
                        fingerprint = "method:unsupported:" .. tostring(step.method),
                    }
                    self.session_disabled[step.id] = true
                else
                    if not set_phase(step, "method_call_" .. tostring(step.method)) then finish_step(step, "checkpoint_failed"); return end
                    self.logger:write("NATIVE_CALL_BEGIN", {
                        operation = "ExactNoArgMethod", step_id = step.id, step_name = step.name,
                        class = step.class_name, path = step.path_text, method = step.method,
                    })
                    local result = method(value)
                    encoded = self.encoder.encode(result, {
                        limit = self.config.max_log_value_length,
                        max_items = self.config.max_table_summary_items,
                    })
                    self.logger:write("NATIVE_CALL_RETURN", {
                        operation = "ExactNoArgMethod", step_id = step.id, step_name = step.name,
                        class = step.class_name, path = step.path_text, method = step.method,
                        status = encoded.status, value_type = encoded.value_type,
                    })
                end
            end
        else
            error("unsupported_step_kind:" .. tostring(step.kind))
        end

        record_observation(step, encoded)
        self.failure_counts[step.id] = 0
        finish_step(step, "observation_recorded", {
            status = encoded.status,
            value_type = encoded.value_type,
            session_disabled = self.session_disabled[step.id] == true,
        })
    end

    function self:queue_next()
        if self.pending then return "busy" end
        local step = get_next_step()
        if not step then return "pass_complete" end
        if not set_phase(step, "queued") then return "checkpoint_failed" end
        self.pending = true
        self.pending_since_ms = now_ms()
        self.generation = self.generation + 1
        local token = self.generation
        self.logger:write("STEP_QUEUED", {
            mode = "automatic", token = token, step_id = step.id, step_name = step.name,
            probe = self.probe.id, kind = step.kind, class = step.class_name,
            path = step.path_text, method = step.method or "none", purpose = step.purpose,
            index = step.index, pass = step.pass,
        })

        local queue_ok, queue_err = xpcall(function()
            ExecuteInGameThread(function()
                if token ~= self.generation then return end
                local ok, err = xpcall(function() execute_step(step) end, debug.traceback)
                if not ok then handle_step_lua_error(step, err) end
            end)
        end, debug.traceback)
        if not queue_ok then handle_step_lua_error(step, queue_err); return "lua_error" end
        return "queued"
    end

    return self
end

return M
