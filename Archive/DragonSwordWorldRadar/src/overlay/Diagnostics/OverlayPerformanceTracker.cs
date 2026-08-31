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
        private int _worldMapTimerTicks;
        private int _modeTransitions;
        private int _timerResolutionAcquireAttempts;
        private int _timerResolutionAcquireSuccesses;
        private int _timerResolutionReleaseAttempts;
        private int _timerResolutionReleaseSuccesses;
        private int _timerResolutionBalance;
        private int _paints;
        private int _motionFrames;
        private int _motionVisualFrames;
        private int _motionSuppressedFrames;
        private int _predictionTicks;
        private int _predictionClamps;
        private int _predictionResets;
        private int _predictionStaleFreezes;
        private double _predictionAgeTotalMs;
        private double _predictionAgeMaxMs;
        private int _invalidates;
        private int _windowVisibilitySamples;
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
        private int _treasureDrawCalls;
        private int _treasureMarkersDrawn;
        private double _treasureDrawTotalMs;
        private double _treasureDrawMaxMs;
        private int _bossDrawCalls;
        private int _bossMarkersDrawn;
        private double _bossDrawTotalMs;
        private double _bossDrawMaxMs;
        private int _assaultDrawCalls, _assaultMarkersDrawn; private double _assaultDrawTotalMs, _assaultDrawMaxMs;
        private int _moleDrawCalls;
        private int _moleMarkersDrawn;
        private double _moleDrawTotalMs;
        private double _moleDrawMaxMs;
        private int _worldStatusDrawCalls;
        private double _worldStatusDrawTotalMs;
        private double _worldStatusDrawMaxMs;

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

        public void RecordMotionPrediction(
            double ageMs,
            bool clamped,
            bool reset,
            bool stale)
        {
            if (!_enabled)
            {
                return;
            }
            _predictionTicks++;
            ageMs = Math.Max(0.0, ageMs);
            _predictionAgeTotalMs += ageMs;
            _predictionAgeMaxMs = Math.Max(
                _predictionAgeMaxMs,
                ageMs);
            if (clamped) _predictionClamps++;
            if (reset) _predictionResets++;
            if (stale) _predictionStaleFreezes++;
        }

        public void RecordWindowVisibilitySample()
        {
            if (_enabled)
            {
                _windowVisibilitySamples++;
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

        public void RecordMoleDraw(long started, int markers)
        {
            if (!_enabled) return;
            double elapsed = ElapsedMilliseconds(started);
            _moleDrawCalls++;
            _moleMarkersDrawn += Math.Max(0, markers);
            _moleDrawTotalMs += elapsed;
            _moleDrawMaxMs = Math.Max(_moleDrawMaxMs, elapsed);
        }

        public void RecordTreasureDraw(long started, int markers)
        {
            if (!_enabled) return;
            double elapsed = ElapsedMilliseconds(started);
            _treasureDrawCalls++;
            _treasureMarkersDrawn += Math.Max(0, markers);
            _treasureDrawTotalMs += elapsed;
            _treasureDrawMaxMs = Math.Max(
                _treasureDrawMaxMs,
                elapsed);
        }

        public void RecordBossDraw(long started, int markers)
        {
            if (!_enabled) return;
            double elapsed = ElapsedMilliseconds(started);
            _bossDrawCalls++;
            _bossMarkersDrawn += Math.Max(0, markers);
            _bossDrawTotalMs += elapsed;
            _bossDrawMaxMs = Math.Max(_bossDrawMaxMs, elapsed);
        }
        public void RecordAssaultDraw(long started,int markers){if(!_enabled)return;double elapsed=ElapsedMilliseconds(started);_assaultDrawCalls++;_assaultMarkersDrawn+=Math.Max(0,markers);_assaultDrawTotalMs+=elapsed;_assaultDrawMaxMs=Math.Max(_assaultDrawMaxMs,elapsed);}

        public void RecordEncounterDraw(
            long started,
            int bossMarkers,
            int assaultMarkers)
        {
            if (!_enabled) return;
            double elapsed = ElapsedMilliseconds(started);
            int bosses = Math.Max(0, bossMarkers);
            int assaults = Math.Max(0, assaultMarkers);
            int total = bosses + assaults;
            double bossElapsed = total == 0
                ? 0.0
                : elapsed * bosses / total;
            double assaultElapsed = total == 0
                ? 0.0
                : elapsed - bossElapsed;
            _bossDrawCalls++;
            _bossMarkersDrawn += bosses;
            _bossDrawTotalMs += bossElapsed;
            _bossDrawMaxMs = Math.Max(
                _bossDrawMaxMs,
                bossElapsed);
            _assaultDrawCalls++;
            _assaultMarkersDrawn += assaults;
            _assaultDrawTotalMs += assaultElapsed;
            _assaultDrawMaxMs = Math.Max(
                _assaultDrawMaxMs,
                assaultElapsed);
        }

        public void RecordWorldStatusDraw(long started)
        {
            if (!_enabled) return;
            double elapsed = ElapsedMilliseconds(started);
            _worldStatusDrawCalls++;
            _worldStatusDrawTotalMs += elapsed;
            _worldStatusDrawMaxMs = Math.Max(_worldStatusDrawMaxMs, elapsed);
        }

        public void RecordTimerTick(
            long started,
            DateTime now,
            int expectedIntervalMs,
            string mode)
        {
            if (!_enabled)
            {
                return;
            }

            double elapsed = ElapsedMilliseconds(started);
            _timerTicks++;
            if (String.Equals(mode, "world", StringComparison.Ordinal))
            {
                _worldMapTimerTicks++;
            }
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

        public void RecordModeTransition(
            string previousMode,
            string currentMode)
        {
            if (_enabled && !String.Equals(
                previousMode,
                currentMode,
                StringComparison.Ordinal))
            {
                _modeTransitions++;
            }
        }

        public void RecordTimerResolutionChange(bool acquire, bool success)
        {
            if (!_enabled)
            {
                return;
            }
            if (acquire)
            {
                _timerResolutionAcquireAttempts++;
                if (success)
                {
                    _timerResolutionAcquireSuccesses++;
                    _timerResolutionBalance++;
                }
            }
            else
            {
                _timerResolutionReleaseAttempts++;
                if (success)
                {
                    _timerResolutionReleaseSuccesses++;
                    _timerResolutionBalance--;
                }
            }
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
                "OVERLAY_PERF windowSec={0:F3}; mode={1}; visible={2}; bounds={3},{4},{5},{6}; pixels={7}; timerMs={8}; timerHz={9:F3}; timerTicks={10}; timerGapAvgMs={11:F3}; timerGapMaxMs={12:F3}; timerLateAvgMs={13:F3}; timerLateMaxMs={14:F3}; gaps50={15}; gaps100={16}; gaps250={17}; bridgeReadHz={18:F3}; bridgeFrames={19}; bridgeRedraws={20}; bridgeSuppressed={21}; bridgeGapMaxMs={22:F3}; invalidates={23}; overlayPaintFps={24:F3}; paints={25}; paintGapMaxMs={26:F3}; refreshAvgMs={27:F3}; refreshMaxMs={28:F3}; paintAvgMs={29:F3}; paintMaxMs={30:F3}; worldTreasures={31}; overlayCpuPct={32:F3}; gameCpuPct={33:F3}; overlayWorkingSetMb={34:F3}; gameWorkingSetMb={35:F3}; fpsMetric=overlayPaintFps_not_game_present_fps",
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
            ErrorLog.WriteDebug(String.Format(
                CultureInfo.InvariantCulture,
                "OVERLAY_LAYER_PERF windowSec={0:F3}; treasureDrawCalls={1}; treasureMarkers={2}; treasureAvgMs={3:F3}; treasureMaxMs={4:F3}; bossDrawCalls={5}; bossMarkers={6}; bossAvgMs={7:F3}; bossMaxMs={8:F3}; moleDrawCalls={9}; moleMarkers={10}; moleAvgMs={11:F3}; moleMaxMs={12:F3}; worldStatusDrawCalls={13}; worldStatusAvgMs={14:F3}; worldStatusMaxMs={15:F3}; assaultDrawCalls={16}; assaultMarkers={17}; assaultAvgMs={18:F3}; assaultMaxMs={19:F3}",
                windowSeconds,
                _treasureDrawCalls,
                _treasureMarkersDrawn,
                Average(_treasureDrawTotalMs, _treasureDrawCalls),
                _treasureDrawMaxMs,
                _bossDrawCalls,
                _bossMarkersDrawn,
                Average(_bossDrawTotalMs, _bossDrawCalls),
                _bossDrawMaxMs,
                _moleDrawCalls,
                _moleMarkersDrawn,
                Average(_moleDrawTotalMs, _moleDrawCalls),
                _moleDrawMaxMs,
                _worldStatusDrawCalls,
                Average(_worldStatusDrawTotalMs, _worldStatusDrawCalls),
                _worldStatusDrawMaxMs,_assaultDrawCalls,_assaultMarkersDrawn,Average(_assaultDrawTotalMs,_assaultDrawCalls),_assaultDrawMaxMs));
            ErrorLog.WriteDebug(String.Format(
                CultureInfo.InvariantCulture,
                "OVERLAY_WORK_PERF windowSec={0:F3}; visualHz={1:F3}; invalidateHz={2:F3}; paintHz={3:F3}; refreshDutyPct={4:F3}; paintDutyPct={5:F3}; composedMPixelsPerSecEstimate={6:F3}; windowVisibilitySampleHz={7:F3}; windowVisibilitySamples={8}; worldMapPresentationHz={9:F3}; worldMapTimerTicks={10}; modeTransitions={11}; timerResolutionAcquireAttempts={12}; timerResolutionAcquireSuccesses={13}; timerResolutionReleaseAttempts={14}; timerResolutionReleaseSuccesses={15}; timerResolutionBalance={16}; estimateNote=window_pixels_times_overlay_paints_not_game_present_cost",
                windowSeconds,
                _motionVisualFrames / windowSeconds,
                _invalidates / windowSeconds,
                _paints / windowSeconds,
                _refreshTotalMs * 0.1 / windowSeconds,
                _paintTotalMs * 0.1 / windowSeconds,
                overlayPixels * (_paints / windowSeconds) / 1000000.0,
                _windowVisibilitySamples / windowSeconds,
                _windowVisibilitySamples,
                _worldMapTimerTicks / windowSeconds,
                _worldMapTimerTicks,
                _modeTransitions,
                _timerResolutionAcquireAttempts,
                _timerResolutionAcquireSuccesses,
                _timerResolutionReleaseAttempts,
                _timerResolutionReleaseSuccesses,
                _timerResolutionBalance));
            ErrorLog.WriteDebug(String.Format(
                CultureInfo.InvariantCulture,
                "OVERLAY_MOTION_PREDICTION windowSec={0:F3}; predictionHz={1:F3}; ticks={2}; ageAvgMs={3:F3}; ageMaxMs={4:F3}; clamps={5}; resets={6}; staleFreezes={7}; maxPredictionMs=250; staleFreezeMs=500; source=uobject_free_scalar_history",
                windowSeconds,
                _predictionTicks / windowSeconds,
                _predictionTicks,
                _predictionTicks > 0
                    ? _predictionAgeTotalMs / _predictionTicks
                    : 0.0,
                _predictionAgeMaxMs,
                _predictionClamps,
                _predictionResets,
                _predictionStaleFreezes));

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
            _worldMapTimerTicks = 0;
            _modeTransitions = 0;
            _timerResolutionAcquireAttempts = 0;
            _timerResolutionAcquireSuccesses = 0;
            _timerResolutionReleaseAttempts = 0;
            _timerResolutionReleaseSuccesses = 0;
            _paints = 0;
            _motionFrames = 0;
            _motionVisualFrames = 0;
            _motionSuppressedFrames = 0;
            _predictionTicks = 0;
            _predictionClamps = 0;
            _predictionResets = 0;
            _predictionStaleFreezes = 0;
            _predictionAgeTotalMs = 0.0;
            _predictionAgeMaxMs = 0.0;
            _invalidates = 0;
            _windowVisibilitySamples = 0;
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
            _treasureDrawCalls = 0;
            _treasureMarkersDrawn = 0;
            _treasureDrawTotalMs = 0.0;
            _treasureDrawMaxMs = 0.0;
            _bossDrawCalls = 0;
            _bossMarkersDrawn = 0;
            _bossDrawTotalMs = 0.0;
            _bossDrawMaxMs = 0.0;
            _assaultDrawCalls=0;_assaultMarkersDrawn=0;_assaultDrawTotalMs=0.0;_assaultDrawMaxMs=0.0;
            _moleDrawCalls = 0;
            _moleMarkersDrawn = 0;
            _moleDrawTotalMs = 0.0;
            _moleDrawMaxMs = 0.0;
            _worldStatusDrawCalls = 0;
            _worldStatusDrawTotalMs = 0.0;
            _worldStatusDrawMaxMs = 0.0;
        }
    }
}
