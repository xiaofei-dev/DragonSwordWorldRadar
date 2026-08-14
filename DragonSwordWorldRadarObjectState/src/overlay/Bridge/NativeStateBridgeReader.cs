using System;
using System.Collections.Generic;
using System.IO;
using System.IO.MemoryMappedFiles;

namespace DragonSwordWorldRadar
{
    internal enum NativeStateEventKind
    {
        TreasureOpened = 1,
        EncounterDefeated = 2
    }

    internal sealed class NativeStateEvent
    {
        public long Sequence;
        public int Activation;
        public int WorldEpoch;
        public NativeStateEventKind Kind;
        public long Id;
        public long TimestampMilliseconds;
    }

    internal sealed class NativeStateFrame
    {
        public bool Enabled;
        public bool PositionValid;
        public bool TransitionActive;
        public bool CatchupComplete;
        public int Activation;
        public int WorldEpoch;
        public long SampleSequence;
        public long SampleTimestampMilliseconds;
        public double PlayerX;
        public double PlayerY;
        public double PlayerZ;
        public long EventHead;
        public long PositionSamples;
        public long ActorBeginCallbacks;
        public long ActorEndCallbacks;
        public long FindAllCalls;
        public long FindAllTotalMicroseconds;
        public long FindAllMaximumMicroseconds;
    }

    internal sealed class NativeStateBridgeReader : IDisposable
    {
        private const uint Magic = 0x534F5344U;
        private const uint ProtocolVersion = 1;
        private const int MappingSize = 5248;
        private const int EventOffset = 128;
        private const int EventSize = 40;
        private const int EventCapacity = 128;
        private static readonly TimeSpan OpenRetryInterval =
            TimeSpan.FromSeconds(1);

        private readonly NativeStateFrame _publishedFrame =
            new NativeStateFrame();
        private readonly NativeStateEvent[] _eventScratch =
            new NativeStateEvent[EventCapacity];
        private MemoryMappedFile _mapping;
        private MemoryMappedViewAccessor _view;
        private DateTime _nextOpenUtc;
        private long _lastEventHead;
        private int _processId;
        private bool _disposed;

        public bool TryRead(
            int processId,
            IList<NativeStateEvent> events,
            out NativeStateFrame frame,
            out long missedEventCount)
        {
            frame = null;
            missedEventCount = 0;
            if (_disposed || events == null || processId <= 0)
            {
                return false;
            }
            if (_processId != processId)
            {
                CloseMapping();
                _processId = processId;
                _nextOpenUtc = DateTime.MinValue;
                _lastEventHead = 0;
            }
            if (_view == null && !TryOpen())
            {
                return false;
            }

            for (int attempt = 0; attempt < 3; attempt++)
            {
                uint sequenceBefore = _view.ReadUInt32(0);
                if ((sequenceBefore & 1U) != 0)
                {
                    continue;
                }
                if (_view.ReadUInt32(4) != Magic
                    || _view.ReadUInt32(8) != ProtocolVersion
                    || _view.ReadUInt32(12) != MappingSize)
                {
                    CloseMapping();
                    return false;
                }

                uint flags = _view.ReadUInt32(16);
                int activation = _view.ReadInt32(20);
                int worldEpoch = _view.ReadInt32(24);
                long sampleSequence = _view.ReadInt64(32);
                long sampleTimestamp = _view.ReadInt64(40);
                double playerX = _view.ReadDouble(48);
                double playerY = _view.ReadDouble(56);
                double playerZ = _view.ReadDouble(64);
                long eventHead = _view.ReadInt64(72);
                long previousEventHead = eventHead < _lastEventHead
                    ? 0
                    : _lastEventHead;
                long startEvent = Math.Max(
                    previousEventHead + 1,
                    eventHead - EventCapacity + 1);
                int eventCount = 0;
                bool eventSequenceValid = true;
                for (long eventSequence = startEvent;
                    eventSequence <= eventHead;
                    eventSequence++)
                {
                    int index = (int)((eventSequence - 1)
                        % EventCapacity);
                    long offset = EventOffset + index * EventSize;
                    if (_view.ReadInt64(offset) != eventSequence)
                    {
                        eventSequenceValid = false;
                        break;
                    }
                    NativeStateEvent item = _eventScratch[eventCount];
                    if (item == null)
                    {
                        item = new NativeStateEvent();
                        _eventScratch[eventCount] = item;
                    }
                    item.Sequence = eventSequence;
                    item.Activation = _view.ReadInt32(offset + 8);
                    item.WorldEpoch = _view.ReadInt32(offset + 12);
                    item.Kind = (NativeStateEventKind)
                        _view.ReadInt32(offset + 16);
                    item.Id = _view.ReadInt64(offset + 24);
                    item.TimestampMilliseconds =
                        _view.ReadInt64(offset + 32);
                    eventCount++;
                }

                long positionSamples = _view.ReadInt64(80);
                long actorBeginCallbacks = _view.ReadInt64(88);
                long actorEndCallbacks = _view.ReadInt64(96);
                long findAllCalls = _view.ReadInt64(104);
                long findAllTotalMicroseconds = _view.ReadInt64(112);
                long findAllMaximumMicroseconds = _view.ReadInt64(120);
                uint sequenceAfter = _view.ReadUInt32(0);
                if (!eventSequenceValid
                    || sequenceBefore != sequenceAfter
                    || (sequenceAfter & 1U) != 0)
                {
                    continue;
                }

                missedEventCount = Math.Max(
                    0,
                    eventHead - previousEventHead - EventCapacity);
                _lastEventHead = eventHead;
                for (int index = 0; index < eventCount; index++)
                {
                    events.Add(_eventScratch[index]);
                }
                _publishedFrame.Enabled = (flags & 1U) != 0;
                _publishedFrame.PositionValid = (flags & 2U) != 0;
                _publishedFrame.TransitionActive = (flags & 4U) != 0;
                _publishedFrame.CatchupComplete = (flags & 8U) != 0;
                _publishedFrame.Activation = activation;
                _publishedFrame.WorldEpoch = worldEpoch;
                _publishedFrame.SampleSequence = sampleSequence;
                _publishedFrame.SampleTimestampMilliseconds = sampleTimestamp;
                _publishedFrame.PlayerX = playerX;
                _publishedFrame.PlayerY = playerY;
                _publishedFrame.PlayerZ = playerZ;
                _publishedFrame.EventHead = eventHead;
                _publishedFrame.PositionSamples = positionSamples;
                _publishedFrame.ActorBeginCallbacks = actorBeginCallbacks;
                _publishedFrame.ActorEndCallbacks = actorEndCallbacks;
                _publishedFrame.FindAllCalls = findAllCalls;
                _publishedFrame.FindAllTotalMicroseconds =
                    findAllTotalMicroseconds;
                _publishedFrame.FindAllMaximumMicroseconds =
                    findAllMaximumMicroseconds;
                frame = _publishedFrame;
                return true;
            }
            return false;
        }

        private bool TryOpen()
        {
            DateTime now = DateTime.UtcNow;
            if (now < _nextOpenUtc)
            {
                return false;
            }
            _nextOpenUtc = now.Add(OpenRetryInterval);
            try
            {
                string name = String.Format(
                    System.Globalization.CultureInfo.InvariantCulture,
                    "Local\\DragonSwordWorldRadarObjectState.NativeState.{0}",
                    _processId);
                _mapping = MemoryMappedFile.OpenExisting(
                    name,
                    MemoryMappedFileRights.Read);
                _view = _mapping.CreateViewAccessor(
                    0,
                    MappingSize,
                    MemoryMappedFileAccess.Read);
                return true;
            }
            catch (FileNotFoundException)
            {
                CloseMapping();
                return false;
            }
            catch (UnauthorizedAccessException)
            {
                CloseMapping();
                return false;
            }
            catch (IOException)
            {
                CloseMapping();
                return false;
            }
        }

        private void CloseMapping()
        {
            MemoryMappedViewAccessor view = _view;
            _view = null;
            if (view != null)
            {
                view.Dispose();
            }
            MemoryMappedFile mapping = _mapping;
            _mapping = null;
            if (mapping != null)
            {
                mapping.Dispose();
            }
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }
            _disposed = true;
            CloseMapping();
        }
    }
}
