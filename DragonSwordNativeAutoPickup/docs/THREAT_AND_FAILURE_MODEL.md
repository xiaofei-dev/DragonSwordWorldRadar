# Threat and Failure Model

| Hazard | Mitigation |
|---|---|
| Another guessed action damages state or crashes | 0.6.0 contains no automatic interaction invocation or target-field writes. |
| Vitality evidence is generalized to ordinary drops | Discovery and correlation require a real non-template `DropItemActor` weak identity. |
| A template is mistaken for live loot | CDO, archetype, and default-subobject flags are rejected. |
| Two nearby drops make correlation ambiguous | The target is not locked unless exactly one candidate passes all gates. |
| Unrelated interaction hooks flood logs | Records are retained only when receiver, parameter, or target fields correlate to the locked identity. |
| Global discovery stalls a frame | The initial sweep is incremental with a 2 ms batch budget. |
| Always-on listeners create permanent overhead | Listeners and hooks exist only inside the owner-triggered 60-second window. |
| Stale objects survive travel | World transition resets the state to Off, clears weak identities, and requests safe cleanup. |
| Shutdown dereferences invalid UFunctions | UObject-array shutdown abandons local hook records and removes listeners without unregistering stale UFunction pointers. |
| Deletion is mistaken for universal success | Deletion is logged as one possible transition and never validates a contract. |
| Build success is called gameplay success | Owner evidence capture remains mandatory. |
