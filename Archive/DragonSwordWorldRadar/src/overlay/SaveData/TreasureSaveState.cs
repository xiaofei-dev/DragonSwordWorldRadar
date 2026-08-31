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
        // One non-harmonic interval owns both change detection and refresh.
        // Unchanged fingerprints never queue copy/decrypt work.
        private static readonly TimeSpan RefreshInterval =
            TimeSpan.FromSeconds(19);
        // A slow SQLCipher snapshot must not make the expired change-check
        // deadline queue an immediate catch-up snapshot. The cooldown starts
        // when the worker finishes, including a failed read.
        private static readonly TimeSpan SnapshotCompletionCooldown =
            TimeSpan.FromSeconds(19);
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
        private readonly SaveSnapshotReader _snapshotReader =
            new SaveSnapshotReader();
        private volatile Dictionary<int, BossRespawnRecord> _bossRespawns =
            new Dictionary<int, BossRespawnRecord>();
        private volatile int[] _encounterTargetIds = Array.Empty<int>();

        private DateTime _nextRefreshUtc;
        private DateTime _nextSnapshotEligibleUtc;
        private string _lastDatabasePath;
        private DateTime _lastDatabaseWriteUtc;
        private string _lastSlotSignature;
        private string _lastKey;
        private string _pendingDatabasePath;
        private string _pendingSlotSignature;
        private string _pendingKey;
        private string _lastError;
        private string _lastDatabaseAttemptLog;
        private string _lastDatabaseSuccessLog;
        private int _gameProcessId;
        private int _version;
        private int _openedBitCount;
        private bool _hasLoadedSaveState;
        private bool _loadInProgress;
        private volatile bool _runtimeEnabled;
        private int _loadToken;
        private string _lastEncounterStateLog;
        private string _encounterTaskSignature;

        public TreasureSaveState()
        {
            _overrides.LoadOnce();
        }

        public bool ConfigureEncounterTargets(
            IList<WorldEncounter> points)
        {
            if (points == null || points.Count != 49)
            {
                throw new InvalidOperationException(
                    "The install-bound encounter filter requires exactly 49 catalog records.");
            }
            HashSet<int> seen = new HashSet<int>();
            int[] configured = new int[points.Count];
            for (int index = 0; index < points.Count; index++)
            {
                WorldEncounter point = points[index];
                if (point == null
                    || point.Id <= 0
                    || !seen.Add(point.Id))
                {
                    throw new InvalidOperationException(
                        "The encounter filter contains an invalid or duplicate actor ID.");
                }
                configured[index] = point.Id;
            }
            Array.Sort(configured);
            bool changed;
            lock (_sync)
            {
                if (_runtimeEnabled
                    || _gameProcessId != 0
                    || _hasLoadedSaveState
                    || _loadInProgress)
                {
                    throw new InvalidOperationException(
                        "Encounter targets may only be configured during Overlay startup.");
                }
                changed = !_encounterTargetIds.SequenceEqual(configured);
                if (!changed)
                {
                    return false;
                }
                _encounterTargetIds = configured;
            }
            ErrorLog.WriteDebug(String.Format(
                CultureInfo.InvariantCulture,
                "WORLD_ENCOUNTER_SAVE_FILTER_CONFIGURED targets={0}; firstId={1}; lastId={2}; source=install_generated_catalogs; lifecycle=overlay_startup_once; cacheReset=false",
                configured.Length,
                configured[0],
                configured[configured.Length - 1]));
            return true;
        }

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


        public DateTime GetEncounterNextAvailableUtc(int encounterId)
        {
            BossRespawnRecord record;
            Dictionary<int, BossRespawnRecord> bossRespawns =
                _bossRespawns;
            if (!bossRespawns.TryGetValue(encounterId, out record))
            {
                return DateTime.MinValue;
            }
            return _bossRuleResolver.NextAvailableUtc(
                record.DestroyTimeUtc);
        }

        public bool IsBossAvailable(int bossId)
        {
            DateTime nextAvailableUtc =
                GetEncounterNextAvailableUtc(bossId);
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

        internal static bool IsRawOpened(
            long saveId,
            IDictionary<int, ulong> opened)
        {
            if (saveId <= 0 || opened == null)
            {
                return false;
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

        public void SetRuntimeEnabled(bool enabled)
        {
            if (_runtimeEnabled == enabled)
            {
                return;
            }
            lock (_sync)
            {
                if (_runtimeEnabled == enabled)
                {
                    return;
                }
                _runtimeEnabled = enabled;
                _nextRefreshUtc = DateTime.MinValue;
                if (!enabled)
                {
                    // Invalidate an already queued result so F8 cannot publish
                    // save work after the master gate has closed.
                    _loadToken++;
                    _loadInProgress = false;
                    ClearPendingChange();
                }
            }
        }

        public void Refresh()
        {
            if (!_runtimeEnabled)
            {
                return;
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
            bool includeTreasure;
            lock (_sync)
            {
                if (!_runtimeEnabled)
                {
                    return;
                }
                if (_hasLoadedSaveState
                    && slot.Stem == _lastDatabasePath
                    && slot.Signature == _lastSlotSignature
                    && key == _lastKey)
                {
                    ClearPendingChange();
                    return;
                }

                RememberPendingChange(slot, key);
                if (_loadInProgress)
                {
                    return;
                }
                if (DateTime.UtcNow < _nextSnapshotEligibleUtc)
                {
                    // A changed fingerprint was observed during cooldown.
                    // Revalidate it as soon as the completion cooldown ends
                    // instead of adding another full check interval.
                    _nextRefreshUtc = _nextSnapshotEligibleUtc;
                    return;
                }

                _loadInProgress = true;
                loadToken = ++_loadToken;
                includeTreasure = true;
            }

            SaveLoadRequest request = new SaveLoadRequest
            {
                GameProcessId = game.Id,
                LoadToken = loadToken,
                Slot = slot,
                Key = key,
                EncounterTargetIds = _encounterTargetIds,
                IncludeTreasure = includeTreasure
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
                        _nextSnapshotEligibleUtc =
                            DateTime.UtcNow.Add(
                                SnapshotCompletionCooldown);
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
            Thread worker = Thread.CurrentThread;
            ThreadPriority originalPriority = worker.Priority;
            bool backgroundMode = false;
            try
            {
                worker.Priority = ThreadPriority.BelowNormal;
            }
            catch
            {
                // Thread-pool priority changes are best effort only.
            }
            try
            {
                backgroundMode = NativeMethods.SetThreadPriority(
                    NativeMethods.GetCurrentThread(),
                    NativeMethods.ThreadModeBackgroundBegin);
            }
            catch
            {
                // Native background I/O scheduling is also best effort.
            }
            try
            {
                lock (_sync)
                {
                    if (!_runtimeEnabled
                        || _gameProcessId != request.GameProcessId
                        || _loadToken != request.LoadToken)
                    {
                        return;
                    }
                }

                SaveSnapshotReadMetrics metrics;
                SaveSnapshot snapshot = _snapshotReader.Read(
                    request.Slot,
                    request.Key,
                    request.EncounterTargetIds,
                    request.IncludeTreasure,
                    out metrics);
                bool treasureRefreshAvailable = request.IncludeTreasure
                    && snapshot.OpenedAvailable;
                Dictionary<int, ulong> opened;

                IList<long> newlyOpened;
                IList<long> newlyClosed;
                bool openedChanged;
                bool bossChanged;
                bool encounterTasksChanged;
                bool firstSuccessfulLoad;
                bool recoveredFromStartupPending;
                lock (_sync)
                {
                    if (_gameProcessId != request.GameProcessId
                        || _loadToken != request.LoadToken)
                    {
                        return;
                    }

                    Dictionary<int, ulong> previousOpened = _opened;
                    opened = treasureRefreshAvailable
                        ? snapshot.Opened
                        : previousOpened;
                    Dictionary<int, BossRespawnRecord> previousBossRespawns =
                        _bossRespawns;
                    openedChanged = !OpenedDictionariesEqual(
                        previousOpened,
                        opened);
                    if (DebugSettings.Enabled && openedChanged)
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
                    encounterTasksChanged = false;
                    if (DebugSettings.Enabled)
                    {
                        string taskSignature =
                            snapshot.EncounterTasks == null
                                ? "missing"
                                : snapshot.EncounterTasks.Signature;
                        encounterTasksChanged = !String.Equals(
                            _encounterTaskSignature,
                            taskSignature,
                            StringComparison.Ordinal);
                        _encounterTaskSignature = taskSignature;
                    }
                    firstSuccessfulLoad = !_hasLoadedSaveState;
                    recoveredFromStartupPending =
                        firstSuccessfulLoad && _lastError != null;

                    // Publish immutable snapshots atomically. Paint-time reads
                    // never observe a dictionary while it is being mutated.
                    _opened = opened;
                    _openedBitCount = CountOpenedBits(opened);
                    _bossRespawns = snapshot.BossRespawns;
                    _lastDatabasePath = request.Slot.Stem;
                    _lastDatabaseWriteUtc = request.Slot.LatestWriteUtc;
                    _lastSlotSignature = request.Slot.Signature;
                    _lastKey = request.Key;
                    if (PendingChangeMatches(request))
                    {
                        ClearPendingChange();
                    }
                    _lastError = null;
                    _hasLoadedSaveState = true;
                    if (firstSuccessfulLoad
                        || openedChanged
                        || bossChanged
                        || encounterTasksChanged)
                    {
                        _version++;
                    }
                }

                LogDatabaseLoaded(
                    request.Slot.Stem,
                    opened);
                if (DebugSettings.Enabled)
                {
                    ErrorLog.WriteDebug(String.Format(
                        CultureInfo.InvariantCulture,
                        "SAVE_REFRESH_PERF totalMs={0:F3}; copyMs={1:F3}; keyMs={2:F3}; treasureQueryMs={3:F3}; bossQueryMs={4:F3}; encounterTaskQueryMs={5:F3}; databaseReads={6}; databaseCacheHits={7}; treasureRequested={8}; treasureQueries={9}; changeCheckIntervalMs={10:F0}; completionCooldownMs={11:F0}",
                        metrics.TotalMilliseconds,
                        metrics.CopyMilliseconds,
                        metrics.KeyMilliseconds,
                        metrics.TreasureQueryMilliseconds,
                        metrics.BossQueryMilliseconds,
                        metrics.EncounterTaskQueryMilliseconds,
                        metrics.DatabaseReads,
                        metrics.DatabaseCacheHits,
                        metrics.TreasureRequested ? 1 : 0,
                        metrics.TreasureQueries,
                        RefreshInterval.TotalMilliseconds,
                        SnapshotCompletionCooldown.TotalMilliseconds));
                }
                if (firstSuccessfulLoad)
                {
                    ErrorLog.WriteMessage(
                        recoveredFromStartupPending
                            ? "Save-state snapshot ready after startup wait; treasure visibility enabled."
                            : "Save-state snapshot ready; treasure visibility enabled.");
                }
                LogDatabaseDelta(
                    request.Slot.Stem,
                    newlyOpened,
                    newlyClosed);
                if (firstSuccessfulLoad || bossChanged)
                {
                    LogEncounterStates(
                        snapshot.BossRespawns,
                        request.Slot);
                }
                if (firstSuccessfulLoad || encounterTasksChanged)
                {
                    LogEncounterTaskTable(snapshot.EncounterTasks);
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
                        _nextSnapshotEligibleUtc =
                            DateTime.UtcNow.Add(
                                SnapshotCompletionCooldown);
                    }
                }
                if (backgroundMode)
                {
                    try
                    {
                        NativeMethods.SetThreadPriority(
                            NativeMethods.GetCurrentThread(),
                            NativeMethods.ThreadModeBackgroundEnd);
                    }
                    catch
                    {
                    }
                }
                try
                {
                    worker.Priority = originalPriority;
                }
                catch
                {
                }
            }
        }

        private void LogEncounterStates(
            IDictionary<int, BossRespawnRecord> current,
            SaveSlotFingerprint slot)
        {
            if (!DebugSettings.Enabled)
            {
                return;
            }

            int hidden = 0;
            DateTime now = DateTime.UtcNow;
            List<string> details = new List<string>();
            foreach (BossRespawnRecord record in current.Values)
            {
                DateTime nextAvailableUtc =
                    _bossRuleResolver.NextAvailableUtc(
                        record.DestroyTimeUtc);
                bool isHidden = now < nextAvailableUtc;
                if (isHidden)
                {
                    hidden++;
                }
                details.Add(String.Format(
                    CultureInfo.InvariantCulture,
                    "id={0},respawnType={1},destroyUtc={2:O},nextUtc={3:O},hidden={4}",
                    record.BossId,
                    record.RespawnType,
                    record.DestroyTimeUtc,
                    nextAvailableUtc,
                    isHidden));
            }
            details.Sort(StringComparer.Ordinal);
            string message = String.Format(
                CultureInfo.InvariantCulture,
                "WORLD_ENCOUNTER_SAVE_STATE targets={0}; deathRecords={1}; respawnHidden={2}; rule={3}; records={4}; slot={5}",
                _encounterTargetIds.Length,
                current.Count,
                hidden,
                _bossRuleResolver.RuleSummary,
                details.Count == 0
                    ? "none"
                    : String.Join("|", details.ToArray()),
                slot.Summary);
            if (!String.Equals(
                message,
                _lastEncounterStateLog,
                StringComparison.Ordinal))
            {
                _lastEncounterStateLog = message;
                ErrorLog.WriteDebug(message);
            }
        }

        private static void LogEncounterTaskTable(
            EncounterTaskTableSnapshot snapshot)
        {
            if (!DebugSettings.Enabled)
            {
                return;
            }
            if (snapshot == null || !snapshot.Exists)
            {
                ErrorLog.WriteDebug(
                    "ENCOUNTER_TASK_TABLE table=tb_unexpected_switch_week; exists=false");
                return;
            }
            ErrorLog.WriteDebug(String.Format(
                CultureInfo.InvariantCulture,
                "ENCOUNTER_TASK_TABLE table=tb_unexpected_switch_week; exists=true; columns={0}; rowCount={1}; rows={2}",
                snapshot.Columns.Count == 0
                    ? "none"
                    : String.Join(",", snapshot.Columns.ToArray()),
                snapshot.Rows.Count,
                snapshot.Rows.Count == 0
                    ? "none"
                    : String.Join("|", snapshot.Rows.ToArray())));
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
                    if (!_hasLoadedSaveState
                        && IsStartupInitializationPending(exception))
                    {
                        ErrorLog.WriteDebug(
                            "Save-state initialization pending: " +
                            exception.Message);
                    }
                    else
                    {
                        ErrorLog.Write(
                            "Save-state refresh failed",
                            exception);
                    }
                }
            }
        }

        private static bool IsStartupInitializationPending(
            Exception exception)
        {
            for (Exception current = exception;
                current != null;
                current = current.InnerException)
            {
                if (current.Message ==
                        "Save database owner is not ready."
                    || current.Message ==
                        "Save database key is not ready.")
                {
                    return true;
                }
            }
            return false;
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
                ClearPendingChange();
                _nextSnapshotEligibleUtc = DateTime.MinValue;
                _lastError = null;
                _lastDatabaseAttemptLog = null;
                _lastDatabaseSuccessLog = null;
                _hasLoadedSaveState = false;
                _loadToken++;
                _loadInProgress = false;
                _lastEncounterStateLog = null;
                _encounterTaskSignature = null;
                _version++;
            }
            _keyReader.Reset();
            _databaseLocator.Reset();
            _snapshotReader.Reset();
        }

        private void RememberPendingChange(
            SaveSlotFingerprint slot,
            string key)
        {
            _pendingDatabasePath = slot.Stem;
            _pendingSlotSignature = slot.Signature;
            _pendingKey = key;
        }

        private bool PendingChangeMatches(SaveLoadRequest request)
        {
            return request.Slot.Stem == _pendingDatabasePath
                && request.Slot.Signature == _pendingSlotSignature
                && request.Key == _pendingKey;
        }

        private void ClearPendingChange()
        {
            _pendingDatabasePath = null;
            _pendingSlotSignature = null;
            _pendingKey = null;
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
            public int[] EncounterTargetIds;
            public bool IncludeTreasure;
        }

    }
}
