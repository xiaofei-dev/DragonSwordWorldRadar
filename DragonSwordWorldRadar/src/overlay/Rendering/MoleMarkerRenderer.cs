using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    // Code-native winged upward-arrow marker matching the reviewed Fly symbol.
    // Retained brushes, pens, and paths keep the normal paint path allocation-free.
    internal sealed class MoleMarkerRenderer : IDisposable
    {
        private readonly Brush _shadowBrush = new SolidBrush(RadarMarkerStyle.Shadow);
        private readonly Brush _wingBrush = new SolidBrush(RadarMarkerStyle.FlyWing);
        private readonly Brush _arrowBrush = new SolidBrush(RadarMarkerStyle.FlyArrow);
        private readonly Brush _hammerBrush = new SolidBrush(Color.FromArgb(255, 210, 145, 82));
        private readonly Brush _waveBrush = new SolidBrush(Color.FromArgb(255, 50, 91, 224));
        private readonly Brush _debugOutlineBrush = new SolidBrush(Color.FromArgb(235, 0, 0, 0));
        private readonly Brush _debugForegroundBrush = new SolidBrush(Color.FromArgb(255, 210, 240, 255));
        private readonly Pen _outlinePen = new Pen(RadarMarkerStyle.Outline, 1f);
        private readonly GraphicsPath _leftWing = new GraphicsPath();
        private readonly GraphicsPath _rightWing = new GraphicsPath();
        private readonly GraphicsPath _arrow = new GraphicsPath();
        private readonly GraphicsPath _activityIcon = new GraphicsPath();
        private Font _debugFont;
        private float _debugFontSize;
        private bool _disposed;

        public MoleMarkerRenderer()
        {
            _outlinePen.LineJoin = LineJoin.Round;
            _outlinePen.StartCap = LineCap.Round;
            _outlinePen.EndCap = LineCap.Round;
        }

        public void DrawMarker(Graphics graphics, float centerX, float centerY, float diameter, float displayScale)
        {
            if (_disposed || graphics == null || diameter <= 0f) return;
            float d = diameter;
            float x = centerX;
            float y = centerY;
            float shadow = Math.Max(1f, 1.3f * displayScale);
            _outlinePen.Width = RadarMarkerStyle.GetFlyOutlineWidth(displayScale);

            BuildWing(_leftWing, x, y, d, -1f);
            BuildWing(_rightWing, x, y, d, 1f);
            BuildArrow(_arrow, x, y, d);

            GraphicsState state = graphics.Save();
            graphics.TranslateTransform(shadow, shadow);
            graphics.FillPath(_shadowBrush, _leftWing);
            graphics.FillPath(_shadowBrush, _rightWing);
            graphics.FillPath(_shadowBrush, _arrow);
            graphics.Restore(state);

            graphics.FillPath(_wingBrush, _leftWing);
            graphics.FillPath(_wingBrush, _rightWing);
            graphics.DrawPath(_outlinePen, _leftWing);
            graphics.DrawPath(_outlinePen, _rightWing);
            graphics.FillPath(_arrowBrush, _arrow);
            graphics.DrawPath(_outlinePen, _arrow);
        }

        public void DrawMarker(Graphics graphics, WorldMole miniGame, float centerX, float centerY, float diameter, float displayScale)
        {
            if (miniGame == null || String.Equals(miniGame.MiniGameType, "fly", StringComparison.Ordinal))
            {
                DrawMarker(graphics, centerX, centerY, diameter, displayScale);
                return;
            }
            if (_disposed || graphics == null || diameter <= 0f) return;
            float shadow = Math.Max(1f, 1.3f * displayScale);
            _outlinePen.Width = RadarMarkerStyle.GetFlyOutlineWidth(displayScale);
            if (String.Equals(miniGame.MiniGameType, "wave", StringComparison.Ordinal))
            {
                BuildWave(_activityIcon, centerX, centerY, diameter);
            }
            else
            {
                BuildHammer(_activityIcon, centerX, centerY, diameter);
            }
            GraphicsState state = graphics.Save();
            graphics.TranslateTransform(shadow, shadow);
            graphics.FillPath(_shadowBrush, _activityIcon);
            graphics.Restore(state);
            graphics.FillPath(String.Equals(miniGame.MiniGameType, "wave", StringComparison.Ordinal)
                ? _waveBrush : _hammerBrush, _activityIcon);
            graphics.DrawPath(_outlinePen, _activityIcon);
        }

        private static void BuildHammer(GraphicsPath path, float x, float y, float d)
        {
            path.Reset();
            PointF[] head = { new PointF(x-d*.27f,y-d*.34f), new PointF(x+d*.30f,y-d*.08f), new PointF(x+d*.18f,y+d*.14f), new PointF(x-d*.39f,y-d*.12f) };
            path.AddPolygon(head);
            path.AddLine(x-d*.11f,y-d*.27f,x-d*.22f,y-d*.04f);
            path.AddLine(x+d*.10f,y-d*.16f,x-d*.01f,y+d*.07f);
            PointF[] handle = { new PointF(x-d*.06f,y-d*.01f), new PointF(x+d*.07f,y+d*.05f), new PointF(x-d*.27f,y+d*.40f), new PointF(x-d*.40f,y+d*.34f) };
            path.AddPolygon(handle);
        }

        private static void BuildWave(GraphicsPath path, float x, float y, float d)
        {
            path.Reset();
            for (int strand = 0; strand < 4; strand++)
            {
                float offset = (strand - 1.5f) * d * .11f;
                path.StartFigure();
                path.AddBezier(x-d*.42f+offset,y+d*.28f,x-d*.23f+offset,y-d*.18f,x-d*.02f+offset,y-d*.43f,x+d*.18f+offset,y-d*.22f);
                path.AddBezier(x+d*.18f+offset,y-d*.22f,x+d*.04f+offset,y-d*.06f,x+d*.01f+offset,y+d*.15f,x+d*.12f+offset,y+d*.34f);
                path.AddLine(x+d*.12f+offset,y+d*.34f,x-d*.01f+offset,y+d*.35f);
                path.AddBezier(x-d*.01f+offset,y+d*.35f,x-d*.17f+offset,y+d*.08f,x-d*.18f+offset,y-d*.04f,x-d*.42f+offset,y+d*.28f);
                path.CloseFigure();
            }
        }

        private static void BuildWing(GraphicsPath path, float x, float y, float d, float side)
        {
            path.Reset();
            path.StartFigure();
            path.AddLine(x + side * d * .10f, y + d * .05f, x + side * d * .31f, y - d * .17f);
            path.AddLine(x + side * d * .31f, y - d * .17f, x + side * d * .48f, y - d * .18f);
            path.AddLine(x + side * d * .48f, y - d * .18f, x + side * d * .40f, y - d * .04f);
            path.AddLine(x + side * d * .40f, y - d * .04f, x + side * d * .50f, y + d * .01f);
            path.AddLine(x + side * d * .50f, y + d * .01f, x + side * d * .35f, y + d * .09f);
            path.AddLine(x + side * d * .35f, y + d * .09f, x + side * d * .40f, y + d * .18f);
            path.AddLine(x + side * d * .40f, y + d * .18f, x + side * d * .13f, y + d * .13f);
            path.CloseFigure();
        }

        private static void BuildArrow(GraphicsPath path, float x, float y, float d)
        {
            path.Reset();
            path.StartFigure();
            path.AddLine(x - d * .10f, y + d * .40f, x + d * .10f, y + d * .40f);
            path.AddLine(x + d * .10f, y + d * .40f, x + d * .10f, y - d * .13f);
            path.AddLine(x + d * .10f, y - d * .13f, x + d * .27f, y - d * .13f);
            path.AddLine(x + d * .27f, y - d * .13f, x, y - d * .43f);
            path.AddLine(x, y - d * .43f, x - d * .27f, y - d * .13f);
            path.AddLine(x - d * .27f, y - d * .13f, x - d * .10f, y - d * .13f);
            path.CloseFigure();
        }

        public void DrawDebugLabel(Graphics graphics, WorldMole mole, float x, float y, float diameter, float textScale, float displayScale)
        {
            if (_disposed || graphics == null || mole == null) return;
            string text = String.Format(CultureInfo.InvariantCulture, "{0}_{1}", (mole.MiniGameType ?? "fly").ToUpperInvariant(), mole.MiniGameId);
            float size = Math.Max(8f, 9.5f * textScale * displayScale);
            if (_debugFont == null || Math.Abs(_debugFontSize - size) >= 0.01f)
            {
                if (_debugFont != null) _debugFont.Dispose();
                _debugFontSize = size;
                _debugFont = new Font(FontFamily.GenericSansSerif, size, FontStyle.Bold, GraphicsUnit.Pixel);
            }
            float left = x + diameter * .55f;
            float top = y - size * .55f;
            graphics.DrawString(text, _debugFont, _debugOutlineBrush, left + 1f, top + 1f);
            graphics.DrawString(text, _debugFont, _debugForegroundBrush, left, top);
        }

        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;
            _shadowBrush.Dispose();
            _wingBrush.Dispose();
            _arrowBrush.Dispose();
            _hammerBrush.Dispose();
            _waveBrush.Dispose();
            _debugOutlineBrush.Dispose();
            _debugForegroundBrush.Dispose();
            _outlinePen.Dispose();
            _leftWing.Dispose();
            _rightWing.Dispose();
            _arrow.Dispose();
            _activityIcon.Dispose();
            if (_debugFont != null) _debugFont.Dispose();
        }
    }
}
