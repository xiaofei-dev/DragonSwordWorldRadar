using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace DragonSwordWorldRadar
{
    internal sealed class SaveSnapshot
    {
        public Dictionary<int, ulong> Opened;
        public bool OpenedAvailable;
        public Dictionary<int, BossRespawnRecord> BossRespawns;
        public EncounterTaskTableSnapshot EncounterTasks;
    }

    internal sealed class EncounterTaskTableSnapshot
    {
        public bool Exists;
        public string Source;
        public readonly List<string> Columns = new List<string>();
        public readonly List<string> Rows = new List<string>();

        public string Signature
        {
            get
            {
                return (Exists ? "1" : "0") + "|" +
                    (Source ?? "unknown") + "|" +
                    String.Join(",", Columns.ToArray()) + "|" +
                    String.Join("\n", Rows.ToArray());
            }
        }
    }

    internal sealed class BossRespawnRecord
    {
        public int BossId;
        public int RespawnType;
        public DateTime DestroyTimeUtc;
    }

    internal sealed class SaveSnapshotReadMetrics
    {
        public int DatabaseReads;
        public int DatabaseCacheHits;
        public double CopyMilliseconds;
        public double KeyMilliseconds;
        public double TreasureQueryMilliseconds;
        public double BossQueryMilliseconds;
        public double EncounterTaskQueryMilliseconds;
        public double TotalMilliseconds;
    }

    internal sealed class SaveSnapshotReader
    {
        private const int SqliteOpenReadWrite = 0x00000002;
        private readonly object _cacheSync = new object();
        private readonly Dictionary<string, CachedDatabaseSnapshot> _cache =
            new Dictionary<string, CachedDatabaseSnapshot>(
                StringComparer.OrdinalIgnoreCase);

        public SaveSnapshot Read(
            SaveSlotFingerprint slot,
            string key,
            IList<int> encounterTargetIds,
            bool includeTreasure,
            out SaveSnapshotReadMetrics metrics)
        {
            string actorFilterSignature = BuildActorFilterSignature(
                encounterTargetIds) +
                (includeTreasure ? "|treasure" : "|encounter");
            metrics = new SaveSnapshotReadMetrics();
            Stopwatch totalWatch = Stopwatch.StartNew();
            SaveSnapshot merged = new SaveSnapshot
            {
                Opened = new Dictionary<int, ulong>(),
                OpenedAvailable = false,
                BossRespawns = new Dictionary<int, BossRespawnRecord>(),
                EncounterTasks = new EncounterTaskTableSnapshot()
            };
            string temporaryDirectory = Path.Combine(
                Path.GetTempPath(),
                "DragonSwordNativeWorldRadar",
                Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(temporaryDirectory);
            Exception lastError = null;
            int loaded = 0;
            try
            {
                foreach (SaveDatabaseFingerprint database
                    in slot.Databases)
                {
                    if (!database.Exists)
                    {
                        continue;
                    }
                    try
                    {
                        SaveSnapshot current;
                        if (TryGetCached(
                                database,
                                key,
                                actorFilterSignature,
                                out current))
                        {
                            metrics.DatabaseCacheHits++;
                            if (current.OpenedAvailable)
                            {
                                MergeOpened(merged.Opened, current.Opened);
                                merged.OpenedAvailable = true;
                            }
                            MergeBossRespawns(
                                merged.BossRespawns,
                                current.BossRespawns);
                            MergeEncounterTasks(
                                merged.EncounterTasks,
                                current.EncounterTasks);
                            loaded++;
                            continue;
                        }

                        Stopwatch copyWatch = Stopwatch.StartNew();
                        string snapshotPath =
                            database.CopyConsistentSnapshot(
                                temporaryDirectory);
                        copyWatch.Stop();
                        metrics.CopyMilliseconds +=
                            copyWatch.Elapsed.TotalMilliseconds;
                        current = ReadFile(
                            snapshotPath,
                            key,
                            encounterTargetIds,
                            includeTreasure,
                            Path.GetFileName(database.Path),
                            metrics);
                        StoreCached(
                            database,
                            key,
                            actorFilterSignature,
                            current);
                        metrics.DatabaseReads++;
                        if (current.OpenedAvailable)
                        {
                            MergeOpened(merged.Opened, current.Opened);
                            merged.OpenedAvailable = true;
                        }
                        MergeBossRespawns(
                            merged.BossRespawns,
                            current.BossRespawns);
                        MergeEncounterTasks(
                            merged.EncounterTasks,
                            current.EncounterTasks);
                        loaded++;
                    }
                    catch (Exception exception)
                    {
                        lastError = exception;
                    }
                }
            }
            finally
            {
                try
                {
                    Directory.Delete(temporaryDirectory, true);
                }
                catch
                {
                    // A stale temporary directory is harmless and can be
                    // removed by the operating system later.
                }
            }

            if (loaded == 0)
            {
                throw new InvalidOperationException(
                    "No active save database snapshot could be read.",
                    lastError);
            }
            PruneCache(slot, key);
            totalWatch.Stop();
            metrics.TotalMilliseconds =
                totalWatch.Elapsed.TotalMilliseconds;
            return merged;
        }

        public void Reset()
        {
            lock (_cacheSync)
            {
                _cache.Clear();
            }
        }

        private bool TryGetCached(
            SaveDatabaseFingerprint database,
            string key,
            string actorFilterSignature,
            out SaveSnapshot snapshot)
        {
            lock (_cacheSync)
            {
                CachedDatabaseSnapshot cached;
                if (_cache.TryGetValue(database.Path, out cached)
                    && cached.Signature == database.Signature
                    && cached.Key == key
                    && cached.ActorFilterSignature ==
                        actorFilterSignature)
                {
                    snapshot = cached.Snapshot;
                    return true;
                }
            }
            snapshot = null;
            return false;
        }

        private void StoreCached(
            SaveDatabaseFingerprint database,
            string key,
            string actorFilterSignature,
            SaveSnapshot snapshot)
        {
            lock (_cacheSync)
            {
                _cache[database.Path] = new CachedDatabaseSnapshot
                {
                    Signature = database.Signature,
                    Key = key,
                    ActorFilterSignature = actorFilterSignature,
                    Snapshot = snapshot
                };
            }
        }

        private void PruneCache(
            SaveSlotFingerprint slot,
            string key)
        {
            HashSet<string> active = new HashSet<string>(
                slot.Databases.Where(item => item.Exists)
                    .Select(item => item.Path),
                StringComparer.OrdinalIgnoreCase);
            lock (_cacheSync)
            {
                List<string> removed = _cache
                    .Where(pair => !active.Contains(pair.Key)
                        || pair.Value.Key != key)
                    .Select(pair => pair.Key)
                    .ToList();
                foreach (string path in removed)
                {
                    _cache.Remove(path);
                }
            }
        }

        private static SaveSnapshot ReadFile(
            string source,
            string key,
            IList<int> encounterTargetIds,
            bool includeTreasure,
            string taskSource,
            SaveSnapshotReadMetrics metrics)
        {
            IntPtr database = IntPtr.Zero;
            try
            {
                int result = NativeMethods.sqlite3_open_v2(
                    Utf8(source),
                    out database,
                    SqliteOpenReadWrite,
                    IntPtr.Zero);
                if (result != 0)
                {
                    throw SqliteError(
                        database,
                        result,
                        IntPtr.Zero);
                }
                Stopwatch keyWatch = Stopwatch.StartNew();
                ApplyKey(database, key);
                keyWatch.Stop();
                metrics.KeyMilliseconds +=
                    keyWatch.Elapsed.TotalMilliseconds;

                Dictionary<int, ulong> opened =
                    new Dictionary<int, ulong>();
                if (includeTreasure)
                {
                    Stopwatch treasureWatch = Stopwatch.StartNew();
                    opened = QueryOpenedTreasureBits(database);
                    treasureWatch.Stop();
                    metrics.TreasureQueryMilliseconds +=
                        treasureWatch.Elapsed.TotalMilliseconds;
                }

                Stopwatch bossWatch = Stopwatch.StartNew();
                Dictionary<int, BossRespawnRecord> bossRespawns =
                    QueryBossRespawns(database, encounterTargetIds);
                bossWatch.Stop();
                metrics.BossQueryMilliseconds +=
                    bossWatch.Elapsed.TotalMilliseconds;
                Stopwatch taskWatch = Stopwatch.StartNew();
                EncounterTaskTableSnapshot encounterTasks =
                    QueryEncounterTaskTable(database);
                encounterTasks.Source = taskSource;
                taskWatch.Stop();
                metrics.EncounterTaskQueryMilliseconds +=
                    taskWatch.Elapsed.TotalMilliseconds;
                return new SaveSnapshot
                {
                    Opened = opened,
                    OpenedAvailable = includeTreasure,
                    BossRespawns = bossRespawns,
                    EncounterTasks = encounterTasks
                };
            }
            finally
            {
                if (database != IntPtr.Zero)
                {
                    NativeMethods.sqlite3_close_v2(database);
                }
            }
        }

        private static void MergeOpened(
            IDictionary<int, ulong> target,
            IDictionary<int, ulong> source)
        {
            foreach (KeyValuePair<int, ulong> pair in source)
            {
                ulong existing;
                target.TryGetValue(pair.Key, out existing);
                target[pair.Key] = existing | pair.Value;
            }
        }

        private static void MergeBossRespawns(
            IDictionary<int, BossRespawnRecord> target,
            IDictionary<int, BossRespawnRecord> source)
        {
            foreach (KeyValuePair<int, BossRespawnRecord> pair in source)
            {
                BossRespawnRecord existing;
                if (!target.TryGetValue(pair.Key, out existing)
                    || pair.Value.DestroyTimeUtc > existing.DestroyTimeUtc)
                {
                    target[pair.Key] = pair.Value;
                }
            }
        }

        private static string BuildActorFilterSignature(
            IList<int> encounterTargetIds)
        {
            List<int> normalized = NormalizeEncounterTargetIds(
                encounterTargetIds);
            StringBuilder signature = new StringBuilder();
            for (int index = 0; index < normalized.Count; index++)
            {
                if (index > 0)
                {
                    signature.Append(',');
                }
                signature.Append(normalized[index]);
            }
            return signature.ToString();
        }

        private static void MergeEncounterTasks(
            EncounterTaskTableSnapshot target,
            EncounterTaskTableSnapshot source)
        {
            if (target == null || source == null || !source.Exists)
            {
                return;
            }
            target.Exists = true;
            if (target.Columns.Count == 0)
            {
                target.Columns.AddRange(source.Columns);
            }
            foreach (string row in source.Rows)
            {
                string sourcedRow = (source.Source ?? "unknown") +
                    ":" + row;
                if (!target.Rows.Contains(sourcedRow))
                {
                    target.Rows.Add(sourcedRow);
                }
            }
            target.Rows.Sort(StringComparer.Ordinal);
        }

        private static string BuildActorFilterSql(
            IList<int> encounterTargetIds)
        {
            List<int> ids = NormalizeEncounterTargetIds(
                encounterTargetIds);
            StringBuilder filter = new StringBuilder("(");
            for (int index = 0; index < ids.Count; index++)
            {
                if (index > 0)
                {
                    filter.Append(',');
                }
                filter.Append(ids[index]);
            }
            return filter.Append(')').ToString();
        }

        private static List<int> NormalizeEncounterTargetIds(
            IList<int> encounterTargetIds)
        {
            List<int> normalized = new List<int>();
            HashSet<int> seen = new HashSet<int>();
            if (encounterTargetIds != null)
            {
                foreach (int actorCid in encounterTargetIds)
                {
                    if (actorCid <= 0 || !seen.Add(actorCid))
                    {
                        throw new InvalidDataException(
                            "World encounter filter contains an invalid or duplicate actor ID.");
                    }
                    normalized.Add(actorCid);
                }
            }
            normalized.Sort();
            return normalized;
        }

        private static Dictionary<int, BossRespawnRecord>
            QueryBossRespawns(
                IntPtr database,
                IList<int> encounterTargetIds)
        {
            Dictionary<int, BossRespawnRecord> rows =
                new Dictionary<int, BossRespawnRecord>();
            if (encounterTargetIds == null
                || encounterTargetIds.Count == 0)
            {
                return rows;
            }
            string sql =
                "SELECT ACTOR_CID,RESPAWN_TYPE,DESTROY_TIME " +
                "FROM tb_actor_respawn WHERE ACTOR_CID IN " +
                BuildActorFilterSql(encounterTargetIds) + ";";
            CallbackFailure failure = new CallbackFailure();
            NativeMethods.ExecCallback callback = delegate(
                IntPtr context,
                int count,
                IntPtr values,
                IntPtr names)
            {
                try
                {
                    if (count < 3)
                    {
                        return 0;
                    }

                    int bossId;
                    int respawnType;
                    long destroyTime;
                    if (Int32.TryParse(
                            PointerString(Marshal.ReadIntPtr(values, 0)),
                            out bossId)
                        && Int32.TryParse(
                            PointerString(Marshal.ReadIntPtr(
                                values,
                                IntPtr.Size)),
                            out respawnType)
                        && Int64.TryParse(
                            PointerString(Marshal.ReadIntPtr(
                                values,
                                IntPtr.Size * 2)),
                            out destroyTime))
                    {
                        DateTime destroyUtc;
                        try
                        {
                            destroyUtc = DateTimeOffset
                                .FromUnixTimeSeconds(destroyTime)
                                .UtcDateTime;
                        }
                        catch (ArgumentOutOfRangeException)
                        {
                            // Ignore a corrupt timestamp and continue scanning.
                            return 0;
                        }

                        BossRespawnRecord existing;
                        if (!rows.TryGetValue(bossId, out existing)
                            || destroyUtc > existing.DestroyTimeUtc)
                        {
                            rows[bossId] = new BossRespawnRecord
                            {
                                BossId = bossId,
                                RespawnType = respawnType,
                                DestroyTimeUtc = destroyUtc
                            };
                        }
                    }
                    return 0;
                }
                catch (Exception exception)
                {
                    // Never let a managed exception cross sqlite3's native
                    // callback boundary. Abort, then rethrow on managed code.
                    failure.Exception = exception;
                    return 1;
                }
            };

            Execute(database, sql, callback, failure);
            return rows;
        }

        private static Dictionary<int, ulong>
            QueryOpenedTreasureBits(IntPtr database)
        {
            Dictionary<int, ulong> opened =
                new Dictionary<int, ulong>();
            string sql =
                "SELECT CATEGORY,OPENED_BIT_FIELD " +
                "FROM tb_treasure_box;";
            CallbackFailure failure = new CallbackFailure();
            NativeMethods.ExecCallback callback = delegate(
                IntPtr context,
                int count,
                IntPtr values,
                IntPtr names)
            {
                try
                {
                    AddOpenedField(opened, count, values);
                    return 0;
                }
                catch (Exception exception)
                {
                    // Never let a managed exception cross sqlite3's native
                    // callback boundary. Abort, then rethrow on managed code.
                    failure.Exception = exception;
                    return 1;
                }
            };

            Execute(database, sql, callback, failure);
            return opened;
        }

        private static EncounterTaskTableSnapshot
            QueryEncounterTaskTable(IntPtr database)
        {
            EncounterTaskTableSnapshot result =
                new EncounterTaskTableSnapshot();
            CallbackFailure existenceFailure = new CallbackFailure();
            NativeMethods.ExecCallback existenceCallback = delegate(
                IntPtr context,
                int count,
                IntPtr values,
                IntPtr names)
            {
                try
                {
                    result.Exists = count > 0
                        && !String.IsNullOrEmpty(PointerString(
                            Marshal.ReadIntPtr(values, 0)));
                    return 0;
                }
                catch (Exception exception)
                {
                    existenceFailure.Exception = exception;
                    return 1;
                }
            };
            Execute(
                database,
                "SELECT name FROM sqlite_master WHERE type='table' " +
                    "AND name='tb_unexpected_switch_week' LIMIT 1;",
                existenceCallback,
                existenceFailure);
            if (!result.Exists)
            {
                return result;
            }

            CallbackFailure rowFailure = new CallbackFailure();
            NativeMethods.ExecCallback rowCallback = delegate(
                IntPtr context,
                int count,
                IntPtr values,
                IntPtr names)
            {
                try
                {
                    int boundedCount = Math.Min(count, 32);
                    if (result.Columns.Count == 0)
                    {
                        for (int index = 0;
                            index < boundedCount;
                            index++)
                        {
                            result.Columns.Add(SafeTaskValue(
                                PointerString(Marshal.ReadIntPtr(
                                    names,
                                    IntPtr.Size * index))));
                        }
                    }
                    if (result.Rows.Count >= 256)
                    {
                        return 0;
                    }
                    string[] fields = new string[boundedCount];
                    for (int index = 0;
                        index < boundedCount;
                        index++)
                    {
                        fields[index] = SafeTaskValue(PointerString(
                            Marshal.ReadIntPtr(
                                values,
                                IntPtr.Size * index)));
                    }
                    result.Rows.Add(String.Join("\t", fields));
                    return 0;
                }
                catch (Exception exception)
                {
                    rowFailure.Exception = exception;
                    return 1;
                }
            };
            Execute(
                database,
                "SELECT * FROM tb_unexpected_switch_week LIMIT 256;",
                rowCallback,
                rowFailure);
            result.Rows.Sort(StringComparer.Ordinal);
            return result;
        }

        private static string SafeTaskValue(string value)
        {
            if (String.IsNullOrEmpty(value))
            {
                return "<null>";
            }
            string safe = value.Replace("\r", " ")
                .Replace("\n", " ")
                .Replace("\t", " ");
            return safe.Length <= 160
                ? safe
                : safe.Substring(0, 160) + "...";
        }

        private static void ApplyKey(
            IntPtr database,
            string key)
        {
            string escapedKey = key.Replace("'", "''");
            Execute(
                database,
                "PRAGMA key = '" + escapedKey + "';" +
                    "PRAGMA cipher_compatibility = 4;",
                null,
                null);
        }

        private static void Execute(
            IntPtr database,
            string sql,
            NativeMethods.ExecCallback callback,
            CallbackFailure callbackFailure)
        {
            IntPtr error;
            int result = NativeMethods.sqlite3_exec(
                database,
                Utf8(sql),
                callback,
                IntPtr.Zero,
                out error);
            GC.KeepAlive(callback);

            Exception callbackException = callbackFailure == null
                ? null
                : callbackFailure.Exception;
            if (callbackException != null)
            {
                if (error != IntPtr.Zero)
                {
                    NativeMethods.sqlite3_free(error);
                }
                throw new InvalidOperationException(
                    "SQLCipher row callback failed.",
                    callbackException);
            }
            if (result == 0)
            {
                return;
            }

            Exception exception = SqliteError(
                database,
                result,
                error);
            if (error != IntPtr.Zero)
            {
                NativeMethods.sqlite3_free(error);
            }
            throw exception;
        }

        private static void AddOpenedField(
            IDictionary<int, ulong> opened,
            int count,
            IntPtr values)
        {
            if (count < 2)
            {
                return;
            }

            int category;
            long signedField;
            string categoryText = PointerString(
                Marshal.ReadIntPtr(values, 0));
            string fieldText = PointerString(
                Marshal.ReadIntPtr(values, IntPtr.Size));
            if (Int32.TryParse(categoryText, out category)
                && Int64.TryParse(fieldText, out signedField))
            {
                opened[category] = unchecked((ulong)signedField);
            }
        }

        private static Exception SqliteError(
            IntPtr database,
            int result,
            IntPtr error)
        {
            string message = error == IntPtr.Zero
                ? PointerString(NativeMethods.sqlite3_errmsg(database))
                : PointerString(error);
            return new InvalidOperationException(
                "SQLCipher error " + result + ": " + message);
        }


        private sealed class CallbackFailure
        {
            public Exception Exception;
        }

        private sealed class CachedDatabaseSnapshot
        {
            public string Signature;
            public string Key;
            public string ActorFilterSignature;
            public SaveSnapshot Snapshot;
        }

        private static byte[] Utf8(string value)
        {
            return Encoding.UTF8.GetBytes(value + "\0");
        }

        private static string PointerString(IntPtr pointer)
        {
            return pointer == IntPtr.Zero
                ? string.Empty
                : Marshal.PtrToStringAnsi(pointer) ?? string.Empty;
        }
    }
}
