using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using DragonSwordWorldRadar.Installer;

namespace DragonSwordWorldRadar
{
    internal static class AssaultIntegrationHarness
    {
        private const string Fingerprint = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        private static int _assertions;
        public static int Main(string[] args)
        {
            string root=Path.Combine(Path.GetTempPath(),"DragonSwordWorldRadar-AssaultTests-"+Guid.NewGuid().ToString("N"));
            try
            {
                Directory.CreateDirectory(root); string place=Path.Combine(root,"place.xml"),kind=Path.Combine(root,"kind.xml"),monster=Path.Combine(root,"monster.xml"),reveal=Path.Combine(root,"reveal.xml");
                WriteFixtures(place,kind,monster,reveal);
                TestScenario(root,place,kind,monster,reveal,"zero",new string[0],0);
                TestScenario(root,place,kind,monster,reveal,"current",new[]{Condition(104,143,"5945914773662957327","DSkeletonLeader_Nam_1041101",10001,"world_time_window","confirmed-cycle_inferred-binding","direct binding unavailable")},1);
                TestScenario(root,place,kind,monster,reveal,"multiple",new[]{Condition(104,143,"5945914773662957327","DSkeletonLeader_Nam_1041101",10001,"world_time_window","p1","m1"),Condition(105,1004,"8000000000000000005","Name5",10002,"world_time_window","p2","m2")},2);
                ExpectFailure(delegate { Run(root,place,kind,monster,reveal,"bad-fingerprint",Policy(new string[0],new string('b',64)),Fingerprint); },"fingerprint mismatch");
                ExpectFailure(delegate { Run(root,place,kind,monster,reveal,"bad-identity",Policy(new[]{Condition(104,999,"5945914773662957327","DSkeletonLeader_Nam_1041101",10001,"world_time_window","p","m")},Fingerprint),Fingerprint); },"identity mismatch");
                ExpectFailure(delegate { Run(root,place,kind,monster,reveal,"unknown-type",Policy(new[]{Condition(104,143,"5945914773662957327","DSkeletonLeader_Nam_1041101",10001,"weather","p","m")},Fingerprint),Fingerprint); },"unknown condition");
                ExpectFailure(delegate { Run(root,place,kind,monster,reveal,"malformed",Policy(new[]{"<Condition place_id=\"104\" />"},Fingerprint),Fingerprint); },"malformed condition");
                string hour24=Path.Combine(root,"reveal-hour24.xml");File.WriteAllText(hour24,File.ReadAllText(reveal).Replace("HideIngametime=\"6\"","HideIngametime=\"24\""));
                ExpectFailure(delegate { Run(root,place,kind,monster,hour24,"hour24",Policy(new[]{Condition(104,143,"5945914773662957327","DSkeletonLeader_Nam_1041101",10001,"world_time_window","p","m")},Fingerprint),Fingerprint); },"hour 24 rejected");
                WorldEncounter conditioned=new WorldEncounter();
                conditioned.Conditions.Add(new WorldEncounterCondition{Type="world_time_window",VisibleFromHour=23,HiddenFromHour=6});
                Assert(!EncounterAvailabilityTracker.AreConditionsSatisfied(conditioned,true,22*3600+59*60),"22:59 hidden");
                Assert(EncounterAvailabilityTracker.AreConditionsSatisfied(conditioned,true,23*3600),"23:00 visible");
                Assert(EncounterAvailabilityTracker.AreConditionsSatisfied(conditioned,true,5*3600+59*60),"05:59 visible");
                Assert(!EncounterAvailabilityTracker.AreConditionsSatisfied(conditioned,true,6*3600),"06:00 hidden");
                Assert(!EncounterAvailabilityTracker.AreConditionsSatisfied(conditioned,false,23*3600),"conditioned unavailable time fail closed");
                Assert(EncounterAvailabilityTracker.AreConditionsSatisfied(new WorldEncounter(),false,0),"ordinary target ignores unavailable time");
                Console.WriteLine("ASSAULT_TESTS_OK assertions="+_assertions+"; fixtures=zero,current-one,multiple; runtimeCatalog=actual; runtimeTimeGate=actual"); return 0;
            }
            catch(Exception e){Console.Error.WriteLine("ASSAULT_TESTS_FAILED\n"+e);return 1;}
            finally{try{Directory.Delete(root,true);}catch{}}
        }
        private static void TestScenario(string root,string place,string kind,string monster,string reveal,string name,string[] conditions,int expected)
        {
            string output=Run(root,place,kind,monster,reveal,name,Policy(conditions,Fingerprint),Fingerprint);
            List<WorldAssault> loaded=WorldAssaultCatalog.LoadValidated(output);int actual=0;foreach(WorldAssault target in loaded)if(target.HasTimeCondition)actual++;
            Assert(loaded.Count==40,name+" parsed 40");Assert(actual==expected,name+" condition count");Assert(loaded[0].Uid=="17806813628890497727",name+" UID precision");
            if(name=="current")
            {
                string partial=Path.Combine(root,"partial.lua");File.WriteAllText(partial,File.ReadAllText(output).Replace(", missing_confirmation = \"direct binding unavailable\"",String.Empty));
                ExpectFailure(delegate{WorldAssaultCatalog.LoadValidated(partial);},"partial runtime tuple");
            }
        }
        private static string Run(string root,string place,string kind,string monster,string reveal,string name,string policyText,string fingerprint)
        {string policy=Path.Combine(root,name+"-policy.xml"),output=Path.Combine(root,name+".lua");File.WriteAllText(policy,policyText);Assert(AssaultLuaGenerator.Generate(place,kind,monster,reveal,policy,fingerprint,output)==40,name+" generator count");return output;}
        private static string Policy(string[] conditions,string fingerprint){return "<AssaultInferencePolicy schema_version=\"1\" expected_game_fingerprint=\""+fingerprint+"\">"+String.Join("",conditions)+"</AssaultInferencePolicy>";}
        private static string Condition(int place,int cid,string uid,string name,int cycle,string type,string provenance,string missing){return String.Format(CultureInfo.InvariantCulture,"<Condition place_id=\"{0}\" cid=\"{1}\" uid=\"{2}\" uid_name=\"{3}\" reveal_cycle_id=\"{4}\" condition_type=\"{5}\" provenance=\"{6}\" missing_confirmation=\"{7}\" />",place,cid,uid,name,cycle,type,provenance,missing);}
        private static void WriteFixtures(string place,string kind,string monster,string reveal)
        {
            StringBuilder p=new StringBuilder("<Rows>"),k=new StringBuilder("<Rows>"),m=new StringBuilder("<Rows>");
            for(int i=1;i<=40;i++){int placeId=100+i,cid=i==4?143:999+i;string uid=i==1?"17806813628890497727":(i==4?"5945914773662957327":(8000000000000000000UL+(ulong)i).ToString(CultureInfo.InvariantCulture));string name=i==4?"DSkeletonLeader_Nam_1041101":"Name"+i;
                p.AppendFormat(CultureInfo.InvariantCulture,"<Row ID=\"{0}\" MissionKindData=\"{0}\" MapID=\"100\" />",placeId);
                k.AppendFormat(CultureInfo.InvariantCulture,"<Row GroupID=\"{0}\" MissionType=\"DEFEAT_MONSTER\" MissionValue1=\"{1}\" AcceptConditionType=\"MONSTER_ALIVE\" AcceptConditionValue3=\"{2}\" />",placeId,cid,name);
                m.AppendFormat(CultureInfo.InvariantCulture,"<Row CID=\"{0}\" UID=\"{1}\" UIDName=\"{2}\" PosX=\"{3}\" PosY=\"{4}\" PosZ=\"5\" RespawnCycleID=\"105\" ServerDeathCheck=\"True\" SpawnConditionID=\"0\" SectionUID=\"2022130000100\" />",cid,uid,name,i,i+1);}
            File.WriteAllText(place,p.Append("</Rows>").ToString());File.WriteAllText(kind,k.Append("</Rows>").ToString());File.WriteAllText(monster,m.Append("</Rows>").ToString());
            File.WriteAllText(reveal,"<Rows><Row RevealCycleID=\"10001\" RevealIngametime=\"23\" HideIngametime=\"6\"/><Row RevealCycleID=\"10002\" RevealIngametime=\"12\" HideIngametime=\"18\"/></Rows>");
        }
        private static void ExpectFailure(Action action,string name){bool failed=false;try{action();}catch(InvalidDataException){failed=true;}Assert(failed,name);}
        private static void Assert(bool value,string name){_assertions++;if(!value)throw new InvalidOperationException("Assertion failed: "+name);}
    }
}
