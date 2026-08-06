using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Web.Script.Serialization;
using System.Windows.Forms;

namespace DragonSwordWorldRadar
{
    internal sealed class RadarForm : Form
    {
        private const float ReferenceWindowWidth = 2560f;
        private const float ReferenceWindowHeight = 1440f;
        private const int ReferenceOverlaySize = 360;
        private const int ReferenceRightMargin = 40;
        private const int ReferenceTopMargin = 37;
        private const float ReferenceRadarRadius = 170f;
        private const int ActiveTimerIntervalMs = 24;
        private const int WorldIdleTimerIntervalMs = 50;
        private const int RadarIdleTimerIntervalMs = 75;
        private const int DisabledTimerIntervalMs = 125;
        private const int BackgroundTimerIntervalMs = 500;
        private const int StaticStatePollIntervalMs = 200;
        private const int MaintenanceIntervalMs = 500;
        private const int GeometryCheckIntervalMs = 1000;
        private const int GameLifetimeCheckIntervalMs = 1000;
        private const int MotionActivityWindowMs = 300;
        private const int MotionStaleTimeoutMs = 2500;

        // Height indicators use a continuous 180-degree scale. The player
        // actor origin sits above the treasure reference point, so the
        // player Z value is shifted before comparison. Near-equal heights
        // keep a horizontal pointer instead of hiding the indicator.
        private const double ComparablePlayerZOffset = -150.0;

        private readonly Timer _timer;
        private readonly JavaScriptSerializer _serializer;
        private readonly StaticStateBridgeReader _staticStateBridge;
        private readonly MotionBridgeReader _motionBridge;
        private readonly NotifyIcon _trayIcon;
        private readonly ContextMenuStrip _trayMenu;
        private readonly TreasureSaveState _saveState;
        private readonly WorldTreasureCatalog _worldTreasures;
        private readonly Brush _otherTreasureBrush;
        private readonly Brush _miniGameTreasureBrush;
        private readonly Brush _mapTreasureBrush;
        private readonly Brush _puzzleTreasureBrush;
        private readonly Pen _treasureOutlinePen;
        private readonly bool _highResolutionTimerEnabled;
        private readonly OverlayPerformanceTracker _performance;
        private readonly BossAvailabilityTracker _bossAvailability;
        private readonly WorldTreasureVisibilityIndex _worldTreasureIndex;
        private readonly WorldTreasureRenderBuffer _worldTreasureRenderBuffer;
        private readonly BossMarkerRenderer _bossMarkerRenderer;
        private readonly HeightIndicatorRenderer _heightIndicatorRenderer;
        private readonly Dictionary<string, string> _moduleFailureSignatures =
            new Dictionary<string, string>();
        private readonly Dictionary<string, DateTime> _nextModuleFailureLogUtc =
            new Dictionary<string, DateTime>();

        private RadarState _state;
        private MotionFrame _motion;
        private readonly MotionVisualSnapshot _motionVisualSnapshot =
            new MotionVisualSnapshot();
        private DateTime _stateMissingSinceUtc;
        private DateTime _nextStaticStatePollUtc;
        private DateTime _nextMaintenanceUtc;
        private DateTime _nextGeometryCheckUtc;
        private DateTime _lastMotionVisualChangeUtc;
        private DateTime _lastMotionFrameUtc;
        private DateTime _lastModeTransitionUtc = DateTime.UtcNow;
        private float _displayScale = 1f;
        private int _overlaySize = ReferenceOverlaySize;
        private string _lastGeometryLog;
        private string _lastSaveFilterLog;
        private DateTime _nextSaveFilterLogUtc;
        private int _gameProcessId;
        private IntPtr _gameWindowHandle;
        private bool _overlaySuppressed;
        private string _overlaySuppressionReason = "initial";
        private bool _hasSeenGameProcess;
        private DateTime _gameProcessMissingSinceUtc;
        private DateTime _nextGameLifetimeCheckUtc;

        public RadarForm()
        {
            string bridgeDirectory = Path.Combine(
                ModPath.BaseDirectory,
                "runtime",
                "bridge");
            _serializer = new JavaScriptSerializer();
            _staticStateBridge = new StaticStateBridgeReader(
                bridgeDirectory,
                _serializer);
            _motionBridge = new MotionBridgeReader(
                bridgeDirectory);
            _saveState = new TreasureSaveState();
            _worldTreasures = new WorldTreasureCatalog();
            _performance = new OverlayPerformanceTracker();
            _bossAvailability = new BossAvailabilityTracker();
            _worldTreasureIndex = new WorldTreasureVisibilityIndex();
            _worldTreasureRenderBuffer =
                new WorldTreasureRenderBuffer();
            _bossMarkerRenderer = new BossMarkerRenderer();
            _heightIndicatorRenderer = new HeightIndicatorRenderer();
            _otherTreasureBrush = new SolidBrush(
                RadarMarkerStyle.TreasureOther);
            _miniGameTreasureBrush = new SolidBrush(
                RadarMarkerStyle.TreasureMiniGame);
            _mapTreasureBrush = new SolidBrush(
                RadarMarkerStyle.TreasureMap);
            _puzzleTreasureBrush = new SolidBrush(
                RadarMarkerStyle.TreasurePuzzle);

            Rectangle primaryBounds = Screen.PrimaryScreen.Bounds;
            UpdateGeometry(
                primaryBounds.Width,
                primaryBounds.Height);
            _treasureOutlinePen = CreateTreasureOutlinePen();
            ConfigureWindow();
            bool highResolutionTimerEnabled = false;
            try
            {
                highResolutionTimerEnabled =
                    NativeMethods.timeBeginPeriod(1) == 0;
            }
            catch
            {
                // The overlay remains functional with the default Windows
                // timer resolution if winmm is unavailable.
            }
            _highResolutionTimerEnabled =
                highResolutionTimerEnabled;

            _trayMenu = CreateTrayMenu();
            _trayIcon = new NotifyIcon
            {
                Icon = SystemIcons.Application,
                Text = "DragonSwordWorldRadar",
                ContextMenuStrip = _trayMenu,
                Visible = true
            };

            _timer = new Timer
            {
                Interval = 100
            };
            _timer.Tick += OnTimerTick;
            _timer.Start();
        }

        protected override bool ShowWithoutActivation
        {
            get { return true; }
        }

        protected override CreateParams CreateParams
        {
            get
            {
                CreateParams parameters = base.CreateParams;
                parameters.ExStyle |=
                    NativeMethods.WsExTransparent |
                    NativeMethods.WsExToolWindow |
                    NativeMethods.WsExLayered |
                    NativeMethods.WsExNoActivate;
                return parameters;
            }
        }

        protected override void OnFormClosed(
            FormClosedEventArgs eventArgs)
        {
            _timer.Stop();
            _timer.Tick -= OnTimerTick;
            _timer.Dispose();
            _trayIcon.Visible = false;
            _trayIcon.Dispose();
            _trayMenu.Dispose();
            _otherTreasureBrush.Dispose();
            _miniGameTreasureBrush.Dispose();
            _mapTreasureBrush.Dispose();
            _puzzleTreasureBrush.Dispose();
            _treasureOutlinePen.Dispose();
            _worldTreasureRenderBuffer.Dispose();
            _bossMarkerRenderer.Dispose();
            _heightIndicatorRenderer.Dispose();
            if (_highResolutionTimerEnabled)
            {
                try
                {
                    NativeMethods.timeEndPeriod(1);
                }
                catch
                {
                    // Resource cleanup and FormClosed propagation must still
                    // complete if winmm is unavailable during shutdown.
                }
            }
            base.OnFormClosed(eventArgs);
        }

        protected override void OnPaint(PaintEventArgs eventArgs)
        {
            base.OnPaint(eventArgs);
            long paintStarted = Stopwatch.GetTimestamp();
            bool paintFailed = false;
            try
            {
                RadarState state = _state;
                MotionFrame motion = GetCompatibleMotion(state);
                if (state == null
                    || !state.enabled
                    || (motion != null && !motion.Enabled))
                {
                    return;
                }

                eventArgs.Graphics.SmoothingMode =
                    SmoothingMode.AntiAlias;
                bool showDebugCoordinates = DebugSettings.Enabled;
                float textScale = NormalizeTextScale(state.textScale);
                string mode = motion == null
                    ? state.mode
                    : motion.Mode;
                double playerX = motion == null
                    ? state.playerX
                    : motion.PlayerX;
                double playerY = motion == null
                    ? state.playerY
                    : motion.PlayerY;
                double playerZ = motion == null
                    ? state.playerZ
                    : motion.PlayerZ;
                bool hasPlayerZ = motion == null
                    ? state.hasPlayerZ
                    : motion.HasPlayerZ;
                double radius = motion == null
                    || motion.Radius <= 0
                    ? state.radius
                    : motion.Radius;

                if (String.Equals(
                    mode,
                    "world",
                    StringComparison.Ordinal))
                {
                    WorldMapState map = motion != null
                        && motion.WorldMap != null
                        ? motion.WorldMap
                        : state.worldMap;
                    if (map != null)
                    {
                        DrawWorldMap(
                            eventArgs.Graphics,
                            map,
                            state.showHeight,
                            state.showTreasureTypes,
                            state.showTreasures,
                            showDebugCoordinates,
                            textScale,
                            playerZ,
                            hasPlayerZ,
                            state.showBosses,
                            state.bosses);
                    }
                    return;
                }
                if (radius <= 0)
                {
                    return;
                }

                float center = _overlaySize / 2f;
                float radarRadius =
                    ReferenceRadarRadius * _displayScale;
                IList<RadarPoint> points = state.points;
                if (points == null)
                {
                    points = Array.Empty<RadarPoint>();
                }
                RadarPoint nearest = FindNearestRadarPoint(
                    points,
                    playerX,
                    playerY,
                    playerZ,
                    hasPlayerZ,
                    radius);

                if (state.showBosses)
                {
                    IList<BossPoint> bosses = state.bosses;
                    if (bosses == null)
                    {
                        bosses = Array.Empty<BossPoint>();
                    }
                    foreach (BossPoint boss in bosses)
                    {
                        if (boss == null
                            || !boss.visible
                            || !_bossAvailability.IsAvailable(boss.bossId))
                        {
                            continue;
                        }
                        DrawBossPoint(
                            eventArgs.Graphics,
                            boss,
                            playerX,
                            playerY,
                            radius,
                            radarRadius,
                            center,
                            showDebugCoordinates,
                            textScale);
                    }
                }

                _worldTreasureRenderBuffer.Reset();
                double radiusSquared = radius * radius;
                float normalDiameter = 10f * _displayScale;
                float normalHalf = normalDiameter / 2f;
                for (int index = points.Count - 1;
                    index >= 0;
                    index--)
                {
                    RadarPoint point = points[index];
                    if (point == null
                        || _saveState.IsOpened(point.saveId))
                    {
                        continue;
                    }
                    double deltaX = point.x - playerX;
                    double deltaY = point.y - playerY;
                    if (deltaX * deltaX + deltaY * deltaY
                        > radiusSquared)
                    {
                        continue;
                    }
                    if (Object.ReferenceEquals(point, nearest))
                    {
                        continue;
                    }

                    float x = center +
                        (float)(deltaX / radius * radarRadius);
                    float y = center +
                        (float)(deltaY / radius * radarRadius);
                    WorldTreasure metadata =
                        _worldTreasures.FindBySaveIdAndCoordinates(
                            point.saveId,
                            point.x,
                            point.y);
                    _worldTreasureRenderBuffer.AddMarker(
                        metadata == null
                            ? TreasureKind.Other
                            : metadata.Kind,
                        new RectangleF(
                            x - normalHalf,
                            y - normalHalf,
                            normalDiameter,
                            normalDiameter));
                }

                DrawBatchedTreasurePaths(eventArgs.Graphics);

                if (nearest != null
                    && !_saveState.IsOpened(nearest.saveId))
                {
                    DrawPoint(
                        eventArgs.Graphics,
                        nearest,
                        playerX,
                        playerY,
                        radius,
                        radarRadius,
                        center,
                        true,
                        state.showHeight,
                        state.showTreasureTypes,
                        showDebugCoordinates,
                        textScale,
                        playerZ,
                        hasPlayerZ);
                }
            }
            catch (Exception exception)
            {
                paintFailed = true;
                ReportModuleFailure("renderer", exception);
            }
            finally
            {
                if (!paintFailed)
                {
                    ClearModuleFailure("renderer");
                }
                _performance.RecordPaint(paintStarted);
            }
        }

        private RadarPoint FindNearestRadarPoint(
            IList<RadarPoint> points,
            double playerX,
            double playerY,
            double playerZ,
            bool hasPlayerZ,
            double radius)
        {
            RadarPoint nearest = null;
            double nearestDistance = Double.MaxValue;
            double radiusSquared = radius * radius;
            foreach (RadarPoint point in points)
            {
                if (point == null
                    || _saveState.IsOpened(point.saveId))
                {
                    continue;
                }
                double deltaX = point.x - playerX;
                double deltaY = point.y - playerY;
                double planar = deltaX * deltaX + deltaY * deltaY;
                if (planar > radiusSquared)
                {
                    continue;
                }
                double deltaZ = 0.0;
                if (point.hasZ && hasPlayerZ)
                {
                    deltaZ = point.z - GetComparablePlayerZ(playerZ);
                }
                double distance = planar + deltaZ * deltaZ;
                if (distance < nearestDistance)
                {
                    nearestDistance = distance;
                    nearest = point;
                }
            }
            return nearest;
        }

        private void DrawWorldMap(
            Graphics graphics,
            WorldMapState map,
            bool showHeight,
            bool showTreasureTypes,
            bool showTreasures,
            bool showDebugCoordinates,
            float textScale,
            double playerZ,
            bool hasPlayerZ,
            bool showBosses,
            IList<BossPoint> bosses)
        {
            if (map.dimensions <= 0
                || map.uiSize <= 0
                || map.zoom <= 0
                || map.viewportWidth <= 0
                || map.viewportHeight <= 0
                || map.viewportScale <= 0)
            {
                return;
            }

            bool showLabel =
                showDebugCoordinates || showTreasureTypes;
            float windowScaleX =
                ClientSize.Width / (float)map.viewportWidth;
            float windowScaleY =
                ClientSize.Height / (float)map.viewportHeight;
            float coordinateScale = (float)map.viewportScale;
            RectangleF clip = new RectangleF(
                0,
                0,
                ClientSize.Width,
                ClientSize.Height);
            GraphicsState saved = graphics.Save();
            try
            {
                graphics.SetClip(clip);
                IList<WorldTreasure> mapTreasures =
                    _worldTreasureIndex.GetMap(map.mapId);

                if (showBosses)
                {
                    IList<BossPoint> visibleBosses = bosses;
                    if (visibleBosses == null)
                    {
                        visibleBosses = Array.Empty<BossPoint>();
                    }

                    foreach (BossPoint boss in visibleBosses)
                    {
                        if (boss == null
                            || !boss.visible
                            || !_bossAvailability.IsAvailable(boss.bossId)
                            || boss.mapId != map.mapId)
                        {
                            continue;
                        }

                        float bossX;
                        float bossY;
                        float bossDiameter = Math.Max(
                            44f,
                            54f * _displayScale);
                        if (!TryProjectWorldPoint(
                            boss.x,
                            boss.y,
                            map,
                            coordinateScale,
                            windowScaleX,
                            windowScaleY,
                            bossDiameter / 2f,
                            out bossX,
                            out bossY))
                        {
                            continue;
                        }

                        _bossMarkerRenderer.DrawMarker(
                            graphics,
                            bossX,
                            bossY,
                            bossDiameter,
                            _displayScale);
                        if (showDebugCoordinates)
                        {
                            _bossMarkerRenderer.DrawDebugLabel(
                                graphics,
                                boss,
                                bossX,
                                bossY,
                                bossDiameter,
                                textScale,
                                _displayScale);
                        }
                    }
                }

                if (showTreasures)
                {
                    float normalDiameter = Math.Max(
                        6f,
                        10f * _displayScale);
                    float nearestDiameter = Math.Max(
                        10f,
                        16f * _displayScale);
                    int nearestIndex = -1;
                    double nearestDistanceSquared = Double.MaxValue;

                    // Project each visible treasure exactly once. The reusable
                    // buffer retains its list, hash set, and GraphicsPath
                    // capacity across world-map frames, avoiding high-frequency
                    // managed and GDI allocations while the map is moving.
                    _worldTreasureRenderBuffer.Reset();
                    foreach (WorldTreasure treasure in mapTreasures)
                    {
                        float projectedX;
                        float projectedY;
                        if (!TryProjectWorldTreasure(
                            treasure,
                            map,
                            coordinateScale,
                            windowScaleX,
                            windowScaleY,
                            normalDiameter / 2f,
                            out projectedX,
                            out projectedY))
                        {
                            continue;
                        }

                        double deltaX =
                            treasure.X - map.playerWorldX;
                        double deltaY =
                            treasure.Y - map.playerWorldY;
                        double deltaZ = 0.0;
                        if (hasPlayerZ && treasure.HasZ)
                        {
                            deltaZ = treasure.Z -
                                GetComparablePlayerZ(playerZ);
                        }
                        double distanceSquared =
                            deltaX * deltaX +
                            deltaY * deltaY +
                            deltaZ * deltaZ;
                        int projectedIndex =
                            _worldTreasureRenderBuffer.Add(
                                treasure,
                                projectedX,
                                projectedY);
                        if (distanceSquared < nearestDistanceSquared)
                        {
                            nearestDistanceSquared = distanceSquared;
                            nearestIndex = projectedIndex;
                        }
                    }

                    if (nearestIndex >= 0)
                    {
                        ProjectedWorldTreasure nearest =
                            _worldTreasureRenderBuffer[nearestIndex];
                        _worldTreasureRenderBuffer.ReservePixel(
                            GetProjectedMarkerPixelKey(
                                nearest.X,
                                nearest.Y));
                    }

                    float half = normalDiameter / 2f;
                    for (int index = 0;
                        index < _worldTreasureRenderBuffer.Count;
                        index++)
                    {
                        if (index == nearestIndex)
                        {
                            continue;
                        }

                        ProjectedWorldTreasure projected =
                            _worldTreasureRenderBuffer[index];
                        long projectedPixel =
                            GetProjectedMarkerPixelKey(
                                projected.X,
                                projected.Y);
                        if (!_worldTreasureRenderBuffer.ReservePixel(
                            projectedPixel))
                        {
                            continue;
                        }

                        _worldTreasureRenderBuffer.AddMarker(
                            projected.Treasure.Kind,
                            new RectangleF(
                                projected.X - half,
                                projected.Y - half,
                                normalDiameter,
                                normalDiameter));
                    }

                    DrawBatchedTreasurePaths(graphics);

                    if (nearestIndex >= 0)
                    {
                        ProjectedWorldTreasure nearest =
                            _worldTreasureRenderBuffer[nearestIndex];
                        WorldTreasure nearestTreasure =
                            nearest.Treasure;
                        DrawTreasureMarker(
                            graphics,
                            nearest.X,
                            nearest.Y,
                            nearestDiameter,
                            GetTreasureBrush(nearestTreasure),
                            _treasureOutlinePen);

                        Color nearestColor =
                            GetTreasureColor(nearestTreasure);
                        if (showHeight)
                        {
                            _heightIndicatorRenderer.Draw(
                                graphics,
                                nearest.X,
                                nearest.Y,
                                nearestDiameter,
                                playerZ,
                                hasPlayerZ,
                                nearestTreasure.Z,
                                nearestTreasure.HasZ,
                                nearestColor,
                                _displayScale,
                                ComparablePlayerZOffset);
                        }
                        if (showLabel)
                        {
                            DrawNearestLabel(
                                graphics,
                                nearest.X,
                                nearest.Y,
                                nearestDiameter,
                                textScale,
                                showDebugCoordinates,
                                showTreasureTypes,
                                playerZ,
                                hasPlayerZ,
                                nearestTreasure.Z,
                                nearestTreasure.HasZ,
                                nearestTreasure.SaveId,
                                nearestTreasure);
                        }
                    }
                }
            }
            finally
            {
                graphics.Restore(saved);
            }
        }

        private bool TryProjectWorldTreasure(
            WorldTreasure treasure,
            WorldMapState map,
            float coordinateScale,
            float windowScaleX,
            float windowScaleY,
            float margin,
            out float x,
            out float y)
        {
            return TryProjectWorldPoint(
                treasure.X,
                treasure.Y,
                map,
                coordinateScale,
                windowScaleX,
                windowScaleY,
                margin,
                out x,
                out y);
        }

        private bool TryProjectWorldPoint(
            double worldX,
            double worldY,
            WorldMapState map,
            float coordinateScale,
            float windowScaleX,
            float windowScaleY,
            float margin,
            out float x,
            out float y)
        {
            double localX = map.playerMapX
                + (worldX - map.playerWorldX)
                / map.dimensions * map.uiSize;
            double localY = map.playerMapY
                + (worldY - map.playerWorldY)
                / map.dimensions * map.uiSize;
            x = (float)(
                (map.left + localX * map.zoom)
                * coordinateScale * windowScaleX);
            y = (float)(
                (map.top + localY * map.zoom)
                * coordinateScale * windowScaleY);

            return x >= -margin
                && x <= ClientSize.Width + margin
                && y >= -margin
                && y <= ClientSize.Height + margin;
        }

        private void ConfigureWindow()
        {
            AutoScaleMode = AutoScaleMode.None;
            FormBorderStyle = FormBorderStyle.None;
            ShowInTaskbar = false;
            TopMost = true;
            BackColor = Color.Magenta;
            TransparencyKey = Color.Magenta;
            DoubleBuffered = true;
        }

        private ContextMenuStrip CreateTrayMenu()
        {
            ContextMenuStrip menu = new ContextMenuStrip();
            ToolStripMenuItem exitItem = new ToolStripMenuItem(
                "Exit DragonSwordWorldRadar");
            exitItem.Click += delegate
            {
                _trayIcon.Visible = false;
                Close();
                Application.Exit();
            };
            menu.Items.Add(exitItem);
            return menu;
        }

        private void OnTimerTick(
            object sender,
            EventArgs eventArgs)
        {
            long refreshStarted = Stopwatch.GetTimestamp();
            try
            {
                CheckGameLifetime();
                if (IsDisposed || Disposing)
                {
                    return;
                }
                RefreshState();
                UpdateOverlayVisibility();
                ClearModuleFailure("timer");
            }
            catch (Exception exception)
            {
                ReportModuleFailure(
                    "timer",
                    exception);
            }
            finally
            {
                _performance.RecordTimerTick(refreshStarted);
                _performance.LogIfDue(
                    GetEffectiveMode(),
                    _timer.Interval,
                    _worldTreasureIndex.Count);
            }
        }

        private void DrawBossPoint(
            Graphics graphics,
            BossPoint boss,
            double playerX,
            double playerY,
            double stateRadius,
            float radarRadius,
            float center,
            bool showDebugCoordinates,
            float textScale)
        {
            float x = center +
                (float)((boss.x - playerX)
                    / stateRadius * radarRadius);
            float y = center +
                (float)((boss.y - playerY)
                    / stateRadius * radarRadius);
            float diameter = Math.Max(
                38f,
                46f * _displayScale);
            _bossMarkerRenderer.DrawMarker(
                graphics,
                x,
                y,
                diameter,
                _displayScale);
            if (showDebugCoordinates)
            {
                _bossMarkerRenderer.DrawDebugLabel(
                    graphics,
                    boss,
                    x,
                    y,
                    diameter,
                    textScale,
                    _displayScale);
            }
        }

        private void DrawPoint(
            Graphics graphics,
            RadarPoint point,
            double playerX,
            double playerY,
            double stateRadius,
            float radarRadius,
            float center,
            bool nearest,
            bool showHeight,
            bool showTreasureTypes,
            bool showDebugCoordinates,
            float textScale,
            double playerZ,
            bool hasPlayerZ)
        {
            float x = center +
                (float)((point.x - playerX)
                    / stateRadius * radarRadius);
            float y = center +
                (float)((point.y - playerY)
                    / stateRadius * radarRadius);
            float diameter =
                (nearest ? 16f : 10f) * _displayScale;
            WorldTreasure metadata =
                _worldTreasures.FindBySaveIdAndCoordinates(
                    point.saveId,
                    point.x,
                    point.y);
            Color color = GetTreasureColor(metadata);

            DrawTreasureMarker(
                graphics,
                x,
                y,
                diameter,
                GetTreasureBrush(metadata),
                _treasureOutlinePen);

            if (nearest)
            {
                if (showHeight)
                {
                    _heightIndicatorRenderer.Draw(
                        graphics,
                        x,
                        y,
                        diameter,
                        playerZ,
                        hasPlayerZ,
                        point.z,
                        point.hasZ,
                        color,
                        _displayScale,
                        ComparablePlayerZOffset);
                }
                if (showDebugCoordinates || showTreasureTypes)
                {
                    DrawNearestLabel(
                        graphics,
                        x,
                        y,
                        diameter,
                        textScale,
                        showDebugCoordinates,
                        showTreasureTypes,
                        playerZ,
                        hasPlayerZ,
                        point.z,
                        point.hasZ,
                        point.saveId,
                        metadata);
                }
            }
        }

        // The nearest marker keeps its acquisition-type color while using
        // the larger nearest-marker diameter for visibility.
        private void DrawTreasureMarker(
            Graphics graphics,
            float centerX,
            float centerY,
            float diameter,
            Brush brush,
            Pen outline)
        {
            float half = diameter / 2f;
            SmoothingMode previousSmoothing = graphics.SmoothingMode;
            graphics.SmoothingMode = SmoothingMode.None;
            try
            {
                graphics.FillEllipse(
                    brush,
                    centerX - half,
                    centerY - half,
                    diameter,
                    diameter);
                graphics.DrawEllipse(
                    outline,
                    centerX - half,
                    centerY - half,
                    diameter,
                    diameter);
            }
            finally
            {
                graphics.SmoothingMode = previousSmoothing;
            }
        }

        private Pen CreateTreasureOutlinePen()
        {
            // Every treasure type uses the same neutral outline. A single retained
            // pen is sufficient because world-map paths are drawn sequentially.
            return new Pen(
                RadarMarkerStyle.Outline,
                RadarMarkerStyle.GetTreasureOutlineWidth(_displayScale));
        }

        private static long GetProjectedMarkerPixelKey(
            float x,
            float y)
        {
            int pixelX = (int)Math.Round(
                x,
                MidpointRounding.AwayFromZero);
            int pixelY = (int)Math.Round(
                y,
                MidpointRounding.AwayFromZero);
            return ((long)pixelX << 32) ^ (uint)pixelY;
        }

        private void DrawBatchedTreasurePaths(Graphics graphics)
        {
            SmoothingMode previousSmoothing = graphics.SmoothingMode;
            graphics.SmoothingMode = SmoothingMode.None;
            try
            {
                DrawTreasurePathCore(
                    graphics,
                    _worldTreasureRenderBuffer.OtherPath,
                    _otherTreasureBrush,
                    _treasureOutlinePen);
                DrawTreasurePathCore(
                    graphics,
                    _worldTreasureRenderBuffer.MiniGamePath,
                    _miniGameTreasureBrush,
                    _treasureOutlinePen);
                DrawTreasurePathCore(
                    graphics,
                    _worldTreasureRenderBuffer.MapPath,
                    _mapTreasureBrush,
                    _treasureOutlinePen);
                DrawTreasurePathCore(
                    graphics,
                    _worldTreasureRenderBuffer.PuzzlePath,
                    _puzzleTreasureBrush,
                    _treasureOutlinePen);
            }
            finally
            {
                graphics.SmoothingMode = previousSmoothing;
            }
        }

        private static void DrawTreasurePathCore(
            Graphics graphics,
            GraphicsPath path,
            Brush brush,
            Pen outline)
        {
            if (path == null || path.PointCount <= 0)
            {
                return;
            }
            graphics.FillPath(brush, path);
            graphics.DrawPath(outline, path);
        }

        private void UpdateTreasureOutlinePenWidths()
        {
            if (_treasureOutlinePen != null)
            {
                _treasureOutlinePen.Width =
                    RadarMarkerStyle.GetTreasureOutlineWidth(
                        _displayScale);
            }
        }

        private Brush GetTreasureBrush(
            WorldTreasure treasure)
        {
            TreasureKind kind = treasure == null
                ? TreasureKind.Other
                : treasure.Kind;
            if (kind == TreasureKind.MiniGame)
            {
                return _miniGameTreasureBrush;
            }
            if (kind == TreasureKind.Map)
            {
                return _mapTreasureBrush;
            }
            if (kind == TreasureKind.PressurePuzzle
                || kind == TreasureKind.StatuePuzzle)
            {
                return _puzzleTreasureBrush;
            }
            return _otherTreasureBrush;
        }

        // show_height controls the retained height pointer renderer. Exact Z
        // values are displayed only while debug_logging is enabled, and
        // treasure type labels remain independently configurable.

        private void DrawNearestLabel(
            Graphics graphics,
            float anchorX,
            float anchorY,
            float markerDiameter,
            float textScale,
            bool showDebugCoordinates,
            bool showTreasureTypes,
            double playerZ,
            bool hasPlayerZ,
            double treasureZ,
            bool hasTreasureZ,
            long saveId,
            WorldTreasure metadata)
        {
            string coordinateText = null;
            if (showDebugCoordinates)
            {
                string playerText = hasPlayerZ
                    ? GetComparablePlayerZ(playerZ).ToString(
                        "0",
                        CultureInfo.InvariantCulture)
                    : "?";
                string treasureText = hasTreasureZ
                    ? treasureZ.ToString(
                        "0",
                        CultureInfo.InvariantCulture)
                    : "?";
                coordinateText = string.Format(
                    CultureInfo.InvariantCulture,
                    "({0}, {1})",
                    playerText,
                    treasureText);
            }

            string typeText = null;
            if (showTreasureTypes)
            {
                typeText = metadata == null
                    ? TreasureIdentity.GetDebugName(null, saveId)
                    : metadata.DebugName;
            }

            if (String.IsNullOrEmpty(typeText)
                && String.IsNullOrEmpty(coordinateText))
            {
                return;
            }

            Color baseColor = GetTreasureColor(metadata);
            Color typeColor = Color.FromArgb(
                210,
                baseColor.R,
                baseColor.G,
                baseColor.B);
            Color coordinateColor = Color.FromArgb(
                200,
                baseColor.R,
                baseColor.G,
                baseColor.B);

            // Pixel units prevent Windows DPI scaling from changing label
            // size unexpectedly. text_scale provides one user-facing control
            // for both treasure type and debug coordinate labels.
            float baseFontSize = Math.Min(
                26f,
                Math.Max(16f, 18f * _displayScale));
            float fontSize = baseFontSize * textScale;
            float verticalGap = Math.Min(
                8f,
                Math.Max(4f, 5f * _displayScale));
            float lineGap = Math.Min(
                4f,
                RadarMarkerStyle.GetTreasureOutlineWidth(_displayScale));

            using (Font font = new Font(
                SystemFonts.MessageBoxFont.FontFamily,
                fontSize,
                FontStyle.Bold,
                GraphicsUnit.Pixel))
            {
                SizeF typeSize = String.IsNullOrEmpty(typeText)
                    ? SizeF.Empty
                    : graphics.MeasureString(typeText, font);
                SizeF coordinateSize =
                    String.IsNullOrEmpty(coordinateText)
                        ? SizeF.Empty
                        : graphics.MeasureString(
                            coordinateText,
                            font);
                float blockHeight = typeSize.Height
                    + coordinateSize.Height;
                if (!typeSize.IsEmpty && !coordinateSize.IsEmpty)
                {
                    blockHeight += lineGap;
                }

                float top = anchorY
                    + markerDiameter / 2f
                    + verticalGap;
                if (top + blockHeight > ClientSize.Height)
                {
                    top = anchorY
                        - markerDiameter / 2f
                        - verticalGap
                        - blockHeight;
                }
                if (top < 0f)
                {
                    top = 0f;
                }

                float outlineOffset = Math.Min(
                    2.5f,
                    Math.Max(1.25f, 1.5f * _displayScale));
                if (!String.IsNullOrEmpty(typeText))
                {
                    DrawOutlinedLabelLine(
                        graphics,
                        typeText,
                        font,
                        typeColor,
                        anchorX,
                        top,
                        typeSize,
                        outlineOffset);
                    top += typeSize.Height;
                    if (!String.IsNullOrEmpty(coordinateText))
                    {
                        top += lineGap;
                    }
                }
                if (!String.IsNullOrEmpty(coordinateText))
                {
                    DrawOutlinedLabelLine(
                        graphics,
                        coordinateText,
                        font,
                        coordinateColor,
                        anchorX,
                        top,
                        coordinateSize,
                        outlineOffset);
                }
            }
        }

        private void DrawOutlinedLabelLine(
            Graphics graphics,
            string text,
            Font font,
            Color textColor,
            float centerX,
            float top,
            SizeF measured,
            float outlineOffset)
        {
            float left = centerX - measured.Width / 2f;
            if (left < 0f)
            {
                left = 0f;
            }
            if (left + measured.Width > ClientSize.Width)
            {
                left = ClientSize.Width - measured.Width;
            }

            using (Brush outlineBrush = new SolidBrush(
                Color.FromArgb(220, 0, 0, 0)))
            using (Brush textBrush = new SolidBrush(textColor))
            {
                graphics.DrawString(
                    text,
                    font,
                    outlineBrush,
                    left - outlineOffset,
                    top);
                graphics.DrawString(
                    text,
                    font,
                    outlineBrush,
                    left + outlineOffset,
                    top);
                graphics.DrawString(
                    text,
                    font,
                    outlineBrush,
                    left,
                    top - outlineOffset);
                graphics.DrawString(
                    text,
                    font,
                    outlineBrush,
                    left,
                    top + outlineOffset);
                graphics.DrawString(
                    text,
                    font,
                    textBrush,
                    left,
                    top);
            }
        }

        private static float NormalizeTextScale(double configuredScale)
        {
            if (Double.IsNaN(configuredScale)
                || Double.IsInfinity(configuredScale)
                || configuredScale <= 0)
            {
                return 1f;
            }

            return (float)Math.Max(
                0.5,
                Math.Min(2.0, configuredScale));
        }

        // Type colors intentionally describe acquisition mechanics, not rarity.
        // The marker fill and outline resolve through the same global palette.
        private static Color GetTreasureColor(
            WorldTreasure treasure)
        {
            TreasureKind kind = treasure == null
                ? TreasureKind.Other
                : treasure.Kind;
            return RadarMarkerStyle.GetTreasureFill(kind);
        }

        private void UpdateTimerInterval(DateTime now)
        {
            string mode = GetEffectiveMode();
            DateTime activityUtc = _lastMotionVisualChangeUtc;
            if (_lastModeTransitionUtc > activityUtc)
            {
                activityUtc = _lastModeTransitionUtc;
            }
            bool motionActive = activityUtc != DateTime.MinValue
                && now - activityUtc
                    <= TimeSpan.FromMilliseconds(
                        MotionActivityWindowMs);

            int interval;
            if (_overlaySuppressed)
            {
                interval = BackgroundTimerIntervalMs;
            }
            else if (String.Equals(
                    mode,
                    "world",
                    StringComparison.Ordinal))
            {
                interval = motionActive
                    ? ActiveTimerIntervalMs
                    : WorldIdleTimerIntervalMs;
            }
            else if (String.Equals(
                mode,
                "radar",
                StringComparison.Ordinal))
            {
                interval = motionActive
                    ? ActiveTimerIntervalMs
                    : RadarIdleTimerIntervalMs;
            }
            else
            {
                interval = DisabledTimerIntervalMs;
            }
            if (_timer.Interval != interval)
            {
                _timer.Interval = interval;
            }
        }

        private void RefreshState()
        {
            DateTime now = DateTime.UtcNow;
            bool redraw = false;
            bool geometryChanged = false;
            string previousMode = GetEffectiveMode();

            if (now >= _nextStaticStatePollUtc)
            {
                _nextStaticStatePollUtc = now.AddMilliseconds(
                    StaticStatePollIntervalMs);
                RadarState loaded;
                if (_staticStateBridge.TryReadLatest(out loaded))
                {
                    RadarState previousState = _state;
                    bool hasCompatibleMotion = _motion != null
                        && _motion.Generation == loaded.producerGeneration
                        && now - _lastMotionFrameUtc
                            <= TimeSpan.FromMilliseconds(
                                MotionStaleTimeoutMs);
                    bool staticVisualChange =
                        StaticStateRequiresRedraw(
                            previousState,
                            loaded,
                            !hasCompatibleMotion);
                    _performance.RecordStaticFrame(
                        staticVisualChange);
                    _state = loaded;
                    if (_motion != null
                        && _motion.Generation != loaded.producerGeneration)
                    {
                        _motion = null;
                        _lastMotionFrameUtc = DateTime.MinValue;
                        _motionVisualSnapshot.Reset();
                    }
                    _stateMissingSinceUtc = DateTime.MinValue;
                    redraw |= staticVisualChange;
                }
                else if (!_staticStateBridge.HasAnyFile)
                {
                    if (_stateMissingSinceUtc == DateTime.MinValue)
                    {
                        _stateMissingSinceUtc = now;
                    }
                    if (_state != null
                        && now - _stateMissingSinceUtc
                            >= TimeSpan.FromSeconds(1))
                    {
                        _state = null;
                        _motion = null;
                        _lastMotionFrameUtc = DateTime.MinValue;
                        _motionVisualSnapshot.Reset();
                        redraw = true;
                    }
                }
            }

            MotionFrame motion;
            if (_motionBridge.TryReadLatest(out motion))
            {
                bool motionVisualChange =
                    _motionVisualSnapshot.Update(motion);
                _performance.RecordMotionFrame(
                    motionVisualChange);
                _motion = motion;
                _lastMotionFrameUtc = now;
                if (motionVisualChange)
                {
                    _lastMotionVisualChangeUtc = now;
                }
                if (motionVisualChange
                    && (_state == null
                        || motion.Generation ==
                            _state.producerGeneration))
                {
                    redraw = true;
                }
            }

            if (now >= _nextMaintenanceUtc)
            {
                _nextMaintenanceUtc = now.AddMilliseconds(
                    MaintenanceIntervalMs);

                try
                {
                    _saveState.Refresh();
                    ClearModuleFailure("save-state");
                }
                catch (Exception exception)
                {
                    ReportModuleFailure("save-state", exception);
                }

                try
                {
                    _worldTreasures.Refresh();
                    ClearModuleFailure("world-treasure-catalog");
                }
                catch (Exception exception)
                {
                    ReportModuleFailure(
                        "world-treasure-catalog",
                        exception);
                }

                try
                {
                    if (_worldTreasureIndex.Refresh(
                            _worldTreasures,
                            _saveState))
                    {
                        redraw = true;
                    }
                    ClearModuleFailure("world-treasure-index");
                }
                catch (Exception exception)
                {
                    ReportModuleFailure(
                        "world-treasure-index",
                        exception);
                }

                try
                {
                    RadarState state = _state;
                    if (_bossAvailability.Refresh(
                            state == null ? null : state.bosses,
                            _saveState))
                    {
                        redraw = true;
                    }
                    ClearModuleFailure("boss-availability");
                }
                catch (Exception exception)
                {
                    ReportModuleFailure(
                        "boss-availability",
                        exception);
                }

                try
                {
                    LogSaveFilterStatus();
                    ClearModuleFailure("diagnostics");
                }
                catch (Exception exception)
                {
                    ReportModuleFailure("diagnostics", exception);
                }
            }

            string currentMode = GetEffectiveMode();
            geometryChanged = !String.Equals(
                previousMode,
                currentMode,
                StringComparison.Ordinal);
            if (geometryChanged)
            {
                _lastModeTransitionUtc = now;
            }
            if (geometryChanged
                || now >= _nextGeometryCheckUtc)
            {
                _nextGeometryCheckUtc = now.AddMilliseconds(
                    GeometryCheckIntervalMs);
                MoveOverGameWindow();
            }
            UpdateTimerInterval(now);
            if ((redraw || geometryChanged)
                && !_overlaySuppressed)
            {
                _performance.RecordInvalidate();
                Invalidate();
            }
        }

        private static bool StaticStateRequiresRedraw(
            RadarState previous,
            RadarState current,
            bool includeMotionFallback)
        {
            if (Object.ReferenceEquals(previous, current))
            {
                return false;
            }
            if (previous == null || current == null)
            {
                return true;
            }
            if (previous.producerGeneration != current.producerGeneration
                || previous.enabled != current.enabled
                || previous.showHeight != current.showHeight
                || previous.showTreasureTypes != current.showTreasureTypes
                || previous.showTreasures != current.showTreasures
                || previous.showBosses != current.showBosses
                || previous.textScale != current.textScale
                || previous.radius != current.radius
                || !String.Equals(
                    previous.mode,
                    current.mode,
                    StringComparison.Ordinal)
                || !RadarPointsEqual(previous.points, current.points)
                || !BossPointsEqual(previous.bosses, current.bosses))
            {
                return true;
            }

            if (!includeMotionFallback)
            {
                return false;
            }
            return previous.playerX != current.playerX
                || previous.playerY != current.playerY
                || previous.playerZ != current.playerZ
                || previous.hasPlayerZ != current.hasPlayerZ
                || !WorldMapsEqual(
                    previous.worldMap,
                    current.worldMap);
        }

        private static bool RadarPointsEqual(
            IList<RadarPoint> left,
            IList<RadarPoint> right)
        {
            int leftCount = left == null ? 0 : left.Count;
            int rightCount = right == null ? 0 : right.Count;
            if (leftCount != rightCount)
            {
                return false;
            }
            for (int index = 0; index < leftCount; index++)
            {
                RadarPoint leftPoint = left[index];
                RadarPoint rightPoint = right[index];
                if (Object.ReferenceEquals(leftPoint, rightPoint))
                {
                    continue;
                }
                if (leftPoint == null
                    || rightPoint == null
                    || leftPoint.saveId != rightPoint.saveId
                    || leftPoint.x != rightPoint.x
                    || leftPoint.y != rightPoint.y
                    || leftPoint.z != rightPoint.z
                    || leftPoint.hasZ != rightPoint.hasZ)
                {
                    return false;
                }
            }
            return true;
        }

        private static bool BossPointsEqual(
            IList<BossPoint> left,
            IList<BossPoint> right)
        {
            int leftCount = left == null ? 0 : left.Count;
            int rightCount = right == null ? 0 : right.Count;
            if (leftCount != rightCount)
            {
                return false;
            }
            for (int index = 0; index < leftCount; index++)
            {
                BossPoint leftPoint = left[index];
                BossPoint rightPoint = right[index];
                if (Object.ReferenceEquals(leftPoint, rightPoint))
                {
                    continue;
                }
                if (leftPoint == null
                    || rightPoint == null
                    || leftPoint.bossId != rightPoint.bossId
                    || leftPoint.mapId != rightPoint.mapId
                    || leftPoint.x != rightPoint.x
                    || leftPoint.y != rightPoint.y
                    || leftPoint.z != rightPoint.z
                    || leftPoint.hasZ != rightPoint.hasZ
                    || leftPoint.visible != rightPoint.visible
                    || !String.Equals(
                        leftPoint.status,
                        rightPoint.status,
                        StringComparison.Ordinal))
                {
                    return false;
                }
            }
            return true;
        }

        private static bool WorldMapsEqual(
            WorldMapState left,
            WorldMapState right)
        {
            if (Object.ReferenceEquals(left, right))
            {
                return true;
            }
            if (left == null || right == null)
            {
                return false;
            }
            return left.mapId == right.mapId
                && left.dimensions == right.dimensions
                && left.uiSize == right.uiSize
                && left.left == right.left
                && left.top == right.top
                && left.zoom == right.zoom
                && left.viewportWidth == right.viewportWidth
                && left.viewportHeight == right.viewportHeight
                && left.viewportScale == right.viewportScale
                && left.playerWorldX == right.playerWorldX
                && left.playerWorldY == right.playerWorldY
                && left.playerMapX == right.playerMapX
                && left.playerMapY == right.playerMapY;
        }

        private MotionFrame GetCompatibleMotion(
            RadarState state)
        {
            if (state == null
                || _motion == null
                || _motion.Generation != state.producerGeneration
                || DateTime.UtcNow - _lastMotionFrameUtc
                    > TimeSpan.FromMilliseconds(
                        MotionStaleTimeoutMs))
            {
                return null;
            }
            return _motion;
        }

        private string GetEffectiveMode()
        {
            RadarState state = _state;
            MotionFrame motion = GetCompatibleMotion(state);
            if (motion != null)
            {
                return motion.Enabled
                    ? motion.Mode ?? "disabled"
                    : "disabled";
            }
            if (state == null || !state.enabled)
            {
                return "disabled";
            }
            return state.mode ?? "radar";
        }

        private void ReportModuleFailure(
            string module,
            Exception exception)
        {
            DateTime now = DateTime.UtcNow;
            string signature = exception.GetType().FullName + ": " +
                exception.Message;
            string previousSignature;
            DateTime nextLogUtc;
            bool signatureChanged =
                !_moduleFailureSignatures.TryGetValue(
                    module,
                    out previousSignature)
                || previousSignature != signature;
            bool logDue =
                !_nextModuleFailureLogUtc.TryGetValue(
                    module,
                    out nextLogUtc)
                || now >= nextLogUtc;
            if (!signatureChanged && !logDue)
            {
                return;
            }

            _moduleFailureSignatures[module] = signature;
            _nextModuleFailureLogUtc[module] =
                now.AddSeconds(30);
            ErrorLog.Write(
                "Overlay module failed and was skipped: " + module,
                exception);
        }

        private void ClearModuleFailure(string module)
        {
            _moduleFailureSignatures.Remove(module);
            _nextModuleFailureLogUtc.Remove(module);
        }

        private void LogSaveFilterStatus()
        {
            if (!DebugSettings.Enabled
                || DateTime.UtcNow < _nextSaveFilterLogUtc)
            {
                return;
            }

            _nextSaveFilterLogUtc =
                DateTime.UtcNow.AddSeconds(2);
            RadarState state = _state;
            IList<RadarPoint> points = Array.Empty<RadarPoint>();
            if (state != null && state.points != null)
            {
                points = state.points;
            }
            int hidden = points.Count(
                point => _saveState.IsOpened(point.saveId));
            string mode = state == null
                ? "none"
                : state.mode ?? "minimap";
            string playerZ = state != null
                && state.hasPlayerZ
                ? GetComparablePlayerZ(state.playerZ).ToString(
                    "0",
                    CultureInfo.InvariantCulture)
                : "?";
            string details = IsWorldMapMode()
                ? BuildWorldMapDebugDetails(state)
                : BuildRadarDebugDetails(points);
            string message = string.Format(
                CultureInfo.InvariantCulture,
                "Treasure debug snapshot: mode={0}; " +
                "gameProcessId={1}; saveLoaded={2}; " +
                "database={3}; databaseWrite={4}; " +
                "openedBits={5}; radarEnabled={6}; " +
                "playerZ={7}; radarPoints={8}; hidden={9}; " +
                "visible={10}; lastError={11}",
                mode,
                _saveState.GameProcessId,
                _saveState.HasLoadedSaveState,
                _saveState.DatabaseName,
                _saveState.DatabaseWriteSummary,
                _saveState.OpenedBitCount,
                state != null && state.enabled,
                playerZ,
                points.Count,
                hidden,
                points.Count - hidden,
                _saveState.LastErrorSummary)
                + Environment.NewLine
                + details;

            if (message != _lastSaveFilterLog)
            {
                _lastSaveFilterLog = message;
                ErrorLog.WriteDebug(message);
            }
        }

        private string BuildRadarDebugDetails(
            IList<RadarPoint> points)
        {
            if (points == null || points.Count == 0)
            {
                return "Nearby treasure details: none";
            }

            List<string> rows = new List<string>();
            int count = Math.Min(points.Count, 12);
            for (int index = 0; index < count; index++)
            {
                RadarPoint point = points[index];
                WorldTreasure metadata =
                    _worldTreasures.FindBySaveIdAndCoordinates(
                        point.saveId,
                        point.x,
                        point.y);
                double horizontal = Math.Sqrt(
                    point.dx * point.dx +
                    point.dy * point.dy);
                rows.Add(string.Format(
                    CultureInfo.InvariantCulture,
                    "  [{0}] name={1}; id={2}; uidName={3}; groupId={4}; " +
                    "x={5:0}; y={6:0}; z={7}; " +
                    "dxy={8:0}({9:0.0}m); dz={10}; {11}; overlaps={12}",
                    index,
                    metadata == null
                        ? TreasureIdentity.GetDebugName(
                            null,
                            point.saveId)
                        : metadata.DebugName,
                    point.saveId,
                    metadata == null
                        || String.IsNullOrWhiteSpace(metadata.UidName)
                        ? "(missing)"
                        : metadata.UidName,
                    metadata == null
                        ? 0
                        : metadata.GroupId,
                    point.x,
                    point.y,
                    point.hasZ
                        ? point.z.ToString(
                            "0",
                            CultureInfo.InvariantCulture)
                        : "?",
                    horizontal,
                    horizontal / 100.0,
                    GetVerticalDeltaText(
                        _state,
                        point.z,
                        point.hasZ),
                    _saveState.Describe(point.saveId),
                    FindRadarOverlapSummary(
                        point,
                        points)));
            }

            return "Nearby treasure details:"
                + Environment.NewLine
                + string.Join(
                    Environment.NewLine,
                    rows.ToArray());
        }

        private string BuildWorldMapDebugDetails(
            RadarState state)
        {
            if (state == null || state.worldMap == null)
            {
                return "World-map treasure details: none";
            }

            WorldMapState map = state.worldMap;
            List<WorldTreasure> nearest =
                _worldTreasures.Points
                    .Where(treasure =>
                        treasure.MapId == map.mapId)
                    .OrderBy(treasure =>
                    {
                        double deltaX =
                            treasure.X - map.playerWorldX;
                        double deltaY =
                            treasure.Y - map.playerWorldY;
                        return deltaX * deltaX +
                            deltaY * deltaY;
                    })
                    .Take(12)
                    .ToList();

            if (nearest.Count == 0)
            {
                return "World-map treasure details: none";
            }

            List<string> rows = new List<string>();
            for (int index = 0;
                index < nearest.Count;
                index++)
            {
                WorldTreasure treasure = nearest[index];
                double deltaX =
                    treasure.X - map.playerWorldX;
                double deltaY =
                    treasure.Y - map.playerWorldY;
                double horizontal = Math.Sqrt(
                    deltaX * deltaX +
                    deltaY * deltaY);
                rows.Add(string.Format(
                    CultureInfo.InvariantCulture,
                    "  [{0}] name={1}; id={2}; uidName={3}; groupId={4}; " +
                    "x={5:0}; y={6:0}; z={7}; " +
                    "dxy={8:0}({9:0.0}m); dz={10}; {11}; overlaps={12}",
                    index,
                    treasure.DebugName,
                    treasure.SaveId,
                    treasure.UidName ?? "(missing)",
                    treasure.GroupId,
                    treasure.X,
                    treasure.Y,
                    treasure.HasZ
                        ? treasure.Z.ToString(
                            "0",
                            CultureInfo.InvariantCulture)
                        : "?",
                    horizontal,
                    horizontal / 100.0,
                    GetVerticalDeltaText(
                        state,
                        treasure.Z,
                        treasure.HasZ),
                    _saveState.Describe(
                        treasure.SaveId),
                    FindWorldOverlapSummary(
                        treasure,
                        nearest)));
            }

            return "World-map treasure details:"
                + Environment.NewLine
                + string.Join(
                    Environment.NewLine,
                    rows.ToArray());
        }

        private static string GetVerticalDeltaText(
            RadarState state,
            double treasureZ,
            bool hasTreasureZ)
        {
            if (state == null
                || !state.hasPlayerZ
                || !hasTreasureZ)
            {
                return "?";
            }

            return (treasureZ
                - GetComparablePlayerZ(state.playerZ)).ToString(
                "0",
                CultureInfo.InvariantCulture);
        }

        private static double GetComparablePlayerZ(double playerZ)
        {
            return playerZ + ComparablePlayerZOffset;
        }

        private static string FindRadarOverlapSummary(
            RadarPoint source,
            IEnumerable<RadarPoint> points)
        {
            string[] overlaps = points
                .Where(candidate =>
                    !object.ReferenceEquals(candidate, source)
                    && CandidateIsClose(
                        source.x,
                        source.y,
                        candidate.x,
                        candidate.y))
                .Take(5)
                .Select(candidate => string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}@z{1}",
                    candidate.saveId,
                    candidate.hasZ
                        ? candidate.z.ToString(
                            "0",
                            CultureInfo.InvariantCulture)
                        : "?"))
                .ToArray();
            return overlaps.Length == 0
                ? "none"
                : string.Join(",", overlaps);
        }

        private static string FindWorldOverlapSummary(
            WorldTreasure source,
            IEnumerable<WorldTreasure> points)
        {
            string[] overlaps = points
                .Where(candidate =>
                    !object.ReferenceEquals(candidate, source)
                    && CandidateIsClose(
                        source.X,
                        source.Y,
                        candidate.X,
                        candidate.Y))
                .Take(5)
                .Select(candidate => string.Format(
                    CultureInfo.InvariantCulture,
                    "{0}@z{1}",
                    candidate.SaveId,
                    candidate.HasZ
                        ? candidate.Z.ToString(
                            "0",
                            CultureInfo.InvariantCulture)
                        : "?"))
                .ToArray();
            return overlaps.Length == 0
                ? "none"
                : string.Join(",", overlaps);
        }

        private static bool CandidateIsClose(
            double leftX,
            double leftY,
            double rightX,
            double rightY)
        {
            double deltaX = leftX - rightX;
            double deltaY = leftY - rightY;
            return deltaX * deltaX + deltaY * deltaY
                <= 250.0 * 250.0;
        }

        private void CheckGameLifetime()
        {
            DateTime now = DateTime.UtcNow;
            if (now < _nextGameLifetimeCheckUtc)
            {
                return;
            }
            _nextGameLifetimeCheckUtc = now.AddMilliseconds(
                GameLifetimeCheckIntervalMs);

            bool gamePresent = false;
            try
            {
                using (Process process =
                    GameProcessFinder.OpenTracked(_gameProcessId))
                {
                    gamePresent = process != null;
                    if (gamePresent)
                    {
                        _gameProcessId = process.Id;
                    }
                    else
                    {
                        _gameProcessId = 0;
                    }
                }
            }
            catch
            {
                // Treat transient process enumeration failures as presence
                // when a game process was already observed.
                gamePresent = _gameProcessId > 0;
            }

            if (gamePresent)
            {
                _hasSeenGameProcess = true;
                _gameProcessMissingSinceUtc = DateTime.MinValue;
                return;
            }

            if (!_hasSeenGameProcess)
            {
                return;
            }
            if (_gameProcessMissingSinceUtc == DateTime.MinValue)
            {
                _gameProcessMissingSinceUtc = now;
                return;
            }
            if (now - _gameProcessMissingSinceUtc < TimeSpan.FromSeconds(3))
            {
                return;
            }

            ErrorLog.WriteDebug(
                "Game process exited; closing DragonSwordWorldRadar overlay.");
            Close();
        }


        private void UpdateOverlayVisibility()
        {
            string reason = null;
            IntPtr gameWindow = _gameWindowHandle;
            if (_gameProcessId <= 0 || gameWindow == IntPtr.Zero)
            {
                reason = "game-window-unavailable";
            }
            else if (!NativeMethods.IsWindowVisible(gameWindow))
            {
                reason = "game-window-hidden";
            }
            else if (NativeMethods.IsIconic(gameWindow))
            {
                reason = "game-window-minimized";
            }
            else
            {
                IntPtr foregroundWindow = NativeMethods.GetForegroundWindow();
                uint foregroundProcessId = 0;
                if (foregroundWindow != IntPtr.Zero)
                {
                    NativeMethods.GetWindowThreadProcessId(
                        foregroundWindow,
                        out foregroundProcessId);
                }
                if (foregroundProcessId != (uint)_gameProcessId)
                {
                    reason = "game-not-foreground";
                }
            }

            bool shouldSuppress = reason != null;
            if (shouldSuppress == _overlaySuppressed)
            {
                return;
            }

            _overlaySuppressed = shouldSuppress;
            _overlaySuppressionReason = shouldSuppress
                ? reason
                : "visible";
            if (!IsHandleCreated)
            {
                return;
            }

            if (shouldSuppress)
            {
                NativeMethods.ShowWindow(
                    Handle,
                    NativeMethods.SwHide);
            }
            else
            {
                MoveOverGameWindow();
                NativeMethods.ShowWindow(
                    Handle,
                    NativeMethods.SwShowNoActivate);
                _performance.RecordInvalidate();
                Invalidate();
            }
            UpdateTimerInterval(DateTime.UtcNow);
            ErrorLog.WriteDebug(
                "Overlay visibility changed: " +
                _overlaySuppressionReason);
        }

        private void MoveOverGameWindow()
        {
            Rectangle primaryBounds = Screen.PrimaryScreen.Bounds;
            Rectangle workingArea = Screen.PrimaryScreen.WorkingArea;
            UpdateGeometry(
                primaryBounds.Width,
                primaryBounds.Height);

            int targetX = workingArea.Right
                - _overlaySize
                - ScalePixels(ReferenceRightMargin);
            int targetY = workingArea.Top
                + ScalePixels(ReferenceTopMargin);
            int targetWidth = _overlaySize;
            int targetHeight = _overlaySize;
            IntPtr gameWindow = IntPtr.Zero;
            NativeRect gameRectangle = new NativeRect();
            bool hasGameRectangle = false;
            bool usedClientRectangle = false;

            try
            {
                using (Process process =
                    GameProcessFinder.OpenTracked(_gameProcessId))
                {
                    if (process != null)
                    {
                        _gameProcessId = process.Id;
                        process.Refresh();
                        gameWindow = process.MainWindowHandle;
                        if (gameWindow == IntPtr.Zero)
                        {
                            gameWindow = FindLargestVisibleWindow(
                                process.Id);
                        }
                        _gameWindowHandle = gameWindow;

                        NativeRect rectangle;
                        if (gameWindow != IntPtr.Zero
                            && NativeMethods.GetWindowRect(
                                gameWindow,
                                out rectangle))
                        {
                            NativeRect clientRectangle;
                            if (TryGetClientScreenRect(
                                gameWindow,
                                out clientRectangle))
                            {
                                rectangle = clientRectangle;
                                usedClientRectangle = true;
                            }

                            gameRectangle = rectangle;
                            hasGameRectangle = true;
                            UpdateGeometry(
                                rectangle.Width,
                                rectangle.Height);
                            targetX = rectangle.Right
                                - _overlaySize
                                - ScalePixels(
                                    ReferenceRightMargin);
                            targetY = rectangle.Top
                                + ScalePixels(
                                    ReferenceTopMargin);
                            if (IsWorldMapMode())
                            {
                                targetX = rectangle.Left;
                                targetY = rectangle.Top;
                                targetWidth = rectangle.Width;
                                targetHeight = rectangle.Height;
                            }
                        }
                    }
                }
            }
            catch (Exception exception)
            {
                ErrorLog.Write(
                    "Game window detection failed; " +
                    "using desktop position",
                    exception);
            }

            if (IsWorldMapMode() && !hasGameRectangle)
            {
                targetX = primaryBounds.Left;
                targetY = primaryBounds.Top;
                targetWidth = primaryBounds.Width;
                targetHeight = primaryBounds.Height;
            }

            PositionOverlay(
                targetX,
                targetY,
                targetWidth,
                targetHeight);
            if (hasGameRectangle)
            {
                LogGeometry(
                    gameWindow,
                    gameRectangle,
                    usedClientRectangle,
                    targetX,
                    targetY,
                    targetWidth,
                    targetHeight);
            }
        }

        private bool IsWorldMapMode()
        {
            return String.Equals(
                GetEffectiveMode(),
                "world",
                StringComparison.Ordinal);
        }

        private void PositionOverlay(
            int targetX,
            int targetY,
            int targetWidth,
            int targetHeight)
        {
            NativeRect current;
            bool alreadyPositioned =
                IsHandleCreated
                && NativeMethods.GetWindowRect(Handle, out current)
                && current.Left == targetX
                && current.Top == targetY
                && current.Width == targetWidth
                && current.Height == targetHeight;
            if (alreadyPositioned)
            {
                return;
            }

            if (!NativeMethods.SetWindowPos(
                Handle,
                NativeMethods.HwndTopmost,
                targetX,
                targetY,
                targetWidth,
                targetHeight,
                NativeMethods.SwpNoActivate))
            {
                ErrorLog.WriteMessage(
                    "Overlay SetWindowPos failed: Win32 error " +
                    Marshal.GetLastWin32Error());
            }
        }

        private void LogGeometry(
            IntPtr gameWindow,
            NativeRect gameRectangle,
            bool usedClientRectangle,
            int targetX,
            int targetY,
            int targetWidth,
            int targetHeight)
        {
            if (!DebugSettings.Enabled)
            {
                return;
            }

            NativeRect actualOverlay;
            bool hasActualOverlay =
                NativeMethods.GetWindowRect(
                    Handle,
                    out actualOverlay);
            string message = string.Format(
                CultureInfo.InvariantCulture,
                "Geometry: game={0},{1} {2}x{3}; source={4}; " +
                "gameDpi={5}; target={6},{7} {8}x{9}; " +
                "actual={10}; overlayDpi={11}; scale={12:0.####}",
                gameRectangle.Left,
                gameRectangle.Top,
                gameRectangle.Width,
                gameRectangle.Height,
                usedClientRectangle ? "client" : "window",
                GetWindowDpi(gameWindow),
                targetX,
                targetY,
                targetWidth,
                targetHeight,
                hasActualOverlay
                    ? string.Format(
                        CultureInfo.InvariantCulture,
                        "{0},{1} {2}x{3}",
                        actualOverlay.Left,
                        actualOverlay.Top,
                        actualOverlay.Width,
                        actualOverlay.Height)
                    : "unavailable",
                GetWindowDpi(Handle),
                _displayScale);
            if (!string.Equals(
                message,
                _lastGeometryLog,
                StringComparison.Ordinal))
            {
                _lastGeometryLog = message;
                GeometryLog.Write(message);
            }
        }

        private static uint GetWindowDpi(IntPtr window)
        {
            try
            {
                return NativeMethods.GetDpiForWindow(window);
            }
            catch (EntryPointNotFoundException)
            {
                return 0;
            }
            catch (DllNotFoundException)
            {
                return 0;
            }
        }

        private void UpdateGeometry(
            int windowWidth,
            int windowHeight)
        {
            if (windowWidth <= 0)
            {
                windowWidth = (int)ReferenceWindowWidth;
            }
            if (windowHeight <= 0)
            {
                windowHeight = (int)ReferenceWindowHeight;
            }

            _displayScale = Math.Min(
                windowWidth / ReferenceWindowWidth,
                windowHeight / ReferenceWindowHeight);
            _overlaySize = Math.Max(
                1,
                (int)Math.Round(
                    ReferenceOverlaySize * _displayScale));
            UpdateTreasureOutlinePenWidths();
        }

        private int ScalePixels(int referencePixels)
        {
            return (int)Math.Round(
                referencePixels * _displayScale);
        }

        private static bool TryGetClientScreenRect(
            IntPtr window,
            out NativeRect rectangle)
        {
            NativeRect client;
            NativePoint topLeft = new NativePoint();
            if (NativeMethods.GetClientRect(window, out client)
                && NativeMethods.ClientToScreen(
                    window,
                    ref topLeft)
                && client.Width > 0
                && client.Height > 0)
            {
                rectangle = new NativeRect
                {
                    Left = topLeft.X,
                    Top = topLeft.Y,
                    Right = topLeft.X + client.Width,
                    Bottom = topLeft.Y + client.Height
                };
                return true;
            }

            rectangle = new NativeRect();
            return false;
        }

        private static IntPtr FindLargestVisibleWindow(
            int processId)
        {
            IntPtr bestWindow = IntPtr.Zero;
            long bestArea = 0;

            NativeMethods.EnumWindows(delegate(
                IntPtr window,
                IntPtr parameter)
            {
                uint ownerProcessId;
                NativeMethods.GetWindowThreadProcessId(
                    window,
                    out ownerProcessId);
                if (ownerProcessId != (uint)processId
                    || !NativeMethods.IsWindowVisible(window))
                {
                    return true;
                }

                NativeRect rectangle;
                if (!NativeMethods.GetWindowRect(
                    window,
                    out rectangle))
                {
                    return true;
                }

                long area =
                    rectangle.Width > 0 && rectangle.Height > 0
                        ? (long)rectangle.Width * rectangle.Height
                        : 0;
                if (area > bestArea)
                {
                    bestArea = area;
                    bestWindow = window;
                }
                return true;
            }, IntPtr.Zero);

            return bestWindow;
        }

        private sealed class MotionVisualSnapshot
        {
            private readonly WorldMapState _worldMap =
                new WorldMapState();
            private bool _hasValue;
            private bool _hasWorldMap;
            private int _generation;
            private bool _enabled;
            private string _mode;
            private double _playerX;
            private double _playerY;
            private double _playerZ;
            private bool _hasPlayerZ;
            private double _radius;

            public bool Update(MotionFrame frame)
            {
                if (frame == null)
                {
                    bool resetChanged = _hasValue;
                    Reset();
                    return resetChanged;
                }

                bool hasWorldMap = frame.WorldMap != null;
                bool changed = !_hasValue
                    || _generation != frame.Generation
                    || _enabled != frame.Enabled
                    || !String.Equals(
                        _mode,
                        frame.Mode,
                        StringComparison.Ordinal)
                    || _playerX != frame.PlayerX
                    || _playerY != frame.PlayerY
                    || _playerZ != frame.PlayerZ
                    || _hasPlayerZ != frame.HasPlayerZ
                    || _radius != frame.Radius
                    || _hasWorldMap != hasWorldMap
                    || (hasWorldMap
                        && !WorldMapsEqual(
                            _worldMap,
                            frame.WorldMap));

                _hasValue = true;
                _generation = frame.Generation;
                _enabled = frame.Enabled;
                _mode = frame.Mode;
                _playerX = frame.PlayerX;
                _playerY = frame.PlayerY;
                _playerZ = frame.PlayerZ;
                _hasPlayerZ = frame.HasPlayerZ;
                _radius = frame.Radius;
                _hasWorldMap = hasWorldMap;
                if (hasWorldMap)
                {
                    CopyWorldMap(frame.WorldMap, _worldMap);
                }
                return changed;
            }

            public void Reset()
            {
                _hasValue = false;
                _hasWorldMap = false;
                _generation = 0;
                _enabled = false;
                _mode = null;
                _playerX = 0.0;
                _playerY = 0.0;
                _playerZ = 0.0;
                _hasPlayerZ = false;
                _radius = 0.0;
            }

            private static void CopyWorldMap(
                WorldMapState source,
                WorldMapState destination)
            {
                destination.mapId = source.mapId;
                destination.dimensions = source.dimensions;
                destination.uiSize = source.uiSize;
                destination.left = source.left;
                destination.top = source.top;
                destination.zoom = source.zoom;
                destination.viewportWidth = source.viewportWidth;
                destination.viewportHeight = source.viewportHeight;
                destination.viewportScale = source.viewportScale;
                destination.playerWorldX = source.playerWorldX;
                destination.playerWorldY = source.playerWorldY;
                destination.playerMapX = source.playerMapX;
                destination.playerMapY = source.playerMapY;
            }
        }
    }
}
