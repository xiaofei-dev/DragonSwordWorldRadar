# World Time and Weather Evidence

## Hidden world clock

The current read-only source is `DGameSingleton.TimeOfDay`.

- Observed representation: seconds within the game day.
- Display conversion: normalize to `0..86399`, then convert to `HH:mm:ss`.
- Observed rate: approximately sixty game seconds per real second, or approximately one game hour per real minute.
- Observed continuity: samples remained monotonic across the tested session and did not show a teleport reset.
- Startup boundary: the property can report `0` while the world and environment managers are not ready. A future consumer must treat this as unavailable during startup rather than immediately displaying midnight.
- Related field: `DGameSingleton.LoginStartTimeOfDay` was observed as a stable session-start value.

Evidence status:

- Direct property readability: confirmed research.
- Seconds-to-clock conversion: confirmed from observed values and progression.
- Global continuity across all regions and every teleport route: inferred, not fully exhaustive.
- Production clock UI: future candidate only; not implemented by DataProbe.

CID 143 was absent at game time 22:51:13 and first observed at 23:01:46. RevealCycle row 10001 is statically confirmed as 23:00-06:00 and identifies a cemetery skeleton. The CID 143 mapping is therefore strongly inferred, but still lacks a direct `MonsterSpawnBase.TableKey_RevealCycle` or generator-asset binding. The 06:00 hide transition was not observed at runtime.

## Weather state

The current read-only sources are:

- `DsEnvironmentManager.CurrentWeatherState`
- `DsEnvironmentManager.CurrentWeatherBTState`

The controlled samples repeatedly observed `CurrentWeatherState=2` and `CurrentWeatherBTState=3`. These are internal enum or state-machine values, not a probability or a user-facing weather label.

Unknowns:

- The mapping from numeric values to visible weather names.
- Whether the values are global, region-local, or replaced during some teleport routes.
- Whether CID 143 or CID 148 uses either field directly as a spawn condition.

No production behavior may label weather from these scalar values. Weather is excluded from the current Assault production model because no target-specific dependency was established.

## Safety boundary

The probe reads scalar properties only. It does not call world-time functions, change time, change weather, enumerate environment containers, or mutate game state.
