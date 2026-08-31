using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;

namespace DragonSwordWorldRadar
{
    // Bosses and Assault targets are one immutable install-bound encounter
    // catalog. The two source parsers retain their exact dataset validation,
    // but no feature may load or reconfigure either dataset from the F7 path.
    internal sealed class WorldEncounterCatalog
    {
        private const int ExpectedBossCount = 9;
        private const int ExpectedAssaultCount = 40;
        private readonly List<WorldEncounter> _points =
            new List<WorldEncounter>();
        private bool _hasLoaded;
        private int _bossCount;
        private int _assaultCount;

        public IList<WorldEncounter> Points
        {
            get { return _points; }
        }

        public int BossCount
        {
            get { return _bossCount; }
        }

        public int AssaultCount
        {
            get { return _assaultCount; }
        }

        public void Load()
        {
            if (_hasLoaded)
            {
                return;
            }

            Stopwatch watch = Stopwatch.StartNew();
            WorldBossCatalog bossCatalog = new WorldBossCatalog();
            bossCatalog.Refresh();
            WorldAssaultCatalog assaultCatalog =
                new WorldAssaultCatalog();
            assaultCatalog.Refresh();

            if (bossCatalog.Points.Count != ExpectedBossCount
                || assaultCatalog.Points.Count != ExpectedAssaultCount)
            {
                throw new InvalidDataException(
                    "The unified encounter catalog has an invalid dataset shape.");
            }

            HashSet<int> ids = new HashSet<int>();
            List<WorldEncounter> loaded = new List<WorldEncounter>(
                ExpectedBossCount + ExpectedAssaultCount);
            foreach (BossPoint boss in bossCatalog.Points)
            {
                if (boss == null || !ids.Add(boss.bossId))
                {
                    throw new InvalidDataException(
                        "The unified encounter catalog contains an invalid or duplicate Boss ID.");
                }
                loaded.Add(new WorldEncounter
                {
                    Id = boss.bossId,
                    MapId = boss.mapId,
                    X = boss.x,
                    Y = boss.y,
                    Z = boss.z,
                    HasZ = boss.hasZ,
                    Kind = WorldEncounterKind.Boss,
                    BossMetadata = boss
                });
            }

            int conditioned = 0;
            foreach (WorldAssault assault in assaultCatalog.Points)
            {
                if (assault == null || !ids.Add(assault.Cid))
                {
                    throw new InvalidDataException(
                        "The unified encounter catalog contains an invalid or duplicate Assault CID.");
                }
                WorldEncounter encounter = new WorldEncounter
                {
                    Id = assault.Cid,
                    MapId = assault.MapId,
                    X = assault.X,
                    Y = assault.Y,
                    Z = assault.Z,
                    HasZ = true,
                    Kind = WorldEncounterKind.Assault
                };
                if (assault.HasTimeCondition)
                {
                    encounter.Conditions.Add(
                        new WorldEncounterCondition
                        {
                            Type = "world_time_window",
                            VisibleFromHour = assault.VisibleFromHour,
                            HiddenFromHour = assault.HiddenFromHour,
                            Provenance = assault.ConditionProvenance,
                            MissingConfirmation =
                                assault.MissingConfirmation
                        });
                    conditioned++;
                }
                loaded.Add(encounter);
            }

            _points.Clear();
            _points.AddRange(loaded);
            _bossCount = ExpectedBossCount;
            _assaultCount = ExpectedAssaultCount;
            _hasLoaded = true;
            watch.Stop();
            ErrorLog.WriteDebug(String.Format(
                CultureInfo.InvariantCulture,
                "WORLD_ENCOUNTER_CATALOG_LOADED records={0}; bosses={1}; assaults={2}; conditioned={3}; loadMs={4:F3}; lifecycle=overlay_startup_once; f7_reconfiguration=false",
                _points.Count,
                _bossCount,
                _assaultCount,
                conditioned,
                watch.Elapsed.TotalMilliseconds));
        }
    }
}
