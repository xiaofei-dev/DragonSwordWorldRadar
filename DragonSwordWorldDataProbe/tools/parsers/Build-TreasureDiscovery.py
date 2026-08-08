from __future__ import annotations

import argparse
import csv
import json
import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

FUNCTION_RE = re.compile(r"\bFunction\s+(?P<path>/Script/DS\.[^\s\[]+)")
PROPERTY_RE = re.compile(
    r"\b(?P<type>[A-Za-z0-9_]+Property)\s+"
    r"(?P<function>/Script/DS\.[^:\s\[]+:[^:\s\[]+):"
    r"(?P<name>[^\s\[]+)"
)
TYPE_RE = re.compile(
    r"\b(?P<kind>Class|ScriptStruct|Struct|Enum)\s+"
    r"(?P<path>/Script/DS\.[^\s\[]+)"
)

STRONG_TOKENS = ("treasurebox", "treasure_box", "treasure", "chest")
STATE_TOKENS = (
    "opened", "isopen", "clear", "collect", "claim", "gather",
    "obtain", "standalone", "completion", "complete", "state",
)
QUERY_PREFIXES = ("cis", "is", "has", "can", "check", "get", "query")
MUTATION_TOKENS = (
    "set", "add", "remove", "grant", "spawn", "destroy", "unlock",
    "refresh", "save", "claimreward", "openbox", "opentreasure",
)

NUMERIC_PROPERTY_TOKENS = (
    "intproperty", "int64property", "uintproperty", "uint64property",
    "byteproperty", "enumproperty",
)


@dataclass
class Property:
    name: str
    property_type: str


@dataclass
class Function:
    path: str
    name: str
    properties: list[Property] = field(default_factory=list)
    runtime_seen: bool = False
    runtime_score: int | None = None
    runtime_classification: str = ""
    runtime_call_shape: str = ""


def lower(value: str) -> str:
    return value.lower()


def function_name(path: str) -> str:
    return path.rsplit(":", 1)[-1]


def owner_name(path: str) -> str:
    return path.split(":", 1)[0]


def is_relevant(path: str) -> bool:
    text = lower(path)
    if any(token in text for token in STRONG_TOKENS):
        return True

    if text.startswith("/script/ds.detutil:") and any(
        token in text for token in STATE_TOKENS
    ):
        return True

    return False


def classify(fn: Function) -> dict[str, object]:
    name = lower(fn.name)
    path = lower(fn.path)

    score = 0
    if "treasurebox" in path or "treasure_box" in path:
        score += 60
    elif "treasure" in path:
        score += 50
    elif "chest" in path:
        score += 40

    if "instandalone" in name:
        score += 25

    query_prefix = any(name.startswith(prefix) for prefix in QUERY_PREFIXES)
    if query_prefix:
        score += 20

    return_type = ""
    bool_return = False
    world_context = False
    numeric_inputs = 0
    bool_inputs = 0
    parameters: list[str] = []

    for prop in fn.properties:
        p_name = lower(prop.name)
        p_type = lower(prop.property_type)

        if p_name == "returnvalue":
            return_type = prop.property_type
            bool_return = "boolproperty" in p_type
            continue

        parameters.append(f"{prop.name}:{prop.property_type}")

        if p_name == "worldcontextobject":
            world_context = True
        if any(token in p_type for token in NUMERIC_PROPERTY_TOKENS):
            numeric_inputs += 1
        if "boolproperty" in p_type:
            bool_inputs += 1

    if bool_return:
        score += 30
    if world_context:
        score += 15
    if numeric_inputs == 1:
        score += 20
    elif numeric_inputs == 2:
        score += 10

    mutation = any(token in name for token in MUTATION_TOKENS)
    if mutation and not query_prefix:
        classification = "mutation_or_event"
        score -= 80
    elif (
        bool_return
        and query_prefix
        and 1 <= numeric_inputs <= 2
        and any(token in path for token in STRONG_TOKENS)
    ):
        classification = "safe_bool_query"
    elif bool_return and any(token in path for token in STRONG_TOKENS):
        classification = "read_candidate"
    elif any(token in path for token in STRONG_TOKENS):
        classification = "schema_only"
    else:
        classification = "indirect_candidate"

    if bool_return and world_context and numeric_inputs == 1 and bool_inputs == 0:
        call_shape = "world_id"
    elif bool_return and not world_context and numeric_inputs == 1 and bool_inputs == 0:
        call_shape = "id_only"
    elif bool_return and world_context and numeric_inputs == 1 and bool_inputs == 1:
        call_shape = "world_id_bool"
    else:
        call_shape = "unknown"

    return {
        "FunctionName": fn.name,
        "FunctionPath": fn.path,
        "Owner": owner_name(fn.path),
        "Parameters": ";".join(parameters),
        "ReturnType": return_type,
        "Score": score,
        "Classification": classification,
        "CallShape": call_shape,
        "RuntimeSeen": fn.runtime_seen,
        "DiscoverySources": "object_dump+runtime" if fn.runtime_seen else "object_dump",
    }


def parse_object_dump(path: Path) -> tuple[list[Function], list[dict[str, str]], list[str]]:
    if not path.exists():
        return [], [], []

    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    functions: dict[str, Function] = {}
    raw: list[str] = []
    types: dict[tuple[str, str], dict[str, str]] = {}

    for line in lines:
        f_match = FUNCTION_RE.search(line)
        if f_match:
            f_path = f_match.group("path")
            if is_relevant(f_path):
                functions.setdefault(
                    f_path,
                    Function(path=f_path, name=function_name(f_path)),
                )
                raw.append(line)
            continue

        p_match = PROPERTY_RE.search(line)
        if p_match:
            f_path = p_match.group("function")
            if f_path in functions:
                functions[f_path].properties.append(
                    Property(
                        name=p_match.group("name"),
                        property_type=p_match.group("type"),
                    )
                )
                raw.append(line)
            elif is_relevant(f_path):
                functions.setdefault(
                    f_path,
                    Function(path=f_path, name=function_name(f_path)),
                ).properties.append(
                    Property(
                        name=p_match.group("name"),
                        property_type=p_match.group("type"),
                    )
                )
                raw.append(line)
            continue

        t_match = TYPE_RE.search(line)
        if t_match and any(
            token in lower(t_match.group("path")) for token in STRONG_TOKENS
        ):
            key = (t_match.group("kind"), t_match.group("path"))
            types[key] = {
                "Kind": t_match.group("kind"),
                "Path": t_match.group("path"),
            }
            raw.append(line)
            continue

        text = lower(line)
        if (
            "/script/ds." in text
            and any(token in text for token in STRONG_TOKENS)
            and len(raw) < 50000
        ):
            raw.append(line)

    return list(functions.values()), list(types.values()), raw


def read_runtime_functions(path: Path) -> dict[str, dict[str, str]]:
    if not path.exists():
        return {}

    with path.open("r", encoding="utf-8-sig", newline="") as handle:
        return {
            row.get("FunctionPath", ""): row
            for row in csv.DictReader(handle, delimiter="\t")
            if row.get("FunctionPath")
        }


def merge_runtime(functions: list[Function], runtime: dict[str, dict[str, str]]) -> None:
    by_path = {fn.path: fn for fn in functions}

    for path, row in runtime.items():
        fn = by_path.get(path)
        if fn is None:
            fn = Function(path=path, name=row.get("FunctionName") or function_name(path))
            parameters = row.get("Parameters", "")
            for token in [x for x in parameters.split(";") if x]:
                name, _, property_type = token.partition(":")
                fn.properties.append(Property(name=name, property_type=property_type))
            return_type = row.get("ReturnType", "")
            if return_type:
                fn.properties.append(Property(name="ReturnValue", property_type=return_type))
            functions.append(fn)
            by_path[path] = fn

        fn.runtime_seen = True
        try:
            fn.runtime_score = int(row.get("Score", ""))
        except ValueError:
            fn.runtime_score = None
        fn.runtime_classification = row.get("Classification", "")
        fn.runtime_call_shape = row.get("CallShape", "")


def write_tsv(path: Path, rows: list[dict[str, object]]) -> None:
    if not rows:
        path.write_text("", encoding="utf-8")
        return

    fields: list[str] = []
    seen: set[str] = set()
    for row in rows:
        for key in row:
            if key not in seen:
                seen.add(key)
                fields.append(key)

    with path.open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)


def parse_catalog(path: Path) -> list[int]:
    if not path.exists():
        return []
    text = path.read_text(encoding="utf-8", errors="ignore")
    return sorted({int(x) for x in re.findall(r"\bsave_id\s*=\s*(\d+)", text)})


def load_bridge(path: Path) -> dict:
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError):
        return {}


def newest_bridge(paths: Iterable[Path]) -> dict:
    states = [load_bridge(path) for path in paths]
    states = [state for state in states if state]
    if not states:
        return {}
    return max(
        states,
        key=lambda state: (
            int(state.get("producerGeneration", -1)),
            int(state.get("stateSequence", -1)),
        ),
    )


def parse_visible_unopened(bridge: dict) -> list[dict[str, object]]:
    result = []
    for point in bridge.get("points") or []:
        save_id = point.get("saveId")
        if isinstance(save_id, (int, float)):
            result.append(
                {
                    "SaveID": int(save_id),
                    "Evidence": "WorldRadar bridge point",
                    "GroundTruth": "unopened_one_sided",
                    "Caveat": "Visible/current-state subset only; absence is not opened evidence.",
                }
            )
    unique = {row["SaveID"]: row for row in result}
    return [unique[key] for key in sorted(unique)]


def parse_opened_deltas(path: Path) -> list[dict[str, object]]:
    if not path.exists():
        return []

    rows: dict[int, dict[str, object]] = {}
    text = path.read_text(encoding="utf-8", errors="ignore")
    for line in text.splitlines():
        if "Save-state bit delta:" not in line:
            continue

        match = re.search(r"newlyOpened=([^;]+)", line)
        if not match:
            continue

        payload = match.group(1).strip()
        if payload.lower() == "none":
            continue

        for token in payload.split(","):
            token = token.strip()
            if token.isdigit():
                save_id = int(token)
                rows[save_id] = {
                    "SaveID": save_id,
                    "Evidence": "DragonSwordWorldRadar debug save-state delta",
                    "GroundTruth": "opened",
                    "Caveat": "Authoritative bit transition from tb_treasure_box.",
                }

    return [rows[key] for key in sorted(rows)]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--object-dump", required=False, default="")
    ap.add_argument("--runtime-functions", required=False, default="")
    ap.add_argument("--catalog", required=True)
    ap.add_argument("--bridge-a", required=False, default="")
    ap.add_argument("--bridge-b", required=False, default="")
    ap.add_argument("--radar-log", required=False, default="")
    ap.add_argument("--output", required=True)
    args = ap.parse_args()

    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)

    object_dump = Path(args.object_dump) if args.object_dump else Path("__missing__")
    runtime_path = (
        Path(args.runtime_functions) if args.runtime_functions else Path("__missing__")
    )

    functions, types, raw = parse_object_dump(object_dump)
    runtime = read_runtime_functions(runtime_path)
    merge_runtime(functions, runtime)

    rows = [classify(fn) for fn in functions]
    rows.sort(key=lambda row: (-int(row["Score"]), str(row["FunctionPath"])))

    safe = [row for row in rows if row["Classification"] == "safe_bool_query"]
    read_candidates = [
        row for row in rows
        if row["Classification"] in {"safe_bool_query", "read_candidate"}
    ]

    catalog_ids = parse_catalog(Path(args.catalog))

    bridge = newest_bridge(
        [
            Path(args.bridge_a) if args.bridge_a else Path("__missing__"),
            Path(args.bridge_b) if args.bridge_b else Path("__missing__"),
        ]
    )
    visible = parse_visible_unopened(bridge)
    opened = parse_opened_deltas(
        Path(args.radar_log) if args.radar_log else Path("__missing__")
    )

    write_tsv(output / "treasure-function-candidates.tsv", rows)
    write_tsv(output / "treasure-safe-query-candidates.tsv", safe)
    write_tsv(output / "treasure-types.tsv", types)
    write_tsv(output / "treasure-visible-unopened-sample.tsv", visible)
    write_tsv(output / "treasure-opened-delta-sample.tsv", opened)
    (output / "treasure-objectdump-raw.txt").write_text(
        "\n".join(raw) + ("\n" if raw else ""),
        encoding="utf-8",
    )

    top = safe[0] if safe else (read_candidates[0] if read_candidates else None)
    validation_ready = bool(safe and visible and opened)

    plan = {
        "schema_version": 1,
        "selected_candidate": top,
        "opened_ground_truth_ids": [row["SaveID"] for row in opened[:5]],
        "unopened_ground_truth_ids": [row["SaveID"] for row in visible[:5]],
        "validation_ready": validation_ready,
        "requirements": [
            "Exact read-only bool-returning function path.",
            "At least one authoritative opened ID.",
            "At least one authoritative or one-sided unopened ID.",
            "Both sides must match before replacing tb_treasure_box in Radar.",
        ],
        "safety": {
            "unknown_functions_invoked_in_discovery": 0,
            "mutation_candidates_auto_called": False,
            "candidate_validator_default_enabled": False,
        },
    }
    (output / "treasure-validation-plan.json").write_text(
        json.dumps(plan, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    summary = {
        "schema_version": 1,
        "object_dump_found": object_dump.exists(),
        "runtime_function_report_found": runtime_path.exists(),
        "catalog_record_count": len(catalog_ids),
        "function_candidate_count": len(rows),
        "read_candidate_count": len(read_candidates),
        "safe_bool_query_count": len(safe),
        "top_candidate": top,
        "visible_unopened_sample_count": len(visible),
        "opened_delta_sample_count": len(opened),
        "validation_ready": validation_ready,
        "unknown_functions_invoked": 0,
        "next_action": (
            "Populate treasure_candidate.lua and validate both opened and unopened IDs."
            if validation_ready
            else "Review candidate signatures; obtain both opened and unopened ground-truth IDs before invoking any candidate."
        ),
    }
    (output / "treasure-discovery-summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
