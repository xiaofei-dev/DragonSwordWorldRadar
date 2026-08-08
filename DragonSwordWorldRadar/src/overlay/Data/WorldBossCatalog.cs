using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal sealed class WorldBossCatalog
    {
        private const int ExpectedBossCount = 9;

        private readonly string _path;
        private DateTime _lastWriteUtc;
        private long _lastLength;
        private DateTime _nextRefreshUtc;
        private bool _hasLoaded;
        private int _version;
        private List<BossPoint> _points = new List<BossPoint>();

        public WorldBossCatalog()
        {
            _path = Path.Combine(
                ModPath.BaseDirectory,
                "data",
                "generated",
                "bosses.lua");
        }

        public IList<BossPoint> Points
        {
            get { return _points; }
        }

        public int Version
        {
            get { return _version; }
        }

        public void Refresh()
        {
            DateTime now = DateTime.UtcNow;
            if (now < _nextRefreshUtc)
            {
                return;
            }
            _nextRefreshUtc = now.AddSeconds(5);

            FileInfo before = new FileInfo(_path);
            before.Refresh();
            if (!before.Exists)
            {
                return;
            }
            DateTime writeTime = before.LastWriteTimeUtc;
            long length = before.Length;
            if (_hasLoaded
                && writeTime == _lastWriteUtc
                && length == _lastLength)
            {
                return;
            }

            List<BossPoint> loaded = new List<BossPoint>();
            HashSet<int> bossIds = new HashSet<int>();
            foreach (string line in File.ReadLines(_path))
            {
                Dictionary<string, string> fields =
                    WorldTreasureCatalog.ParseFields(line);
                string bossIdText;
                string mapIdText;
                string xText;
                string yText;
                if (!fields.TryGetValue("boss_id", out bossIdText)
                    || !fields.TryGetValue("map_id", out mapIdText)
                    || !fields.TryGetValue("x", out xText)
                    || !fields.TryGetValue("y", out yText))
                {
                    continue;
                }

                int bossId;
                int mapId;
                double x;
                double y;
                if (!Int32.TryParse(
                        bossIdText,
                        NumberStyles.None,
                        CultureInfo.InvariantCulture,
                        out bossId)
                    || !Int32.TryParse(
                        mapIdText,
                        NumberStyles.None,
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
                    || bossId <= 0
                    || mapId <= 0
                    || Double.IsNaN(x)
                    || Double.IsInfinity(x)
                    || Double.IsNaN(y)
                    || Double.IsInfinity(y))
                {
                    throw new InvalidDataException(
                        "World-boss catalog contains an invalid required field.");
                }
                if (!bossIds.Add(bossId))
                {
                    throw new InvalidDataException(
                        "World-boss catalog contains duplicate boss ID " +
                        bossId.ToString(CultureInfo.InvariantCulture) + ".");
                }

                double z = 0.0;
                bool hasZ = false;
                string zText;
                if (fields.TryGetValue("z", out zText))
                {
                    hasZ = Double.TryParse(
                        zText,
                        NumberStyles.Float,
                        CultureInfo.InvariantCulture,
                        out z)
                        && !Double.IsNaN(z)
                        && !Double.IsInfinity(z);
                    if (!hasZ)
                    {
                        throw new InvalidDataException(
                            "World-boss catalog contains an invalid Z value.");
                    }
                }

                loaded.Add(new BossPoint
                {
                    bossId = bossId,
                    mapId = mapId,
                    x = x,
                    y = y,
                    z = z,
                    hasZ = hasZ,
                    dx = 0.0,
                    dy = 0.0,
                    visible = true,
                    status = "save-cooldown-driven"
                });
            }

            if (loaded.Count != ExpectedBossCount)
            {
                throw new InvalidDataException(
                    "World-boss catalog must contain exactly " +
                    ExpectedBossCount.ToString(CultureInfo.InvariantCulture) +
                    " records; loaded " +
                    loaded.Count.ToString(CultureInfo.InvariantCulture) + ".");
            }

            FileInfo after = new FileInfo(_path);
            after.Refresh();
            if (!after.Exists
                || after.LastWriteTimeUtc != writeTime
                || after.Length != length)
            {
                throw new IOException(
                    "World-boss catalog changed while it was being read.");
            }

            _points = loaded;
            _lastWriteUtc = writeTime;
            _lastLength = length;
            _hasLoaded = true;
            _version++;
        }
    }
}
