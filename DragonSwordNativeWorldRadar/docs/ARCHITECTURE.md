# Architecture

## Runtime flow

1. UE4SS loads `dlls/main.dll`.
2. The native provider resolves Engine metadata once and resolves `K2_GetActorLocation` static function metadata once.
3. F10 starts a 750 ms stability window.
4. Every 250 ms, the game-thread sampler walks `Engine -> GameViewport -> GameInstance -> LocalPlayers[0] -> PlayerController -> Pawn`.
5. The provider calls `K2_GetActorLocation` through `ProcessEvent`, copies only X/Y/Z, and immediately discards all gameplay pointers.
6. A two-slot protocol-v5 bridge publishes scalar data under `runtime/bridge/native_radar_motion_{a,b}.dat`.
7. The independent WinForms Overlay reads the latest complete slot and extrapolates only numeric coordinates on its 50 ms presentation timer.

## Transition safety

`InitGameState` pre/post callbacks create an explicit suspension window. Missing links, invalid numeric output, and structured access violations all fail closed. A failure never triggers a global retry scan; it hides the Overlay and retries the fixed property chain after cooldown.

The Engine pointer and static UFunction metadata are process-lifetime references. Controller, Pawn, and every other gameplay instance are sample-local only.

## Isolation

The project uses its own Mod directory, F10 hotkey, mutexes, environment variables, runtime directory, logs, host process, and bridge filenames. The copied renderer keeps its historical C# namespace only as a source-level implementation detail.
