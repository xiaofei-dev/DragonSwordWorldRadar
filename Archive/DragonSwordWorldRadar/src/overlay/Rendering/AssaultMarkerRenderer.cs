using System;
using System.Drawing;
using System.Drawing.Drawing2D;

namespace DragonSwordWorldRadar
{
    internal sealed class AssaultMarkerRenderer : IDisposable
    {
        private static readonly PointF[] UnitDiamond =
        {
            new PointF(0f, -0.50f),
            new PointF(0.50f, 0f),
            new PointF(0f, 0.50f),
            new PointF(-0.50f, 0f)
        };

        private readonly PointF[] _innerDiamond = new PointF[4];
        private readonly Brush _shadowBrush =
            new SolidBrush(RadarMarkerStyle.Shadow);
        private readonly Brush _backingBrush =
            new SolidBrush(RadarMarkerStyle.BossBacking);
        private readonly Pen _outerPen =
            new Pen(RadarMarkerStyle.Outline, 1f);
        private readonly Pen _innerPen =
            new Pen(RadarMarkerStyle.BossInner, 1f);
        private readonly Pen _swordOutlinePen =
            new Pen(RadarMarkerStyle.Outline, 1f);
        private readonly Pen _swordPen =
            new Pen(Color.FromArgb(255, 255, 191, 67), 1f);
        private readonly Pen _swordHighlightPen =
            new Pen(Color.FromArgb(235, 255, 248, 220), 1f);
        private bool _disposed;

        public AssaultMarkerRenderer()
        {
            foreach (Pen pen in new[]
            {
                _outerPen,
                _innerPen,
                _swordOutlinePen,
                _swordPen,
                _swordHighlightPen
            })
            {
                pen.LineJoin = LineJoin.Round;
                pen.StartCap = LineCap.Round;
                pen.EndCap = LineCap.Round;
            }
        }

        public void DrawMarker(
            Graphics graphics,
            float centerX,
            float centerY,
            float diameter,
            float displayScale)
        {
            if (_disposed || graphics == null || diameter <= 0f)
            {
                return;
            }

            float inverseDiameter = 1f / diameter;
            float inset = Math.Max(3f, 4f * displayScale);
            float insetUnit = Math.Min(0.49f, inset * inverseDiameter);
            float shadowOffset = Math.Max(1.2f, 1.8f * displayScale)
                * inverseDiameter;

            _outerPen.Width = Math.Max(
                0.001f,
                RadarMarkerStyle.GetBossOutlineWidth(displayScale)
                    * inverseDiameter);
            _innerPen.Width = Math.Max(
                0.001f,
                RadarMarkerStyle.GetBossInnerWidth(displayScale)
                    * inverseDiameter);
            _swordOutlinePen.Width = Math.Max(
                0.001f,
                7.2f * displayScale * inverseDiameter);
            _swordPen.Width = Math.Max(
                0.001f,
                4.6f * displayScale * inverseDiameter);
            _swordHighlightPen.Width = Math.Max(
                0.001f,
                1.25f * displayScale * inverseDiameter);

            _innerDiamond[0] = new PointF(0f, -0.50f + insetUnit);
            _innerDiamond[1] = new PointF(0.50f - insetUnit, 0f);
            _innerDiamond[2] = new PointF(0f, 0.50f - insetUnit);
            _innerDiamond[3] = new PointF(-0.50f + insetUnit, 0f);

            GraphicsState saved = graphics.Save();
            try
            {
                graphics.TranslateTransform(centerX, centerY);
                graphics.ScaleTransform(diameter, diameter);

                graphics.TranslateTransform(shadowOffset, shadowOffset);
                graphics.FillPolygon(_shadowBrush, UnitDiamond);
                graphics.TranslateTransform(-shadowOffset, -shadowOffset);

                graphics.FillPolygon(_backingBrush, UnitDiamond);
                graphics.DrawPolygon(_outerPen, UnitDiamond);
                graphics.DrawPolygon(_innerPen, _innerDiamond);

                DrawSword(graphics, -0.23f, 0.25f, 0.23f, -0.25f);
                DrawSword(graphics, 0.23f, 0.25f, -0.23f, -0.25f);
            }
            finally
            {
                graphics.Restore(saved);
            }
        }

        private void DrawSword(
            Graphics graphics,
            float x1,
            float y1,
            float x2,
            float y2)
        {
            graphics.DrawLine(_swordOutlinePen, x1, y1, x2, y2);
            graphics.DrawLine(_swordPen, x1, y1, x2, y2);
            graphics.DrawLine(_swordHighlightPen, x1, y1, x2, y2);
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }
            _disposed = true;
            _shadowBrush.Dispose();
            _backingBrush.Dispose();
            _outerPen.Dispose();
            _innerPen.Dispose();
            _swordOutlinePen.Dispose();
            _swordPen.Dispose();
            _swordHighlightPen.Dispose();
        }
    }
}
