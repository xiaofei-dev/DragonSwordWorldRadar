using System;
using System.IO;
using System.Threading;

namespace DragonSwordWorldRadar
{
    internal sealed class MotionFrame
    {
        public long Sequence;
        public int ProtocolVersion;
        public int Generation;
        public int WorldEpoch;
        public double SampleTimestampMs;
        public bool Enabled;
        public string Mode;
        public int DiagnosticMode;
        public bool ShowHeight;
        public bool ShowTreasureTypes;
        public bool ShowTreasures;
        public bool ShowBosses;
        public bool ShowMoles;
        public long MoleMask;
        public bool ShowWorldStatus;
        public bool WorldTimeAvailable;
        public int WorldTimeSeconds;
        public bool WeatherAvailable;
        public int WeatherState;
        public int WeatherBtState;
        public double TextScale;
        public double PlayerX;
        public double PlayerY;
        public double PlayerZ;
        public bool HasPlayerZ;
        public double Radius;
        public WorldMapState WorldMap;
    }

    internal sealed class MotionBridgeReader : IDisposable
    {
        private const int MotionRecordBufferSize = 1024;
        internal const int SlotAMask = 1;
        internal const int SlotBMask = 2;
        internal const int AllSlotsMask = SlotAMask | SlotBMask;

        private readonly Slot _slotA;
        private readonly Slot _slotB;
        private readonly MotionBridgeDirtyGate _dirtyGate =
            new MotionBridgeDirtyGate(AllSlotsMask);
        private readonly FileSystemWatcher _watcher;
        private readonly Action _changeSignal;
        private readonly MotionFrame _publishedFrame = new MotionFrame();
        private readonly WorldMapState _publishedWorldMap =
            new WorldMapState();
        private int _lastDeliveredGeneration = -1;
        private long _lastDeliveredSequence = -1;
        private int _notificationsHealthy;
        private int _notificationsEnabled;
        private int _disposed;

        public MotionBridgeReader(
            string bridgeDirectory,
            Action changeSignal)
        {
            if (String.IsNullOrWhiteSpace(bridgeDirectory))
            {
                throw new ArgumentException(
                    "A bridge directory is required.",
                    "bridgeDirectory");
            }
            _changeSignal = changeSignal;
            _slotA = new Slot(Path.Combine(
                bridgeDirectory,
                "radar_motion_a.dat"));
            _slotB = new Slot(Path.Combine(
                bridgeDirectory,
                "radar_motion_b.dat"));

            if (!Directory.Exists(bridgeDirectory))
            {
                return;
            }

            FileSystemWatcher watcher = null;
            try
            {
                watcher = new FileSystemWatcher(
                    bridgeDirectory,
                    "radar_motion_*.dat");
                watcher.IncludeSubdirectories = false;
                watcher.NotifyFilter = NotifyFilters.FileName
                    | NotifyFilters.LastWrite
                    | NotifyFilters.Size;
                watcher.Changed += OnBridgeChanged;
                watcher.Created += OnBridgeChanged;
                watcher.Deleted += OnBridgeChanged;
                watcher.Renamed += OnBridgeRenamed;
                watcher.Error += OnWatcherError;
                Volatile.Write(ref _notificationsHealthy, 1);
                Volatile.Write(ref _notificationsEnabled, 1);
                watcher.EnableRaisingEvents = true;
                if (Volatile.Read(ref _notificationsHealthy) == 0)
                {
                    DisposeUnpublishedWatcher(watcher);
                    return;
                }
                _watcher = watcher;
            }
            catch (ArgumentException)
            {
                DisposeUnpublishedWatcher(watcher);
            }
            catch (IOException)
            {
                DisposeUnpublishedWatcher(watcher);
            }
            catch (UnauthorizedAccessException)
            {
                DisposeUnpublishedWatcher(watcher);
            }
        }

        public bool ChangeNotificationsAvailable
        {
            get
            {
                return _watcher != null
                    && Volatile.Read(ref _notificationsHealthy) != 0
                    && Volatile.Read(ref _disposed) == 0;
            }
        }

        public bool HasPendingChanges
        {
            get { return _dirtyGate.HasPending; }
        }

        public void SetChangeNotificationsEnabled(bool enabled)
        {
            FileSystemWatcher watcher = _watcher;
            if (watcher == null
                || Volatile.Read(ref _notificationsHealthy) == 0
                || Volatile.Read(ref _disposed) != 0)
            {
                return;
            }

            int requested = enabled ? 1 : 0;
            if (Volatile.Read(ref _notificationsEnabled) == requested)
            {
                return;
            }

            try
            {
                watcher.EnableRaisingEvents = enabled;
                Volatile.Write(ref _notificationsEnabled, requested);
                if (enabled)
                {
                    MarkDirty(AllSlotsMask, false);
                }
            }
            catch (ObjectDisposedException)
            {
                FailNotifications(watcher);
            }
            catch (InvalidOperationException)
            {
                FailNotifications(watcher);
            }
        }

        public bool TryReadLatest(
            bool forceFullScan,
            out MotionFrame frame)
        {
            int pendingMask = _dirtyGate.Take(false);
            int scanMask = forceFullScan
                ? AllSlotsMask
                : pendingMask;
            if (scanMask == 0)
            {
                frame = null;
                return false;
            }

            if ((scanMask & SlotAMask) != 0)
            {
                UpdateOrRearm(
                    _slotA,
                    SlotAMask,
                    (pendingMask & SlotAMask) != 0);
            }
            if ((scanMask & SlotBMask) != 0)
            {
                UpdateOrRearm(
                    _slotB,
                    SlotBMask,
                    (pendingMask & SlotBMask) != 0);
            }

            MotionFrame newest = Newer(
                _slotA.HasCandidate ? _slotA.CandidateFrame : null,
                _slotB.HasCandidate ? _slotB.CandidateFrame : null);
            if (newest == null
                || CompareVersion(
                    newest.Generation,
                    newest.Sequence,
                    _lastDeliveredGeneration,
                    _lastDeliveredSequence) <= 0)
            {
                frame = null;
                return false;
            }

            CopyForPublication(
                newest,
                _publishedFrame,
                _publishedWorldMap);
            _lastDeliveredGeneration = _publishedFrame.Generation;
            _lastDeliveredSequence = _publishedFrame.Sequence;
            frame = _publishedFrame;
            return true;
        }

        public void Dispose()
        {
            if (Interlocked.Exchange(ref _disposed, 1) != 0)
            {
                return;
            }

            FileSystemWatcher watcher = _watcher;
            if (watcher == null)
            {
                return;
            }
            Volatile.Write(ref _notificationsHealthy, 0);
            Volatile.Write(ref _notificationsEnabled, 0);
            try
            {
                watcher.EnableRaisingEvents = false;
            }
            catch
            {
            }
            watcher.Changed -= OnBridgeChanged;
            watcher.Created -= OnBridgeChanged;
            watcher.Deleted -= OnBridgeChanged;
            watcher.Renamed -= OnBridgeRenamed;
            watcher.Error -= OnWatcherError;
            watcher.Dispose();
        }

        internal static int CompareVersion(
            int leftGeneration,
            long leftSequence,
            int rightGeneration,
            long rightSequence)
        {
            int generation = leftGeneration.CompareTo(rightGeneration);
            return generation != 0
                ? generation
                : leftSequence.CompareTo(rightSequence);
        }

        private static void CopyForPublication(
            MotionFrame source,
            MotionFrame destination,
            WorldMapState destinationWorldMap)
        {
            destination.Sequence = source.Sequence;
            destination.ProtocolVersion = source.ProtocolVersion;
            destination.Generation = source.Generation;
            destination.WorldEpoch = source.WorldEpoch;
            destination.SampleTimestampMs = source.SampleTimestampMs;
            destination.Enabled = source.Enabled;
            destination.Mode = source.Mode;
            destination.DiagnosticMode = source.DiagnosticMode;
            destination.ShowHeight = source.ShowHeight;
            destination.ShowTreasureTypes = source.ShowTreasureTypes;
            destination.ShowTreasures = source.ShowTreasures;
            destination.ShowBosses = source.ShowBosses;
            destination.ShowMoles = source.ShowMoles;
            destination.MoleMask = source.MoleMask;
            destination.ShowWorldStatus = source.ShowWorldStatus;
            destination.WorldTimeAvailable = source.WorldTimeAvailable;
            destination.WorldTimeSeconds = source.WorldTimeSeconds;
            destination.WeatherAvailable = source.WeatherAvailable;
            destination.WeatherState = source.WeatherState;
            destination.WeatherBtState = source.WeatherBtState;
            destination.TextScale = source.TextScale;
            destination.PlayerX = source.PlayerX;
            destination.PlayerY = source.PlayerY;
            destination.PlayerZ = source.PlayerZ;
            destination.HasPlayerZ = source.HasPlayerZ;
            destination.Radius = source.Radius;

            WorldMapState sourceWorldMap = source.WorldMap;
            if (sourceWorldMap == null)
            {
                destination.WorldMap = null;
                return;
            }

            destinationWorldMap.mapId = sourceWorldMap.mapId;
            destinationWorldMap.dimensions = sourceWorldMap.dimensions;
            destinationWorldMap.uiSize = sourceWorldMap.uiSize;
            destinationWorldMap.left = sourceWorldMap.left;
            destinationWorldMap.top = sourceWorldMap.top;
            destinationWorldMap.zoom = sourceWorldMap.zoom;
            destinationWorldMap.viewportWidth =
                sourceWorldMap.viewportWidth;
            destinationWorldMap.viewportHeight =
                sourceWorldMap.viewportHeight;
            destinationWorldMap.viewportScale =
                sourceWorldMap.viewportScale;
            destinationWorldMap.playerWorldX =
                sourceWorldMap.playerWorldX;
            destinationWorldMap.playerWorldY =
                sourceWorldMap.playerWorldY;
            destinationWorldMap.playerMapX = sourceWorldMap.playerMapX;
            destinationWorldMap.playerMapY = sourceWorldMap.playerMapY;
            destination.WorldMap = destinationWorldMap;
        }

        private static MotionFrame Newer(
            MotionFrame left,
            MotionFrame right)
        {
            if (left == null)
            {
                return right;
            }
            if (right == null)
            {
                return left;
            }
            return CompareVersion(
                right.Generation,
                right.Sequence,
                left.Generation,
                left.Sequence) > 0
                    ? right
                    : left;
        }

        private void UpdateOrRearm(
            Slot slot,
            int slotMask,
            bool bypassMetadataGate)
        {
            if (Update(slot, bypassMetadataGate))
            {
                slot.ImmediateRetryAvailable = true;
                return;
            }
            if (slot.ImmediateRetryAvailable)
            {
                slot.ImmediateRetryAvailable = false;
                MarkDirty(slotMask, false);
            }
        }

        private static bool Update(
            Slot slot,
            bool bypassMetadataGate)
        {
            try
            {
                slot.Info.Refresh();
                if (!slot.Info.Exists)
                {
                    ClearMissingSlot(slot);
                    return true;
                }
                if (!bypassMetadataGate
                    && slot.HasMetadata
                    && slot.LastWriteUtc == slot.Info.LastWriteTimeUtc
                    && slot.Length == slot.Info.Length)
                {
                    return true;
                }
            }
            catch (IOException)
            {
                return false;
            }
            catch (UnauthorizedAccessException)
            {
                return false;
            }

            try
            {
                // Motion slots are ASCII records under 768 bytes. Read into a
                // reusable fixed buffer and parse in place; this removes the
                // per-frame string, Split(), numeric substring, MotionFrame,
                // and WorldMapState allocations from the high-frequency overlay path.
                int length = SharedBridgeFile.ReadInto(
                    slot.Path,
                    slot.ReadBuffer);
                if (SameContent(slot, length))
                {
                    slot.LastWriteUtc = slot.Info.LastWriteTimeUtc;
                    slot.Length = slot.Info.Length;
                    slot.HasMetadata = true;
                    return true;
                }

                if (MotionRecordParser.TryParse(
                    slot.ReadBuffer,
                    length,
                    slot.CandidateFrame,
                    slot.CandidateWorldMap))
                {
                    slot.HasCandidate = true;
                    Buffer.BlockCopy(
                        slot.ReadBuffer,
                        0,
                        slot.LastBuffer,
                        0,
                        length);
                    slot.LastLength = length;
                    slot.LastWriteUtc = slot.Info.LastWriteTimeUtc;
                    slot.Length = slot.Info.Length;
                    slot.HasMetadata = true;
                    return true;
                }
                return false;
            }
            catch (FileNotFoundException)
            {
                ClearMissingSlot(slot);
                return true;
            }
            catch (DirectoryNotFoundException)
            {
                ClearMissingSlot(slot);
                return true;
            }
            catch (IOException)
            {
                // A slot can be observed while the producer is truncating and
                // rewriting it. The trailing sequence check rejects partial
                // records, and the other slot remains a complete frame.
                return false;
            }
            catch (UnauthorizedAccessException)
            {
                return false;
            }
        }

        private static void ClearMissingSlot(Slot slot)
        {
            slot.HasMetadata = false;
            slot.HasCandidate = false;
            slot.LastLength = -1;
        }

        private void OnBridgeChanged(
            object sender,
            FileSystemEventArgs eventArgs)
        {
            MarkDirtyForName(eventArgs == null ? null : eventArgs.Name);
        }

        private void OnBridgeRenamed(
            object sender,
            RenamedEventArgs eventArgs)
        {
            if (eventArgs == null)
            {
                MarkDirty(AllSlotsMask, true);
                return;
            }
            MarkDirtyForName(eventArgs.OldName);
            MarkDirtyForName(eventArgs.Name);
        }

        private void OnWatcherError(
            object sender,
            ErrorEventArgs eventArgs)
        {
            FailNotifications(sender as FileSystemWatcher);
        }

        private void MarkDirtyForName(string name)
        {
            if (String.Equals(
                name,
                "radar_motion_a.dat",
                StringComparison.OrdinalIgnoreCase))
            {
                MarkDirty(SlotAMask, true);
            }
            else if (String.Equals(
                name,
                "radar_motion_b.dat",
                StringComparison.OrdinalIgnoreCase))
            {
                MarkDirty(SlotBMask, true);
            }
        }

        private void FailNotifications(FileSystemWatcher failedWatcher)
        {
            if (Interlocked.Exchange(
                ref _notificationsHealthy,
                0) == 0)
            {
                return;
            }
            Volatile.Write(ref _notificationsEnabled, 0);
            try
            {
                // The Error callback can run after event delivery starts but
                // just before the constructor publishes _watcher. Always
                // disable the sender as a fallback so no live watcher escapes
                // ownership in that narrow window.
                FileSystemWatcher watcher = _watcher ?? failedWatcher;
                if (watcher != null)
                {
                    watcher.EnableRaisingEvents = false;
                }
            }
            catch
            {
            }
            MarkDirty(AllSlotsMask, true);
        }

        private void DisposeUnpublishedWatcher(
            FileSystemWatcher watcher)
        {
            Volatile.Write(ref _notificationsHealthy, 0);
            Volatile.Write(ref _notificationsEnabled, 0);
            if (watcher == null)
            {
                return;
            }
            try
            {
                watcher.EnableRaisingEvents = false;
            }
            catch
            {
            }
            watcher.Changed -= OnBridgeChanged;
            watcher.Created -= OnBridgeChanged;
            watcher.Deleted -= OnBridgeChanged;
            watcher.Renamed -= OnBridgeRenamed;
            watcher.Error -= OnWatcherError;
            watcher.Dispose();
        }

        private void MarkDirty(int slotMask, bool signal)
        {
            if (Volatile.Read(ref _disposed) != 0)
            {
                return;
            }
            _dirtyGate.Mark(slotMask);
            if (!signal || _changeSignal == null)
            {
                return;
            }
            try
            {
                _changeSignal();
            }
            catch
            {
                // The dirty bit remains set. The UI timer or fallback scan
                // will consume it even if the form is closing.
            }
        }

        private static bool SameContent(Slot slot, int length)
        {
            if (length != slot.LastLength)
            {
                return false;
            }
            for (int index = 0; index < length; index++)
            {
                if (slot.ReadBuffer[index] != slot.LastBuffer[index])
                {
                    return false;
                }
            }
            return true;
        }

        private sealed class Slot
        {
            public readonly string Path;
            public readonly FileInfo Info;
            public readonly byte[] ReadBuffer =
                new byte[MotionRecordBufferSize];
            public readonly byte[] LastBuffer =
                new byte[MotionRecordBufferSize];
            public int LastLength = -1;
            public bool HasMetadata;
            public DateTime LastWriteUtc;
            public long Length;
            public bool HasCandidate;
            public bool ImmediateRetryAvailable = true;
            public readonly MotionFrame CandidateFrame =
                new MotionFrame();
            public readonly WorldMapState CandidateWorldMap =
                new WorldMapState();

            public Slot(string path)
            {
                Path = path;
                Info = new FileInfo(path);
            }
        }
    }

    internal sealed class MotionBridgeDirtyGate
    {
        private readonly int _allMask;
        private int _dirtyMask;

        public MotionBridgeDirtyGate(int allMask)
        {
            if (allMask <= 0)
            {
                throw new ArgumentOutOfRangeException("allMask");
            }
            _allMask = allMask;
            _dirtyMask = allMask;
        }

        public bool HasPending
        {
            get { return Volatile.Read(ref _dirtyMask) != 0; }
        }

        public void Mark(int mask)
        {
            int accepted = mask & _allMask;
            if (accepted == 0)
            {
                return;
            }
            int observed;
            int combined;
            do
            {
                observed = Volatile.Read(ref _dirtyMask);
                combined = observed | accepted;
                if (combined == observed)
                {
                    return;
                }
            }
            while (Interlocked.CompareExchange(
                ref _dirtyMask,
                combined,
                observed) != observed);
        }

        public int Take(bool forceAll)
        {
            int pending = Interlocked.Exchange(ref _dirtyMask, 0);
            return forceAll ? _allMask : pending;
        }
    }
}
