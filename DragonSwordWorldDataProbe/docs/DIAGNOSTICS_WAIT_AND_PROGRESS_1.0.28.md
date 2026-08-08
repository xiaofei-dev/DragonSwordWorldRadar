# Diagnostics wait/progress workflow — 1.0.28

`Collect-Diagnostics.cmd` may now be started before the test session is complete.

## Visible stages

1. initialize
2. wait_for_game_exit
3. wait_for_automatic_monitor
4. collect_external_modules
5. create_staging_directory
6. copy_probe_data
7. resolve_game_layout
8. copy_ue4ss_files
9. extract_objectdump_evidence
10. write_collection_summary
11. create_zip
12. verify_and_complete

While the game remains open, the window displays:

- elapsed wait time;
- game process IDs;
- current runtime probe;
- pending step name;
- native-call/checkpoint phase.

After the game exits, the diagnostic process waits for the automatically launched
monitor to complete its post-exit module work. The named ModuleHost mutex prevents
duplicate collection; the visible collect run then reuses valid success/partial
results for the same framework version and game build.

If no automatic monitor exists, the diagnostic process applies a short file-flush
grace and runs the modules itself.

External modules can publish `progress.json`. ModuleHost polls it while the child
PowerShell process is running and prints module step/total, percent, stage and
detail. A heartbeat is printed every five seconds when no new stage arrives.

Progress files:

- runtime/reports/diagnostics-progress.json
- runtime/reports/diagnostics-progress.log
- runtime/reports/latest-module-host-progress.json
- runtime/runs/<run>/host-progress.log
- runtime/modules/<module>/current/progress.json
- runtime/modules/<module>/current/progress.log
