return {
    version = "1.0.55",
    state_schema = 155,
    auto_start_external_monitor = true,

    probe_order = {
        "healthcheck",
    },

    probe_modules = {
        healthcheck = "modules\\healthcheck.lua",
        mole_completion_state = "modules\\mole_completion_state.lua",
        assault_runtime_table = "modules\\assault_runtime_table.lua",
        treasure_runtime_discovery = "modules\\treasure_runtime_discovery.lua",
        treasure_candidate_validator = "modules\\treasure_candidate_validator.lua",
        treasure_actor_state_chain = "modules\\treasure_actor_state_chain.lua",
        treasure_real_actor_proximity = "modules\\treasure_real_actor_proximity.lua",
        treasure_blueprint_catalog = "modules\\treasure_blueprint_catalog.lua",
        assault_environment_snapshot = "modules\\assault_environment_snapshot.lua",
        assault_quest_trigger_correlation = "modules\\assault_quest_trigger_correlation.lua",
        treasure_state_hooks = "modules\\treasure_state_hooks.lua",
        treasure_actor_snapshot = "modules\\treasure_actor_snapshot.lua",
        assault_condition_transition = "modules\\assault_condition_transition.lua",
        assault_target_presence_pair = "modules\\assault_target_presence_pair.lua",
        assault_spawn_condition_diagnostics = "modules\\assault_spawn_condition_diagnostics.lua",
        assault_condition_correlation = "modules\\assault_condition_correlation.lua",
        class_presence = "modules\\class_presence.lua",
        property_snapshot = "modules\\property_snapshot.lua",
        bounded_object_snapshot = "modules\\bounded_object_snapshot.lua",
        ui_snapshot = "modules\\ui_snapshot.lua",
        targeted_hook = "modules\\targeted_hook.lua",
    },

    automatic = {
        enabled = true,
        startup_delay_ms = 1000,
        step_interval_ms = 500,
        pass_pause_ms = 500,
        scheduler_tick_ms = 500,
        probe_intervals_ms = {
        },
        stalled_pending_warning_ms = 60000,
        status_refresh_ms = 15000,
    },

    module_settings = {
        class_presence = { targets = {} },
        property_snapshot = { targets = {} },
        bounded_object_snapshot = {
            targets = {},
            default_max_objects = 64,
            default_fields = { "ID", "CID", "UID", "UIDName", "GroupID", "SectionUID" },
        },
        ui_snapshot = { targets = {}, default_max_objects = 128 },
        targeted_hook = {
            hooks = {},
            denied_path_tokens = { "ReceiveDestroyed", "ReceiveEndPlay" },
        },
    },

    safety = {
        automatic_sampling = true,
        single_native_step = true,
        explicit_whitelist_only = true,
        property_reads = true,
        exact_no_argument_method_calls = true,
        max_lua_errors_per_step = 2,
        targeted_find_all_of_character = false,
        find_all_of = true,
        passive_actor_lifecycle_hooks = false,
        native_hooks = false,
        reflection = false,
        notify_on_new_object = false,
        global_begin_play_hooks = false,
        mutate_game_state = false,
        enumerate_unknown_containers = false,
        stringify_userdata = false,
    },

    external_save_monitor = { enabled = false, table_name = "none" },
    max_log_value_length = 2000,
    max_table_summary_items = 24,
}
