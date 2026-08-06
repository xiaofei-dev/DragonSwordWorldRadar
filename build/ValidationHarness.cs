using System;
using System.Collections.Generic;
using System.Drawing;
using System.Globalization;
using System.Text;

namespace DragonSwordWorldRadar
{
    internal static class ValidationHarness
    {
        private static int _assertions;

        public static int Main(string[] args)
        {
            try
            {
                TestMotionParser();
                TestMotionParserRejectsInvalidRecords();
                TestMotionVersionOrdering();
                TestWorldTreasureRenderBuffer();
                TestTreasureIdentityAndCatalogParser();
                TestBossRuleDefault();
                TestSaveSnapshotShape();
                Console.WriteLine(
                    "REFACTOR_TESTS_OK assertions=" +
                    _assertions.ToString(CultureInfo.InvariantCulture));
                return 0;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("REFACTOR_TESTS_FAILED");
                Console.Error.WriteLine(exception.ToString());
                return 1;
            }
        }

        private static void TestMotionParser()
        {
            string record =
                "41|7|1|world|100.5|-200.25|900|1|4500|101|" +
                "8192|1024|-12.5|33.25|1.75|2560|1440|1|" +
                "500.5|600.25|41\r\n";
            byte[] bytes = Encoding.ASCII.GetBytes(record);
            MotionFrame frame = new MotionFrame();
            WorldMapState map = new WorldMapState();
            Assert(MotionRecordParser.TryParse(bytes, bytes.Length, frame, map),
                "valid world motion record");
            Assert(frame.Sequence == 41, "sequence");
            Assert(frame.Generation == 7, "generation");
            Assert(frame.Enabled, "enabled");
            Assert(frame.Mode == "world", "mode");
            Assert(Almost(frame.PlayerX, 100.5), "player x");
            Assert(Almost(frame.PlayerY, -200.25), "player y");
            Assert(Almost(frame.PlayerZ, 900.0), "player z");
            Assert(frame.HasPlayerZ, "has player z");
            Assert(Almost(frame.Radius, 4500.0), "radius");
            Assert(Object.ReferenceEquals(frame.WorldMap, map),
                "retained world-map instance");
            Assert(map.mapId == 101, "map id");
            Assert(Almost(map.zoom, 1.75), "zoom");

            string radarRecord =
                "42|7|1|radar|1|2|3|1|4000|0|0|0|0|0|0|0|0|0|0|0|42";
            bytes = Encoding.ASCII.GetBytes(radarRecord);
            Assert(MotionRecordParser.TryParse(bytes, bytes.Length, frame, map),
                "valid radar motion record");
            Assert(frame.WorldMap == null, "radar clears published map");
            Assert(frame.Sequence == 42, "reused frame updated");
        }

        private static void TestMotionParserRejectsInvalidRecords()
        {
            string valid =
                "5|1|1|radar|1|2|3|1|4000|0|0|0|0|0|0|0|0|0|0|0|5";
            string[] invalid =
            {
                valid + "|extra",
                valid + "|",
                valid.Substring(0, valid.Length - 1) + "6",
                valid.Replace("|1|2|", "|NaN|2|"),
                valid.Replace("radar", "other"),
                "",
                "5|1|1|radar"
            };
            foreach (string record in invalid)
            {
                byte[] bytes = Encoding.ASCII.GetBytes(record);
                Assert(!MotionRecordParser.TryParse(
                    bytes, bytes.Length, new MotionFrame(), new WorldMapState()),
                    "invalid motion record rejected");
            }
            Assert(!MotionRecordParser.TryParse(
                null, 0, new MotionFrame(), new WorldMapState()),
                "null motion buffer rejected");
        }

        private static void TestMotionVersionOrdering()
        {
            Assert(MotionBridgeReader.CompareVersion(2, 1, 1, 999) > 0,
                "generation wins over sequence");
            Assert(MotionBridgeReader.CompareVersion(2, 10, 2, 9) > 0,
                "sequence ordering");
            Assert(MotionBridgeReader.CompareVersion(2, 10, 2, 10) == 0,
                "equal version");
        }

        private static void TestWorldTreasureRenderBuffer()
        {
            using (WorldTreasureRenderBuffer buffer = new WorldTreasureRenderBuffer())
            {
                WorldTreasure treasure = new WorldTreasure
                {
                    SaveId = 100,
                    MapId = 101,
                    X = 12,
                    Y = 34,
                    UidName = "DT_Map_G1_Test"
                };
                int index = buffer.Add(treasure, 10f, 20f);
                Assert(index == 0 && buffer.Count == 1,
                    "projected marker retained");
                Assert(buffer.ReservePixel(123), "first pixel reservation");
                Assert(!buffer.ReservePixel(123), "pixel dedupe");
                buffer.AddMarker(TreasureKind.Map, new RectangleF(1f, 2f, 3f, 4f));
                Assert(buffer.MapPath.PointCount > 0, "map path populated");
                buffer.Reset();
                Assert(buffer.Count == 0, "buffer reset count");
                Assert(buffer.MapPath.PointCount == 0, "buffer reset path");
                Assert(buffer.ReservePixel(123), "pixel set cleared on reset");
            }
        }

        private static void TestTreasureIdentityAndCatalogParser()
        {
            Assert(TreasureIdentity.GetKind(null) == TreasureKind.Other,
                "null UID kind");
            Assert(TreasureIdentity.GetCategoryCode(String.Empty) == "TB",
                "empty category code");
            Assert(TreasureIdentity.GetCategoryCode("MiniGame") == "MG",
                "known category code");
            Dictionary<string, string> fields = WorldTreasureCatalog.ParseFields(
                "{ section=\"Map_101\", save_id=42, x=1.5, y=-2, z=3 }");
            Assert(fields["section"] == "Map_101", "catalog section");
            Assert(fields["save_id"] == "42", "catalog save id");
            Assert(fields["y"] == "-2", "catalog coordinate");
        }

        private static void TestBossRuleDefault()
        {
            BossRespawnRuleResolver resolver = new BossRespawnRuleResolver();
            DateTime destroyedLocal = new DateTime(
                2026, 8, 5, 8, 0, 0, DateTimeKind.Local);
            DateTime expectedLocal = new DateTime(
                2026, 8, 5, 9, 0, 0, DateTimeKind.Local);
            Assert(resolver.NextAvailableUtc(destroyedLocal.ToUniversalTime()) ==
                expectedLocal.ToUniversalTime(),
                "default daily 09:00 boss respawn rule");
        }

        private static void TestSaveSnapshotShape()
        {
            SaveSnapshot snapshot = new SaveSnapshot
            {
                Opened = new Dictionary<int, ulong>(),
                BossRespawns = new Dictionary<int, BossRespawnRecord>()
            };
            snapshot.Opened[1] = 2;
            snapshot.BossRespawns[3] = new BossRespawnRecord
            {
                BossId = 3,
                RespawnType = 106,
                DestroyTimeUtc = DateTime.UtcNow
            };
            Assert(snapshot.Opened.Count == 1, "opened snapshot shape");
            Assert(snapshot.BossRespawns.Count == 1, "boss snapshot shape");
        }

        private static bool Almost(double left, double right)
        {
            return Math.Abs(left - right) < 0.0000001;
        }

        private static void Assert(bool condition, string name)
        {
            _assertions++;
            if (!condition)
            {
                throw new InvalidOperationException("Assertion failed: " + name);
            }
        }
    }
}
