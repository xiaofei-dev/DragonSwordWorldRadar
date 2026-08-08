from __future__ import annotations

import argparse
import csv
import json
import re
import xml.etree.ElementTree as ET
from collections import Counter, defaultdict
from pathlib import Path


def local(name: str) -> str:
    return name.rsplit("}", 1)[-1].split(":", 1)[-1]


def rows_from_xml(path: Path) -> list[dict[str, str]]:
    root = ET.parse(path).getroot()
    rows = []
    for element in list(root):
        row = {local(k): v for k, v in element.attrib.items()}
        row["_tag"] = local(element.tag)
        rows.append(row)
    return rows


def first(row: dict[str, str], *names: str) -> str:
    for name in names:
        value = row.get(name, "")
        if value != "":
            return value
    return ""


def blueprint_info(value: str) -> tuple[str, str, str]:
    raw = value.strip().replace("\\", "/")
    if not raw:
        return "", "", ""

    # Soft object paths can look like:
    # /Game/Path/BP_Box.BP_Box_C
    # /Game/Path/BP_Box.BP_Box
    # Blueprint'/Game/Path/BP_Box.BP_Box'
    raw = raw.strip("'\"")
    if "'" in raw:
        raw = raw.split("'", 1)[-1].rstrip("'")

    tail = raw.rsplit("/", 1)[-1]
    object_name = tail.split(".", 1)[-1] if "." in tail else tail
    base = object_name[:-2] if object_name.endswith("_C") else object_name
    generated = object_name if object_name.endswith("_C") else base + "_C"
    return raw, base, generated


def write_tsv(path: Path, rows: list[dict[str, object]]) -> None:
    if not rows:
        path.write_text("", encoding="utf-8")
        return
    fields = []
    seen = set()
    for row in rows:
        for key in row:
            if key not in seen:
                seen.add(key)
                fields.append(key)
    with path.open("w", encoding="utf-8-sig", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter="\t")
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--prop", required=True)
    ap.add_argument("--section", required=True)
    ap.add_argument("--output", required=True)
    args = ap.parse_args()

    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)

    prop_rows = rows_from_xml(Path(args.prop))
    section_rows = rows_from_xml(Path(args.section))

    normalized_props = []
    blueprint_counts = Counter()

    for row in prop_rows:
        raw_bp = first(row, "BluePrintPath", "BlueprintPath", "Blueprint", "ActorBlueprintPath")
        bp_path, bp_base, generated = blueprint_info(raw_bp)

        record = dict(row)
        record["BlueprintPathNormalized"] = bp_path
        record["BlueprintBase"] = bp_base
        record["GeneratedClassShortName"] = generated
        normalized_props.append(record)

        if generated:
            blueprint_counts[(bp_path, bp_base, generated)] += 1

    blueprint_rows = [
        {
            "BlueprintPath": path,
            "BlueprintBase": base,
            "GeneratedClassShortName": generated,
            "RecordCount": count,
        }
        for (path, base, generated), count in blueprint_counts.items()
    ]
    blueprint_rows.sort(key=lambda row: (
        row["GeneratedClassShortName"],
        row["BlueprintPath"],
    ))

    # Join evidence is deliberately broad and explicit rather than silently
    # assuming the relation. We produce candidate join counts for ID/CID/TableKey.
    prop_by_id = defaultdict(list)
    for row in normalized_props:
        for key in ("ID", "CID", "TableKey", "PropID"):
            value = row.get(key, "")
            if value:
                prop_by_id[value].append((key, row))

    join_rows = []
    resolved = 0

    for section in section_rows:
        candidates = []
        tested = []

        for section_key in ("CID", "TreasureBoxID", "PropID", "ID", "TableKey"):
            value = section.get(section_key, "")
            if not value:
                continue
            tested.append(f"{section_key}={value}")
            for prop_key, prop in prop_by_id.get(value, []):
                candidates.append((section_key, prop_key, prop))

        unique_props = {}
        for section_key, prop_key, prop in candidates:
            identity = (
                prop.get("ID", ""),
                prop.get("CID", ""),
                prop.get("BlueprintPathNormalized", ""),
            )
            unique_props[identity] = (section_key, prop_key, prop)

        if len(unique_props) == 1:
            resolved += 1
            section_key, prop_key, prop = next(iter(unique_props.values()))
            join_rows.append({
                "SectionUID": first(section, "UID", "SectionUID"),
                "SectionUIDName": first(section, "UIDName"),
                "SectionCID": first(section, "CID"),
                "SectionX": first(section, "PosX", "X"),
                "SectionY": first(section, "PosY", "Y"),
                "SectionZ": first(section, "PosZ", "Z"),
                "JoinStatus": "unique",
                "SectionJoinField": section_key,
                "PropJoinField": prop_key,
                "PropID": first(prop, "ID"),
                "PropCID": first(prop, "CID"),
                "BlueprintPath": prop.get("BlueprintPathNormalized", ""),
                "GeneratedClassShortName": prop.get("GeneratedClassShortName", ""),
                "Tested": ";".join(tested),
            })
        else:
            join_rows.append({
                "SectionUID": first(section, "UID", "SectionUID"),
                "SectionUIDName": first(section, "UIDName"),
                "SectionCID": first(section, "CID"),
                "SectionX": first(section, "PosX", "X"),
                "SectionY": first(section, "PosY", "Y"),
                "SectionZ": first(section, "PosZ", "Z"),
                "JoinStatus": "none" if len(unique_props) == 0 else "ambiguous",
                "CandidateCount": len(unique_props),
                "Tested": ";".join(tested),
            })

    write_tsv(output/"PropTreasureBoxData.tsv", normalized_props)
    write_tsv(output/"SectionTreasureBoxData.tsv", section_rows)
    write_tsv(output/"treasure-blueprint-types.tsv", blueprint_rows)
    write_tsv(output/"treasure-section-prop-join.tsv", join_rows)

    summary = {
        "schema_version": 1,
        "prop_rows": len(prop_rows),
        "section_rows": len(section_rows),
        "unique_blueprints": len(blueprint_rows),
        "section_unique_join_count": resolved,
        "section_unresolved_count": len(section_rows) - resolved,
        "top_blueprints": blueprint_rows[:50],
        "unknown_functions_invoked": 0,
        "next_action": (
            "Match exact GeneratedClassShortName values in UE4SS ObjectDump, then "
            "identify opened/state/save/interact members on the real Treasure actor classes."
        ),
    }
    (output/"treasure-static-blueprint-summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
