return {
    -- Unified DragonSwordWorldRadar layer control. Treasure is the only implemented layer.
    show_treasures = true,
    show_bosses = false,
    show_groundhog = false,
    show_assault = false,

    -- F7 enables configured layers; F8 disables them.
    refresh_key = "F7",
    toggle_key = "F8",
    auto_start_overlay = true,

    -- DragonSwordTreasureRadar 1.6.1 behavior and display settings.
    world_map_markers = true,
    show_height = true,
    show_treasure_types = false,
    text_scale = 1.0,

    -- Low-overhead diagnostics. No per-frame disk writes are performed.
    diagnostic_logging = true,
    diagnostic_verbose = false,
    diagnostic_perf_interval_seconds = 5,
    debug_logging = false,
}
