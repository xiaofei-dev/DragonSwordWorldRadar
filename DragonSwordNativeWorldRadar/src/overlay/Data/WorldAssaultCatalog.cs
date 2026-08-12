using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;
using System.Diagnostics;

namespace DragonSwordWorldRadar
{
    internal sealed class WorldAssaultCatalog
    {
        private readonly string _path = Path.Combine(ModPath.BaseDirectory, "data", "generated", "assaults.lua");
        private bool _loaded;
        private List<WorldAssault> _points = new List<WorldAssault>();
        public IList<WorldAssault> Points { get { return _points; } }

        public bool Refresh()
        {
            if (_loaded) return false;
            if (!File.Exists(_path))
            {
                throw new FileNotFoundException(
                    "The install-generated Assault catalog is missing.",
                    _path);
            }
            Stopwatch watch = Stopwatch.StartNew();
            _points = LoadValidated(_path);
            _loaded = true;
            watch.Stop();
            int conditioned = 0;
            foreach (WorldAssault point in _points)
            {
                if (point.HasTimeCondition) conditioned++;
            }
            ErrorLog.WriteDebug(String.Format(
                CultureInfo.InvariantCulture,
                "ASSAULT_CATALOG_LOADED records={0}; conditioned={1}; fingerprint={2}; loadMs={3:F3}; source=install_generated_current_pak; validation=exact_shape",
                _points.Count,
                conditioned,
                _points[0].GameFingerprint,
                watch.Elapsed.TotalMilliseconds));
            return true;
        }

        internal static List<WorldAssault> LoadValidated(string path)
        {
            List<WorldAssault> list = new List<WorldAssault>();
            HashSet<int> places = new HashSet<int>(), kinds = new HashSet<int>(), cids = new HashSet<int>();
            HashSet<string> uids = new HashSet<string>(), names = new HashSet<string>();
            string catalogFingerprint = null;
            foreach (string line in File.ReadLines(path))
            {
                Dictionary<string,string> fields = WorldTreasureCatalog.ParseFields(line);
                string cidText, mapText, xText, yText, zText, placeText, kindText, uid, uidName,
                    sectionUid, respawn, deathCheck, spawnCondition, fingerprint;
                if (!fields.TryGetValue("cid",out cidText)) continue;
                if (!fields.TryGetValue("map_id",out mapText) || !fields.TryGetValue("x",out xText)
                    || !fields.TryGetValue("y",out yText) || !fields.TryGetValue("z",out zText)
                    || !fields.TryGetValue("place_id",out placeText) || !fields.TryGetValue("kind_id",out kindText)
                    || !fields.TryGetValue("uid",out uid) || !fields.TryGetValue("uid_name",out uidName)
                    || !fields.TryGetValue("section_uid",out sectionUid) || !fields.TryGetValue("respawn_cycle_id",out respawn)
                    || !fields.TryGetValue("server_death_check",out deathCheck) || !fields.TryGetValue("spawn_condition_id",out spawnCondition)
                    || !fields.TryGetValue("game_fingerprint",out fingerprint))
                    throw new InvalidDataException("Assault catalog record is missing required common fields.");
                int cid,map,place,kind; double x,y,z; ulong parsedUid,parsedSection;
                if (!Int32.TryParse(cidText,NumberStyles.None,CultureInfo.InvariantCulture,out cid)
                    || !Int32.TryParse(mapText,NumberStyles.None,CultureInfo.InvariantCulture,out map)
                    || !Int32.TryParse(placeText,NumberStyles.None,CultureInfo.InvariantCulture,out place)
                    || !Int32.TryParse(kindText,NumberStyles.None,CultureInfo.InvariantCulture,out kind)
                    || !Double.TryParse(xText,NumberStyles.Float,CultureInfo.InvariantCulture,out x)
                    || !Double.TryParse(yText,NumberStyles.Float,CultureInfo.InvariantCulture,out y)
                    || !Double.TryParse(zText,NumberStyles.Float,CultureInfo.InvariantCulture,out z)
                    || !UInt64.TryParse(uid,NumberStyles.None,CultureInfo.InvariantCulture,out parsedUid)
                    || !UInt64.TryParse(sectionUid,NumberStyles.None,CultureInfo.InvariantCulture,out parsedSection)
                    || map != 100 || respawn != "105" || deathCheck != "true" || spawnCondition != "0"
                    || String.IsNullOrWhiteSpace(uidName) || !Regex.IsMatch(fingerprint,"^[0-9a-f]{64}$"))
                    throw new InvalidDataException("Assault catalog contains invalid common fields.");
                if (!places.Add(place)||!kinds.Add(kind)||!cids.Add(cid)||!uids.Add(uid)||!names.Add(uidName))
                    throw new InvalidDataException("Assault catalog contains duplicate identity fields.");
                if (catalogFingerprint == null) catalogFingerprint = fingerprint;
                else if (catalogFingerprint != fingerprint) throw new InvalidDataException("Assault catalog mixes game fingerprints.");

                string cycleText,type,fromText,hideText,provenance,missing;
                bool hasCycle=fields.TryGetValue("reveal_cycle_id",out cycleText), hasType=fields.TryGetValue("condition_type",out type),
                    hasFrom=fields.TryGetValue("visible_from_hour",out fromText), hasHide=fields.TryGetValue("hidden_from_hour",out hideText),
                    hasProvenance=fields.TryGetValue("condition_provenance",out provenance), hasMissing=fields.TryGetValue("missing_confirmation",out missing);
                int tupleCount=(hasCycle?1:0)+(hasType?1:0)+(hasFrom?1:0)+(hasHide?1:0)+(hasProvenance?1:0)+(hasMissing?1:0);
                bool conditioned=tupleCount==6; int cycle=0,from=0,hide=0;
                if (tupleCount!=0 && !conditioned) throw new InvalidDataException("Assault catalog contains a partial condition tuple.");
                if (conditioned && (!Int32.TryParse(cycleText,out cycle)||cycle<=0||type!="world_time_window"
                    || !Int32.TryParse(fromText,out from)||!Int32.TryParse(hideText,out hide)||from<0||from>23||hide<0||hide>23
                    || String.IsNullOrWhiteSpace(provenance)||String.IsNullOrWhiteSpace(missing)))
                    throw new InvalidDataException("Assault catalog contains a malformed condition tuple.");
                list.Add(new WorldAssault { PlaceId=place,KindId=kind,Cid=cid,MapId=map,Uid=uid,UidName=uidName,
                    GameFingerprint=fingerprint,X=x,Y=y,Z=z,HasTimeCondition=conditioned,RevealCycleId=cycle,
                    VisibleFromHour=from,HiddenFromHour=hide,ConditionType=type,ConditionProvenance=provenance,MissingConfirmation=missing });
            }
            if (list.Count!=40) throw new InvalidDataException("Assault catalog must contain exactly 40 records.");
            return list;
        }
    }
}
