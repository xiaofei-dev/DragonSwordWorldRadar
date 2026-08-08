from __future__ import annotations

import argparse
import csv
import json
from collections import Counter
from pathlib import Path


def read_tsv(path: Path) -> list[dict[str, str]]:
    if not path.exists() or path.stat().st_size == 0:
        return []
    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle, delimiter="\t"))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--reports", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    reports = Path(args.reports)
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)

    treasure_current = read_tsv(reports / "treasure-actor-current.tsv")
    treasure_history = read_tsv(reports / "treasure-actor-history.tsv")
    treasure_events = read_tsv(reports / "treasure-state-events.tsv")
    proximity_current = read_tsv(reports / "treasure-proximity-current-v2.tsv")
    proximity_history = read_tsv(reports / "treasure-proximity-history-v2.tsv")
    loaded_actors = read_tsv(reports / "treasure-real-actor-current.tsv")
    assault_current = read_tsv(reports / "assault-condition-current.tsv")
    assault_history = read_tsv(reports / "assault-condition-history.tsv")

    exact_object_matches = [
        row for row in treasure_current
        if row.get("MatchMethod") == "object_id"
    ]
    exact_name_matches = [
        row for row in treasure_current
        if row.get("MatchMethod") == "uid_name"
    ]
    interact_type4 = [
        row for row in treasure_current
        if row.get("InteractTypeValue", "").strip() == "4"
        or "treasurebox" in row.get("InteractTypeValue", "").lower()
    ]

    treasure_events_by_path = Counter(
        row.get("HookPath", "") for row in treasure_events if row.get("HookPath")
    )
    treasure_history_by_event = Counter(
        row.get("Event", "") for row in treasure_history if row.get("Event")
    )
    proximity_states = Counter(
        row.get("InferredState", "")
        for row in proximity_history
        if row.get("InferredState")
    )
    proximity_events = Counter(
        row.get("Event", "")
        for row in proximity_history
        if row.get("Event")
    )
    exact_loaded_actor_matches = [
        row for row in loaded_actors
        if row.get("WithinExactMatch", "").lower() == "true"
    ]

    assault_changes = [
        row for row in assault_history if row.get("Event") == "changed"
    ]
    assault_changed_ids = sorted({
        int(row["PlaceID"])
        for row in assault_changes
        if row.get("PlaceID", "").isdigit()
    })
    if assault_current and "InStandAlone" in assault_current[0]:
        assault_patterns = Counter(
            str(row.get("InStandAlone", ""))
            for row in assault_current
        )
    else:
        assault_patterns = Counter(
            f"{row.get('ISAllFalse')}/{row.get('ISAllTrue')}"
            for row in assault_current
        )

    summary = {
        "schema_version": 2,
        "treasure": {
            "current_candidates": len(treasure_current),
            "history_rows": len(treasure_history),
            "hook_event_rows": len(treasure_events),
            "unique_save_ids": len({
                row.get("SaveID")
                for row in treasure_current
                if row.get("SaveID")
            }),
            "exact_object_id_matches": len(exact_object_matches),
            "exact_uid_name_matches": len(exact_name_matches),
            "interact_type_treasurebox_rows": len(interact_type4),
            "history_events": dict(treasure_history_by_event),
            "hook_paths": dict(treasure_events_by_path),
            "candidate_save_ids": sorted({
                int(row["SaveID"])
                for row in treasure_current + treasure_history + treasure_events
                if row.get("SaveID", "").isdigit()
            }),
            "proximity_current_rows": len(proximity_current),
            "proximity_history_rows": len(proximity_history),
            "loaded_actor_rows": len(loaded_actors),
            "exact_loaded_actor_matches": len(exact_loaded_actor_matches),
            "proximity_states": dict(proximity_states),
            "proximity_events": dict(proximity_events),
            "opened_after_observed_presence": proximity_states.get(
                "opened_after_observed_presence", 0
            ),
            "selective_absence_observed": proximity_states.get(
                "selective_absence_observed", 0
            ),
            "reappeared_rows": proximity_events.get("reappeared", 0),
        },
        "assault": {
            "current_rows": len(assault_current),
            "history_rows": len(assault_history),
            "changed_rows": len(assault_changes),
            "changed_place_ids": assault_changed_ids,
            "current_patterns": dict(assault_patterns),
        },
        "interpretation": {
            "treasure": (
                "A directly observed exact Actor presence-to-absence transition is the "
                "strongest runtime diagnostic. Production acceptance still requires "
                "opened and unopened save-database correlation."
            ),
            "assault": (
                "A PlaceID transition during the user's known task identifies which "
                "CUnexpectedMissionInStandAlone mode tracks availability/activity/completion."
            ),
        },
    }

    (output / "transition-capture-summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
