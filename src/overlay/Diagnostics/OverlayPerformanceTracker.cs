using System;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    internal sealed class OverlayPerformanceTracker
    {
        private const int DefaultLogIntervalSeconds = 5;

        private bool _enabled;
        private DateTime _windowStartedUtc;
        private DateTime _nextLogUtc;
        private DateTime _lastTimerTickUtc;
        private DateTime _lastPaintUtc;
        private DateTime _lastMotionFrameUtc;
        private DateTime _lastProcessSampleUtc;
        private TimeSpan _lastOverlayProcessorTime;
        private TimeSpan _lastGameProcessorTime;
        private int _lastGameProcessId;
        private bool _hasOverlayProcessSample;
        private bool _hasGameProcessSample;

        private int _timerTicks;
        private int _paints;
        private int _motionFrames;
        private int _motionVisualFrames;
        private int _motionSuppressedFrames;
        private int _staticFrames;
        private int _staticVisualFrames;
        private int _staticSuppressedFrames;
        private int _invalidates;
        private int _timerGapSamples;
        private int _timerGap50Count;
        private int _timerGap100Count;
        private int _timerGap250Count;
        private double _refreshTotalMs;
        private double _refreshMaxMs;
        private double _paintTotalMs;
        private double _paintMaxMs;
        private double _timerGapTotalMs;
        private double _timerGapMaxMs;
        private double _timerLateTotalMs;
        private double _timerLateMaxMs;
        private double _paintGapMaxMs;
        private double _motionGapMaxMs;

        public void SetEnabled(bool enabled)
        {
            if (_enabled == enabled)
            {
                return;
            }

            _enabled = enabled;
            ResetAll(DateTime.UtcNow);
        }

        public long BeginTiming()
        {
            return _enabled
                ? Stopwatch.GetTimestamp()
                : 0L;
        }

        public void RecordStaticFrame(bool visualChange)
        {
            if (!_enabled)
            {
                return;
            }

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

        public void RecordMotionFrame(
            bool visualChange,
            DateTime now)
        {
            if (!_enabled)
            {
                return;
            }

            _motionFrames++;
            if (visualChange)
            {
                _motionVisualFrames++;
            }
            else
            {
                _motionSuppressedFrames++;
            }

            if (_lastMotionFrameUtc != DateTime.MinValue)
            {
                _motionGapMaxMs = Math.Max(
                    _motionGapMaxMs,
                    (now - _lastMotionFrameUtc).TotalMilliseconds);
            }
            _lastMotionFrameUtc = now;
        }

        public void RecordInvalidate()
        {
            if (_enabled)
            {
                _invalidates++;
            }
        }

        public void RecordPaint(
            long started,
            DateTime now)
        {
            if (!_enabled)
            {
                return;
            }

            double elapsed = ElapsedMilliseconds(started);
            _paints++;
            _paintTotalMs += elapsed;
            _paintMaxMs = Math.Max(_paintMaxMs, elapsed);
            if (_lastPaintUtc != DateTime.MinValue)
            {
                _paintGapMaxMs = Math.Max(
                    _paintGapMaxMs,
                    (now - _lastPaintUtc).TotalMilliseconds);
            }
            _lastPaintUtc = now;
        }

        public void RecordTimerTick(
            long started,
            DateTime now,
            int expectedIntervalMs)
        {
            if (!_enabled)
            {
                return;
            }

            double elapsed = ElapsedMilliseconds(started);
            _timerTicks++;
            _refreshTotalMs += elapsed;
            _refreshMaxMs = Math.Max(_refreshMaxMs, elapsed);

            if (_lastTimerTickUtc != DateTime.MinValue)
            {
                double gap = (now - _lastTimerTickUtc).TotalMilliseconds;
                double late = Math.Max(
                    0.0,
                    gap - Math.Max(1, expectedIntervalMs));
                _timerGapTotalMs += gap;
                _timerGapMaxMs = Math.Max(_timerGapMaxMs, gap);
                _timerLateTotalMs += late;
                _timerLateMaxMs = Math.Max(_timerLateMaxMs, late);
                _timerGapSamples++;
                if (gap >= 50.0)
                {
                    _timerGap50Count++;
                }
                if (gap >= 100.0)
                {
                    _timerGap100Count++;
                }
                if (gap >= 250.0)
                {
                    _timerGap250Count++;
                }
            }
            _lastTimerTickUtc = now;
        }

        public void LogIfDue(
            string mode,
            int timerInterval,
            int visibleWorldTreasureCount,
            int gameProcessId,
            bool overlayVisible,
            Rectangle overlayBounds)
        {
            if (!_enabled)
            {
                return;
            }

            DateTime now = DateTime.UtcNow;
            if (now < _nextLogUtc)
            {
                return;
            }

            double windowSeconds = Math.Max(
                0.001,
                (now - _windowStartedUtc).TotalSeconds);
            double refreshAverage = Average(
                _refreshTotalMs,
                _timerTicks);
            double paintAverage = Average(
                _paintTotalMs,
                _paints);
            double timerGapAverage = Average(
                _timerGapTotalMs,
                _timerGapSamples);
            double timerLateAverage = Average(
                _timerLateTotalMs,
                _timerGapSamples);

            double overlayCpuPercent;
            double gameCpuPercent;
            double overlayWorkingSetMb;
            double gameWorkingSetMb;
            SampleProcesses(
                gameProcessId,
                now,
                out overlayCpuPercent,
                out gameCpuPercent,
                out overlayWorkingSetMb,
                out gameWorkingSetMb);

            long overlayPixels = Math.Max(0, overlayBounds.Width)
                * (long)Math.Max(0, overlayBounds.Height);
            ErrorLog.WriteDebug(String.Format(
                CultureInfo.InvariantCulture,
                "OVERLAY_PERF windowSec={0:F3}; mode={1}; visible={2}; bounds={3},{4},{5},{6}; pixels={7}; timerMs={8}; timerHz={9:F3}; timerTicks={10}; timerGapAvgMs={11:F3}; timerGapMaxMs={12:F3}; timerLateAvgMs={13:F3}; timerLateMaxMs={14:F3}; gaps50={15}; gaps100={16}; gaps250={17}; staticReadHz={18:F3}; staticFrames={19}; staticRedraws={20}; staticSuppressed={21}; motionReadHz={22:F3}; motionFrames={23}; motionRedraws={24}; motionSuppressed={25}; motionGapMaxMs={26:F3}; invalidates={27}; overlayPaintFps={28:F3}; paints={29}; paintGapMaxMs={30:F3}; refreshAvgMs={31:F3}; refreshMaxMs={32:F3}; paintAvgMs={33:F3}; paintMaxMs={34:F3}; worldTreasures={35}; overlayCpuPct={36:F3}; gameCpuPct={37:F3}; overlayWorkingSetMb={38:F3}; gameWorkingSetMb={39:F3}; fpsMetric=overlayPaintFps_not_game_present_fps",
                windowSeconds,
                mode,
                overlayVisible,
                overlayBounds.X,
                overlayBounds.Y,
                overlayBounds.Width,
                overlayBounds.Height,
                overlayPixels,
                timerInterval,
                _timerTicks / windowSeconds,
                _timerTicks,
                timerGapAverage,
                _timerGapMaxMs,
                timerLateAverage,
                _timerLateMaxMs,
                _timerGap50Count,
                _timerGap100Count,
                _timerGap250Count,
                _staticFrames / windowSeconds,
                _staticFrames,
                _staticVisualFrames,
                _staticSuppressedFrames,
                _motionFrames / windowSeconds,
                _motionFrames,
                _motionVisualFrames,
                _motionSuppressedFrames,
                _motionGapMaxMs,
                _invalidates,
                _paints / windowSeconds,
                _paints,
                _paintGapMaxMs,
                refreshAverage,
                _refreshMaxMs,
                paintAverage,
                _paintMaxMs,
                visibleWorldTreasureCount,
                overlayCpuPercent,
                gameCpuPercent,
                overlayWorkingSetMb,
                gameWorkingSetMb));

            ResetWindow(now);
        }

        private void SampleProcesses(
            int gameProcessId,
            DateTime now,
            out double overlayCpuPercent,
            out double gameCpuPercent,
            out double overlayWorkingSetMb,
            out double gameWorkingSetMb)
        {
            overlayCpuPercent = -1.0;
            gameCpuPercent = -1.0;
            overlayWorkingSetMb = -1.0;
            gameWorkingSetMb = -1.0;

            double sampleMilliseconds = _lastProcessSampleUtc ==
                DateTime.MinValue
                ? 0.0
                : (now - _lastProcessSampleUtc).TotalMilliseconds;

            try
            {
                using (Process overlay = Process.GetCurrentProcess())
                {
                    overlay.Refresh();
                    TimeSpan processorTime = overlay.TotalProcessorTime;
                    overlayWorkingSetMb = BytesToMegabytes(
                        overlay.WorkingSet64);
                    if (_hasOverlayProcessSample
                        && sampleMilliseconds > 0.0)
                    {
                        overlayCpuPercent = CpuPercent(
                            processorTime - _lastOverlayProcessorTime,
                            sampleMilliseconds);
                    }
                    _lastOverlayProcessorTime = processorTime;
                    _hasOverlayProcessSample = true;
                }
            }
            catch
            {
                _hasOverlayProcessSample = false;
            }

            try
            {
                using (Process game =
                    GameProcessFinder.OpenExact(gameProcessId))
                {
                    if (game != null)
                    {
                        game.Refresh();
                        TimeSpan processorTime = game.TotalProcessorTime;
                        gameWorkingSetMb = BytesToMegabytes(
                            game.WorkingSet64);
                        if (_hasGameProcessSample
                            && _lastGameProcessId == game.Id
                            && sampleMilliseconds > 0.0)
                        {
                            gameCpuPercent = CpuPercent(
                                processorTime - _lastGameProcessorTime,
                                sampleMilliseconds);
                        }
                        _lastGameProcessorTime = processorTime;
                        _lastGameProcessId = game.Id;
                        _hasGameProcessSample = true;
                    }
                    else
                    {
                        _hasGameProcessSample = false;
                        _lastGameProcessId = 0;
                    }
                }
            }
            catch
            {
                _hasGameProcessSample = false;
                _lastGameProcessId = 0;
            }

            _lastProcessSampleUtc = now;
        }

        private static double CpuPercent(
            TimeSpan processorDelta,
            double sampleMilliseconds)
        {
            if (sampleMilliseconds <= 0.0)
            {
                return -1.0;
            }
            return Math.Max(
                0.0,
                processorDelta.TotalMilliseconds * 100.0 /
                (sampleMilliseconds * Math.Max(
                    1,
                    Environment.ProcessorCount)));
        }

        private static double BytesToMegabytes(long bytes)
        {
            return bytes / (1024.0 * 1024.0);
        }

        private static double Average(
            double total,
            int count)
        {
            return count <= 0
                ? 0.0
                : total / count;
        }

        private static double ElapsedMilliseconds(long started)
        {
            if (started <= 0L)
            {
                return 0.0;
            }
            return (Stopwatch.GetTimestamp() - started)
                * 1000.0 / Stopwatch.Frequency;
        }

        private void ResetAll(DateTime now)
        {
            _lastTimerTickUtc = DateTime.MinValue;
            _lastPaintUtc = DateTime.MinValue;
            _lastMotionFrameUtc = DateTime.MinValue;
            _lastProcessSampleUtc = DateTime.MinValue;
            _lastOverlayProcessorTime = TimeSpan.Zero;
            _lastGameProcessorTime = TimeSpan.Zero;
            _lastGameProcessId = 0;
            _hasOverlayProcessSample = false;
            _hasGameProcessSample = false;
            ResetWindow(now);
        }

        private void ResetWindow(DateTime now)
        {
            _windowStartedUtc = now;
            _nextLogUtc = now.AddSeconds(
                DefaultLogIntervalSeconds);
            _timerTicks = 0;
            _paints = 0;
            _motionFrames = 0;
            _motionVisualFrames = 0;
            _motionSuppressedFrames = 0;
            _staticFrames = 0;
            _staticVisualFrames = 0;
            _staticSuppressedFrames = 0;
            _invalidates = 0;
            _timerGapSamples = 0;
            _timerGap50Count = 0;
            _timerGap100Count = 0;
            _timerGap250Count = 0;
            _refreshTotalMs = 0.0;
            _refreshMaxMs = 0.0;
            _paintTotalMs = 0.0;
            _paintMaxMs = 0.0;
            _timerGapTotalMs = 0.0;
            _timerGapMaxMs = 0.0;
            _timerLateTotalMs = 0.0;
            _timerLateMaxMs = 0.0;
            _paintGapMaxMs = 0.0;
            _motionGapMaxMs = 0.0;
        }
    }
}
