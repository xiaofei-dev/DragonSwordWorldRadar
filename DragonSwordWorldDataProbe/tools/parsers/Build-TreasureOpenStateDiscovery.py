from __future__ import annotations
import argparse,csv,json,re
from pathlib import Path

LINE_RE=re.compile(r"^\[(?P<addr>[0-9A-Fa-f]+)\]\s+(?P<kind>\S+)\s+(?P<name>[^\[]+?)\s+\[(?P<meta>.*)\]$")
WORLD_SEEDS=("treasurebox","treasure_box","treasurechest","chestactor","dproptreasureboxdata","treasureboxpropdatamap","treasureboxlinkmonster","opened_bit_field","tb_treasure_box")
STATE_OWNER_HINTS=("save","savedata","playerdata","clientdata","userdata","accountdata","worldstate","worlddata","progress","record","flag","prop","interaction","interact","actorstate","persistent","database","db")
ACTOR_HINTS=("actor","component","interaction","interact","prop","treasure","chest")
QUERY_PREFIX=("cis","is","has","can","check","get","find","query","read")
OPEN_WORDS=("open","opened","clear","complete","collect","obtain","state","status","flag","bit","record","progress")
MUTATION_WORDS=("set","add","remove","grant","spawn","destroy","unlock","claim","reward","write","update","create","delete","reset","save")

def owner_from_path(name): return name.rsplit(":",1)[0] if ":" in name else ""
def member_name(name): return name.rsplit(":",1)[-1]
def low(s): return s.lower()

def parse(path):
    entries=[]
    with path.open("r",encoding="utf-8",errors="ignore") as f:
        for i,line in enumerate(f,1):
            m=LINE_RE.match(line.rstrip())
            if m:
                entries.append({"line":i,"kind":m.group("kind"),"name":m.group("name").strip()})
    return entries

def classify_owner(name):
    s=low(name); score=0; tags=[]
    if any(x in s for x in WORLD_SEEDS): score+=80; tags.append("treasure")
    if any(x in s for x in STATE_OWNER_HINTS): score+=35; tags.append("state_owner")
    if any(x in s for x in ACTOR_HINTS): score+=25; tags.append("actor_or_component")
    if "adventurebook" in s: score-=70; tags.append("adventurebook_penalty")
    if "layer" in s or "/ui/" in s or "widget" in s or "panel" in s: score-=50; tags.append("ui_penalty")
    return score,";".join(tags)

def owner_candidates(entries):
    owners={}
    for e in entries:
        names=[]
        if e["kind"] in ("Class","ScriptStruct","Struct"): names.append(e["name"])
        if ":" in e["name"]: names.append(owner_from_path(e["name"]))
        for name in names:
            score,tags=classify_owner(name)
            if score>0 and (name not in owners or score>owners[name]["Score"]):
                owners[name]={"OwnerName":name,"Kind":e["kind"],"Score":score,"Tags":tags,"Line":e["line"]}
    return owners

def function_candidates(entries,owners):
    rows=[]
    for e in entries:
        if e["kind"]!="Function": continue
        path=e["name"]; owner=owner_from_path(path); fn=member_name(path); lo=low(fn)
        owner_score=owners.get(owner,{}).get("Score",0)
        owner_tags=owners.get(owner,{}).get("Tags","")
        if owner_score<=0:
            ol=low(owner)
            if any(x in ol for x in STATE_OWNER_HINTS):
                owner_score=30; owner_tags="generic_state_owner"
            else: continue
        score=owner_score
        if any(lo.startswith(x) for x in QUERY_PREFIX): score+=35
        if any(x in lo for x in OPEN_WORDS): score+=35
        if any(x in lo for x in MUTATION_WORDS): score-=90
        if "treasure" in lo or "chest" in lo: score+=40
        classification="query_shaped" if score>=90 else "related" if score>=55 else "low_priority"
        rows.append({"Score":score,"Classification":classification,"OwnerName":owner,"OwnerTags":owner_tags,"FunctionPath":path,"FunctionName":fn,"Line":e["line"]})
    rows.sort(key=lambda r:(-int(r["Score"]),r["FunctionPath"]))
    return rows

def property_candidates(entries,owners):
    rows=[]
    prop_kinds=("BoolProperty","IntProperty","Int64Property","UInt32Property","UInt64Property","ByteProperty","MapProperty","ArrayProperty","StructProperty")
    for e in entries:
        if e["kind"] not in prop_kinds or ":" not in e["name"]: continue
        owner=owner_from_path(e["name"]); name=member_name(e["name"]); text=low(name+" "+owner)
        score=owners.get(owner,{}).get("Score",0)
        if any(x in text for x in OPEN_WORDS): score+=45
        if any(x in text for x in ("treasure","chest")): score+=50
        if score<60: continue
        rows.append({"Score":score,"Kind":e["kind"],"OwnerName":owner,"PropertyName":name,"FullPath":e["name"],"Line":e["line"]})
    rows.sort(key=lambda r:(-int(r["Score"]),r["FullPath"]))
    return rows

def write(path,rows):
    if not rows: path.write_text("",encoding="utf-8"); return
    fields=list(rows[0])
    with path.open("w",encoding="utf-8-sig",newline="") as f:
        w=csv.DictWriter(f,fieldnames=fields,delimiter="\t"); w.writeheader(); w.writerows(rows)

def main():
    ap=argparse.ArgumentParser();ap.add_argument("--object-dump",required=True);ap.add_argument("--output",required=True)
    a=ap.parse_args();out=Path(a.output);out.mkdir(parents=True,exist_ok=True)
    entries=parse(Path(a.object_dump)); owners=owner_candidates(entries); funcs=function_candidates(entries,owners); props=property_candidates(entries,owners)
    owner_rows=sorted(owners.values(),key=lambda r:(-int(r["Score"]),r["OwnerName"]))
    write(out/"treasure-openstate-owners.tsv",owner_rows)
    write(out/"treasure-openstate-functions.tsv",funcs)
    write(out/"treasure-openstate-properties.tsv",props)
    summary={"schema_version":1,"owner_count":len(owner_rows),"function_count":len(funcs),"query_shaped_count":sum(1 for r in funcs if r["Classification"]=="query_shaped"),"property_count":len(props),"top_owners":owner_rows[:30],"top_functions":funcs[:40],"top_properties":props[:40],"unknown_functions_invoked":0,"focus":"world treasure save_id -> opened state; UI/adventure-book owners penalized","next_action":"Runtime-inventory only the strongest non-UI owner classes and identify a read-only ID/state accessor."}
    (out/"treasure-openstate-summary.json").write_text(json.dumps(summary,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    return 0
if __name__=="__main__": raise SystemExit(main())
