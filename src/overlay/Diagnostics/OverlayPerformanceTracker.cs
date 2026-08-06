using System;
using System.Diagnostics;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    internal sealed class OverlayPerformanceTracker
    {
        private DateTime _nextLogUtc = DateTime.UtcNow.AddSeconds(5);
        private int _timerTicks;
        private int _paints;
        private int _motionFrames;
        private int _motionVisualFrames;
        private int _motionSuppressedFrames;
        private int _staticFrames;
        private int _staticVisualFrames;
        private int _staticSuppressedFrames;
        private int _invalidates;
        private double _refreshTotalMs;
        private double _refreshMaxMs;
        private double _paintTotalMs;
        private double _paintMaxMs;

        public void RecordStaticFrame(bool visualChange)
        {
            _staticFrames++;
            if (visualChange)
            {
                _staticVisualFrames++;
            }
            else
            {
                _staticSuppressedFrames++;
            }
        }

        public void RecordMotionFrame(bool visualChange)
        {
            _motionFrames++;
            if (visualChange)
            {
                _motionVisualFrames++;
            }
            else
            {
                _motionSuppressedFrames++;
            }
        }

        public void RecordInvalidate()
        {
            _invalidates++;
        }

        public void RecordPaint(long started)
        {
            double elapsed = ElapsedMilliseconds(started);
            _paints++;
            _paintTotalMs += elapsed;
            _paintMaxMs = Math.Max(_paintMaxMs, elapsed);
        }

        public void RecordTimerTick(long started)
        {
            double elapsed = ElapsedMilliseconds(started);
            _timerTicks++;
            _refreshTotalMs += elapsed;
            _refreshMaxMs = Math.Max(_refreshMaxMs, elapsed);
        }

        public void LogIfDue(
            string mode,
            int timerInterval,
            int visibleWorldTreasureCount)
        {
            DateTime now = DateTime.UtcNow;
            if (now < _nextLogUtc)
            {
                return;
            }

            double refreshAverage = _timerTicks == 0
                ? 0.0
                : _refreshTotalMs / _timerTicks;
            double paintAverage = _paints == 0
                ? 0.0
                : _paintTotalMs / _paints;
            ErrorLog.WriteMessage(String.Format(
                CultureInfo.InvariantCulture,
                "Overlay performance: mode={0}; timerMs={1}; ticks={2}; staticFrames={3}; staticRedraws={4}; staticSuppressed={5}; motionFrames={6}; motionRedraws={7}; motionSuppressed={8}; invalidates={9}; paints={10}; refreshAvgMs={11:F3}; refreshMaxMs={12:F3}; paintAvgMs={13:F3}; paintMaxMs={14:F3}; worldTreasures={15}",
                mode,
                timerInterval,
                _timerTicks,
                _staticFrames,
                _staticVisualFrames,
                _staticSuppressedFrames,
                _motionFrames,
                _motionVisualFrames,
                _motionSuppressedFrames,
                _invalidates,
                _paints,
                refreshAverage,
                _refreshMaxMs,
                paintAverage,
                _paintMaxMs,
                visibleWorldTreasureCount));

            _nextLogUtc = now.AddSeconds(5);
            ResetWindow();
        }

        private static double ElapsedMilliseconds(long started)
        {
            return (Stopwatch.GetTimestamp() - started)
                * 1000.0 / Stopwatch.Frequency;
        }

        private void ResetWindow()
        {
            _timerTicks = 0;
            _paints = 0;
            _motionFrames = 0;
            _motionVisualFrames = 0;
            _motionSuppressedFrames = 0;
            _staticFrames = 0;
            _staticVisualFrames = 0;
            _staticSuppressedFrames = 0;
            _invalidates = 0;
            _refreshTotalMs = 0.0;
            _refreshMaxMs = 0.0;
            _paintTotalMs = 0.0;
            _paintMaxMs = 0.0;
        }
    }
}
