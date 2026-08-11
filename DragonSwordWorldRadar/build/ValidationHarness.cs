using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Reflection;
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
                TestMotionParserProtocolV5();
                TestMotionParserRejectsInvalidRecords();
                TestMotionVersionOrdering();
                TestScalarMotionPrediction();
                TestWorldStatusClockFormatting();
                TestWorldStatusLayout();
                TestTreasureVisibilityStartupFailClosed();
                TestDev15RadarTreasureQueryBuffer();
                TestMarkerVectorRenderers();
                TestWorldTreasureRenderBuffer();
                TestTreasureIdentityAndCatalogParser();
                TestMoleCatalogParserContract();
                TestMoleRewardTreasureVisibility();
                TestBossRuleDefault();
                TestSaveSnapshotShape();
                TestGeneratedOwnerPointerConfig();
                TestOwnerPointerPeResolver();
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

        private static void TestMotionParserProtocolV5()
        {
            string worldRecord =
                "41|5|7|12|1250.5|1|world|1|1|1|1|1|17179869183|1|1|82800|1|2|3|1.25|100.5|-200.25|900|1|4500|101|" +
                "8192|1024|-12.5|33.25|1.75|2560|1440|1|500.5|600.25|41\r\n";
            byte[] bytes = Encoding.ASCII.GetBytes(worldRecord);
            MotionFrame frame = new MotionFrame();
            WorldMapState map = new WorldMapState();
            Assert(MotionRecordParser.TryParse(bytes, bytes.Length, frame, map),
                "valid protocol-v5 world record");
            Assert(frame.Sequence == 41, "sequence");
            Assert(frame.ProtocolVersion == 5, "protocol version");
            Assert(frame.WorldEpoch == 12, "world epoch");
            Assert(Almost(frame.SampleTimestampMs, 1250.5), "sample timestamp");
            Assert(frame.ShowMoles, "show moles");
            Assert(frame.MoleMask == 17179869183L, "mole mask");
            Assert(frame.ShowWorldStatus, "show world status");
            Assert(frame.WorldTimeAvailable, "world time available");
            Assert(frame.WorldTimeSeconds == 82800, "world time seconds");
            Assert(frame.WeatherAvailable, "weather available");
            Assert(frame.WeatherState == 2, "weather state");
            Assert(frame.WeatherBtState == 3, "weather BT state");
            Assert(frame.Generation == 7, "generation");
            Assert(frame.Enabled, "enabled");
            Assert(frame.Mode == "world", "world mode");
            Assert(frame.ShowHeight, "show height");
            Assert(frame.ShowTreasureTypes, "show treasure types");
            Assert(frame.ShowTreasures, "show treasures");
            Assert(frame.ShowBosses, "show bosses");
            Assert(Almost(frame.TextScale, 1.25), "text scale");
            Assert(Almost(frame.PlayerX, 100.5), "player x");
            Assert(Almost(frame.PlayerY, -200.25), "player y");
            Assert(Almost(frame.PlayerZ, 900.0), "player z");
            Assert(frame.HasPlayerZ, "has player z");
            Assert(Almost(frame.Radius, 4500.0), "radius");
            Assert(Object.ReferenceEquals(frame.WorldMap, map),
                "retained world-map instance");
            Assert(map.mapId == 101, "map id");
            Assert(Almost(map.zoom, 1.75), "zoom");
            Assert(Almost(map.playerMapX, 500.5), "player map x");

            string radarRecord =
                "42|5|7|12|1500.5|1|radar|1|0|1|1|1|7|1|1|3661|1|2|3|1|1|2|3|1|4000|0|0|0|0|0|0|0|0|0|0|0|42";
            bytes = Encoding.ASCII.GetBytes(radarRecord);
            Assert(MotionRecordParser.TryParse(bytes, bytes.Length, frame, map),
                "valid protocol-v5 radar record");
            Assert(frame.WorldMap == null, "radar clears published map");
            Assert(frame.Sequence == 42, "reused frame updated");
            Assert(!frame.ShowTreasureTypes, "radar display flag updated");

            string disabledRecord =
                "43|5|7|13|0|0|disabled|1|1|1|1|1|0|1|0|0|0|0|0|1|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|43";
            bytes = Encoding.ASCII.GetBytes(disabledRecord);
            Assert(MotionRecordParser.TryParse(bytes, bytes.Length, frame, map),
                "valid protocol-v5 disabled record");
            Assert(!frame.Enabled, "disabled state");
            Assert(frame.Mode == "disabled", "disabled mode");
        }

        private static void TestMotionParserRejectsInvalidRecords()
        {
            string validRadar =
                "5|5|1|2|1000|1|radar|1|1|1|1|1|3|1|1|3661|1|2|3|1|1|2|3|1|4000|0|0|0|0|0|0|0|0|0|0|0|5";
            string[] invalid =
            {
                // Legacy protocol-v1 / 21-field record.
                "5|1|1|radar|1|2|3|1|4000|0|0|0|0|0|0|0|0|0|0|0|5",
                // Superseded protocol-v3 / 29-field record.
                "5|3|1|1|radar|1|1|1|1|1|3|1|1|2|3|1|4000|0|0|0|0|0|0|0|0|0|0|0|5",
                validRadar + "|extra",
                validRadar + "|",
                ReplaceField(validRadar, 36, "6"),
                ReplaceField(validRadar, 5, "2"),
                ReplaceField(validRadar, 6, "disabled"),
                ReplaceField(validRadar, 5, "0"),
                ReplaceField(validRadar, 7, "2"),
                ReplaceField(validRadar, 11, "2"),
                ReplaceField(validRadar, 12, "-1"),
                ReplaceField(validRadar, 12, "17179869184"),
                ReplaceField(validRadar, 13, "2"),
                ReplaceField(validRadar, 14, "2"),
                ReplaceField(validRadar, 15, "-1"),
                ReplaceField(validRadar, 15, "86400"),
                ReplaceField(ReplaceField(validRadar, 14, "0"), 15, "1"),
                ReplaceField(validRadar, 16, "2"),
                ReplaceField(validRadar, 17, "1000001"),
                ReplaceField(validRadar, 18, "-1000001"),
                ReplaceField(validRadar, 16, "0"),
                ReplaceField(validRadar, 19, "0.4"),
                ReplaceField(validRadar, 20, "NaN"),
                ReplaceField(validRadar, 3, "-1"),
                ReplaceField(validRadar, 4, "NaN"),
                "",
                "5|5|1|2|1000|1|radar"
            };
            foreach (string record in invalid)
            {
                byte[] bytes = Encoding.ASCII.GetBytes(record);
                Assert(!MotionRecordParser.TryParse(
                    bytes, bytes.Length, new MotionFrame(), new WorldMapState()),
                    "invalid protocol-v5 record rejected");
            }
            Assert(!MotionRecordParser.TryParse(
                null, 0, new MotionFrame(), new WorldMapState()),
                "null motion buffer rejected");
        }

        private static void TestWorldStatusClockFormatting()
        {
            Assert(WorldStatusRenderer.FormatClock(0) == "00:00",
                "clock midnight");
            Assert(WorldStatusRenderer.FormatClock(3661) == "01:01",
                "clock hour and minute");
            Assert(WorldStatusRenderer.FormatClock(86399) == "23:59",
                "clock end of day");
            Assert(WorldStatusRenderer.FormatClock(-1) == "--:--",
                "clock rejects negative");
            Assert(WorldStatusRenderer.FormatClock(86400) == "--:--",
                "clock rejects next day");
            Assert(WorldStatusRenderer.FormatPhase(6 * 3600) == "MORNING",
                "morning phase boundary");
            Assert(WorldStatusRenderer.FormatPhase(12 * 3600) == "AFTERNOON",
                "afternoon phase boundary");
            Assert(WorldStatusRenderer.FormatPhase(18 * 3600) == "EVENING",
                "evening phase boundary");
            Assert(WorldStatusRenderer.FormatPhase(21 * 3600) == "NIGHT",
                "night phase boundary");
            Assert(WorldStatusRenderer.FormatPhase(5 * 3600 + 3599) == "NIGHT",
                "pre-morning remains night");
        }

        private static void TestScalarMotionPrediction()
        {
            RadarForm.ScalarMotionPredictor predictor =
                new RadarForm.ScalarMotionPredictor();
            MotionFrame first = NewScalarFrame(1, 1, 1000.0, 0.0);
            MotionFrame second = NewScalarFrame(1, 1, 1250.0, 250.0);
            Assert(predictor.Accept(first, 0L), "first scalar resets");
            Assert(!predictor.Accept(second, 0L), "second scalar forms history");
            double age;
            bool clamped;
            bool stale;
            long hundredMs = Stopwatch.Frequency / 10;
            Assert(predictor.Apply(second, hundredMs,
                out age, out clamped, out stale), "prediction changes position");
            Assert(Almost(second.PlayerX, 350.0), "100 ms scalar extrapolation");
            Assert(!clamped && !stale, "100 ms prediction is bounded normally");

            second.PlayerX = 250.0;
            Assert(predictor.Apply(second, Stopwatch.Frequency * 4 / 10,
                out age, out clamped, out stale), "clamped prediction changes position");
            Assert(Almost(second.PlayerX, 500.0), "prediction clamps at 250 ms");
            Assert(clamped && !stale, "prediction clamp reported");

            second.PlayerX = 250.0;
            predictor.Apply(second, Stopwatch.Frequency * 6 / 10,
                out age, out clamped, out stale);
            Assert(Almost(second.PlayerX, 250.0), "stale prediction freezes raw scalar");
            Assert(stale, "stale freeze reported after 500 ms");

            MotionFrame nextEpoch = NewScalarFrame(1, 2, 1500.0, 900.0);
            Assert(predictor.Accept(nextEpoch, 0L), "epoch change resets history");
            MotionFrame teleport = NewScalarFrame(1, 2, 1750.0, 200000.0);
            Assert(predictor.Accept(teleport, 0L), "teleport outlier resets history");
        }

        private static MotionFrame NewScalarFrame(
            int generation,
            int epoch,
            double timestampMs,
            double playerX)
        {
            return new MotionFrame
            {
                Generation = generation,
                WorldEpoch = epoch,
                SampleTimestampMs = timestampMs,
                Enabled = true,
                Mode = "radar",
                PlayerX = playerX,
                PlayerY = 0.0,
                PlayerZ = 0.0,
                HasPlayerZ = true,
                Radius = 22500.0
            };
        }

        private static void TestWorldStatusLayout()
        {
            RectangleF normal = WorldStatusRenderer.CalculateGroupBounds(
                360,
                400,
                360,
                1f);
            Assert(normal.Left >= 0 && normal.Right <= 360,
                "status group clamped inside compact overlay");
            Assert(normal.Top >= 352
                && normal.Bottom <= 400,
                "status group remains inside compact raised strip");
            Assert(Math.Abs(normal.Top - 354f) < 0.01f,
                "status group uses six-reference-pixel upward offset");
            RectangleF highDpi = WorldStatusRenderer.CalculateGroupBounds(
                720,
                800,
                720,
                2f);
            Assert(highDpi.Right <= 720 && highDpi.Left >= 0
                && highDpi.Top >= 704 && highDpi.Bottom <= 800,
                "status group high-DPI bounds");
            Assert(Math.Abs(highDpi.Top - 708f) < 0.01f,
                "raised status offset scales at high DPI");
            Assert(WorldStatusRenderer.PhaseFontReferencePixels >= 13f,
                "phase font readable reference size");
        }

        private static void TestMarkerVectorRenderers()
        {
            using (Bitmap surface = new Bitmap(128, 128))
            using (Graphics graphics = Graphics.FromImage(surface))
            using (BossMarkerRenderer boss = new BossMarkerRenderer())
            using (MoleMarkerRenderer mole = new MoleMarkerRenderer())
            {
                graphics.Clear(Color.Transparent);
                graphics.SmoothingMode =
                    System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
                boss.DrawMarker(graphics, 32, 32, 54, 1f);
                boss.DrawMarker(graphics, 64, 32, 40, 1.25f);
                mole.DrawMarker(graphics, 32, 80, 28, 1f);
                mole.DrawMarker(graphics, 64, 80, 34, 1.25f);
                Assert(surface.GetPixel(32, 32).A > 0,
                    "boss vector renderer paints its center");
                Assert(surface.GetPixel(32, 80).A > 0,
                    "mole vector renderer paints its center");
            }
        }

        private static void TestTreasureVisibilityStartupFailClosed()
        {
            WorldTreasure openedTreasure = new WorldTreasure
            {
                SaveId = 65,
                MapId = 100,
                X = 1,
                Y = 1
            };
            WorldTreasure unopenedTreasure = new WorldTreasure
            {
                SaveId = 66,
                MapId = 100,
                X = 2,
                Y = 2
            };
            WorldTreasure secondMapTreasure = new WorldTreasure
            {
                SaveId = 129,
                MapId = 200,
                X = 3,
                Y = 3
            };
            WorldTreasureCatalog catalog = new WorldTreasureCatalog();
            SetPrivateField(catalog, "_points", new List<WorldTreasure>
            {
                openedTreasure,
                unopenedTreasure,
                secondMapTreasure
            });
            SetPrivateField(catalog, "_version", 1);

            TreasureSaveState save = new TreasureSaveState();
            SetPrivateField(save, "_version", 0);
            SetPrivateField(save, "_hasLoadedSaveState", false);
            WorldTreasureVisibilityIndex index =
                new WorldTreasureVisibilityIndex();

            Assert(index.Refresh(catalog, save),
                "unknown-save publication changed index");
            Assert(index.Count == 0
                && index.GetMap(100).Count == 0
                && index.GetMap(200).Count == 0,
                "unknown-save publishes no treasure records");

            SetPrivateField(save, "_opened",
                new Dictionary<int, ulong> { { 1, 1UL << 1 } });
            SetPrivateField(save, "_hasLoadedSaveState", true);
            SetPrivateField(save, "_version", 1);
            Assert(index.Refresh(catalog, save),
                "first loaded save rebuilds visibility index");
            Assert(index.Count == 2
                && index.GetMap(100).Count == 1
                && Object.ReferenceEquals(
                    index.GetMap(100)[0],
                    unopenedTreasure)
                && index.GetMap(200).Count == 1,
                "loaded save filters opened IDs without losing maps");
        }

        private static void TestDev15RadarTreasureQueryBuffer()
        {
            WorldTreasure map100 = new WorldTreasure
            {
                SaveId = 1001,
                MapId = 100,
                X = 10,
                Y = 0
            };
            WorldTreasure map200 = new WorldTreasure
            {
                SaveId = 2001,
                MapId = 200,
                X = 20,
                Y = 0
            };
            Type bufferType = typeof(RadarForm).GetNestedType(
                "RadarTreasureQueryBuffer",
                BindingFlags.NonPublic);
            Assert(bufferType != null,
                "dev15 private radar query buffer exists");
            object buffer = Activator.CreateInstance(
                bufferType,
                BindingFlags.Instance | BindingFlags.Public |
                    BindingFlags.NonPublic,
                null,
                new object[] { 80 },
                CultureInfo.InvariantCulture);
            MethodInfo refresh = bufferType.GetMethod(
                "RefreshIfNeeded",
                BindingFlags.Instance | BindingFlags.Public);
            MethodInfo reset = bufferType.GetMethod(
                "Reset",
                BindingFlags.Instance | BindingFlags.Public);
            PropertyInfo count = bufferType.GetProperty("Count");
            PropertyInfo item = bufferType.GetProperty("Item");
            DateTime now = DateTime.UtcNow;
            object[] first = { new List<WorldTreasure> { map100 }, 7,
                0.0, 0.0, 0.0, false, 1000.0, now, 1000, -150.0 };
            Assert((bool)refresh.Invoke(buffer, first),
                "dev15 initial compact selection builds query buffer");
            Assert((int)count.GetValue(buffer, null) == 1
                && Object.ReferenceEquals(
                    item.GetValue(buffer, new object[] { 0 }), map100),
                "dev15 initial compact source selected");

            object[] sameVersion = { new List<WorldTreasure> { map200 }, 7,
                0.0, 0.0, 0.0, false, 1000.0,
                now.AddMilliseconds(1), 1000, -150.0 };
            Assert(!(bool)refresh.Invoke(buffer, sameVersion)
                && Object.ReferenceEquals(
                    item.GetValue(buffer, new object[] { 0 }), map100),
                "dev15 query ownership ignores replacement source until index version or movement changes");

            sameVersion[1] = 8;
            Assert((bool)refresh.Invoke(buffer, sameVersion)
                && Object.ReferenceEquals(
                    item.GetValue(buffer, new object[] { 0 }), map200),
                "dev15 index-version change rebuilds selection");
            Assert((bool)reset.Invoke(buffer, null)
                && (int)count.GetValue(buffer, null) == 0,
                "dev15 query reset clears selection");
        }

        private static void SetPrivateField(
            object target,
            string name,
            object value)
        {
            FieldInfo field = target.GetType().GetField(
                name,
                BindingFlags.Instance | BindingFlags.NonPublic);
            if (field == null)
            {
                throw new InvalidOperationException(
                    "Missing test field: " + name);
            }
            field.SetValue(target, value);
        }

        private static string ReplaceField(
            string record,
            int index,
            string value)
        {
            string[] fields = record.Split('|');
            if (index < 0 || index >= fields.Length)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            fields[index] = value;
            return String.Join("|", fields);
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

            fields = WorldTreasureCatalog.ParseFields(
                "{ mini_game_id=11001, has_z=true, optional=false, missing=nil }");
            Assert(fields["has_z"] == "true", "catalog true literal");
            Assert(fields["optional"] == "false", "catalog false literal");
            Assert(fields["missing"] == "nil", "catalog nil literal");
        }

        private static void TestBossRuleDefault()
        {
            BossRespawnRuleResolver resolver = new BossRespawnRuleResolver();

            DateTime beforeReset = new DateTime(
                2026, 8, 4, 23, 0, 0, DateTimeKind.Utc);
            DateTime sameDayReset = new DateTime(
                2026, 8, 5, 0, 0, 0, DateTimeKind.Utc);
            Assert(resolver.NextAvailableUtc(beforeReset.ToUniversalTime()) ==
                sameDayReset,
                "daily 09:00 KST reset before boundary");

            DateTime atReset = new DateTime(
                2026, 8, 5, 0, 0, 0, DateTimeKind.Utc);
            DateTime nextDayReset = new DateTime(
                2026, 8, 6, 0, 0, 0, DateTimeKind.Utc);
            Assert(resolver.NextAvailableUtc(atReset.ToUniversalTime()) ==
                nextDayReset,
                "daily 09:00 KST reset at boundary");

            DateTime afterReset = new DateTime(
                2026, 8, 5, 9, 30, 0, DateTimeKind.Utc);
            Assert(resolver.NextAvailableUtc(afterReset.ToUniversalTime()) ==
                nextDayReset,
                "daily 09:00 KST reset after boundary");
            Assert(resolver.RuleSummary ==
                "type=DAILY; resetKst=09:00; resetUtc=00:00; source=user-validated-runtime",
                "fixed boss rule summary");
        }

        private static void TestMoleCatalogParserContract()
        {
            HashSet<int> ids = new HashSet<int>();
            HashSet<int> bits = new HashSet<int>();
            for (int index = 0; index < 34; index++)
            {
                int id = 11001 + index;
                string line = String.Format(
                    CultureInfo.InvariantCulture,
                    "{{ mini_game_id = {0}, reward_save_id = {0}, mask_bit = {1}, map_id = 100, " +
                    "x = {2}, y = {3}, z = 157, has_z = true, " +
                    "position_role = \"NPC_Start\", notice_title = \"109208\", " +
                    "notice_description = \"109202\" }}",
                    id,
                    index,
                    1000 + index,
                    2000 + index);
                Dictionary<string, string> fields =
                    WorldTreasureCatalog.ParseFields(line);
                int parsedId;
                int parsedBit;
                bool hasZ;
                Assert(Int32.TryParse(fields["mini_game_id"], out parsedId),
                    "mole parser id");
                Assert(Int32.TryParse(fields["mask_bit"], out parsedBit),
                    "mole parser bit");
                Assert(Boolean.TryParse(fields["has_z"], out hasZ) && hasZ,
                    "mole parser has_z");
                Assert(ids.Add(parsedId), "mole parser unique id");
                Assert(bits.Add(parsedBit), "mole parser unique bit");
            }
            Assert(ids.Count == 34 && bits.Count == 34,
                "mole parser complete 34-record shape");
        }

        private static void TestMoleRewardTreasureVisibility()
        {
            List<WorldMole> moles = new List<WorldMole>();
            for (int index = 0; index < 34; index++)
            {
                moles.Add(new WorldMole
                {
                    MiniGameId = 11001 + index,
                    RewardSaveId = 11001 + index,
                    MaskBit = index,
                    MapId = 100,
                    X = index + 1,
                    Y = index + 2
                });
            }
            WorldMoleCatalog catalog = new WorldMoleCatalog();
            SetPrivateField(catalog, "_points", moles);
            SetPrivateField(catalog, "_version", 1);
            TreasureSaveState save = new TreasureSaveState();
            MoleRewardVisibilityIndex indexer =
                new MoleRewardVisibilityIndex();

            SetPrivateField(save, "_version", 0);
            SetPrivateField(save, "_hasLoadedSaveState", false);
            Assert(!indexer.Refresh(catalog, save)
                && indexer.VisibleMask == 0,
                "moles fail closed before save snapshot");

            Dictionary<int, ulong> opened =
                new Dictionary<int, ulong>();
            SetPrivateField(save, "_opened", opened);
            SetPrivateField(save, "_hasLoadedSaveState", true);
            SetPrivateField(save, "_version", 1);
            Assert(indexer.Refresh(catalog, save)
                && indexer.VisibleMask == (1L << 34) - 1,
                "all 34 unclaimed reward IDs remain visible");

            opened = new Dictionary<int, ulong>
            {
                {
                    11001 / 64,
                    1UL << (11001 % 64)
                },
                {
                    11034 / 64,
                    1UL << (11034 % 64)
                }
            };
            SetPrivateField(save, "_opened", opened);
            SetPrivateField(save, "_version", 2);
            long expected = ((1L << 34) - 1)
                & ~(1L << 0)
                & ~(1L << 33);
            Assert(indexer.Refresh(catalog, save)
                && indexer.VisibleMask == expected,
                "claimed same-ID rewards hide only matching moles");
            Assert((indexer.VisibleMask & (1L << 2)) != 0,
                "mole 11003 bypasses treasure ignore overrides");
            Assert(!TreasureSaveState.IsRawOpened(11003, opened),
                "raw reward lookup does not coerce ignored 11003 open");
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

        private static void TestGeneratedOwnerPointerConfig()
        {
            string root = Path.Combine(
                Path.GetTempPath(),
                "DragonSwordWorldRadar-OwnerConfig-" +
                    Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(root);
            try
            {
                string executable = Path.Combine(root, "game.exe");
                string pak = Path.Combine(root, "game.pak");
                string config = Path.Combine(root, "owner.cfg");
                File.WriteAllBytes(executable, new byte[4096]);
                File.WriteAllBytes(pak, new byte[128]);
                string fingerprint = GeneratedOwnerPointerConfig
                    .ComputeGameFingerprint(executable, pak);
                WriteOwnerConfig(config, fingerprint, 4096, "0x100");
                GeneratedOwnerPointerConfig loaded =
                    GeneratedOwnerPointerConfig.Load(
                        config,
                        executable,
                        pak);
                Assert(loaded.Rva == 0x100, "generated RVA exact fingerprint");

                WriteOwnerConfig(
                    config,
                    new string('0', 64),
                    4096,
                    "0x100");
                AssertThrows(delegate
                {
                    GeneratedOwnerPointerConfig.Load(config, executable, pak);
                }, "generated RVA mismatched fingerprint");

                WriteOwnerConfig(config, fingerprint, 4096, "not-hex");
                AssertThrows(delegate
                {
                    GeneratedOwnerPointerConfig.Load(config, executable, pak);
                }, "generated RVA malformed config");

                WriteOwnerConfig(config, fingerprint, 4096, "0x1000");
                AssertThrows(delegate
                {
                    GeneratedOwnerPointerConfig.Load(config, executable, pak);
                }, "generated RVA out of range");
            }
            finally
            {
                Directory.Delete(root, true);
            }
        }

        private static void TestOwnerPointerPeResolver()
        {
            byte[] one = BuildOwnerPointerPe(1);
            Assert(
                DragonSwordWorldRadar.Installer.OwnerPointerRvaResolver
                    .ResolveImage(one) == 0x3000,
                "installer owner pattern unique match");
            AssertThrows(delegate
            {
                DragonSwordWorldRadar.Installer.OwnerPointerRvaResolver
                    .ResolveImage(BuildOwnerPointerPe(0));
            }, "installer owner pattern no match");
            AssertThrows(delegate
            {
                DragonSwordWorldRadar.Installer.OwnerPointerRvaResolver
                    .ResolveImage(BuildOwnerPointerPe(2));
            }, "installer owner pattern ambiguous");
            byte[] invalidBounds = BuildOwnerPointerPe(1);
            WriteUInt32(invalidBounds, 0x188 + 20, 0xF00);
            WriteUInt32(invalidBounds, 0x188 + 16, 0x400);
            AssertThrows(delegate
            {
                DragonSwordWorldRadar.Installer.OwnerPointerRvaResolver
                    .ResolveImage(invalidBounds);
            }, "installer owner invalid PE bounds");
        }

        private static byte[] BuildOwnerPointerPe(int matchCount)
        {
            byte[] image = new byte[0x1000];
            image[0] = (byte)'M';
            image[1] = (byte)'Z';
            WriteInt32(image, 0x3C, 0x80);
            WriteUInt32(image, 0x80, 0x00004550);
            WriteUInt16(image, 0x80 + 6, 1);
            WriteUInt16(image, 0x80 + 20, 0xF0);
            WriteUInt32(image, 0x80 + 24 + 56, 0x5000);
            int section = 0x80 + 24 + 0xF0;
            WriteUInt32(image, section + 12, 0x1000);
            WriteUInt32(image, section + 16, 0x400);
            WriteUInt32(image, section + 20, 0x400);
            byte[] pattern =
            {
                0x48, 0x8B, 0x0D, 0, 0, 0, 0,
                0xE8, 0, 0, 0, 0,
                0x8B, 0xC7, 0x48, 0x8B, 0x5C, 0x24,
                0x40, 0x48, 0x8B, 0x6C, 0x24, 0x50,
            };
            for (int index = 0; index < matchCount; index++)
            {
                int offset = 0x420 + index * 0x60;
                Buffer.BlockCopy(pattern, 0, image, offset, pattern.Length);
                int instructionRva = 0x1000 + offset - 0x400;
                int target = 0x3000 + index * 0x100;
                WriteInt32(image, offset + 3, target - (instructionRva + 7));
            }
            return image;
        }

        private static void WriteOwnerConfig(
            string path,
            string fingerprint,
            long length,
            string rva)
        {
            File.WriteAllText(path, String.Join("\n", new[]
            {
                "schema_version=1",
                "game_fingerprint=" + fingerprint,
                "executable_length=" + length.ToString(CultureInfo.InvariantCulture),
                "owner_pointer_rva=" + rva,
                "provenance=install-time-exact-executable-pattern",
            }));
        }

        private static void AssertThrows(Action action, string name)
        {
            bool threw = false;
            try { action(); }
            catch { threw = true; }
            Assert(threw, name);
        }

        private static void WriteUInt16(byte[] data, int offset, ushort value)
        {
            Buffer.BlockCopy(BitConverter.GetBytes(value), 0, data, offset, 2);
        }

        private static void WriteUInt32(byte[] data, int offset, uint value)
        {
            Buffer.BlockCopy(BitConverter.GetBytes(value), 0, data, offset, 4);
        }

        private static void WriteInt32(byte[] data, int offset, int value)
        {
            Buffer.BlockCopy(BitConverter.GetBytes(value), 0, data, offset, 4);
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
