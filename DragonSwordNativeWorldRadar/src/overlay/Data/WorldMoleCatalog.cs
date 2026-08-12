using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal sealed class WorldMoleCatalog
    {
        private const int ExpectedCount = 34;
        private readonly string _path;
        private DateTime _nextRetryUtc;
        private bool _hasLoaded;
        private int _version;
        private List<WorldMole> _points = new List<WorldMole>();
        private Dictionary<int, List<WorldMole>> _byMap =
            new Dictionary<int, List<WorldMole>>();

        public WorldMoleCatalog()
        {
            _path = Path.Combine(
                ModPath.BaseDirectory,
                "data",
                "generated",
                "moles.lua");
        }

        public IList<WorldMole> Points
        {
            get { return _points; }
        }

        public int Version
        {
            get { return _version; }
        }

        public int Count
        {
            get { return _points.Count; }
        }

        public IList<WorldMole> GetMap(int mapId)
        {
            List<WorldMole> points;
            return _byMap.TryGetValue(mapId, out points)
                ? points
                : (IList<WorldMole>)Array.Empty<WorldMole>();
        }

        public bool Refresh()
        {
            // Install-generated coordinates are immutable for one game/Mod
            // session. After one validated load, avoid recurring filesystem
            // metadata checks on the Overlay maintenance path.
            if (_hasLoaded)
            {
                return false;
            }
            DateTime now = DateTime.UtcNow;
            if (now < _nextRetryUtc)
            {
                return false;
            }
            _nextRetryUtc = now.AddSeconds(5);

            FileInfo before = new FileInfo(_path);
            before.Refresh();
            if (!before.Exists)
            {
                return false;
            }
            DateTime writeTime = before.LastWriteTimeUtc;
            long length = before.Length;

            List<WorldMole> loaded = new List<WorldMole>();
            HashSet<int> ids = new HashSet<int>();
            HashSet<int> bits = new HashSet<int>();
            foreach (string line in File.ReadLines(_path))
            {
                Dictionary<string, string> fields =
                    WorldTreasureCatalog.ParseFields(line);
                string idText;
                string rewardSaveIdText;
                string bitText;
                string mapText;
                string xText;
                string yText;
                string hasZText;
                string roleText;
                string noticeTitleText;
                string noticeDescriptionText;
                if (!fields.TryGetValue("mini_game_id", out idText)
                    || !fields.TryGetValue(
                        "reward_save_id",
                        out rewardSaveIdText)
                    || !fields.TryGetValue("mask_bit", out bitText)
                    || !fields.TryGetValue("map_id", out mapText)
                    || !fields.TryGetValue("x", out xText)
                    || !fields.TryGetValue("y", out yText)
                    || !fields.TryGetValue("has_z", out hasZText)
                    || !fields.TryGetValue("position_role", out roleText)
                    || !fields.TryGetValue("notice_title", out noticeTitleText)
                    || !fields.TryGetValue(
                        "notice_description",
                        out noticeDescriptionText))
                {
                    continue;
                }

                int id;
                long rewardSaveId;
                int bit;
                int mapId;
                double x;
                double y;
                if (!Int32.TryParse(
                        idText,
                        NumberStyles.Integer,
                        CultureInfo.InvariantCulture,
                        out id)
                    || !Int64.TryParse(
                        rewardSaveIdText,
                        NumberStyles.Integer,
                        CultureInfo.InvariantCulture,
                        out rewardSaveId)
                    || !Int32.TryParse(
                        bitText,
                        NumberStyles.Integer,
                        CultureInfo.InvariantCulture,
                        out bit)
                    || !Int32.TryParse(
                        mapText,
                        NumberStyles.Integer,
                        CultureInfo.InvariantCulture,
                        out mapId)
                    || !Double.TryParse(
                        xText,
                        NumberStyles.Float,
                        CultureInfo.InvariantCulture,
                        out x)
                    || !Double.TryParse(
                        yText,
                        NumberStyles.Float,
                        CultureInfo.InvariantCulture,
                        out y)
                    || id < 11001 || id > 11034
                    || rewardSaveId <= 0
                    || bit < 0 || bit >= ExpectedCount
                    || mapId <= 0
                    || Double.IsNaN(x) || Double.IsInfinity(x)
                    || Double.IsNaN(y) || Double.IsInfinity(y)
                    || (Math.Abs(x) < 0.001 && Math.Abs(y) < 0.001)
                    || (!String.Equals(
                            roleText,
                            "Teleport_Start",
                            StringComparison.OrdinalIgnoreCase)
                        && !String.Equals(
                            roleText,
                            "NPC_Start",
                            StringComparison.OrdinalIgnoreCase)
                        && !String.Equals(
                            roleText,
                            "Fly_Linked",
                            StringComparison.OrdinalIgnoreCase))
                    || !String.Equals(
                        noticeTitleText,
                        "109208",
                        StringComparison.Ordinal)
                    || !String.Equals(
                        noticeDescriptionText,
                        "109202",
                        StringComparison.Ordinal)
                    || !ids.Add(id)
                    || !bits.Add(bit))
                {
                    throw new InvalidDataException(
                        "The generated Mole catalog contains an invalid or duplicate record.");
                }

                bool declaredHasZ;
                if (!Boolean.TryParse(hasZText, out declaredHasZ))
                {
                    throw new InvalidDataException(
                        "The generated Mole catalog contains an invalid has_z flag.");
                }

                double z = 0.0;
                bool hasZ = false;
                string zText;
                if (declaredHasZ)
                {
                    if (!fields.TryGetValue("z", out zText)
                        || !Double.TryParse(
                            zText,
                            NumberStyles.Float,
                            CultureInfo.InvariantCulture,
                            out z)
                        || Double.IsNaN(z)
                        || Double.IsInfinity(z))
                    {
                        throw new InvalidDataException(
                            "The generated Mole catalog contains an invalid or missing Z coordinate.");
                    }
                    hasZ = true;
                }

                string uidName;
                fields.TryGetValue("uid_name", out uidName);
                string sourceEntry;
                fields.TryGetValue("source_entry", out sourceEntry);
                loaded.Add(new WorldMole
                {
                    MiniGameId = id,
                    RewardSaveId = rewardSaveId,
                    MaskBit = bit,
                    MapId = mapId,
                    X = x,
                    Y = y,
                    Z = z,
                    HasZ = hasZ,
                    UidName = uidName,
                    SourceEntry = sourceEntry
                });
            }

            if (loaded.Count != ExpectedCount
                || ids.Count != ExpectedCount
                || bits.Count != ExpectedCount)
            {
                throw new InvalidDataException(
                    "The generated Mole catalog must contain exactly 34 validated Fly records.");
            }

            loaded.Sort(delegate(WorldMole left, WorldMole right)
            {
                return left.MaskBit.CompareTo(right.MaskBit);
            });
            for (int index = 0; index < loaded.Count; index++)
            {
                if (loaded[index].MaskBit != index)
                {
                    throw new InvalidDataException(
                        "The generated Mole catalog must use contiguous mask bits starting at 0.");
                }
            }
            loaded.Sort(delegate(WorldMole left, WorldMole right)
            {
                return left.MiniGameId.CompareTo(right.MiniGameId);
            });
            Dictionary<int, List<WorldMole>> byMap =
                new Dictionary<int, List<WorldMole>>();
            foreach (WorldMole mole in loaded)
            {
                List<WorldMole> mapPoints;
                if (!byMap.TryGetValue(mole.MapId, out mapPoints))
                {
                    mapPoints = new List<WorldMole>();
                    byMap[mole.MapId] = mapPoints;
                }
                mapPoints.Add(mole);
            }

            FileInfo after = new FileInfo(_path);
            after.Refresh();
            if (!after.Exists
                || after.LastWriteTimeUtc != writeTime
                || after.Length != length)
            {
                throw new IOException(
                    "Mole catalog changed while it was being read.");
            }

            _points = loaded;
            _byMap = byMap;
            _hasLoaded = true;
            _version++;
            return true;
        }
    }
}
