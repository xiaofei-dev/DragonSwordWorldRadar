using System;
using System.IO;
using System.Web.Script.Serialization;

namespace DragonSwordWorldRadar
{
    internal sealed class StaticStateBridgeReader
    {
        private readonly Slot _slotA;
        private readonly Slot _slotB;
        private readonly JavaScriptSerializer _serializer;
        private int _lastDeliveredGeneration = -1;
        private long _lastDeliveredSequence = -1;

        public StaticStateBridgeReader(
            string bridgeDirectory,
            JavaScriptSerializer serializer)
        {
            _slotA = new Slot(Path.Combine(
                bridgeDirectory,
                "radar_state_a.json"));
            _slotB = new Slot(Path.Combine(
                bridgeDirectory,
                "radar_state_b.json"));
            _serializer = serializer;
        }

        public bool HasAnyFile
        {
            get
            {
                return File.Exists(_slotA.Path)
                    || File.Exists(_slotB.Path);
            }
        }

        public bool TryReadLatest(out RadarState state)
        {
            Update(_slotA);
            Update(_slotB);

            RadarState newest = Newer(_slotA.State, _slotB.State);
            if (newest == null
                || MotionBridgeReader.CompareVersion(
                    newest.producerGeneration,
                    newest.stateSequence,
                    _lastDeliveredGeneration,
                    _lastDeliveredSequence) <= 0)
            {
                state = null;
                return false;
            }

            _lastDeliveredGeneration = newest.producerGeneration;
            _lastDeliveredSequence = newest.stateSequence;
            state = newest;
            return true;
        }

        private static RadarState Newer(
            RadarState left,
            RadarState right)
        {
            if (left == null) return right;
            if (right == null) return left;
            return MotionBridgeReader.CompareVersion(
                right.producerGeneration,
                right.stateSequence,
                left.producerGeneration,
                left.stateSequence) > 0
                    ? right
                    : left;
        }

        private void Update(Slot slot)
        {
            FileInfo info;
            try
            {
                info = new FileInfo(slot.Path);
                info.Refresh();
                if (!info.Exists)
                {
                    return;
                }
                if (slot.HasMetadata
                    && slot.LastWriteUtc == info.LastWriteTimeUtc
                    && slot.Length == info.Length)
                {
                    return;
                }
            }
            catch
            {
                return;
            }

            try
            {
                string text = SharedBridgeFile.ReadAllText(slot.Path);
                RadarState loaded =
                    _serializer.Deserialize<RadarState>(text);
                if (loaded == null
                    || loaded.stateSequence < 0
                    || loaded.producerGeneration < 0)
                {
                    return;
                }

                slot.State = loaded;
                slot.LastWriteUtc = info.LastWriteTimeUtc;
                slot.Length = info.Length;
                slot.HasMetadata = true;
            }
            catch (IOException)
            {
            }
            catch (UnauthorizedAccessException)
            {
            }
            catch (InvalidOperationException)
            {
                // A partial slot is ignored. The previously parsed frame in
                // either slot remains available until the next poll.
            }
            catch (ArgumentException)
            {
            }
        }

        private sealed class Slot
        {
            public readonly string Path;
            public bool HasMetadata;
            public DateTime LastWriteUtc;
            public long Length;
            public RadarState State;

            public Slot(string path)
            {
                Path = path;
            }
        }
    }
}
