from __future__ import annotations
import argparse,csv,json,re
from pathlib import Path
TOKENS=("UnexpectedMission","DETTask_TagQuestStepLoopInStandAlone","DLayerEvent_GoalQuest","DClientQuestSystem","DsPGQuestSubsystem","DsPGQuestMainSubsystem","QuestID","ISRegister","Register","Weather","Climate","DaySwitch","RespawnCycle")
def main():
 ap=argparse.ArgumentParser();ap.add_argument("--object-dump",required=True);ap.add_argument("--output",required=True)
 a=ap.parse_args();out=Path(a.output);out.mkdir(parents=True,exist_ok=True)
 rows=[]
 with Path(a.object_dump).open("r",encoding="utf-8",errors="ignore") as f:
  for i,line in enumerate(f,1):
   hits=[t for t in TOKENS if t.lower() in line.lower()]
   if hits:rows.append({"Line":i,"Tokens":";".join(hits),"Text":line.rstrip()})
 p=out/"assault-quest-condition-objectdump.tsv"
 with p.open("w",encoding="utf-8-sig",newline="") as f:
  w=csv.DictWriter(f,fieldnames=["Line","Tokens","Text"],delimiter="\t");w.writeheader();w.writerows(rows)
 counts={t:sum(1 for r in rows if t in r["Tokens"].split(";")) for t in TOKENS}
 (out/"assault-quest-condition-summary.json").write_text(json.dumps({"schema_version":1,"rows":len(rows),"token_counts":counts,"unknown_functions_invoked":0},ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
 return 0
if __name__=="__main__":raise SystemExit(main())
