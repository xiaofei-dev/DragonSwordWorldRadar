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

        // Height indicators use a continuous 180-degree scale. The player
        // actor origin sits above the treasure reference point, so the
        // player Z value is shifted before comparison. Near-equal heights
        // keep a horizontal pointer instead of hiding the indicator.
        private const double ComparablePlayerZOffset = -150.0;
        private const double HeightIndicatorDeadZone = 100.0;
        private const double HeightIndicatorSensitivity = 2500.0;
        private const double HeightIndicatorMaximumAngleDegrees = 85.0;

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
        private readonly Pen _otherTreasureOutlinePen;
        private readonly Pen _miniGameTreasureOutlinePen;
        private readonly Pen _mapTreasureOutlinePen;
        private readonly Pen _puzzleTreasureOutlinePen;
        private readonly bool _highResolutionTimerEnabled;

        private List<WorldTreasure> _visibleWorldTreasures =
            new List<WorldTreasure>();
        private Dictionary<int, List<WorldTreasure>>
            _visibleWorldTreasuresByMap =
                new Dictionary<int, List<WorldTreasure>>();

        private RadarState _state;
        private MotionFrame _motion;
        private DateTime _stateMissingSinceUtc;
        private DateTime _nextStaticStatePollUtc;
        private DateTime _nextMaintenanceUtc;
        private DateTime _nextGeometryCheckUtc;
        private float _displayScale = 1f;
        private int _overlaySize = ReferenceOverlaySize;
        private string _lastGeometryLog;
        private string _lastSaveFilterLog;
        private string _lastBossAvailabilitySignature;
        private DateTime _nextSaveFilterLogUtc;
        private int _lastSaveStateVersion = -1;
        private int _lastWorldTreasureCatalogVersion = -1;
        private int _lastWorldTreasureSaveVersion = -1;
        private int _gameProcessId;
        private bool _hasSeenGameProcess;
        private DateTime _gameProcessMissingSinceUtc;
        private DateTime _nextGameLifetimeCheckUtc;
        private DateTime _nextPerformanceLogUtc =
            DateTime.UtcNow.AddSeconds(5);
        private int _performanceTimerTicks;
        private int _performancePaints;
        private int _performanceMotionFrames;
        private int _performanceStaticFrames;
        private int _performanceInvalidates;
        private double _performanceRefreshTotalMs;
        private double _performanceRefreshMaxMs;
        private double _performancePaintTotalMs;
        private double _performancePaintMaxMs;

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
            _otherTreasureOutlinePen = CreateTreasureOutlinePen(
                TreasureKind.Other);
            _miniGameTreasureOutlinePen = CreateTreasureOutlinePen(
                TreasureKind.MiniGame);
            _mapTreasureOutlinePen = CreateTreasureOutlinePen(
                TreasureKind.Map);
            _puzzleTreasureOutlinePen = CreateTreasureOutlinePen(
                TreasureKind.PressurePuzzle);
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
            _otherTreasureOutlinePen.Dispose();
            _miniGameTreasureOutlinePen.Dispose();
            _mapTreasureOutlinePen.Dispose();
            _puzzleTreasureOutlinePen.Dispose();
            if (_highResolutionTimerEnabled)
            {
                NativeMethods.timeEndPeriod(1);
            }
            base.OnFormClosed(eventArgs);
        }

        protected override void OnPaint(PaintEventArgs eventArgs)
        {
            base.OnPaint(eventArgs);
            long paintStarted = Stopwatch.GetTimestamp();
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
                IList<RadarPoint> points =
                    state.points ?? new List<RadarPoint>();
                RadarPoint nearest = FindNearestRadarPoint(
                    points,
                    playerX,
                    playerY,
                    playerZ,
                    hasPlayerZ,
                    radius);

                if (state.showBosses)
                {
                    foreach (BossPoint boss in
                        (state.bosses ?? new List<BossPoint>()))
                    {
                        if (!boss.visible
                            || !_saveState.IsBossAvailable(boss.bossId))
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

                for (int index = points.Count - 1;
                    index >= 0;
                    index--)
                {
                    RadarPoint point = points[index];
                    if (_saveState.IsOpened(point.saveId))
                    {
                        continue;
                    }
                    double deltaX = point.x - playerX;
                    double deltaY = point.y - playerY;
                    if (deltaX * deltaX + deltaY * deltaY
                        > radius * radius)
                    {
                        continue;
                    }
                    DrawPoint(
                        eventArgs.Graphics,
                        point,
                        playerX,
                        playerY,
                        radius,
                        radarRadius,
                        center,
                        Object.ReferenceEquals(point, nearest),
                        state.showHeight,
                        state.showTreasureTypes,
                        showDebugCoordinates,
                        textScale,
                        playerZ,
                        hasPlayerZ);
                }
            }
            finally
            {
                RecordPaintDuration(paintStarted);
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
                if (_saveState.IsOpened(point.saveId))
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
            graphics.SetClip(clip);
            List<WorldTreasure> mapTreasures;
            if (!_visibleWorldTreasuresByMap.TryGetValue(
                map.mapId,
                out mapTreasures))
            {
                mapTreasures = new List<WorldTreasure>();
            }

            if (showBosses)
            {
                foreach (BossPoint boss in
                    (bosses ?? new List<BossPoint>()))
                {
                    if (!boss.visible
                        || !_saveState.IsBossAvailable(boss.bossId)
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
                    BossMarkerRenderer.DrawMarker(
                        graphics,
                        bossX,
                        bossY,
                        bossDiameter,
                        _displayScale);
                    if (showDebugCoordinates)
                    {
                        BossMarkerRenderer.DrawDebugLabel(
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
                WorldTreasure nearestTreasure = null;
                float nearestX = 0f;
                float nearestY = 0f;
                double nearestDistanceSquared = Double.MaxValue;

                // The first pass finds the nearest visible marker. It performs
                // only projection and distance math; no GDI objects are
                // created per marker.
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

                    double deltaX = treasure.X - map.playerWorldX;
                    double deltaY = treasure.Y - map.playerWorldY;
                    double deltaZ = 0.0;
                    if (hasPlayerZ && treasure.HasZ)
                    {
                        deltaZ = treasure.Z - GetComparablePlayerZ(playerZ);
                    }
                    double distanceSquared =
                        deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ;
                    if (distanceSquared < nearestDistanceSquared)
                    {
                        nearestDistanceSquared = distanceSquared;
                        nearestTreasure = treasure;
                        nearestX = projectedX;
                        nearestY = projectedY;
                    }
                }

                // Batch normal world-map markers by type. Winding fill is
                // required here: Alternate fill treats overlapping projected
                // circles as even/odd holes, which produces transparent rings.
                using (GraphicsPath otherPath = new GraphicsPath(FillMode.Winding))
                using (GraphicsPath miniGamePath = new GraphicsPath(FillMode.Winding))
                using (GraphicsPath mapPath = new GraphicsPath(FillMode.Winding))
                using (GraphicsPath puzzlePath = new GraphicsPath(FillMode.Winding))
                {
                    HashSet<long> projectedMarkerPixels =
                        new HashSet<long>();
                    if (nearestTreasure != null)
                    {
                        projectedMarkerPixels.Add(
                            GetProjectedMarkerPixelKey(nearestX, nearestY));
                    }
                    foreach (WorldTreasure treasure in mapTreasures)
                    {
                        if (Object.ReferenceEquals(
                            treasure,
                            nearestTreasure))
                        {
                            continue;
                        }

                        float x;
                        float y;
                        if (!TryProjectWorldTreasure(
                            treasure,
                            map,
                            coordinateScale,
                            windowScaleX,
                            windowScaleY,
                            normalDiameter / 2f,
                            out x,
                            out y))
                        {
                            continue;
                        }

                        long projectedPixel =
                            GetProjectedMarkerPixelKey(x, y);
                        if (!projectedMarkerPixels.Add(projectedPixel))
                        {
                            continue;
                        }

                        float half = normalDiameter / 2f;
                        RectangleF marker = new RectangleF(
                            x - half,
                            y - half,
                            normalDiameter,
                            normalDiameter);
                        TreasureKind kind = treasure.Kind;
                        if (kind == TreasureKind.MiniGame)
                        {
                            miniGamePath.AddEllipse(marker);
                        }
                        else if (kind == TreasureKind.Map)
                        {
                            mapPath.AddEllipse(marker);
                        }
                        else if (kind == TreasureKind.PressurePuzzle
                            || kind == TreasureKind.StatuePuzzle)
                        {
                            puzzlePath.AddEllipse(marker);
                        }
                        else
                        {
                            otherPath.AddEllipse(marker);
                        }
                    }

                    DrawTreasurePath(
                        graphics,
                        otherPath,
                        _otherTreasureBrush,
                        _otherTreasureOutlinePen);
                    DrawTreasurePath(
                        graphics,
                        miniGamePath,
                        _miniGameTreasureBrush,
                        _miniGameTreasureOutlinePen);
                    DrawTreasurePath(
                        graphics,
                        mapPath,
                        _mapTreasureBrush,
                        _mapTreasureOutlinePen);
                    DrawTreasurePath(
                        graphics,
                        puzzlePath,
                        _puzzleTreasureBrush,
                        _puzzleTreasureOutlinePen);

                    // Draw the nearest marker separately so its larger marker
                    // remains above the batched paths and can anchor the height
                    // pointer and label.
                    if (nearestTreasure != null)
                    {
                        DrawTreasureMarker(
                            graphics,
                            nearestX,
                            nearestY,
                            nearestDiameter,
                            GetTreasureBrush(nearestTreasure),
                            true,
                            GetTreasureOutlinePen(nearestTreasure));
                    }
                }

                if (nearestTreasure != null)
                {
                    Color nearestColor =
                        GetTreasureColor(nearestTreasure);
                    if (showHeight)
                    {
                        DrawHeightIndicator(
                            graphics,
                            nearestX,
                            nearestY,
                            nearestDiameter,
                            playerZ,
                            hasPlayerZ,
                            nearestTreasure.Z,
                            nearestTreasure.HasZ,
                            nearestColor);
                    }
                    if (showLabel)
                    {
                        DrawNearestLabel(
                            graphics,
                            nearestX,
                            nearestY,
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

            graphics.Restore(saved);
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
            }
            catch (Exception exception)
            {
                ErrorLog.Write(
                    "Radar refresh failed",
                    exception);
            }
            finally
            {
                RecordRefreshDuration(refreshStarted);
                LogPerformanceIfDue();
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
            BossMarkerRenderer.DrawMarker(
                graphics,
                x,
                y,
                diameter,
                _displayScale);
            if (showDebugCoordinates)
            {
                BossMarkerRenderer.DrawDebugLabel(
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
                nearest,
                GetTreasureOutlinePen(metadata));

            if (nearest)
            {
                if (showHeight)
                {
                    DrawHeightIndicator(
                        graphics,
                        x,
                        y,
                        diameter,
                        playerZ,
                        hasPlayerZ,
                        point.z,
                        point.hasZ,
                        color);
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
            bool nearest,
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

        private Pen CreateTreasureOutlinePen(
            TreasureKind kind)
        {
            // Keep the type-specific fill, but use the same neutral outline
            // as the boss layer. This preserves contrast on every map color
            // and avoids a purple/magenta fringe around white markers.
            return new Pen(
                RadarMarkerStyle.Outline,
                RadarMarkerStyle.GetTreasureOutlineWidth(_displayScale));
        }

        private Pen GetTreasureOutlinePen(
            WorldTreasure treasure)
        {
            TreasureKind kind = treasure == null
                ? TreasureKind.Other
                : treasure.Kind;
            if (kind == TreasureKind.MiniGame)
            {
                return _miniGameTreasureOutlinePen;
            }
            if (kind == TreasureKind.Map)
            {
                return _mapTreasureOutlinePen;
            }
            if (kind == TreasureKind.PressurePuzzle
                || kind == TreasureKind.StatuePuzzle)
            {
                return _puzzleTreasureOutlinePen;
            }
            return _otherTreasureOutlinePen;
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

        private static void DrawTreasurePath(
            Graphics graphics,
            GraphicsPath path,
            Brush brush,
            Pen outline)
        {
            if (path == null || path.PointCount <= 0)
            {
                return;
            }

            SmoothingMode previousSmoothing = graphics.SmoothingMode;
            graphics.SmoothingMode = SmoothingMode.None;
            try
            {
                graphics.FillPath(brush, path);
                graphics.DrawPath(outline, path);
            }
            finally
            {
                graphics.SmoothingMode = previousSmoothing;
            }
        }

        private void UpdateTreasureOutlinePenWidths()
        {
            float width = RadarMarkerStyle.GetTreasureOutlineWidth(_displayScale);
            if (_otherTreasureOutlinePen != null)
            {
                _otherTreasureOutlinePen.Width = width;
            }
            if (_miniGameTreasureOutlinePen != null)
            {
                _miniGameTreasureOutlinePen.Width = width;
            }
            if (_mapTreasureOutlinePen != null)
            {
                _mapTreasureOutlinePen.Width = width;
            }
            if (_puzzleTreasureOutlinePen != null)
            {
                _puzzleTreasureOutlinePen.Width = width;
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

        // show_height controls the normal height pointer. Exact Z values are
        // displayed only while debug_logging is enabled, and treasure type
        // labels remain independently configurable.
        private void DrawHeightIndicator(
            Graphics graphics,
            float anchorX,
            float anchorY,
            float markerDiameter,
            double playerZ,
            bool hasPlayerZ,
            double treasureZ,
            bool hasTreasureZ,
            Color color)
        {
            if (!hasPlayerZ || !hasTreasureZ)
            {
                return;
            }

            double comparablePlayerZ = GetComparablePlayerZ(playerZ);
            double deltaZ = treasureZ - comparablePlayerZ;
            if (Math.Abs(deltaZ) <= HeightIndicatorDeadZone)
            {
                deltaZ = 0.0;
            }

            // The pointer rotates through the right semicircle: 12 o'clock
            // means above, 3 o'clock means near the same height, and
            // 6 o'clock means below. It is anchored just left of the marker.
            double pointerAngle = -Math.Atan(
                deltaZ / HeightIndicatorSensitivity);
            double maximumAngle =
                HeightIndicatorMaximumAngleDegrees
                * Math.PI / 180.0;
            pointerAngle = Math.Max(
                -maximumAngle,
                Math.Min(maximumAngle, pointerAngle));

            float directionX = (float)Math.Cos(pointerAngle);
            float directionY = (float)Math.Sin(pointerAngle);
            float perpendicularX = -directionY;
            float perpendicularY = directionX;

            // Keep the pointer short, thick, and close to the treasure marker.
            float indicatorLength = Math.Min(
                36f,
                Math.Max(20f, 24f * _displayScale));
            float markerGap = Math.Min(
                3f,
                Math.Max(1.5f, 2f * _displayScale));
            float pointerCenterX = anchorX
                - markerDiameter / 2f
                - markerGap
                - indicatorLength / 2f;
            float pointerCenterY = anchorY;
            float halfLength = indicatorLength / 2f;
            float startX = pointerCenterX - directionX * halfLength;
            float startY = pointerCenterY - directionY * halfLength;
            float tipX = pointerCenterX + directionX * halfLength;
            float tipY = pointerCenterY + directionY * halfLength;

            float headLength = Math.Min(
                15f,
                Math.Max(8f, 10f * _displayScale));
            float headHalfWidth = Math.Min(
                8f,
                Math.Max(4f, 5f * _displayScale));
            float baseX = tipX - directionX * headLength;
            float baseY = tipY - directionY * headLength;

            float outlineWidth = Math.Min(
                11f,
                Math.Max(6f, 8f * _displayScale));
            float innerWidth = Math.Min(
                8.5f,
                Math.Max(4.5f, 6f * _displayScale));
            Color outlineColor = Color.FromArgb(235, 5, 12, 22);
            Color indicatorColor = Color.FromArgb(
                255,
                color.R,
                color.G,
                color.B);

            using (Pen outlinePen = new Pen(
                outlineColor,
                outlineWidth))
            using (Pen indicatorPen = new Pen(
                indicatorColor,
                innerWidth))
            {
                outlinePen.StartCap = LineCap.Round;
                outlinePen.EndCap = LineCap.Round;
                indicatorPen.StartCap = LineCap.Round;
                indicatorPen.EndCap = LineCap.Round;
                graphics.DrawLine(
                    outlinePen,
                    startX,
                    startY,
                    baseX,
                    baseY);
                graphics.DrawLine(
                    indicatorPen,
                    startX,
                    startY,
                    baseX,
                    baseY);
            }

            float headOutlineExpansion = Math.Min(
                3f,
                Math.Max(2f, 2f * _displayScale));
            PointF[] outlineHead =
            {
                new PointF(tipX, tipY),
                new PointF(
                    baseX + perpendicularX
                        * (headHalfWidth + headOutlineExpansion),
                    baseY + perpendicularY
                        * (headHalfWidth + headOutlineExpansion)),
                new PointF(
                    baseX - perpendicularX
                        * (headHalfWidth + headOutlineExpansion),
                    baseY - perpendicularY
                        * (headHalfWidth + headOutlineExpansion))
            };
            PointF[] colorHead =
            {
                new PointF(tipX, tipY),
                new PointF(
                    baseX + perpendicularX * headHalfWidth,
                    baseY + perpendicularY * headHalfWidth),
                new PointF(
                    baseX - perpendicularX * headHalfWidth,
                    baseY - perpendicularY * headHalfWidth)
            };
            using (Brush outlineBrush = new SolidBrush(outlineColor))
            using (Brush indicatorBrush = new SolidBrush(indicatorColor))
            {
                graphics.FillPolygon(outlineBrush, outlineHead);
                graphics.FillPolygon(indicatorBrush, colorHead);
            }
        }

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

        private void UpdateTimerInterval()
        {
            string mode = GetEffectiveMode();
            int interval = String.Equals(
                mode,
                "world",
                StringComparison.Ordinal)
                    ? 8
                    : String.Equals(
                        mode,
                        "radar",
                        StringComparison.Ordinal)
                        ? 16
                        : 100;
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
                _nextStaticStatePollUtc = now.AddMilliseconds(75);
                RadarState loaded;
                if (_staticStateBridge.TryReadLatest(out loaded))
                {
                    _performanceStaticFrames++;
                    _state = loaded;
                    if (_motion != null
                        && _motion.Generation != loaded.producerGeneration)
                    {
                        _motion = null;
                    }
                    _stateMissingSinceUtc = DateTime.MinValue;
                    redraw = true;
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
                        redraw = true;
                    }
                }
            }

            MotionFrame motion;
            if (_motionBridge.TryReadLatest(out motion))
            {
                _performanceMotionFrames++;
                _motion = motion;
                if (_state == null
                    || motion.Generation == _state.producerGeneration)
                {
                    redraw = true;
                }
            }

            if (now >= _nextMaintenanceUtc)
            {
                _nextMaintenanceUtc = now.AddMilliseconds(250);
                _saveState.Refresh();
                _worldTreasures.Refresh();
                bool saveVersionChanged = UpdateSaveStateVersion();
                bool filterChanged = UpdateWorldTreasureFilter();
                if (saveVersionChanged || filterChanged)
                {
                    redraw = true;
                }
                string bossSignature =
                    BuildBossAvailabilitySignature();
                if (bossSignature != _lastBossAvailabilitySignature)
                {
                    _lastBossAvailabilitySignature = bossSignature;
                    redraw = true;
                }
                LogSaveFilterStatus();
            }

            string currentMode = GetEffectiveMode();
            geometryChanged = !String.Equals(
                previousMode,
                currentMode,
                StringComparison.Ordinal);
            if (geometryChanged
                || now >= _nextGeometryCheckUtc)
            {
                _nextGeometryCheckUtc = now.AddMilliseconds(500);
                MoveOverGameWindow();
            }
            UpdateTimerInterval();
            if (redraw || geometryChanged)
            {
                _performanceInvalidates++;
                Invalidate();
            }
        }

        private void RecordPaintDuration(long started)
        {
            double elapsed = ElapsedMilliseconds(started);
            _performancePaints++;
            _performancePaintTotalMs += elapsed;
            _performancePaintMaxMs = Math.Max(
                _performancePaintMaxMs,
                elapsed);
        }

        private void RecordRefreshDuration(long started)
        {
            double elapsed = ElapsedMilliseconds(started);
            _performanceTimerTicks++;
            _performanceRefreshTotalMs += elapsed;
            _performanceRefreshMaxMs = Math.Max(
                _performanceRefreshMaxMs,
                elapsed);
        }

        private static double ElapsedMilliseconds(long started)
        {
            return (Stopwatch.GetTimestamp() - started)
                * 1000.0 / Stopwatch.Frequency;
        }

        private void LogPerformanceIfDue()
        {
            DateTime now = DateTime.UtcNow;
            if (now < _nextPerformanceLogUtc)
            {
                return;
            }

            double refreshAverage = _performanceTimerTicks == 0
                ? 0.0
                : _performanceRefreshTotalMs / _performanceTimerTicks;
            double paintAverage = _performancePaints == 0
                ? 0.0
                : _performancePaintTotalMs / _performancePaints;
            ErrorLog.WriteMessage(String.Format(
                CultureInfo.InvariantCulture,
                "Overlay performance: mode={0}; timerMs={1}; ticks={2}; staticFrames={3}; motionFrames={4}; invalidates={5}; paints={6}; refreshAvgMs={7:F3}; refreshMaxMs={8:F3}; paintAvgMs={9:F3}; paintMaxMs={10:F3}; worldTreasures={11}",
                GetEffectiveMode(),
                _timer.Interval,
                _performanceTimerTicks,
                _performanceStaticFrames,
                _performanceMotionFrames,
                _performanceInvalidates,
                _performancePaints,
                refreshAverage,
                _performanceRefreshMaxMs,
                paintAverage,
                _performancePaintMaxMs,
                _visibleWorldTreasures.Count));

            _nextPerformanceLogUtc = now.AddSeconds(5);
            _performanceTimerTicks = 0;
            _performancePaints = 0;
            _performanceMotionFrames = 0;
            _performanceStaticFrames = 0;
            _performanceInvalidates = 0;
            _performanceRefreshTotalMs = 0.0;
            _performanceRefreshMaxMs = 0.0;
            _performancePaintTotalMs = 0.0;
            _performancePaintMaxMs = 0.0;
        }

        private MotionFrame GetCompatibleMotion(
            RadarState state)
        {
            if (state == null
                || _motion == null
                || _motion.Generation != state.producerGeneration)
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

        private string BuildBossAvailabilitySignature()
        {
            RadarState state = _state;
            if (state == null
                || state.bosses == null
                || state.bosses.Count == 0)
            {
                return "none";
            }
            List<string> values = new List<string>();
            foreach (BossPoint boss in state.bosses)
            {
                values.Add(String.Format(
                    CultureInfo.InvariantCulture,
                    "{0}:{1}",
                    boss.bossId,
                    _saveState.IsBossAvailable(
                        boss.bossId) ? 1 : 0));
            }
            values.Sort(StringComparer.Ordinal);
            return String.Join("|", values.ToArray());
        }

        private bool UpdateSaveStateVersion()
        {
            int version = _saveState.Version;
            if (version == _lastSaveStateVersion)
            {
                return false;
            }

            _lastSaveStateVersion = version;
            return true;
        }

        private bool UpdateWorldTreasureFilter()
        {
            int catalogVersion = _worldTreasures.Version;
            int saveVersion = _saveState.Version;
            if (catalogVersion == _lastWorldTreasureCatalogVersion
                && saveVersion == _lastWorldTreasureSaveVersion)
            {
                return false;
            }

            _lastWorldTreasureCatalogVersion = catalogVersion;
            _lastWorldTreasureSaveVersion = saveVersion;
            bool hasSave = _saveState.HasLoadedSaveState;
            List<WorldTreasure> visible = new List<WorldTreasure>();
            Dictionary<int, List<WorldTreasure>> byMap =
                new Dictionary<int, List<WorldTreasure>>();
            foreach (WorldTreasure treasure in _worldTreasures.Points)
            {
                if (hasSave && _saveState.IsOpened(treasure.SaveId))
                {
                    continue;
                }
                visible.Add(treasure);
                List<WorldTreasure> mapPoints;
                if (!byMap.TryGetValue(treasure.MapId, out mapPoints))
                {
                    mapPoints = new List<WorldTreasure>();
                    byMap[treasure.MapId] = mapPoints;
                }
                mapPoints.Add(treasure);
            }
            _visibleWorldTreasures = visible;
            _visibleWorldTreasuresByMap = byMap;
            return true;
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
            List<RadarPoint> points =
                state == null || state.points == null
                    ? new List<RadarPoint>()
                    : state.points;
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
            _nextGameLifetimeCheckUtc = now.AddMilliseconds(500);

            bool gamePresent = false;
            try
            {
                using (Process process = GameProcessFinder.FindNewest())
                {
                    gamePresent = process != null;
                    if (gamePresent)
                    {
                        _gameProcessId = process.Id;
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
            _gameProcessId = 0;

            try
            {
                using (Process process =
                    GameProcessFinder.FindNewest())
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
    }
}
