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
        private readonly Brush _debugOutlineBrush = new SolidBrush(Color.FromArgb(235, 0, 0, 0));
        private readonly Brush _debugForegroundBrush = new SolidBrush(Color.FromArgb(255, 210, 240, 255));
        private readonly Pen _outlinePen = new Pen(RadarMarkerStyle.Outline, 1f);
        private readonly GraphicsPath _leftWing = new GraphicsPath();
        private readonly GraphicsPath _rightWing = new GraphicsPath();
        private readonly GraphicsPath _arrow = new GraphicsPath();
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
            string text = String.Format(CultureInfo.InvariantCulture, "FLY_{0}", mole.MiniGameId);
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
            _debugOutlineBrush.Dispose();
            _debugForegroundBrush.Dispose();
            _outlinePen.Dispose();
            _leftWing.Dispose();
            _rightWing.Dispose();
            _arrow.Dispose();
            if (_debugFont != null) _debugFont.Dispose();
        }
    }
}
