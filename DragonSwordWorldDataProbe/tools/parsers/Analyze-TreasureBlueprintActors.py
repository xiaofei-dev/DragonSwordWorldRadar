from __future__ import annotations
import argparse,csv,json,re
from pathlib import Path

LINE_RE=re.compile(r"^\[(?P<addr>[0-9A-Fa-f]+)\]\s+(?P<kind>\S+)\s+(?P<name>[^\[]+?)\s+\[(?P<meta>.*)\]$")
QUERY_PREFIX=("cis","is","has","can","check","get","find","query","read","return")
STATE_WORDS=("open","opened","state","status","enable","interact","death","destroy","save","load","clear","complete","respawn","spawn","disposable","contents")
MUTATE=("set","add","remove","grant","spawn","destroy","unlock","claim","reward","write","update","create","delete","reset","save")

def read_tsv(path):
 if not path.exists() or path.stat().st_size==0:return []
 with path.open("r",encoding="utf-8-sig",newline="") as f:
  return list(csv.DictReader(f,delimiter="\t"))

def member(path):return path.rsplit(":",1)[-1]
def owner(path):return path.rsplit(":",1)[0] if ":" in path else ""

def main():
 ap=argparse.ArgumentParser()
 ap.add_argument("--object-dump",required=True)
 ap.add_argument("--blueprints",required=True)
 ap.add_argument("--output",required=True)
 a=ap.parse_args()

 out=Path(a.output);out.mkdir(parents=True,exist_ok=True)
 blueprints=read_tsv(Path(a.blueprints))
 tokens={}
 for row in blueprints:
  base=row.get("BlueprintBase","").strip()
  cls=row.get("GeneratedClassShortName","").strip()
  bp=row.get("BlueprintPath","").strip()
  for token in (base,cls):
   if token:
    tokens[token.lower()]={"BlueprintPath":bp,"BlueprintBase":base,"GeneratedClassShortName":cls}

 matches=[]; funcs=[]; props=[]; classes=[]
 with Path(a.object_dump).open("r",encoding="utf-8",errors="ignore") as f:
  for lineno,line in enumerate(f,1):
   raw=line.rstrip()
   lo=raw.lower()
   hit=None
   for token,meta in tokens.items():
    if token in lo:
     hit=meta;break
   if not hit:continue

   m=LINE_RE.match(raw)
   kind=m.group("kind") if m else ""
   name=m.group("name").strip() if m else raw

   row={
    "Line":lineno,"Kind":kind,"Name":name,
    "BlueprintPath":hit["BlueprintPath"],
    "BlueprintBase":hit["BlueprintBase"],
    "GeneratedClassShortName":hit["GeneratedClassShortName"]
   }
   matches.append(row)

   if kind=="Class":
    classes.append(row)
   elif kind=="Function":
    fn=member(name);low=fn.lower()
    score=50
    if any(low.startswith(x) for x in QUERY_PREFIX):score+=30
    if any(x in low for x in STATE_WORDS):score+=40
    if any(x in low for x in MUTATE):score-=50
    funcs.append({**row,"OwnerName":owner(name),"FunctionName":fn,"Score":score})
   elif kind.endswith("Property"):
    pn=member(name);low=pn.lower()
    score=50
    if any(x in low for x in STATE_WORDS):score+=40
    if any(x in low for x in ("id","uid","guid","flag","bit")):score+=25
    props.append({**row,"OwnerName":owner(name),"PropertyName":pn,"Score":score})

 funcs.sort(key=lambda r:(-int(r["Score"]),r["Name"]))
 props.sort(key=lambda r:(-int(r["Score"]),r["Name"]))

 def write(path,rows):
  if not rows:path.write_text("",encoding="utf-8");return
  fields=list(rows[0])
  with path.open("w",encoding="utf-8-sig",newline="") as f:
   w=csv.DictWriter(f,fieldnames=fields,delimiter="\t");w.writeheader();w.writerows(rows)

 write(out/"treasure-blueprint-objectdump-matches.tsv",matches)
 write(out/"treasure-blueprint-classes.tsv",classes)
 write(out/"treasure-blueprint-functions.tsv",funcs)
 write(out/"treasure-blueprint-properties.tsv",props)

 summary={
  "schema_version":1,
  "blueprint_record_count":len(blueprints),
  "objectdump_match_count":len(matches),
  "class_count":len(classes),
  "function_count":len(funcs),
  "property_count":len(props),
  "top_functions":funcs[:50],
  "top_properties":props[:50],
  "unknown_functions_invoked":0,
  "next_action":"Use the exact generated class(es) that are loaded for ordinary TreasureBox records, then snapshot only those instances and compare open-state transitions."
 }
 (out/"treasure-blueprint-analysis-summary.json").write_text(json.dumps(summary,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
 return 0

if __name__=="__main__":raise SystemExit(main())
