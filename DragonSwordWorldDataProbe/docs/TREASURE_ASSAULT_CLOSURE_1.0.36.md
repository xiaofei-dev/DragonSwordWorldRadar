# Treasure and Assault Closure 1.0.36

## Scope

This release keeps Boss research frozen and changes only the active Treasure and Assault evidence paths.

## Treasure evidence chain

The runtime now records exact TreasureBox Actor coordinates before player initialization, then maps each loaded Actor to the nearest same-class static Treasure record. The Actor report preserves UID, UIDName, CID, PropID, static coordinates, Actor coordinates, and XY/Z match distances.

The expanded proximity and class schemas use `treasure-proximity-current-v2.tsv`, `treasure-proximity-history-v2.tsv`, and `treasure-real-class-current-v2.tsv`. Earlier reports remain immutable.

Nearby state outcomes are deliberately separate:

- `unopened_present`: exactly one Actor matches the static point.
- `opened_after_observed_presence`: a previously observed exact match becomes absent long enough to pass the transition threshold.
- `selective_absence_observed`: the target remains absent while the nearby Treasure Actor zone is demonstrably loaded.
- `absence_unobservable`: no sufficient loaded-zone evidence exists; absence does not accumulate.
- `ambiguous_actor_match`: multiple Actors match; the result fails closed.
- `reappeared`: an absent target becomes present and its diagnostic missing state is cleared.

These remain diagnostic outcomes until correlated with opened and unopened save-database ground truth.

## Scheduling

Treasure and Assault now have independent intervals. The scheduler queues at most one game-thread step at a time because the framework owns a single pending checkpoint file. Treasure receives a fast interval; Assault retains a low-frequency 10-ID batch.

## Assault recovery

A checkpoint whose last phase is only `queued` proves that game-thread execution did not start. It is retried on the next process instead of being quarantined. Any later unfinished phase remains fail-closed and is quarantined.

## Safety

- No unknown function is invoked.
- No global Character or Actor scan is introduced.
- Only the 11 exact TreasureBox generated classes are enumerated.
- Assault remains `ISAll=false`, 10 IDs per batch, with fresh validated handles.
- No lifecycle hook or game-state mutation is introduced.
