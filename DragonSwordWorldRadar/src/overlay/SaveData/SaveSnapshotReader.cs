using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace DragonSwordWorldRadar
{
    internal sealed class SaveSnapshot
    {
        public Dictionary<int, ulong> Opened;
        public Dictionary<int, BossRespawnRecord> BossRespawns;
    }

    internal sealed class BossRespawnRecord
    {
        public int BossId;
        public int RespawnType;
        public DateTime DestroyTimeUtc;
    }

    internal static class SaveSnapshotReader
    {
        private const int SqliteOpenReadWrite = 0x00000002;

        public static SaveSnapshot Read(
            SaveSlotFingerprint slot,
            string key)
        {
            SaveSnapshot merged = new SaveSnapshot
            {
                Opened = new Dictionary<int, ulong>(),
                BossRespawns = new Dictionary<int, BossRespawnRecord>()
            };
            string temporaryDirectory = Path.Combine(
                Path.GetTempPath(),
                "DragonSwordWorldRadar",
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
                        string snapshotPath =
                            database.CopyConsistentSnapshot(
                                temporaryDirectory);
                        SaveSnapshot current = ReadFile(
                            snapshotPath,
                            key);
                        MergeOpened(merged.Opened, current.Opened);
                        MergeBossRespawns(
                            merged.BossRespawns,
                            current.BossRespawns);
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
            return merged;
        }

        private static SaveSnapshot ReadFile(
            string source,
            string key)
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
                return new SaveSnapshot
                {
                    Opened = QueryOpenedTreasureBits(database, key),
                    BossRespawns = QueryBossRespawns(database, key)
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

        private static Dictionary<int, BossRespawnRecord>
            QueryBossRespawns(IntPtr database, string key)
        {
            Dictionary<int, BossRespawnRecord> rows =
                new Dictionary<int, BossRespawnRecord>();
            string escapedKey = key.Replace("'", "''");
            string sql =
                "PRAGMA key = '" + escapedKey + "';" +
                "PRAGMA cipher_compatibility = 4;" +
                "SELECT ACTOR_CID,RESPAWN_TYPE,DESTROY_TIME " +
                "FROM tb_actor_respawn WHERE ACTOR_CID IN " +
                "(9000005,9000007,9000010,9000011,9000012," +
                "9000019,9000022,9000023,9000025," +
                "102,105,106,110,114,134,140,141,142,143,144," +
                "145,146,147,148,149,150,151,153,154,156,157," +
                "158,159,160,161,162,163,164,165,166,167,168," +
                "172,173,174,175,176,177,178);";
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
            QueryOpenedTreasureBits(IntPtr database, string key)
        {
            Dictionary<int, ulong> opened =
                new Dictionary<int, ulong>();
            string escapedKey = key.Replace("'", "''");
            string sql =
                "PRAGMA key = '" + escapedKey + "';" +
                "PRAGMA cipher_compatibility = 4;" +
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
