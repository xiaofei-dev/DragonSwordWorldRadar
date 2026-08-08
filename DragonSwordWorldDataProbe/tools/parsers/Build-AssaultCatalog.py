from __future__ import annotations
import argparse, csv, json, re
from collections import defaultdict
from pathlib import Path
import xml.etree.ElementTree as ET

NUMERIC = re.compile(r"(?<!\\d)-?\\d{1,20}(?!\\d)")


def logical_xml_name(path: Path) -> str:
    return re.sub(r"^\\d+_", "", path.name)

def resolve_xml(xml_dir: Path, logical_name: str) -> Path | None:
    exact = xml_dir / logical_name
    if exact.exists():
        return exact
    matches = [
        p for p in xml_dir.glob("*.xml")
        if logical_xml_name(p).lower() == logical_name.lower()
    ]
    if len(matches) == 1:
        return matches[0]
    return None

def lname(value: str) -> str:
    return value.rsplit('}', 1)[-1]

def read_rows(path: Path, expected_root: str) -> tuple[str, list[dict[str,str]]]:
    root = ET.parse(path).getroot()
    actual = lname(root.tag)
    if expected_root and actual != expected_root:
        raise ValueError(f"root mismatch for {path.name}: expected={expected_root}, actual={actual}")
    rows = []
    for index, child in enumerate(list(root)):
        row = {lname(k): v for k, v in child.attrib.items()}
        row['_row'] = str(index)
        row['_element'] = lname(child.tag)
        row['_source'] = str(path)
        rows.append(row)
    return actual, rows

def write_tsv(path: Path, rows: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = sorted({str(k) for row in rows for k in row.keys()}) if rows else ['status']
    with path.open('w', encoding='utf-8-sig', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter='\t', extrasaction='ignore')
        writer.writeheader()
        if rows:
            writer.writerows(rows)
        else:
            writer.writerow({'status': 'no_rows'})

def read_tsv_rows(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open('r', encoding='utf-8-sig', newline='') as handle:
        return [dict(row) for row in csv.DictReader(handle, delimiter='\t')]


def strip_runtime_metadata(row: dict[str, str]) -> dict[str, str]:
    result = dict(row)
    # Preserve outer/inner keys for auditing, but parser semantics use native fields.
    result['_source'] = result.get('_source', 'runtime_snapshot')
    result['_row'] = result.get('_row', '')
    result['_element'] = result.get('_element', 'runtime')
    return result


def tokens(value: object) -> list[str]:
    return NUMERIC.findall(str(value or ''))

def exact_index(rows_by_table: dict[str,list[dict]], field_map: dict[str,list[str]]) -> dict[str,list[dict]]:
    index: dict[str,list[dict]] = defaultdict(list)
    for table, fields in field_map.items():
        for row in rows_by_table.get(table, []):
            for field in fields:
                value = row.get(field, '')
                if value in ('', '0', None):
                    continue
                for token in tokens(value):
                    index[token].append({
                        'table': table, 'row': row.get('_row',''), 'field': field,
                        'value': value, 'source': row.get('_source',''), 'record': row,
                    })
    return index

def public_hit(hit: dict) -> dict:
    return {k:v for k,v in hit.items() if k != 'record'}

def lua_quote(text: object) -> str:
    value = str(text or '').replace('\\','\\\\').replace('"','\\"').replace('\r','\\r').replace('\n','\\n')
    return '"' + value + '"'

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument('--xml-dir', required=True)
    ap.add_argument('--target-profile', required=True)
    ap.add_argument('--output', required=True)
    ap.add_argument('--runtime-report-dir', required=False, default='')
    args = ap.parse_args()
    xml_dir = Path(args.xml_dir)
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    profile = json.loads(Path(args.target_profile).read_text(encoding='utf-8'))

    rows_by_table: dict[str,list[dict]] = {}
    inventory = []
    blockers = []
    target_by_file = {x['file']:x for x in profile.get('targets', [])}
    for file_name, definition in target_by_file.items():
        path = resolve_xml(xml_dir, file_name)
        table = Path(file_name).stem
        if path is None:
            inventory.append({'file': file_name, 'table': table, 'required': bool(definition.get('required')), 'status':'missing', 'row_count':0, 'expected_root':definition.get('expected_root',''), 'actual_root':''})
            if definition.get('required'):
                blockers.append(file_name)
            rows_by_table[table] = []
            continue
        try:
            actual, rows = read_rows(path, str(definition.get('expected_root','')))
            rows_by_table[table] = rows
            inventory.append({'file': file_name, 'table': table, 'required': bool(definition.get('required')), 'status':'ok', 'row_count':len(rows), 'expected_root':definition.get('expected_root',''), 'actual_root':actual})
            write_tsv(output/(table+'.tsv'), rows)
        except Exception as exc:
            rows_by_table[table] = []
            inventory.append({'file': file_name, 'table': table, 'required': bool(definition.get('required')), 'status':'parse_error', 'row_count':0, 'expected_root':definition.get('expected_root',''), 'actual_root':'', 'error':str(exc)})
            if definition.get('required'):
                blockers.append(file_name)
    runtime_report_dir = Path(args.runtime_report_dir) if args.runtime_report_dir else None
    runtime_fallback_used = []

    if runtime_report_dir:
        runtime_sources = {
            'UnexpectedMissionPlaceData': runtime_report_dir / 'assault-runtime-place.tsv',
            'UnexpectedMissionKindData': runtime_report_dir / 'assault-runtime-kind.tsv',
        }
        for table, runtime_path in runtime_sources.items():
            if not rows_by_table.get(table):
                runtime_rows = [strip_runtime_metadata(r) for r in read_tsv_rows(runtime_path)]
                if runtime_rows:
                    for index, row in enumerate(runtime_rows):
                        row['_row'] = str(index)
                        row['_source'] = str(runtime_path)
                        row['_element'] = 'runtime_snapshot'
                    rows_by_table[table] = runtime_rows
                    runtime_fallback_used.append(table)

                    logical_file = table + '.xml'
                    blockers = [x for x in blockers if x != logical_file]
                    for item in inventory:
                        if item.get('table') == table:
                            item['status'] = 'runtime_snapshot'
                            item['row_count'] = len(runtime_rows)
                            item['actual_root'] = 'DUnexpectedMissionTable'
                            item['runtime_source'] = str(runtime_path)

    write_tsv(output/'table-inventory.tsv', inventory)
    (output/'table-inventory.json').write_text(json.dumps(inventory, ensure_ascii=False, indent=2), encoding='utf-8')

    kind_rows = rows_by_table.get('UnexpectedMissionKindData', [])
    place_rows = rows_by_table.get('UnexpectedMissionPlaceData', [])

    kind_by_id = defaultdict(list)
    kind_by_group = defaultdict(list)
    for row in kind_rows:
        if row.get('ID'):
            kind_by_id[row['ID']].append(row)
        if row.get('GroupID'):
            kind_by_group[row['GroupID']].append(row)

    # Structural interpretation:
    # DUnexpectedMissionKindData is wrapped by GroupID and each group contains
    # weighted Kind rows. Place.MissionKindData therefore uses GroupID as the
    # primary relationship. Direct Kind.ID matches are kept only as diagnostics.
    place_kind = []
    places_by_kind: dict[str,list[dict]] = defaultdict(list)

    for place in place_rows:
        raw = place.get('MissionKindData','')
        group_tokens = tokens(raw)
        if not group_tokens and raw:
            group_tokens = [raw]

        for group_id in group_tokens:
            group_matches = kind_by_group.get(group_id, [])
            direct_id_matches = kind_by_id.get(group_id, [])

            if len(group_matches) > 0:
                confidence = 'structural_group_join'
            elif len(direct_id_matches) == 1:
                confidence = 'fallback_direct_id_candidate'
            elif len(direct_id_matches) > 1:
                confidence = 'fallback_direct_id_multiple'
            else:
                confidence = 'unresolved'

            row = {
                'place_id': place.get('ID',''),
                'map_id': place.get('MapID',''),
                'mission_kind_raw': raw,
                'group_id': group_id,
                'group_match_count': len(group_matches),
                'direct_id_match_count': len(direct_id_matches),
                'confidence': confidence,
                'mission_range': place.get('MissionRange',''),
                'update_target': place.get('UpdateTarget',''),
                'source_row': place.get('_row',''),
            }
            place_kind.append(row)

            if group_matches:
                for kind in group_matches:
                    if kind.get('ID'):
                        places_by_kind[kind['ID']].append(place)
            elif len(direct_id_matches) == 1:
                kind_id = direct_id_matches[0].get('ID','')
                if kind_id:
                    places_by_kind[kind_id].append(place)

    write_tsv(output/'place-kind-links.tsv', place_kind)

    field_map = {
        'SectionMonsterData':['UID','CID','GroupID','SectionUID','LevelCID','SpawnConditionID','RespawnCycleID','RandomSpawnID'],
        'SpawnMonsterGroupData':['ID'] + [f'Position{i}' for i in range(1,21)],
        'MonsterCharacterData':['ID','MonsterLinkID','StatTableID'],
        'ActorPositionData':['ID','MapGroupID'],
        'ActorSpawnConditionData':['ID','ConditionValue1','ConditionValue2','ConditionValue3'],
        'NPCSpawnConditionData':['ID','ConditionValue1','ConditionValue2','ConditionValue3'],
        'RespawnCycleData':['ID','ConditionValue1','ConditionValue2','ConditionValue3'],
        'WeatherData':['ID'], 'ClimateData':['ID'],
    }
    index = exact_index(rows_by_table, field_map)
    objective_fields = ['MissionValue1','MissionValue2','MissionValue3']
    condition_fields = ['AcceptConditionValue1','AcceptConditionValue2','AcceptConditionValue3']
    runtime_condition_rows = []
    if runtime_report_dir:
        runtime_condition_rows = read_tsv_rows(
            runtime_report_dir / 'assault-runtime-condition.tsv'
        )
        if runtime_condition_rows:
            write_tsv(output/'runtime-condition-snapshot.tsv', runtime_condition_rows)

    crossrefs = []
    condition_refs = []
    catalog = []
    unresolved = []

    for kind in kind_rows:
        kind_id = kind.get('ID','')
        objective_evidence = []
        actor_candidates = []
        for field in objective_fields:
            raw = kind.get(field,'')
            for token in tokens(raw):
                hits = [public_hit(x) for x in index.get(token, [])]
                confidence = 'confirmed_exact_unique' if len(hits)==1 else ('candidate_multiple' if hits else 'unresolved')
                evidence = {'kind_id':kind_id,'source_field':field,'source_value':raw,'token':token,'hit_count':len(hits),'confidence':confidence,'hits':hits}
                objective_evidence.append(evidence)
                if not hits:
                    unresolved.append({'kind_id':kind_id,'category':'objective','field':field,'value':raw,'token':token})
                for hit in hits:
                    crossrefs.append({
                        'kind_id':kind_id,'source_field':field,'source_value':raw,'token':token,
                        'candidate_table':hit['table'],'candidate_row':hit['row'],'candidate_field':hit['field'],
                        'candidate_value':hit['value'],'confidence':confidence
                    })
                    if hit['table']=='SectionMonsterData':
                        source_rows = rows_by_table['SectionMonsterData']
                        try:
                            actor_candidates.append(source_rows[int(hit['row'])])
                        except Exception:
                            pass

        condition_evidence = []
        for field in condition_fields:
            raw = kind.get(field,'')
            for token in tokens(raw):
                hits = [public_hit(x) for x in index.get(token, []) if x['table'] in {'ActorSpawnConditionData','NPCSpawnConditionData','WeatherData','ClimateData','RespawnCycleData'}]
                confidence = 'confirmed_exact_unique' if len(hits)==1 else ('candidate_multiple' if hits else 'unresolved')
                condition_evidence.append({'kind_id':kind_id,'source_field':field,'source_value':raw,'token':token,'hit_count':len(hits),'confidence':confidence,'hits':hits})
                if not hits:
                    unresolved.append({'kind_id':kind_id,'category':'condition','field':field,'value':raw,'token':token})
                for hit in hits:
                    condition_refs.append({
                        'kind_id':kind_id,'source_field':field,'source_value':raw,'token':token,
                        'candidate_table':hit['table'],'candidate_row':hit['row'],'candidate_field':hit['field'],
                        'candidate_value':hit['value'],'confidence':confidence
                    })

        # Deduplicate complete actor rows without asserting that they are the gameplay target.
        dedup = {}
        for actor in actor_candidates:
            key = (actor.get('UID',''),actor.get('CID',''),actor.get('SectionUID',''),actor.get('_row',''))
            dedup[key] = actor
        actors = list(dedup.values())
        actor_confidence = 'confirmed_exact_unique' if len(actors)==1 else ('candidate_multiple' if actors else 'unresolved')
        catalog.append({
            'kind_id':kind_id,
            'group_id':kind.get('GroupID',''),
            'mission_type':kind.get('MissionType',''),
            'mission_values':[kind.get('MissionValue1',''),kind.get('MissionValue2',''),kind.get('MissionValue3','')],
            'mission_count':kind.get('MissionCount',''),
            'accept_condition_type':kind.get('AcceptConditionType',''),
            'accept_condition_values':[kind.get('AcceptConditionValue1',''),kind.get('AcceptConditionValue2',''),kind.get('AcceptConditionValue3','')],
            'data_layer':kind.get('DataLayer',''),
            'places':[{
                'id':p.get('ID',''),'map_id':p.get('MapID',''),'mission_range':p.get('MissionRange',''),
                'update_target':p.get('UpdateTarget','')
            } for p in places_by_kind.get(kind_id,[])],
            'objective_cross_references':objective_evidence,
            'condition_cross_references':condition_evidence,
            'section_monster_candidates':[{k:v for k,v in a.items() if not k.startswith('_')} for a in actors],
            'actor_join_confidence':actor_confidence,
            'semantic_status':'not_interpreted',
        })

    write_tsv(output/'kind-value-crossrefs.tsv', crossrefs)
    write_tsv(output/'condition-crossrefs.tsv', condition_refs)
    (output/'assault-catalog-candidates.json').write_text(json.dumps({'schema_version':1,'status':'research_candidates','records':catalog}, ensure_ascii=False, indent=2), encoding='utf-8')
    (output/'unresolved.json').write_text(json.dumps(unresolved, ensure_ascii=False, indent=2), encoding='utf-8')

    lua = ['return {', '  schema_version = 1,', '  status = "research_candidates",', '  records = {']
    for record in catalog:
        lua.extend([
            '    {',
            f'      kind_id = {lua_quote(record["kind_id"])},',
            f'      mission_type = {lua_quote(record["mission_type"])},',
            f'      actor_join_confidence = {lua_quote(record["actor_join_confidence"])},',
            f'      candidate_actor_count = {len(record["section_monster_candidates"])},',
            '    },'
        ])
    lua.extend(['  },','}'])
    (output/'assault-catalog-candidates.lua').write_text('\n'.join(lua)+'\n', encoding='utf-8')

    status = 'success' if not blockers else ('partial' if any(rows_by_table.values()) else 'blocked')
    summary = {
        'schema_version':1, 'status':status, 'blockers':sorted(set(blockers)),
        'record_counts':{k:len(v) for k,v in rows_by_table.items()},
        'place_kind_links':len(place_kind), 'kind_records':len(kind_rows),
        'objective_cross_references':len(crossrefs), 'condition_cross_references':len(condition_refs),
        'candidate_catalog_records':len(catalog), 'unresolved_count':len(unresolved),
        'semantic_policy':'Place.MissionKindData uses structural GroupID join first. MissionType, AcceptConditionType, and CUnexpectedMissionInStandAlone booleans remain uninterpreted until runtime correlation.',
        'runtime_table_fallback_used':runtime_fallback_used,
        'runtime_condition_rows':len(runtime_condition_rows),
        'support_xml_dir':str(xml_dir),
        'final_state_chain':'static active/condition catalog + exact actor identity + existing tb_actor_respawn availability tracker',
        'boss_logic_modified':False,
        'next_action':('Correlate one known active assault with runtime condition booleans, static actor join, and tb_actor_respawn.' if status=='success' else 'Use DUnexpectedMissionTable runtime snapshot as primary Kind/Place source; retain PAK decoder as verification.')
    }
    (output/'pipeline-status.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding='utf-8')
    report = [
        '# Assault static-catalog analysis','',
        f'- Status: {status}',
        f'- Kind rows: {len(kind_rows)}',
        f'- Place rows: {len(place_rows)}',
        f'- Exact objective cross-references: {len(crossrefs)}',
        f'- Exact condition cross-references: {len(condition_refs)}',
        f'- Unresolved references: {len(unresolved)}','',
        'No task/UI completion data is used. No field semantics are inferred from names alone.',
        'The final Radar chain remains static activation/conditions + exact actor identity + existing tb_actor_respawn state.'
    ]
    (output/'REPORT.md').write_text('\n'.join(report)+'\n', encoding='utf-8')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
