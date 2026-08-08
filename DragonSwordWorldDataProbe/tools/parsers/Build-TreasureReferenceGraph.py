from __future__ import annotations
import argparse,csv,json,re
from collections import defaultdict,deque
from pathlib import Path

LINE_RE=re.compile(r"^\[(?P<addr>[0-9A-Fa-f]+)\]\s+(?P<kind>\S+)\s+(?P<name>[^\[]+?)\s+\[(?P<meta>.*)\]$")
OWNER_RE=re.compile(r"\[owr:\s*(?P<a>[0-9A-Fa-f]+)\]")
STRUCT_RE=re.compile(r"\[ss:\s*(?P<a>[0-9A-Fa-f]+)\]")
CLASS_RE=re.compile(r"\[(?:pc|ic|mc):\s*(?P<a>[0-9A-Fa-f]+)\]")

SEEDS=("treasurebox","treasure_box","dproptreasureboxdata","treasureboxpropdatamap","treasureboxlinkmonster")
OWNER_HINTS=("prop","save","state","world","client","adventure","game","database","db","manager","system")
QUERY_PREFIX=("cis","is","has","can","check","get","find","query")
QUERY_WORDS=("open","clear","complete","state","status","flag","prop","save","obtain","collect","exist","available")
MUTATE=("set","add","remove","grant","spawn","destroy","unlock","reward","claim","write","update","create","delete","reset")

def na(a): return (a or "").upper().lstrip("0") or "0"
def owner_from_path(name):
    if ":" not in name:return ""
    return name.rsplit(":",1)[0]
def fn_name(name): return name.rsplit(":",1)[-1]

def parse(path):
    entries=[]; by_addr={}; by_name={}; children=defaultdict(list)
    with path.open("r",encoding="utf-8",errors="ignore") as f:
        for i,line in enumerate(f,1):
            line=line.rstrip()
            m=LINE_RE.match(line)
            if not m:continue
            meta=m.group("meta"); name=m.group("name").strip()
            om=OWNER_RE.search(line); sm=STRUCT_RE.search(line); cm=CLASS_RE.search(line)
            e={"line":i,"addr":na(m.group("addr")),"kind":m.group("kind"),"name":name,
               "owner_addr":na(om.group("a")) if om else "",
               "struct_addr":na(sm.group("a")) if sm else "",
               "class_addr":na(cm.group("a")) if cm else ""}
            entries.append(e); by_addr[e["addr"]]=e; by_name[name]=e
            if e["owner_addr"]:children[e["owner_addr"]].append(e)
    return entries,by_addr,by_name,children

def build(entries,by_addr,by_name,children):
    seeds=[e for e in entries if any(t in e["name"].lower() for t in SEEDS)]
    dist={e["addr"]:0 for e in seeds}; reason=defaultdict(set); q=deque(dist)
    for e in seeds:reason[e["addr"]].add("seed:"+e["name"])
    reverse=defaultdict(list)
    for e in entries:
        for k in ("struct_addr","class_addr"):
            if e[k]:reverse[e[k]].append(e)
    while q:
        a=q.popleft(); d=dist[a]
        if d>=3:continue
        cur=by_addr.get(a)
        if cur and cur["owner_addr"]:
            o=cur["owner_addr"]
            if o not in dist:dist[o]=d+1;q.append(o)
            reason[o].add("owner_of:"+cur["name"])
        for e in reverse.get(a,[]):
            if e["addr"] not in dist:dist[e["addr"]]=d+1;q.append(e["addr"])
            reason[e["addr"]].add("references:"+a)
            if e["owner_addr"]:
                o=e["owner_addr"]
                if o not in dist:dist[o]=d+1;q.append(o)
                reason[o].add("owns_reference:"+e["name"])
        for e in children.get(a,[]):
            if e["addr"] not in dist:dist[e["addr"]]=d+1;q.append(e["addr"])
            reason[e["addr"]].add("owned_by:"+a)
    graph=[]
    for a,d in sorted(dist.items(),key=lambda x:(x[1],x[0])):
        e=by_addr.get(a)
        if e:graph.append({"Depth":d,"Kind":e["kind"],"Name":e["name"],"Address":a,
                           "OwnerAddress":e["owner_addr"],"Reasons":";".join(sorted(reason[a])),"Line":e["line"]})
    return seeds,graph,dist

def discovered_owner_names(entries,by_addr,dist):
    names=set()
    for a in dist:
        e=by_addr.get(a)
        if not e:continue
        if e["kind"] in ("Class","ScriptStruct","Struct"):
            names.add(e["name"])
        # Crucial: a property path names its owner even if Function lines have no [owr].
        if ":" in e["name"]:
            names.add(owner_from_path(e["name"]))
    return names

def score_function(e,owners,mode):
    path=e["name"]; owner=owner_from_path(path); f=fn_name(path); lo=f.lower(); ol=owner.lower()
    score=0
    if owner in owners:score+=70
    if any(t in path.lower() for t in SEEDS):score+=50
    if any(lo.startswith(t) for t in QUERY_PREFIX):score+=35
    if any(t in lo for t in QUERY_WORDS):score+=20
    if any(t in ol for t in OWNER_HINTS):score+=15
    if any(t in lo for t in MUTATE):score-=80
    return {"Score":score,"Mode":mode,"FunctionPath":path,"FunctionName":f,"OwnerName":owner,
            "Line":e["line"],"Classification":"query_shaped" if score>=90 else "related_function" if score>=55 else "low_priority"}

def candidates(entries,owners):
    rows=[]
    seen=set()
    for e in entries:
        if e["kind"]!="Function":continue
        owner=owner_from_path(e["name"])
        if owner in owners:
            r=score_function(e,owners,"reference_owner")
            rows.append(r);seen.add(e["name"])
    # Broad generic query sweep: names need not mention treasure. This is inventory only.
    for e in entries:
        if e["kind"]!="Function" or e["name"] in seen:continue
        owner=owner_from_path(e["name"]); f=fn_name(e["name"]).lower(); ol=owner.lower()
        query=any(f.startswith(t) for t in QUERY_PREFIX) and any(t in f for t in QUERY_WORDS)
        owner_relevant=any(t in ol for t in OWNER_HINTS)
        if query and owner_relevant:
            rows.append(score_function(e,owners,"generic_state_query"))
    rows.sort(key=lambda r:(-r["Score"],r["FunctionPath"]))
    return rows

def write(path,rows):
    if not rows:path.write_text("",encoding="utf-8");return
    fields=list(rows[0])
    with path.open("w",encoding="utf-8-sig",newline="") as f:
        w=csv.DictWriter(f,fields,delimiter="\t");w.writeheader();w.writerows(rows)

def main():
    ap=argparse.ArgumentParser();ap.add_argument("--object-dump",required=True);ap.add_argument("--output",required=True)
    a=ap.parse_args(); out=Path(a.output);out.mkdir(parents=True,exist_ok=True)
    entries,ba,bn,ch=parse(Path(a.object_dump))
    seeds,graph,dist=build(entries,ba,bn,ch)
    owners=discovered_owner_names(entries,ba,dist)
    rows=candidates(entries,owners)
    write(out/"treasure-reference-seeds.tsv",[{"Kind":e["kind"],"Name":e["name"],"Address":e["addr"],"Line":e["line"]} for e in seeds])
    write(out/"treasure-reference-graph.tsv",graph)
    write(out/"treasure-discovered-owners.tsv",[{"OwnerName":x} for x in sorted(owners)])
    write(out/"treasure-owner-function-candidates.tsv",rows)
    q=[r for r in rows if r["Classification"]=="query_shaped"]
    summary={"schema_version":1,"seed_count":len(seeds),"graph_node_count":len(graph),
             "discovered_owner_count":len(owners),"function_candidate_count":len(rows),
             "query_shaped_count":len(q),"top_candidates":rows[:40],
             "unknown_functions_invoked":0,"graph_depth":3,
             "next_action":"Inspect exact query-shaped owner/generic candidates; runtime-inventory the strongest owning class before any invocation."}
    (out/"treasure-reference-summary.json").write_text(json.dumps(summary,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
    return 0
if __name__=="__main__":raise SystemExit(main())
