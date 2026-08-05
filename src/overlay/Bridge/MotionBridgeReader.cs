using System;
using System.Globalization;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal sealed class MotionFrame
    {
        public long Sequence;
        public int Generation;
        public bool Enabled;
        public string Mode;
        public double PlayerX;
        public double PlayerY;
        public double PlayerZ;
        public bool HasPlayerZ;
        public double Radius;
        public WorldMapState WorldMap;
    }

    internal sealed class MotionBridgeReader
    {
        private readonly Slot _slotA;
        private readonly Slot _slotB;
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

            MotionFrame newest = Newer(_slotA.Frame, _slotB.Frame);
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

            _lastDeliveredGeneration = newest.Generation;
            _lastDeliveredSequence = newest.Sequence;
            frame = newest;
            return true;
        }

        private static MotionFrame Newer(
            MotionFrame left,
            MotionFrame right)
        {
            if (left == null) return right;
            if (right == null) return left;
            return CompareVersion(
                right.Generation,
                right.Sequence,
                left.Generation,
                left.Sequence) > 0
                    ? right
                    : left;
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

        private static void Update(Slot slot)
        {
            try
            {
                // Motion slots are tiny (well below 1 KB). Read both slots on
                // every timer tick and trust the embedded generation/sequence
                // pair instead of FileInfo timestamps, which can be coalesced
                // during high-frequency writes on Windows.
                string text = SharedBridgeFile.ReadAllText(slot.Path);
                if (String.Equals(
                    text,
                    slot.LastText,
                    StringComparison.Ordinal))
                {
                    return;
                }

                MotionFrame frame;
                if (TryParse(text, out frame))
                {
                    slot.Frame = frame;
                    slot.LastText = text;
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

        internal static bool TryParse(
            string text,
            out MotionFrame frame)
        {
            frame = null;
            if (String.IsNullOrWhiteSpace(text))
            {
                return false;
            }

            string[] values = text.Trim().Split('|');
            if (values.Length != 21)
            {
                return false;
            }

            long sequence;
            long trailingSequence;
            int generation;
            int enabled;
            double playerX;
            double playerY;
            double playerZ;
            int hasPlayerZ;
            double radius;
            int mapId;
            double dimensions;
            double uiSize;
            double left;
            double top;
            double zoom;
            double viewportWidth;
            double viewportHeight;
            double viewportScale;
            double playerMapX;
            double playerMapY;

            if (!Int64.TryParse(values[0], NumberStyles.Integer,
                    CultureInfo.InvariantCulture, out sequence)
                || !Int32.TryParse(values[1], NumberStyles.Integer,
                    CultureInfo.InvariantCulture, out generation)
                || !Int32.TryParse(values[2], NumberStyles.Integer,
                    CultureInfo.InvariantCulture, out enabled)
                || !Double.TryParse(values[4], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out playerX)
                || !Double.TryParse(values[5], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out playerY)
                || !Double.TryParse(values[6], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out playerZ)
                || !Int32.TryParse(values[7], NumberStyles.Integer,
                    CultureInfo.InvariantCulture, out hasPlayerZ)
                || !Double.TryParse(values[8], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out radius)
                || !Int32.TryParse(values[9], NumberStyles.Integer,
                    CultureInfo.InvariantCulture, out mapId)
                || !Double.TryParse(values[10], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out dimensions)
                || !Double.TryParse(values[11], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out uiSize)
                || !Double.TryParse(values[12], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out left)
                || !Double.TryParse(values[13], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out top)
                || !Double.TryParse(values[14], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out zoom)
                || !Double.TryParse(values[15], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out viewportWidth)
                || !Double.TryParse(values[16], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out viewportHeight)
                || !Double.TryParse(values[17], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out viewportScale)
                || !Double.TryParse(values[18], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out playerMapX)
                || !Double.TryParse(values[19], NumberStyles.Float,
                    CultureInfo.InvariantCulture, out playerMapY)
                || !Int64.TryParse(values[20], NumberStyles.Integer,
                    CultureInfo.InvariantCulture, out trailingSequence)
                || sequence != trailingSequence
                || sequence < 0
                || generation < 0)
            {
                return false;
            }

            string mode = values[3];
            if (!String.Equals(mode, "radar", StringComparison.Ordinal)
                && !String.Equals(mode, "world", StringComparison.Ordinal)
                && !String.Equals(mode, "disabled", StringComparison.Ordinal))
            {
                return false;
            }

            WorldMapState map = null;
            if (String.Equals(mode, "world", StringComparison.Ordinal))
            {
                if (mapId <= 0
                    || dimensions <= 0
                    || uiSize <= 0
                    || zoom <= 0
                    || viewportWidth <= 0
                    || viewportHeight <= 0
                    || viewportScale <= 0)
                {
                    return false;
                }

                map = new WorldMapState
                {
                    mapId = mapId,
                    dimensions = dimensions,
                    uiSize = uiSize,
                    left = left,
                    top = top,
                    zoom = zoom,
                    viewportWidth = viewportWidth,
                    viewportHeight = viewportHeight,
                    viewportScale = viewportScale,
                    playerWorldX = playerX,
                    playerWorldY = playerY,
                    playerMapX = playerMapX,
                    playerMapY = playerMapY
                };
            }

            frame = new MotionFrame
            {
                Sequence = sequence,
                Generation = generation,
                Enabled = enabled != 0,
                Mode = mode,
                PlayerX = playerX,
                PlayerY = playerY,
                PlayerZ = playerZ,
                HasPlayerZ = hasPlayerZ != 0,
                Radius = radius,
                WorldMap = map
            };
            return true;
        }

        private sealed class Slot
        {
            public readonly string Path;
            public string LastText;
            public MotionFrame Frame;

            public Slot(string path)
            {
                Path = path;
            }
        }
    }
}
