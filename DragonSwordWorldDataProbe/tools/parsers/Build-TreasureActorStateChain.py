from __future__ import annotations
import argparse,csv,json,re
from pathlib import Path

LINE_RE=re.compile(r"^\[(?P<addr>[0-9A-Fa-f]+)\]\s+(?P<kind>\S+)\s+(?P<name>[^\[]+?)\s+\[(?P<meta>.*)\]$")

FOCUS_CLASS=("treasurebox","treasure_box","interactable","animationprop","pickupprop","prop","savedata","playerdata","worldstate","progress","record","flag")
FUNCTION_STATE=("open","opened","state","status","enable","clear","complete","flag","save","record","respawn","spawn","destroy","interact","pickup","returncontents")
QUERY_PREFIX=("cis","is","has","can","check","get","find","query","read","return")
MUTATION=("set","add","remove","grant","spawn","destroy","unlock","claim","reward","write","update","create","delete","reset","save")
UI_PENALTY=("adventurebook","layer","panel","widget","worldmap","mapui","tooltip")

def owner(name): return name.rsplit(":",1)[0] if ":" in name else ""
def member(name): return name.rsplit(":",1)[-1]
def low(s): return s.lower()

def parse(path):
 rows=[]
 with path.open("r",encoding="utf-8",errors="ignore") as f:
  for i,line in enumerate(f,1):
   m=LINE_RE.match(line.rstrip())
   if m: rows.append({"line":i,"kind":m.group("kind"),"name":m.group("name").strip()})
 return rows

def owner_score(name):
 s=low(name);score=0;tags=[]
 if any(x in s for x in FOCUS_CLASS):score+=40;tags.append("focus")
 if "treasurebox" in s or "treasure_box" in s:score+=100;tags.append("treasurebox")
 if "interactable" in s:score+=70;tags.append("interaction")
 if "animationprop" in s or "pickupprop" in s:score+=70;tags.append("prop_state")
 if any(x in s for x in ("savedata","playerdata","worldstate","progress","record","flag")):
  score+=50;tags.append("persistent_state")
 if any(x in s for x in UI_PENALTY):score-=100;tags.append("ui_penalty")
 return score,";".join(tags)

def build(entries):
 owners={}
 for e in entries:
  names=[]
  if e["kind"] in ("Class","ScriptStruct","Struct"):names.append(e["name"])
  if ":" in e["name"]:names.append(owner(e["name"]))
  for n in names:
   sc,tg=owner_score(n)
   if sc>0 and (n not in owners or sc>owners[n]["Score"]):
    owners[n]={"OwnerName":n,"Score":sc,"Tags":tg,"Line":e["line"]}

 functions=[]
 for e in entries:
  if e["kind"]!="Function":continue
  o=owner(e["name"]);f=member(e["name"]);lo=low(f)
  osc=owners.get(o,{}).get("Score",0)
  if osc<=0:continue
  score=osc
  if any(lo.startswith(x) for x in QUERY_PREFIX):score+=35
  if any(x in lo for x in FUNCTION_STATE):score+=45
  if any(x in lo for x in MUTATION):score-=80
  if "open" in lo or "state" in lo or "interact" in lo:score+=20
  functions.append({"Score":score,"OwnerName":o,"OwnerTags":owners[o]["Tags"],"FunctionName":f,"FunctionPath":e["name"],"Line":e["line"],"Classification":"query_candidate" if score>=100 else "related"})

 properties=[]
 propk={"BoolProperty","IntProperty","Int64Property","UInt32Property","UInt64Property","ByteProperty","MapProperty","ArrayProperty","StructProperty","ObjectProperty","EnumProperty"}
 for e in entries:
  if e["kind"] not in propk or ":" not in e["name"]:continue
  o=owner(e["name"]);p=member(e["name"]);osc=owners.get(o,{}).get("Score",0)
  if osc<=0:continue
  text=low(p)
  score=osc
  if any(x in text for x in FUNCTION_STATE):score+=50
  if any(x in text for x in ("id","uid","cid","save","flag","bit")):score+=25
  if score>=75:
   properties.append({"Score":score,"OwnerName":o,"OwnerTags":owners[o]["Tags"],"Kind":e["kind"],"PropertyName":p,"FullPath":e["name"],"Line":e["line"]})

 functions.sort(key=lambda r:(-r["Score"],r["FunctionPath"]))
 properties.sort(key=lambda r:(-r["Score"],r["FullPath"]))
 owner_rows=sorted(owners.values(),key=lambda r:(-r["Score"],r["OwnerName"]))
 return owner_rows,functions,properties

def write(path,rows):
 if not rows:path.write_text("",encoding="utf-8");return
 fields=list(rows[0])
 with path.open("w",encoding="utf-8-sig",newline="") as f:
  w=csv.DictWriter(f,fieldnames=fields,delimiter="\t");w.writeheader();w.writerows(rows)

def main():
 ap=argparse.ArgumentParser();ap.add_argument("--object-dump",required=True);ap.add_argument("--output",required=True)
 a=ap.parse_args();out=Path(a.output);out.mkdir(parents=True,exist_ok=True)
 entries=parse(Path(a.object_dump));owners,funcs,props=build(entries)
 write(out/"treasure-actor-chain-owners.tsv",owners)
 write(out/"treasure-actor-chain-functions.tsv",funcs)
 write(out/"treasure-actor-chain-properties.tsv",props)
 summary={
  "schema_version":1,
  "owner_count":len(owners),
  "function_count":len(funcs),
  "query_candidate_count":sum(1 for r in funcs if r["Classification"]=="query_candidate"),
  "property_count":len(props),
  "top_owners":owners[:30],
  "top_functions":funcs[:50],
  "top_properties":props[:50],
  "unknown_functions_invoked":0,
  "focus":"TreasureBox Actor -> DInteractableComponent / Prop state -> persistent opened state",
  "next_action":"Pick exact non-UI read-only state accessors and validate against tb_treasure_box opened/unopened ground truth."
 }
 (out/"treasure-actor-chain-summary.json").write_text(json.dumps(summary,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
 return 0

if __name__=="__main__":raise SystemExit(main())
