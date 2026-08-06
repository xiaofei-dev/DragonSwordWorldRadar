using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Linq;
using System.Threading;

namespace DragonSwordWorldRadar
{
    internal sealed class TreasureSaveState
    {
        // Metadata checks are cheap and run on the UI timer; SQLCipher work is
        // queued only when the active slot or one of its WAL/journal sidecars
        // changes.
        private static readonly TimeSpan RefreshInterval =
            TimeSpan.FromMilliseconds(500);
        private static readonly int[] WorldBossIds =
        {
            9000005, 9000007, 9000010,
            9000011, 9000012, 9000019,
            9000022, 9000023, 9000025
        };
        private readonly object _sync = new object();
        private volatile Dictionary<int, ulong> _opened =
            new Dictionary<int, ulong>();
        private readonly SaveDatabaseKeyReader _keyReader =
            new SaveDatabaseKeyReader();
        private readonly TreasureOverrides _overrides =
            new TreasureOverrides();
        private readonly BossRespawnRuleResolver _bossRuleResolver =
            new BossRespawnRuleResolver();
        private readonly SaveDatabaseLocator _databaseLocator =
            new SaveDatabaseLocator();
        private volatile Dictionary<int, BossRespawnRecord> _bossRespawns =
            new Dictionary<int, BossRespawnRecord>();

        private DateTime _nextRefreshUtc;
        private string _lastDatabasePath;
        private DateTime _lastDatabaseWriteUtc;
        private string _lastSlotSignature;
        private string _lastKey;
        private string _lastError;
        private string _lastDatabaseAttemptLog;
        private string _lastDatabaseSuccessLog;
        private int _gameProcessId;
        private int _version;
        private int _openedBitCount;
        private bool _hasLoadedSaveState;
        private bool _loadInProgress;
        private int _loadToken;
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
                        : SaveDatabaseLocator.SafeSlotName(
                            _lastDatabasePath);
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
                    return _openedBitCount;
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


        public DateTime GetBossNextAvailableUtc(int bossId)
        {
            BossRespawnRecord record;
            Dictionary<int, BossRespawnRecord> bossRespawns =
                _bossRespawns;
            if (!bossRespawns.TryGetValue(bossId, out record))
            {
                return DateTime.MinValue;
            }
            return _bossRuleResolver.NextAvailableUtc(
                record.DestroyTimeUtc);
        }

        public bool IsBossAvailable(int bossId)
        {
            DateTime nextAvailableUtc =
                GetBossNextAvailableUtc(bossId);
            return nextAvailableUtc == DateTime.MinValue
                || DateTime.UtcNow >= nextAvailableUtc;
        }

        public string DescribeBossState(int bossId)
        {
            BossRespawnRecord record;
            Dictionary<int, BossRespawnRecord> bossRespawns =
                _bossRespawns;
            if (!bossRespawns.TryGetValue(bossId, out record))
            {
                return "available-no-death-record";
            }
            return _bossRuleResolver.Describe(record.DestroyTimeUtc);
        }

        public bool IsOpened(long saveId)
        {
            return IsOpened(saveId, _opened);
        }

        internal void CaptureOpenedState(
            out int version,
            out bool hasLoadedSaveState,
            out Dictionary<int, ulong> opened)
        {
            lock (_sync)
            {
                version = _version;
                hasLoadedSaveState = _hasLoadedSaveState;
                opened = _opened;
            }
        }

        internal bool IsOpened(
            long saveId,
            IDictionary<int, ulong> opened)
        {
            if (saveId <= 0 || opened == null)
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
            ulong field;
            return opened.TryGetValue(category, out field)
                && (field & (1UL << bit)) != 0;
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

            Dictionary<int, ulong> opened = _opened;
            ulong sourceField;
            bool hasSourceField = opened.TryGetValue(
                sourceCategory,
                out sourceField);
            bool sourceOpened = hasSourceField
                && (sourceField &
                    (1UL << sourceBit)) != 0;

            ulong resolvedField;
            bool hasResolvedField = opened.TryGetValue(
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
                    GameProcessFinder.OpenTracked(_gameProcessId))
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
            string databasePath = _databaseLocator.FindNewest(
                game,
                out candidateSummary);
            SaveSlotFingerprint slot =
                SaveSlotFingerprint.Capture(databasePath);
            LogDatabaseSelection(
                databasePath,
                candidateSummary + "; activeSlot=" + slot.Summary);

            int loadToken;
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
                loadToken = ++_loadToken;
            }

            SaveLoadRequest request = new SaveLoadRequest
            {
                GameProcessId = game.Id,
                LoadToken = loadToken,
                Slot = slot,
                Key = key
            };
            if (!ThreadPool.QueueUserWorkItem(
                    LoadSaveState,
                    request))
            {
                lock (_sync)
                {
                    if (_loadToken == request.LoadToken)
                    {
                        _loadInProgress = false;
                    }
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
                SaveSnapshot snapshot = SaveSnapshotReader.Read(
                    request.Slot,
                    request.Key);
                Dictionary<int, ulong> opened = snapshot.Opened;

                IList<long> newlyOpened;
                IList<long> newlyClosed;
                bool openedChanged;
                bool bossChanged;
                bool firstSuccessfulLoad;
                lock (_sync)
                {
                    if (_gameProcessId != request.GameProcessId
                        || _loadToken != request.LoadToken)
                    {
                        return;
                    }

                    Dictionary<int, ulong> previousOpened = _opened;
                    Dictionary<int, BossRespawnRecord> previousBossRespawns =
                        _bossRespawns;
                    openedChanged = !OpenedDictionariesEqual(
                        previousOpened,
                        opened);
                    if (openedChanged)
                    {
                        newlyOpened = FindNewlySetIds(
                            previousOpened,
                            opened);
                        newlyClosed = FindNewlySetIds(
                            opened,
                            previousOpened);
                    }
                    else
                    {
                        newlyOpened = Array.Empty<long>();
                        newlyClosed = Array.Empty<long>();
                    }
                    bossChanged = !BossRespawnDictionariesEqual(
                        previousBossRespawns,
                        snapshot.BossRespawns);
                    firstSuccessfulLoad = !_hasLoadedSaveState;

                    // Publish immutable snapshots atomically. Paint-time reads
                    // never observe a dictionary while it is being mutated.
                    _opened = opened;
                    _openedBitCount = CountOpenedBits(opened);
                    _bossRespawns = snapshot.BossRespawns;
                    _lastDatabasePath = request.Slot.Stem;
                    _lastDatabaseWriteUtc = request.Slot.LatestWriteUtc;
                    _lastSlotSignature = request.Slot.Signature;
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
                if (firstSuccessfulLoad || bossChanged)
                {
                    LogBossStates(
                        snapshot.BossRespawns,
                        request.Slot);
                }
            }
            catch (Exception exception)
            {
                lock (_sync)
                {
                    if (_gameProcessId == request.GameProcessId
                        && _loadToken == request.LoadToken)
                    {
                        LogRefreshError(exception);
                    }
                }
            }
            finally
            {
                lock (_sync)
                {
                    if (_loadToken == request.LoadToken)
                    {
                        _loadInProgress = false;
                    }
                }
            }
        }

        private void LogBossStates(
            IDictionary<int, BossRespawnRecord> current,
            SaveSlotFingerprint slot)
        {
            foreach (int bossId in WorldBossIds)
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
                SaveDatabaseLocator.SafeSlotName(databasePath) +
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
                SaveDatabaseLocator.SafeSlotName(databasePath) +
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
                SaveDatabaseLocator.SafeSlotName(databasePath) +
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
                _opened = new Dictionary<int, ulong>();
                _openedBitCount = 0;
                _bossRespawns =
                    new Dictionary<int, BossRespawnRecord>();
                _lastDatabasePath = null;
                _lastDatabaseWriteUtc = DateTime.MinValue;
                _lastSlotSignature = null;
                _lastKey = null;
                _lastError = null;
                _lastDatabaseAttemptLog = null;
                _lastDatabaseSuccessLog = null;
                _hasLoadedSaveState = false;
                _loadToken++;
                _loadInProgress = false;
                _lastBossStateLog.Clear();
                _version++;
            }
            _keyReader.Reset();
            _databaseLocator.Reset();
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

        private sealed class SaveLoadRequest
        {
            public int GameProcessId;
            public int LoadToken;
            public SaveSlotFingerprint Slot;
            public string Key;
        }

    }
}
