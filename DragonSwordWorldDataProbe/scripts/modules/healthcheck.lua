return {
    id = "healthcheck",
    display_name = "Framework Health Check",
    enabled = true,
    steps = {
        {
            name = "framework_healthcheck",
            kind = "custom",
            role = "framework",
            purpose = "prove_the_modular_runner_and_logging_layer_without_reading_game_objects",
            cadence = 1,
            run = function(ctx)
                ctx.log("FRAMEWORK_HEALTHCHECK", {
                    version = ctx.config.version,
                    runtime_profile = "healthcheck_only",
                    property_reads = ctx.config.safety.property_reads,
                    find_all_of = ctx.config.safety.find_all_of,
                    native_hooks = ctx.config.safety.native_hooks,
                    mutation = ctx.config.safety.mutate_game_state,
                })
                return {
                    status = "readable",
                    value_type = "framework_health",
                    value = "ok",
                    fingerprint = "framework:ok:" .. tostring(ctx.config.version),
                }
            end,
        },
    },
}
