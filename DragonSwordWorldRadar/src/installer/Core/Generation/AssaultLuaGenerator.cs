using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace DragonSwordWorldRadar.Installer
{
    public static class AssaultLuaGenerator
    {
        private sealed class Target
        {
            public string PlaceId, KindId, Cid, Uid, UidName, X, Y, Z, SectionUid;
            public Condition Availability;
        }
        private sealed class Condition
        {
            public string RevealCycleId, Type, Provenance, MissingConfirmation;
            public int VisibleFromHour, HiddenFromHour;
        }

        public static int Generate(string placePath, string kindPath, string monsterPath,
            string revealPath, string policyPath, string gameFingerprint, string outputPath)
        {
            ValidateFingerprint(gameFingerprint, "installation game fingerprint");
            XmlDocument placeDocument = Load(placePath);
            Dictionary<string, XmlElement> kinds = Index(Load(kindPath), "GroupID");
            HashSet<string> targetCids = ReadTargetCids(placeDocument, kinds);
            Dictionary<string, XmlElement> monsters = IndexSelected(Load(monsterPath), "CID", targetCids);
            Dictionary<string, XmlElement> cycles = IndexEither(Load(revealPath), "RevealCycleID", "ID");
            List<Target> targets = ReadTargets(placeDocument, kinds, monsters);
            ApplyPolicy(targets, cycles, Load(policyPath), gameFingerprint);

            List<string> lines = new List<string> {
                "-- Generated locally from one fingerprint-locked game PAK snapshot.",
                "-- game_fingerprint = \"" + gameFingerprint + "\"",
                "-- Availability policy preserves confirmed cycle rows and inferred target bindings.",
                "return {" };
            foreach (Target target in targets)
            {
                StringBuilder line = new StringBuilder("    { place_id = ");
                line.Append(target.PlaceId).Append(", kind_id = ").Append(target.KindId)
                    .Append(", cid = ").Append(target.Cid).Append(", map_id = 100, uid = \"")
                    .Append(target.Uid).Append("\", uid_name = \"").Append(Escape(target.UidName))
                    .Append("\", x = ").Append(target.X).Append(", y = ").Append(target.Y)
                    .Append(", z = ").Append(target.Z).Append(", section_uid = \"")
                    .Append(Escape(target.SectionUid)).Append("\", respawn_cycle_id = 105")
                    .Append(", server_death_check = true, spawn_condition_id = 0")
                    .Append(", game_fingerprint = \"").Append(gameFingerprint).Append("\"");
                if (target.Availability != null)
                {
                    Condition condition = target.Availability;
                    line.Append(", reveal_cycle_id = ").Append(condition.RevealCycleId)
                        .Append(", condition_type = \"").Append(Escape(condition.Type))
                        .Append("\", visible_from_hour = ").Append(condition.VisibleFromHour)
                        .Append(", hidden_from_hour = ").Append(condition.HiddenFromHour)
                        .Append(", condition_provenance = \"").Append(Escape(condition.Provenance))
                        .Append("\", missing_confirmation = \"").Append(Escape(condition.MissingConfirmation)).Append("\"");
                }
                line.Append(" },"); lines.Add(line.ToString());
            }
            lines.Add("}");
            Directory.CreateDirectory(Path.GetDirectoryName(outputPath));
            File.WriteAllLines(outputPath, lines, new UTF8Encoding(false));
            return targets.Count;
        }

        private static List<Target> ReadTargets(XmlDocument placeDocument,
            Dictionary<string, XmlElement> kinds, Dictionary<string, XmlElement> monsters)
        {
            List<Target> result = new List<Target>();
            HashSet<string> places = new HashSet<string>(), kindIds = new HashSet<string>(),
                cids = new HashSet<string>(), uids = new HashSet<string>(), names = new HashSet<string>();
            foreach (XmlElement place in Elements(placeDocument))
            {
                if (Attribute(place, "MapID") != "100" || String.IsNullOrEmpty(Attribute(place,"ID")) || String.IsNullOrEmpty(Attribute(place,"MissionKindData"))) continue;
                string kindId = Required(place, "MissionKindData"); XmlElement kind;
                if (!kinds.TryGetValue(kindId, out kind) || Attribute(kind, "MissionType") != "DEFEAT_MONSTER" || Attribute(kind, "AcceptConditionType") != "MONSTER_ALIVE") continue;
                string cid = Required(kind, "MissionValue1"); XmlElement monster;
                if (!monsters.TryGetValue(cid, out monster)) throw new InvalidDataException("Missing Assault monster CID " + cid + ".");
                string placeId = Required(place, "ID"), uid = Required(monster, "UID"), uidName = Required(monster, "UIDName");
                ulong parsedUid; if (!UInt64.TryParse(uid, NumberStyles.None, CultureInfo.InvariantCulture, out parsedUid)) throw new InvalidDataException("Invalid Assault UID.");
                if (!places.Add(placeId) || !kindIds.Add(kindId) || !cids.Add(cid) || !uids.Add(uid) || !names.Add(uidName)) throw new InvalidDataException("Duplicate Assault identity.");
                if (Required(monster, "RespawnCycleID") != "105" || !String.Equals(Required(monster, "ServerDeathCheck"), "True", StringComparison.OrdinalIgnoreCase) || Required(monster, "SpawnConditionID") != "0") throw new InvalidDataException("Invalid Assault common fields.");
                string acceptedName = Required(kind, "AcceptConditionValue3");
                if (!String.Equals(acceptedName, uidName, StringComparison.Ordinal)) throw new InvalidDataException("Assault Kind/monster identity join mismatch.");
                result.Add(new Target { PlaceId=placeId, KindId=kindId, Cid=cid, Uid=uid, UidName=uidName,
                    X=Required(monster,"PosX"), Y=Required(monster,"PosY"), Z=Required(monster,"PosZ"), SectionUid=Required(monster,"SectionUID") });
            }
            if (result.Count != 40) throw new InvalidDataException("Assault shape gate expected exactly 40 records; found " + result.Count.ToString(CultureInfo.InvariantCulture) + ".");
            return result;
        }

        private static HashSet<string> ReadTargetCids(XmlDocument placeDocument,
            Dictionary<string, XmlElement> kinds)
        {
            HashSet<string> result = new HashSet<string>();
            foreach (XmlElement place in Elements(placeDocument))
            {
                if (Attribute(place,"MapID")!="100" || String.IsNullOrEmpty(Attribute(place,"ID")) || String.IsNullOrEmpty(Attribute(place,"MissionKindData"))) continue;
                XmlElement kind; string kindId=Required(place,"MissionKindData");
                if (!kinds.TryGetValue(kindId,out kind) || Attribute(kind,"MissionType")!="DEFEAT_MONSTER" || Attribute(kind,"AcceptConditionType")!="MONSTER_ALIVE") continue;
                result.Add(Required(kind,"MissionValue1"));
            }
            return result;
        }

        private static void ApplyPolicy(List<Target> targets, Dictionary<string, XmlElement> cycles,
            XmlDocument policy, string gameFingerprint)
        {
            XmlElement root = policy.DocumentElement;
            if (root == null || root.Name != "AssaultInferencePolicy" || Required(root,"schema_version") != "1") throw new InvalidDataException("Unknown Assault inference-policy schema.");
            string expected = Required(root, "expected_game_fingerprint"); ValidateFingerprint(expected, "policy fingerprint");
            if (!String.Equals(expected, gameFingerprint, StringComparison.Ordinal)) throw new InvalidDataException("Assault inference-policy fingerprint does not match the installation game fingerprint.");
            HashSet<string> selected = new HashSet<string>();
            foreach (XmlElement policyCondition in ElementsNamed(root, "Condition"))
            {
                string placeId=Required(policyCondition,"place_id"), cid=Required(policyCondition,"cid"), uid=Required(policyCondition,"uid"), uidName=Required(policyCondition,"uid_name");
                Target match = null; foreach(Target candidate in targets) if(candidate.PlaceId==placeId && candidate.Cid==cid && candidate.Uid==uid && candidate.UidName==uidName){match=candidate;break;}
                if (match == null || !selected.Add(placeId)) throw new InvalidDataException("Assault inference-policy target identity does not match exactly one generated target: place="+placeId+" cid="+cid+" uid="+uid+" uid_name="+uidName+".");
                string cycleId=Required(policyCondition,"reveal_cycle_id"); XmlElement cycle;
                if (!cycles.TryGetValue(cycleId,out cycle)) throw new InvalidDataException("Assault inference-policy references an unknown RevealCycle.");
                string type=Required(policyCondition,"condition_type");
                if(type!="world_time_window") throw new InvalidDataException("Unknown Assault condition type.");
                string provenance=Required(policyCondition,"provenance"), missing=Required(policyCondition,"missing_confirmation");
                int reveal=ParseHour(Required(cycle,"RevealIngametime")), hide=ParseHour(Required(cycle,"HideIngametime"));
                match.Availability=new Condition{RevealCycleId=cycleId,Type=type,Provenance=provenance,MissingConfirmation=missing,VisibleFromHour=reveal,HiddenFromHour=hide};
            }
        }

        private static XmlDocument Load(string path) { XmlDocument d=new XmlDocument{XmlResolver=null}; d.Load(path); return d; }
        private static IEnumerable<XmlElement> Elements(XmlDocument d) { foreach(XmlNode n in d.SelectNodes("//*")){XmlElement e=n as XmlElement;if(e!=null)yield return e;} }
        private static IEnumerable<XmlElement> ElementsNamed(XmlElement root,string name){foreach(XmlNode n in root.ChildNodes){XmlElement e=n as XmlElement;if(e!=null&&e.Name==name)yield return e;}}
        private static Dictionary<string,XmlElement> Index(XmlDocument d,string key){return IndexEither(d,key,null);}
        private static Dictionary<string,XmlElement> IndexSelected(XmlDocument d,string key,HashSet<string> selected){Dictionary<string,XmlElement> r=new Dictionary<string,XmlElement>();foreach(XmlElement e in Elements(d)){string v=Attribute(e,key);if(String.IsNullOrEmpty(v)||!selected.Contains(v))continue;if(r.ContainsKey(v))throw new InvalidDataException("Duplicate selected "+key+" "+v+".");r[v]=e;}return r;}
        private static Dictionary<string,XmlElement> IndexEither(XmlDocument d,string first,string second){Dictionary<string,XmlElement> r=new Dictionary<string,XmlElement>();foreach(XmlElement e in Elements(d)){string v=Attribute(e,first)??Attribute(e,second);if(String.IsNullOrEmpty(v))continue;if(r.ContainsKey(v))throw new InvalidDataException("Duplicate "+first+" "+v+".");r[v]=e;}return r;}
        private static string Attribute(XmlElement e,string name){if(e==null||name==null)return null;foreach(XmlAttribute a in e.Attributes)if(String.Equals(a.LocalName,name,StringComparison.OrdinalIgnoreCase))return a.Value.Trim();return null;}
        private static string Required(XmlElement e,string name){string v=Attribute(e,name);if(String.IsNullOrWhiteSpace(v))throw new InvalidDataException("Missing Assault field '"+name+"'.");return v;}
        private static int ParseHour(string value){int hour;if(!Int32.TryParse(value,NumberStyles.Integer,CultureInfo.InvariantCulture,out hour)||hour<0||hour>23)throw new InvalidDataException("Invalid RevealCycle hour.");return hour;}
        private static void ValidateFingerprint(string value,string label){if(value==null||!Regex.IsMatch(value,"^[0-9a-f]{64}$",RegexOptions.CultureInvariant))throw new InvalidDataException("Invalid "+label+".");}
        private static string Escape(string value){return value.Replace("\\","\\\\").Replace("\"","\\\"").Replace("\r","\\r").Replace("\n","\\n");}
    }
}
