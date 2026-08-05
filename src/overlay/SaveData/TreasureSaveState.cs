using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace DragonSwordWorldRadar
{
    internal sealed class TreasureSaveState
    {
        private const int SqliteOpenReadWrite = 0x00000002;

        // Metadata checks are cheap and run on the UI timer; SQLCipher work is
        // queued only when the active slot or one of its WAL/journal sidecars
        // changes.
        private static readonly TimeSpan RefreshInterval =
            TimeSpan.FromMilliseconds(500);
        private static readonly TimeSpan DatabaseDiscoveryInterval =
            TimeSpan.FromSeconds(5);

        private readonly object _sync = new object();
        private readonly Dictionary<int, ulong> _opened =
            new Dictionary<int, ulong>();
        private readonly SaveDatabaseKeyReader _keyReader =
            new SaveDatabaseKeyReader();
        private readonly TreasureOverrides _overrides =
            new TreasureOverrides();
        private readonly BossRespawnRuleResolver _bossRuleResolver =
            new BossRespawnRuleResolver();
        private readonly Dictionary<int, BossRespawnRecord> _bossRespawns =
            new Dictionary<int, BossRespawnRecord>();

        private DateTime _nextRefreshUtc;
        private DateTime _nextDatabaseDiscoveryUtc;
        private string _selectedDatabasePath;
        private string _databaseCandidateSummary;
        private string _lastDatabasePath;
        private DateTime _lastDatabaseWriteUtc;
        private string _lastSlotSignature;
        private string _lastSlotSummary;
        private string _lastKey;
        private string _lastError;
        private string _lastDatabaseAttemptLog;
        private string _lastDatabaseSuccessLog;
        private int _gameProcessId;
        private int _version;
        private bool _hasLoadedSaveState;
        private bool _loadInProgress;
        private int _lastOverrideVersion = -1;
        private readonly Dictionary<int, string> _lastBossStateLog =
            new Dictionary<int, string>();

        public int GameProcessId
        {
            get { return _gameProcessId; }
        }

        public bool HasLoadedSaveState
        {
            get
            {
                lock (_sync)
                {
                    return _hasLoadedSaveState;
                }
            }
        }

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

        public string DatabaseName
        {
            get
            {
                lock (_sync)
                {
                    return _lastDatabasePath == null
                        ? "none"
                        : SafeSlotName(_lastDatabasePath);
                }
            }
        }

        public string DatabaseWriteSummary
        {
            get
            {
                lock (_sync)
                {
                    return _lastDatabaseWriteUtc ==
                        DateTime.MinValue
                        ? "none"
                        : _lastDatabaseWriteUtc.ToString(
                            "O",
                            CultureInfo.InvariantCulture);
                }
            }
        }

        public int OpenedBitCount
        {
            get
            {
                lock (_sync)
                {
                    return CountOpenedBits(_opened);
                }
            }
        }

        public string LastErrorSummary
        {
            get
            {
                lock (_sync)
                {
                    return _lastError ?? "none";
                }
            }
        }


        public bool IsBossAvailable(int bossId)
        {
            BossRespawnRecord record;
            lock (_sync)
            {
                if (!_bossRespawns.TryGetValue(bossId, out record))
                {
                    return true;
                }
            }
            return _bossRuleResolver.IsAvailable(record.DestroyTimeUtc);
        }

        public string DescribeBossState(int bossId)
        {
            BossRespawnRecord record;
            lock (_sync)
            {
                if (!_bossRespawns.TryGetValue(bossId, out record))
                {
                    return "available-no-death-record";
                }
            }
            return _bossRuleResolver.Describe(record.DestroyTimeUtc);
        }

        public bool IsOpened(long saveId)
        {
            if (saveId <= 0)
            {
                return false;
            }

            bool ignored;
            saveId = _overrides.Resolve(
                saveId,
                out ignored);
            if (ignored)
            {
                return true;
            }

            int category = (int)(saveId / 64);
            int bit = (int)(saveId % 64);
            lock (_sync)
            {
                ulong field;
                return _opened.TryGetValue(category, out field)
                    && (field & (1UL << bit)) != 0;
            }
        }

        public string Describe(long sourceSaveId)
        {
            if (sourceSaveId <= 0)
            {
                return "invalidId";
            }

            bool ignored;
            long resolvedSaveId = _overrides.Resolve(
                sourceSaveId,
                out ignored);

            int sourceCategory =
                (int)(sourceSaveId / 64);
            int sourceBit =
                (int)(sourceSaveId % 64);
            int resolvedCategory =
                (int)(resolvedSaveId / 64);
            int resolvedBit =
                (int)(resolvedSaveId % 64);

            lock (_sync)
            {
                ulong sourceField;
                bool hasSourceField = _opened.TryGetValue(
                    sourceCategory,
                    out sourceField);
                bool sourceOpened = hasSourceField
                    && (sourceField &
                        (1UL << sourceBit)) != 0;

                ulong resolvedField;
                bool hasResolvedField = _opened.TryGetValue(
                    resolvedCategory,
                    out resolvedField);
                bool resolvedOpened = ignored
                    || (hasResolvedField
                        && (resolvedField &
                            (1UL << resolvedBit)) != 0);

                return string.Format(
                    CultureInfo.InvariantCulture,
                    "source={0}; resolved={1}; ignored={2}; " +
                    "sourceCategory={3}; sourceBit={4}; " +
                    "sourceField={5}; sourceOpened={6}; " +
                    "resolvedCategory={7}; resolvedBit={8}; " +
                    "resolvedField={9}; opened={10}",
                    sourceSaveId,
                    resolvedSaveId,
                    ignored,
                    sourceCategory,
                    sourceBit,
                    hasSourceField
                        ? "0x" + sourceField.ToString("X16")
                        : "missing",
                    sourceOpened,
                    resolvedCategory,
                    resolvedBit,
                    hasResolvedField
                        ? "0x" + resolvedField.ToString("X16")
                        : "missing",
                    resolvedOpened);
            }
        }

        public void Refresh()
        {
            _overrides.Refresh();
            int overrideVersion = _overrides.Version;
            lock (_sync)
            {
                if (overrideVersion != _lastOverrideVersion)
                {
                    _lastOverrideVersion = overrideVersion;
                    _version++;
                }
            }

            if (DateTime.UtcNow < _nextRefreshUtc)
            {
                return;
            }
            _nextRefreshUtc =
                DateTime.UtcNow.Add(RefreshInterval);

            try
            {
                using (Process game =
                    GameProcessFinder.FindNewest())
                {
                    if (game == null)
                    {
                        ResetForGameProcess(0);
                        return;
                    }

                    if (game.Id != _gameProcessId)
                    {
                        ResetForGameProcess(game.Id);
                    }

                    RefreshFromGame(game);
                }
            }
            catch (Exception exception)
            {
                LogRefreshError(exception);
            }
        }

        private void RefreshFromGame(Process game)
        {
            string key = _keyReader.Read(game);
            string candidateSummary;
            string databasePath = FindNewestSaveDatabaseCached(
                game,
                out candidateSummary);
            SaveSlotFingerprint slot =
                SaveSlotFingerprint.Capture(databasePath);
            LogDatabaseSelection(
                databasePath,
                candidateSummary + "; activeSlot=" + slot.Summary);

            lock (_sync)
            {
                if (slot.Stem == _lastDatabasePath
                    && slot.Signature == _lastSlotSignature
                    && key == _lastKey)
                {
                    return;
                }
                if (_loadInProgress)
                {
                    return;
                }
                _loadInProgress = true;
            }

            SaveLoadRequest request = new SaveLoadRequest
            {
                GameProcessId = game.Id,
                Slot = slot,
                Key = key,
            };
            if (!ThreadPool.QueueUserWorkItem(
                    LoadSaveState,
                    request))
            {
                lock (_sync)
                {
                    _loadInProgress = false;
                }
                throw new InvalidOperationException(
                    "Could not queue save-state refresh.");
            }
        }

        private void LoadSaveState(object state)
        {
            SaveLoadRequest request =
                (SaveLoadRequest)state;
            try
            {
                // Rule scanning and all database I/O stay on the worker thread.
                _bossRuleResolver.Refresh();
                SaveSnapshot snapshot = ReadSaveSlotSnapshot(
                    request.Slot,
                    request.Key);
                Dictionary<int, ulong> opened = snapshot.Opened;

                List<long> newlyOpened;
                List<long> newlyClosed;
                bool openedChanged;
                bool bossChanged;
                bool firstSuccessfulLoad;
                lock (_sync)
                {
                    if (_gameProcessId != request.GameProcessId)
                    {
                        return;
                    }

                    newlyOpened = FindNewlySetIds(_opened, opened);
                    newlyClosed = FindNewlySetIds(opened, _opened);
                    openedChanged = !OpenedDictionariesEqual(
                        _opened,
                        opened);
                    bossChanged = !BossRespawnDictionariesEqual(
                        _bossRespawns,
                        snapshot.BossRespawns);
                    firstSuccessfulLoad = !_hasLoadedSaveState;

                    _opened.Clear();
                    foreach (KeyValuePair<int, ulong> pair in opened)
                    {
                        _opened[pair.Key] = pair.Value;
                    }
                    _bossRespawns.Clear();
                    foreach (KeyValuePair<int, BossRespawnRecord> pair
                        in snapshot.BossRespawns)
                    {
                        _bossRespawns[pair.Key] = pair.Value;
                    }
                    _lastDatabasePath = request.Slot.Stem;
                    _lastDatabaseWriteUtc = request.Slot.LatestWriteUtc;
                    _lastSlotSignature = request.Slot.Signature;
                    _lastSlotSummary = request.Slot.Summary;
                    _lastKey = request.Key;
                    _lastError = null;
                    _hasLoadedSaveState = true;
                    if (firstSuccessfulLoad
                        || openedChanged
                        || bossChanged)
                    {
                        _version++;
                    }
                }

                LogDatabaseLoaded(
                    request.Slot.Stem,
                    opened);
                LogDatabaseDelta(
                    request.Slot.Stem,
                    newlyOpened,
                    newlyClosed);
                LogBossStates(
                    snapshot.BossRespawns,
                    request.Slot);
            }
            catch (Exception exception)
            {
                lock (_sync)
                {
                    if (_gameProcessId == request.GameProcessId)
                    {
                        LogRefreshError(exception);
                    }
                }
            }
            finally
            {
                lock (_sync)
                {
                    _loadInProgress = false;
                }
            }
        }

        private void LogBossStates(
            IDictionary<int, BossRespawnRecord> current,
            SaveSlotFingerprint slot)
        {
            int[] bossIds =
            {
                9000005, 9000007, 9000010,
                9000011, 9000012, 9000019,
                9000022, 9000023, 9000025
            };
            foreach (int bossId in bossIds)
            {
                BossRespawnRecord record;
                string comparisonState;
                string messageState;
                if (!current.TryGetValue(bossId, out record))
                {
                    comparisonState = String.Format(
                        CultureInfo.InvariantCulture,
                        "bossId={0}; available=true; reason=no-death-record; rule={1}",
                        bossId,
                        _bossRuleResolver.RuleSummary);
                    messageState = comparisonState +
                        "; slot=" + slot.Summary;
                }
                else
                {
                    DateTime next = _bossRuleResolver.NextAvailableUtc(
                        record.DestroyTimeUtc);
                    comparisonState = String.Format(
                        CultureInfo.InvariantCulture,
                        "bossId={0}; destroyUtc={1:O}; nextUtc={2:O}; available={3}; respawnType={4}; rule={5}",
                        bossId,
                        record.DestroyTimeUtc,
                        next,
                        DateTime.UtcNow >= next,
                        record.RespawnType,
                        _bossRuleResolver.RuleSummary);
                    messageState = comparisonState +
                        "; slot=" + slot.Summary;
                }

                bool changed;
                lock (_sync)
                {
                    string old;
                    changed = !_lastBossStateLog.TryGetValue(
                        bossId,
                        out old)
                        || old != comparisonState;
                    _lastBossStateLog[bossId] = comparisonState;
                }
                if (changed)
                {
                    ErrorLog.WriteMessage(
                        "World-boss save state: " + messageState);
                }
            }
        }

        private void LogDatabaseSelection(
            string databasePath,
            string candidateSummary)
        {
            if (!DebugSettings.Enabled)
            {
                return;
            }

            string message =
                "Save-state database selection: selected=" +
                SafeSlotName(databasePath) +
                "; candidates=" + candidateSummary;
            if (message != _lastDatabaseAttemptLog)
            {
                _lastDatabaseAttemptLog = message;
                ErrorLog.WriteDebug(message);
            }
        }

        private void LogDatabaseLoaded(
            string databasePath,
            Dictionary<int, ulong> opened)
        {
            if (!DebugSettings.Enabled)
            {
                return;
            }

            string message =
                "Save-state loaded: database=" +
                SafeSlotName(databasePath) +
                "; categories=" + opened.Count +
                "; openedBits=" + CountOpenedBits(opened);
            if (message != _lastDatabaseSuccessLog)
            {
                _lastDatabaseSuccessLog = message;
                ErrorLog.WriteDebug(message);
            }
        }

        private void LogDatabaseDelta(
            string databasePath,
            IList<long> newlyOpened,
            IList<long> newlyClosed)
        {
            if (!DebugSettings.Enabled
                || (newlyOpened.Count == 0
                    && newlyClosed.Count == 0))
            {
                return;
            }

            ErrorLog.WriteDebug(
                "Save-state bit delta: database=" +
                SafeSlotName(databasePath) +
                "; newlyOpened=" +
                FormatIdList(newlyOpened) +
                "; newlyClosed=" +
                FormatIdList(newlyClosed));
        }

        private void LogRefreshError(Exception exception)
        {
            string message =
                exception.GetType().FullName + ": " +
                exception.Message;
            lock (_sync)
            {
                if (message != _lastError)
                {
                    _lastError = message;
                    ErrorLog.Write(
                        "Save-state refresh failed",
                        exception);
                }
            }
        }

        private void ResetForGameProcess(int processId)
        {
            if (_gameProcessId == processId)
            {
                return;
            }

            lock (_sync)
            {
                _gameProcessId = processId;
                _opened.Clear();
                _bossRespawns.Clear();
                _nextDatabaseDiscoveryUtc =
                    DateTime.MinValue;
                _selectedDatabasePath = null;
                _databaseCandidateSummary = null;
                _lastDatabasePath = null;
                _lastDatabaseWriteUtc = DateTime.MinValue;
                _lastSlotSignature = null;
                _lastSlotSummary = null;
                _lastKey = null;
                _lastError = null;
                _lastDatabaseAttemptLog = null;
                _lastDatabaseSuccessLog = null;
                _hasLoadedSaveState = false;
                _loadInProgress = false;
                _lastBossStateLog.Clear();
                _version++;
            }
            _keyReader.Reset();
        }

        private string FindNewestSaveDatabaseCached(
            Process game,
            out string candidateSummary)
        {
            DateTime now = DateTime.UtcNow;

            // The game alternates writes between the active slot's .db and
            // .bak files. Check only those two siblings on every refresh so a
            // newly opened chest is observed immediately without recursively
            // enumerating the complete SaveGames tree every 250 ms.
            if (_selectedDatabasePath != null)
            {
                string newestActiveSlotDatabase =
                    FindNewestActiveSlotDatabase(
                        _selectedDatabasePath);
                if (newestActiveSlotDatabase != null)
                {
                    _selectedDatabasePath =
                        newestActiveSlotDatabase;
                    if (now < _nextDatabaseDiscoveryUtc)
                    {
                        candidateSummary =
                            _databaseCandidateSummary;
                        return _selectedDatabasePath;
                    }
                }
            }

            _selectedDatabasePath =
                FindNewestSaveDatabase(
                    game,
                    out _databaseCandidateSummary);
            _nextDatabaseDiscoveryUtc =
                now.Add(DatabaseDiscoveryInterval);
            candidateSummary = _databaseCandidateSummary;
            return _selectedDatabasePath;
        }

        private static string FindNewestActiveSlotDatabase(
            string selectedDatabasePath)
        {
            string directory =
                Path.GetDirectoryName(selectedDatabasePath);
            string stem =
                Path.GetFileNameWithoutExtension(
                    selectedDatabasePath);
            if (String.IsNullOrEmpty(directory)
                || String.IsNullOrEmpty(stem))
            {
                return null;
            }

            string database = Path.Combine(
                directory,
                stem + ".db");
            string backup = Path.Combine(
                directory,
                stem + ".bak");

            return new[] { database, backup }
                .Where(File.Exists)
                .OrderByDescending(
                    File.GetLastWriteTimeUtc)
                .FirstOrDefault();
        }

        private static string FindNewestSaveDatabase(
            Process game,
            out string candidateSummary)
        {
            string win64 =
                Path.GetDirectoryName(game.MainModule.FileName);
            string saveRoot = Path.GetFullPath(Path.Combine(
                win64,
                "..",
                "..",
                "Saved",
                "SaveGames"));
            List<string> ordered = Directory.GetFiles(
                    saveRoot,
                    "*_Slot*.bak",
                    SearchOption.AllDirectories)
                .Concat(Directory.GetFiles(
                    saveRoot,
                    "*_Slot*.db",
                    SearchOption.AllDirectories))
                .OrderByDescending(
                    File.GetLastWriteTimeUtc)
                .ToList();

            candidateSummary = BuildCandidateSummary(
                ordered,
                win64,
                saveRoot);
            if (ordered.Count == 0)
            {
                throw new FileNotFoundException(
                    "No slot database was found.",
                    saveRoot);
            }

            IGrouping<string, string> activeSlot = ordered
                .GroupBy(
                    path => Path.Combine(
                        Path.GetDirectoryName(path),
                        Path.GetFileNameWithoutExtension(path)),
                    StringComparer.OrdinalIgnoreCase)
                .OrderByDescending(group =>
                    group.Max(path =>
                        File.GetLastWriteTimeUtc(path)))
                .First();

            // The game may write the live treasure state to either
            // the .db or the .bak file. Always use the newest file in
            // the active slot instead of forcing .db, otherwise newly
            // opened chests can remain visible indefinitely.
            return activeSlot
                .OrderByDescending(
                    path => File.GetLastWriteTimeUtc(path))
                .First();
        }

        private static string BuildCandidateSummary(
            IEnumerable<string> candidates,
            string win64,
            string saveRoot)
        {
            string databases = string.Join(
                ",",
                candidates.Select(path =>
                    SafeSlotName(path) + "@" +
                    File.GetLastWriteTimeUtc(path).ToString(
                        "O",
                        CultureInfo.InvariantCulture))
                .ToArray());
            return databases + "; " +
                BuildSlotHints(win64, saveRoot);
        }

        private static string BuildSlotHints(
            string win64,
            string saveRoot)
        {
            string spackSummary = string.Join(
                ",",
                Directory.GetFiles(
                    saveRoot,
                    "SPack_Slot*.sav",
                    SearchOption.AllDirectories)
                .OrderByDescending(
                    File.GetLastWriteTimeUtc)
                .Select(path =>
                    SafeSlotName(path) + "@" +
                    File.GetLastWriteTimeUtc(path).ToString(
                        "O",
                        CultureInfo.InvariantCulture))
                .ToArray());

            string configPath = Path.GetFullPath(Path.Combine(
                win64,
                "..",
                "..",
                "Saved",
                "Config",
                "Windows",
                "Game.ini"));
            string configSummary =
                ReadConfigSlotSummary(configPath);

            return "spack=" +
                (spackSummary.Length == 0
                    ? "none"
                    : spackSummary) +
                "; configSections=" + configSummary;
        }

        private static string ReadConfigSlotSummary(
            string configPath)
        {
            if (!File.Exists(configPath))
            {
                return "none";
            }

            string[] slots = File.ReadLines(configPath)
                .Select(SafeSlotToken)
                .Where(slot => slot != null)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray();
            return slots.Length == 0
                ? "none"
                : string.Join(",", slots);
        }

        private static string SafeSlotName(string path)
        {
            string filename = Path.GetFileName(path);
            string token = SafeSlotToken(filename);
            return token == null
                ? filename
                : token + Path.GetExtension(filename);
        }

        private static string SafeSlotToken(string value)
        {
            int slot = value.IndexOf(
                "_Slot",
                StringComparison.OrdinalIgnoreCase);
            if (slot < 0)
            {
                return null;
            }

            int numberStart = slot + 5;
            int end = numberStart;
            if (end < value.Length && value[end] == '-')
            {
                end++;
            }
            while (end < value.Length
                && char.IsDigit(value[end]))
            {
                end++;
            }

            return end > numberStart
                ? "Slot" + value.Substring(
                    numberStart,
                    end - numberStart)
                : null;
        }

        private static List<long> FindNewlySetIds(
            IDictionary<int, ulong> previous,
            IDictionary<int, ulong> current)
        {
            HashSet<int> categories =
                new HashSet<int>(previous.Keys);
            categories.UnionWith(current.Keys);

            List<long> result = new List<long>();
            foreach (int category in categories)
            {
                ulong previousField;
                if (!previous.TryGetValue(
                    category,
                    out previousField))
                {
                    previousField = 0;
                }

                ulong currentField;
                if (!current.TryGetValue(
                    category,
                    out currentField))
                {
                    currentField = 0;
                }

                ulong newlySet =
                    currentField & ~previousField;
                for (int bit = 0; bit < 64; bit++)
                {
                    if ((newlySet &
                        (1UL << bit)) != 0)
                    {
                        result.Add(
                            category * 64L + bit);
                    }
                }
            }

            result.Sort();
            return result;
        }

        private static string FormatIdList(
            IList<long> ids)
        {
            if (ids == null || ids.Count == 0)
            {
                return "none";
            }

            const int limit = 40;
            string[] values = ids
                .Take(limit)
                .Select(id => id.ToString(
                    CultureInfo.InvariantCulture))
                .ToArray();
            string result = string.Join(",", values);
            return ids.Count > limit
                ? result + ",...(" +
                    ids.Count.ToString(
                        CultureInfo.InvariantCulture) +
                    " total)"
                : result;
        }

        private static int CountOpenedBits(
            IDictionary<int, ulong> opened)
        {
            int count = 0;
            foreach (ulong field in opened.Values)
            {
                ulong remaining = field;
                while (remaining != 0)
                {
                    remaining &= remaining - 1;
                    count++;
                }
            }
            return count;
        }

        private static SaveSnapshot ReadSaveSlotSnapshot(
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
                        SaveSnapshot current =
                            ReadSaveSnapshotFile(
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
                    Directory.Delete(
                        temporaryDirectory,
                        true);
                }
                catch
                {
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

        private static SaveSnapshot ReadSaveSnapshotFile(
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
                    Opened = QueryOpenedTreasureBits(
                        database,
                        key),
                    BossRespawns = QueryBossRespawns(
                        database,
                        key),
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

        private static bool OpenedDictionariesEqual(
            IDictionary<int, ulong> left,
            IDictionary<int, ulong> right)
        {
            if (left.Count != right.Count)
            {
                return false;
            }
            foreach (KeyValuePair<int, ulong> pair in left)
            {
                ulong value;
                if (!right.TryGetValue(pair.Key, out value)
                    || value != pair.Value)
                {
                    return false;
                }
            }
            return true;
        }

        private static bool BossRespawnDictionariesEqual(
            IDictionary<int, BossRespawnRecord> left,
            IDictionary<int, BossRespawnRecord> right)
        {
            if (left.Count != right.Count)
            {
                return false;
            }
            foreach (KeyValuePair<int, BossRespawnRecord> pair in left)
            {
                BossRespawnRecord value;
                if (!right.TryGetValue(pair.Key, out value)
                    || value.RespawnType != pair.Value.RespawnType
                    || value.DestroyTimeUtc != pair.Value.DestroyTimeUtc)
                {
                    return false;
                }
            }
            return true;
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

        private static Dictionary<int, BossRespawnRecord> QueryBossRespawns(
            IntPtr database, string key)
        {
            Dictionary<int, BossRespawnRecord> resultRows =
                new Dictionary<int, BossRespawnRecord>();
            string escapedKey = key.Replace("'", "''");
            string sql =
                "PRAGMA key = '" + escapedKey + "';" +
                "PRAGMA cipher_compatibility = 4;" +
                "SELECT ACTOR_CID,RESPAWN_TYPE,DESTROY_TIME " +
                "FROM tb_actor_respawn WHERE ACTOR_CID IN " +
                "(9000005,9000007,9000010,9000011,9000012," +
                "9000019,9000022,9000023,9000025);";
            NativeMethods.ExecCallback callback = delegate(
                IntPtr context, int count, IntPtr values, IntPtr names)
            {
                if (count >= 3)
                {
                    int cid; int respawnType; long destroyTime;
                    if (int.TryParse(PointerString(Marshal.ReadIntPtr(values, 0)), out cid)
                        && int.TryParse(PointerString(Marshal.ReadIntPtr(values, IntPtr.Size)), out respawnType)
                        && long.TryParse(PointerString(Marshal.ReadIntPtr(values, IntPtr.Size * 2)), out destroyTime))
                    {
                        DateTime destroyUtc = DateTimeOffset
                            .FromUnixTimeSeconds(destroyTime)
                            .UtcDateTime;
                        BossRespawnRecord existing;
                        if (!resultRows.TryGetValue(cid, out existing)
                            || destroyUtc > existing.DestroyTimeUtc)
                        {
                            resultRows[cid] = new BossRespawnRecord
                            {
                                BossId = cid,
                                RespawnType = respawnType,
                                DestroyTimeUtc = destroyUtc,
                            };
                        }
                    }
                }
                return 0;
            };
            IntPtr error;
            int execResult = NativeMethods.sqlite3_exec(
                database, Utf8(sql), callback, IntPtr.Zero, out error);
            GC.KeepAlive(callback);
            if (execResult != 0)
            {
                Exception exception = SqliteError(database, execResult, error);
                if (error != IntPtr.Zero) NativeMethods.sqlite3_free(error);
                throw exception;
            }
            return resultRows;
        }

        private static Dictionary<int, ulong>
            QueryOpenedTreasureBits(
                IntPtr database,
                string key)
        {
            Dictionary<int, ulong> opened =
                new Dictionary<int, ulong>();
            string escapedKey = key.Replace("'", "''");
            string sql =
                "PRAGMA key = '" + escapedKey + "';" +
                "PRAGMA cipher_compatibility = 4;" +
                "SELECT CATEGORY,OPENED_BIT_FIELD " +
                "FROM tb_treasure_box;";
            NativeMethods.ExecCallback callback = delegate(
                IntPtr context,
                int count,
                IntPtr values,
                IntPtr names)
            {
                AddOpenedField(opened, count, values);
                return 0;
            };

            IntPtr error;
            int result = NativeMethods.sqlite3_exec(
                database,
                Utf8(sql),
                callback,
                IntPtr.Zero,
                out error);
            GC.KeepAlive(callback);
            if (result != 0)
            {
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
            return opened;
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
                Marshal.ReadIntPtr(
                    values,
                    IntPtr.Size));
            if (int.TryParse(categoryText, out category)
                && long.TryParse(fieldText, out signedField))
            {
                opened[category] =
                    unchecked((ulong)signedField);
            }
        }

        private static Exception SqliteError(
            IntPtr database,
            int result,
            IntPtr error)
        {
            string message = error == IntPtr.Zero
                ? PointerString(
                    NativeMethods.sqlite3_errmsg(database))
                : PointerString(error);
            return new InvalidOperationException(
                "SQLCipher error " + result + ": " + message);
        }

        private static byte[] Utf8(string value)
        {
            return Encoding.UTF8.GetBytes(value + "\0");
        }

        private static string PointerString(IntPtr pointer)
        {
            return pointer == IntPtr.Zero
                ? string.Empty
                : Marshal.PtrToStringAnsi(pointer) ??
                    string.Empty;
        }

        private sealed class SaveSnapshot
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

        private sealed class SaveLoadRequest
        {
            public int GameProcessId;
            public SaveSlotFingerprint Slot;
            public string Key;
        }

    }
}
