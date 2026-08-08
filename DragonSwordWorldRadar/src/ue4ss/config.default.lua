return {
    -- Unified DragonSwordWorldRadar layer control.
    show_treasures = true,
    show_bosses = true,
    show_groundhog = false,
    show_assault = false,

    -- F7 enables configured layers; F8 disables them.
    refresh_key = "F7",
    toggle_key = "F8",
    auto_start_overlay = true,

    -- Treasure behavior and display settings.
    world_map_markers = true,
    show_height = true,
    show_treasure_types = false,
    text_scale = 1.0,

    -- Normal use log: startup, mode-independent lifecycle messages, warnings,
    -- and errors only. No periodic performance counters are collected.
    use_logging = true,

    -- Debug log: producer timings, bridge rates, Overlay frame pacing,
    -- CPU/memory samples, geometry transitions, and save/Boss diagnostics.
    -- Keep false for normal play.
    debug_logging = false,
    diagnostic_perf_interval_seconds = 5,

    -- Backward-compatible aliases retained for existing user config files.
    diagnostic_logging = true,
    diagnostic_verbose = false,
}
