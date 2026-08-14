using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal sealed class TreasureOverrides
    {
        private readonly object _sync = new object();
        private readonly string _path = Path.Combine(
            ModPath.BaseDirectory,
            "data",
            "treasure_overrides.txt");
        private readonly string _catalogPath = Path.Combine(
            ModPath.BaseDirectory,
            "data",
            "generated",
            "treasures.lua");

        private volatile OverrideSnapshot _snapshot =
            OverrideSnapshot.Empty;
        private bool _loadAttempted;
        private DateTime _lastOverrideWriteUtc;
        private long _lastOverrideLength;
        private DateTime _lastCatalogWriteUtc;
        private long _lastCatalogLength;
        private bool _lastOverrideExists;
        private bool _lastCatalogExists;
        private string _lastError;
        private int _version;

        public int Version
        {
            get
            {
                lock (_sync)
                {
                    return _version;
                }
            }
        }

        public long Resolve(long saveId, out bool ignored)
        {
            // Overrides are loaded once during Overlay startup. The
            // dictionaries are immutable after publication, so marker reads
            // do not need a lock or a filesystem check on every paint.
            OverrideSnapshot snapshot = _snapshot;
            long resolved;
            if (!snapshot.Aliases.TryGetValue(saveId, out resolved))
            {
                resolved = saveId;
            }

            // Resolve aliases before applying ignore rules so ignoring an
            // alias target also hides every source that maps to it.
            ignored = snapshot.Ignored.Contains(saveId)
                || snapshot.Ignored.Contains(resolved);
            return resolved;
        }

        public void LoadOnce()
        {
            lock (_sync)
            {
                if (_loadAttempted)
                {
                    return;
                }
                _loadAttempted = true;
            }

            try
            {
                bool overrideExists;
                DateTime overrideWriteUtc;
                long overrideLength;
                CaptureFileStamp(
                    _path,
                    out overrideExists,
                    out overrideWriteUtc,
                    out overrideLength);
                bool catalogExists;
                DateTime catalogWriteUtc;
                long catalogLength;
                CaptureFileStamp(
                    _catalogPath,
                    out catalogExists,
                    out catalogWriteUtc,
                    out catalogLength);

                lock (_sync)
                {
                    if (overrideExists == _lastOverrideExists
                        && overrideWriteUtc == _lastOverrideWriteUtc
                        && overrideLength == _lastOverrideLength
                        && catalogExists == _lastCatalogExists
                        && catalogWriteUtc == _lastCatalogWriteUtc
                        && catalogLength == _lastCatalogLength)
                    {
                        return;
                    }
                }

                // Symbolic names such as U_10222 are resolved against the
                // locally generated treasure catalog. If a name identifies
                // multiple duplicate records, every matching record is ignored.
                Dictionary<string, HashSet<long>> namedIds =
                    LoadNamedIds(catalogExists);
                Dictionary<long, long> aliases =
                    new Dictionary<long, long>();
                HashSet<long> ignored =
                    new HashSet<long>();

                if (overrideExists)
                {
                    foreach (string rawLine in File.ReadAllLines(_path))
                    {
                        string line = RemoveComment(rawLine).Trim();
                        if (line.Length == 0)
                        {
                            continue;
                        }

                        string[] parts = line.Split(
                            new[] { ' ', '\t' },
                            StringSplitOptions.RemoveEmptyEntries);
                        if (parts.Length == 2
                            && string.Equals(
                                parts[0],
                                "ignore",
                                StringComparison.OrdinalIgnoreCase))
                        {
                            AddIgnoredToken(
                                parts[1],
                                namedIds,
                                ignored);
                            continue;
                        }

                        if (parts.Length == 3
                            && string.Equals(
                                parts[0],
                                "alias",
                                StringComparison.OrdinalIgnoreCase))
                        {
                            long source;
                            long target;
                            if (TryParseId(parts[1], out source)
                                && TryParseId(parts[2], out target))
                            {
                                aliases[source] = target;
                            }
                        }
                    }
                }

                bool overrideExistsAfter;
                DateTime overrideWriteUtcAfter;
                long overrideLengthAfter;
                CaptureFileStamp(
                    _path,
                    out overrideExistsAfter,
                    out overrideWriteUtcAfter,
                    out overrideLengthAfter);
                bool catalogExistsAfter;
                DateTime catalogWriteUtcAfter;
                long catalogLengthAfter;
                CaptureFileStamp(
                    _catalogPath,
                    out catalogExistsAfter,
                    out catalogWriteUtcAfter,
                    out catalogLengthAfter);
                if (overrideExistsAfter != overrideExists
                    || overrideWriteUtcAfter != overrideWriteUtc
                    || overrideLengthAfter != overrideLength
                    || catalogExistsAfter != catalogExists
                    || catalogWriteUtcAfter != catalogWriteUtc
                    || catalogLengthAfter != catalogLength)
                {
                    throw new IOException(
                        "Treasure override inputs changed while being read.");
                }

                lock (_sync)
                {
                    _snapshot = new OverrideSnapshot(aliases, ignored);
                    _lastOverrideExists = overrideExists;
                    _lastOverrideWriteUtc = overrideWriteUtc;
                    _lastOverrideLength = overrideLength;
                    _lastCatalogExists = catalogExists;
                    _lastCatalogWriteUtc = catalogWriteUtc;
                    _lastCatalogLength = catalogLength;
                    _lastError = null;
                    _version++;
                }
            }
            catch (Exception exception)
            {
                string message =
                    exception.GetType().FullName + ": " +
                    exception.Message;
                lock (_sync)
                {
                    if (message == _lastError)
                    {
                        return;
                    }
                    _lastError = message;
                }
                ErrorLog.Write(
                    "Treasure override refresh failed",
                    exception);
            }
        }

        private static void CaptureFileStamp(
            string path,
            out bool exists,
            out DateTime writeUtc,
            out long length)
        {
            FileInfo info = new FileInfo(path);
            info.Refresh();
            exists = info.Exists;
            writeUtc = exists
                ? info.LastWriteTimeUtc
                : DateTime.MinValue;
            length = exists ? info.Length : 0;
        }

        private Dictionary<string, HashSet<long>> LoadNamedIds(
            bool catalogExists)
        {
            Dictionary<string, HashSet<long>> result =
                new Dictionary<string, HashSet<long>>(
                    StringComparer.OrdinalIgnoreCase);
            if (!catalogExists)
            {
                return result;
            }

            foreach (string line in File.ReadLines(_catalogPath))
            {
                Dictionary<string, string> fields =
                    WorldTreasureCatalog.ParseFields(line);
                string saveIdText;
                string uidName;
                long saveId;
                if (!fields.TryGetValue(
                        "save_id",
                        out saveIdText)
                    || !long.TryParse(
                        saveIdText,
                        NumberStyles.None,
                        CultureInfo.InvariantCulture,
                        out saveId)
                    || saveId <= 0)
                {
                    continue;
                }

                fields.TryGetValue("uid_name", out uidName);
                AddNamedId(
                    result,
                    TreasureIdentity.GetDebugName(uidName, saveId),
                    saveId);
                if (!String.IsNullOrWhiteSpace(uidName))
                {
                    AddNamedId(result, uidName, saveId);
                }
            }

            return result;
        }

        private static void AddIgnoredToken(
            string token,
            IDictionary<string, HashSet<long>> namedIds,
            ISet<long> ignored)
        {
            long id;
            if (TryParseId(token, out id))
            {
                ignored.Add(id);
                return;
            }

            HashSet<long> matches;
            if (!namedIds.TryGetValue(token, out matches))
            {
                ErrorLog.WriteDebug(
                    "Treasure override name was not found: " + token);
                return;
            }

            foreach (long matchedId in matches)
            {
                ignored.Add(matchedId);
            }
        }

        private static void AddNamedId(
            IDictionary<string, HashSet<long>> namedIds,
            string name,
            long id)
        {
            if (String.IsNullOrWhiteSpace(name))
            {
                return;
            }

            HashSet<long> ids;
            if (!namedIds.TryGetValue(name.Trim(), out ids))
            {
                ids = new HashSet<long>();
                namedIds[name.Trim()] = ids;
            }
            ids.Add(id);
        }

        private static string RemoveComment(string line)
        {
            int comment = line.IndexOf('#');
            return comment < 0
                ? line
                : line.Substring(0, comment);
        }

        private static bool TryParseId(
            string value,
            out long id)
        {
            return long.TryParse(
                value,
                NumberStyles.Integer,
                CultureInfo.InvariantCulture,
                out id)
                && id > 0;
        }

        private sealed class OverrideSnapshot
        {
            public static readonly OverrideSnapshot Empty =
                new OverrideSnapshot(
                    new Dictionary<long, long>(),
                    new HashSet<long>());

            public readonly Dictionary<long, long> Aliases;
            public readonly HashSet<long> Ignored;

            public OverrideSnapshot(
                Dictionary<long, long> aliases,
                HashSet<long> ignored)
            {
                Aliases = aliases;
                Ignored = ignored;
            }
        }
    }
}
