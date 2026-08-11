using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;

namespace DragonSwordWorldRadar
{
    internal sealed class WorldTreasureCatalog
    {
        private static readonly Regex FieldPattern = new Regex(
            @"([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(?:""([^""]*)""|([-+0-9.eE]+)|(true|false|nil))",
            RegexOptions.Compiled | RegexOptions.CultureInvariant);

        private readonly string _path;
        private DateTime _lastWriteUtc;
        private long _lastLength;
        private DateTime _nextRefreshUtc;
        private bool _hasLoaded;
        private int _version;
        private List<WorldTreasure> _points =
            new List<WorldTreasure>();

        public WorldTreasureCatalog()
        {
            _path = Path.Combine(
                ModPath.BaseDirectory,
                "data",
                "generated",
                "treasures.lua");
        }

        public IList<WorldTreasure> Points
        {
            get { return _points; }
        }

        public int Version
        {
            get { return _version; }
        }

        public void Refresh()
        {
            // Installer-generated coordinates are immutable for one Overlay
            // process. Once validated, avoid recurring filesystem metadata
            // checks on the maintenance path; an install/reinstall starts a
            // new process and performs a fresh validation.
            if (_hasLoaded)
            {
                return;
            }
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

            // The generated catalog now includes Z coordinates and UIDName.
            // The additional metadata remains local and is used only for
            // height diagnostics, type labels, colors, and override aliases.
            List<WorldTreasure> loaded =
                new List<WorldTreasure>();
            foreach (string line in File.ReadLines(_path))
            {
                Dictionary<string, string> fields = ParseFields(line);
                string section;
                string saveIdText;
                string xText;
                string yText;
                if (!fields.TryGetValue("section", out section)
                    || !fields.TryGetValue("save_id", out saveIdText)
                    || !fields.TryGetValue("x", out xText)
                    || !fields.TryGetValue("y", out yText))
                {
                    continue;
                }

                int mapId;
                long saveId;
                double x;
                double y;
                if (section.Length < 3
                    || !int.TryParse(
                        section.Substring(section.Length - 3),
                        NumberStyles.None,
                        CultureInfo.InvariantCulture,
                        out mapId)
                    || !long.TryParse(
                        saveIdText,
                        NumberStyles.None,
                        CultureInfo.InvariantCulture,
                        out saveId)
                    || !double.TryParse(
                        xText,
                        NumberStyles.Float,
                        CultureInfo.InvariantCulture,
                        out x)
                    || !double.TryParse(
                        yText,
                        NumberStyles.Float,
                        CultureInfo.InvariantCulture,
                        out y))
                {
                    continue;
                }

                double z = 0;
                bool hasZ = false;
                string zText;
                if (fields.TryGetValue("z", out zText))
                {
                    hasZ = double.TryParse(
                        zText,
                        NumberStyles.Float,
                        CultureInfo.InvariantCulture,
                        out z);
                }

                string uidName = null;
                string uidNameText;
                if (fields.TryGetValue(
                    "uid_name",
                    out uidNameText))
                {
                    uidName = uidNameText;
                }

                long groupId = 0;
                string groupIdText;
                if (fields.TryGetValue(
                    "group_id",
                    out groupIdText))
                {
                    long.TryParse(
                        groupIdText,
                        NumberStyles.None,
                        CultureInfo.InvariantCulture,
                        out groupId);
                }

                loaded.Add(new WorldTreasure
                {
                    SaveId = saveId,
                    MapId = mapId,
                    X = x,
                    Y = y,
                    Z = z,
                    HasZ = hasZ,
                    UidName = uidName,
                    GroupId = groupId
                });
            }

            FileInfo after = new FileInfo(_path);
            after.Refresh();
            if (!after.Exists
                || after.LastWriteTimeUtc != writeTime
                || after.Length != length)
            {
                throw new IOException(
                    "Treasure catalog changed while it was being read.");
            }

            _points = loaded;
            _lastWriteUtc = writeTime;
            _lastLength = length;
            _hasLoaded = true;
            _version++;
        }

        internal static Dictionary<string, string> ParseFields(
            string line)
        {
            Dictionary<string, string> fields =
                new Dictionary<string, string>(
                    StringComparer.OrdinalIgnoreCase);
            foreach (Match match in FieldPattern.Matches(line))
            {
                string value = match.Groups[2].Success
                    ? match.Groups[2].Value
                    : (match.Groups[3].Success
                        ? match.Groups[3].Value
                        : match.Groups[4].Value);
                fields[match.Groups[1].Value] = value;
            }
            return fields;
        }
    }
}
