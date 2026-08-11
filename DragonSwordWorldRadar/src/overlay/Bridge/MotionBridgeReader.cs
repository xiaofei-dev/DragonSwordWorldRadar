using System;
using System.IO;

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

    internal sealed class MotionBridgeReader
    {
        private const int MotionRecordBufferSize = 1024;

        private readonly Slot _slotA;
        private readonly Slot _slotB;
        private readonly MotionFrame _publishedFrame = new MotionFrame();
        private readonly WorldMapState _publishedWorldMap =
            new WorldMapState();
        private int _lastDeliveredGeneration = -1;
        private long _lastDeliveredSequence = -1;

        public MotionBridgeReader(string bridgeDirectory)
        {
            _slotA = new Slot(Path.Combine(
                bridgeDirectory,
                "radar_motion_a.dat"));
            _slotB = new Slot(Path.Combine(
                bridgeDirectory,
                "radar_motion_b.dat"));
        }

        public bool TryReadLatest(out MotionFrame frame)
        {
            Update(_slotA);
            Update(_slotB);

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

        private static void Update(Slot slot)
        {
            try
            {
                slot.Info.Refresh();
                if (!slot.Info.Exists)
                {
                    slot.HasMetadata = false;
                    return;
                }
                if (slot.HasMetadata
                    && slot.LastWriteUtc == slot.Info.LastWriteTimeUtc
                    && slot.Length == slot.Info.Length)
                {
                    return;
                }
            }
            catch (IOException)
            {
                return;
            }
            catch (UnauthorizedAccessException)
            {
                return;
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
                    return;
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
                }
            }
            catch (FileNotFoundException)
            {
            }
            catch (DirectoryNotFoundException)
            {
            }
            catch (IOException)
            {
                // A slot can be observed while the producer is truncating and
                // rewriting it. The trailing sequence check rejects partial
                // records, and the other slot remains a complete frame.
            }
            catch (UnauthorizedAccessException)
            {
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
}
