using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    internal sealed class BossMarkerRenderer : IDisposable
    {
        private static readonly PointF[] UnitDiamond =
        {
            new PointF(0f, -0.50f),
            new PointF(0.50f, 0f),
            new PointF(0f, 0.50f),
            new PointF(-0.50f, 0f)
        };

        private readonly PointF[] _innerDiamond = new PointF[4];
        private readonly GraphicsPath _unitSilhouette =
            BuildSilhouette(0f, 0f, 0.50f);
        private readonly Brush _shadowBrush =
            new SolidBrush(RadarMarkerStyle.Shadow);
        private readonly Brush _backingBrush =
            new SolidBrush(RadarMarkerStyle.BossBacking);
        private readonly Brush _iconBrush =
            new SolidBrush(RadarMarkerStyle.BossIcon);
        private readonly Brush _debugOutlineBrush =
            new SolidBrush(Color.FromArgb(235, 0, 0, 0));
        private readonly Brush _debugForegroundBrush =
            new SolidBrush(Color.FromArgb(255, 210, 210, 200));
        private readonly Pen _outerPen =
            new Pen(RadarMarkerStyle.Outline, 1f);
        private readonly Pen _innerPen =
            new Pen(RadarMarkerStyle.BossInner, 1f);
        private bool _disposed;

        public BossMarkerRenderer()
        {
            _outerPen.LineJoin = LineJoin.Round;
            _innerPen.LineJoin = LineJoin.Round;
        }

        // The overlay cannot ask Unreal to paint a PaperSprite into an external
        // WinForms surface. This vector silhouette reproduces the game's
        // Icon_Mark_FieldBoss_Sprite role and is shared by all nine bosses.
        //
        // Geometry, brushes, pens, and the silhouette path are retained for the
        // lifetime of the form. Each marker now creates only the GraphicsState
        // needed to compose with the caller's transform; stable6 allocated
        // arrays, paths, brushes, pens, a matrix, and a cloned path per boss.
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
            float insetUnit = Math.Min(
                0.49f,
                inset * inverseDiameter);
            float shadowOffset = Math.Max(
                1.2f,
                1.8f * displayScale) * inverseDiameter;

            _outerPen.Width = Math.Max(
                0.001f,
                RadarMarkerStyle.GetBossOutlineWidth(displayScale)
                    * inverseDiameter);
            _innerPen.Width = Math.Max(
                0.001f,
                RadarMarkerStyle.GetBossInnerWidth(displayScale)
                    * inverseDiameter);

            _innerDiamond[0] = new PointF(0f, -0.50f + insetUnit);
            _innerDiamond[1] = new PointF(0.50f - insetUnit, 0f);
            _innerDiamond[2] = new PointF(0f, 0.50f - insetUnit);
            _innerDiamond[3] = new PointF(-0.50f + insetUnit, 0f);

            GraphicsState saved = graphics.Save();
            try
            {
                // Default GDI+ transform order prepends each operation. Calling
                // Translate then Scale therefore maps unit geometry to
                // (center + diameter * point), which matches the old absolute
                // coordinate implementation.
                graphics.TranslateTransform(centerX, centerY);
                graphics.ScaleTransform(diameter, diameter);

                graphics.TranslateTransform(
                    shadowOffset,
                    shadowOffset);
                graphics.FillPolygon(_shadowBrush, UnitDiamond);
                graphics.TranslateTransform(
                    -shadowOffset,
                    -shadowOffset);

                graphics.FillPolygon(_backingBrush, UnitDiamond);
                graphics.DrawPolygon(_outerPen, UnitDiamond);
                graphics.DrawPolygon(_innerPen, _innerDiamond);

                graphics.TranslateTransform(
                    shadowOffset,
                    shadowOffset);
                graphics.FillPath(_shadowBrush, _unitSilhouette);
                graphics.TranslateTransform(
                    -shadowOffset,
                    -shadowOffset);
                graphics.FillPath(_iconBrush, _unitSilhouette);
            }
            finally
            {
                graphics.Restore(saved);
            }
        }

        public void DrawDebugLabel(
            Graphics graphics,
            BossPoint boss,
            float x,
            float y,
            float diameter,
            float textScale,
            float displayScale)
        {
            if (_disposed || graphics == null || boss == null)
            {
                return;
            }

            string text = String.Format(
                CultureInfo.InvariantCulture,
                "B_{0} {1}",
                boss.bossId,
                boss.status ?? "unknown");
            float size = Math.Max(
                8f,
                10f * textScale * displayScale);
            using (Font font = new Font(
                FontFamily.GenericSansSerif,
                size,
                FontStyle.Bold,
                GraphicsUnit.Pixel))
            {
                float left = x + diameter * 0.55f;
                float top = y - size * 0.55f;
                graphics.DrawString(
                    text,
                    font,
                    _debugOutlineBrush,
                    left + 1,
                    top + 1);
                graphics.DrawString(
                    text,
                    font,
                    _debugForegroundBrush,
                    left,
                    top);
            }
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }
            _disposed = true;
            _unitSilhouette.Dispose();
            _shadowBrush.Dispose();
            _backingBrush.Dispose();
            _iconBrush.Dispose();
            _debugOutlineBrush.Dispose();
            _debugForegroundBrush.Dispose();
            _outerPen.Dispose();
            _innerPen.Dispose();
        }

        private static GraphicsPath BuildSilhouette(
            float centerX,
            float centerY,
            float diameter)
        {
            float top = centerY - diameter * 0.56f;
            float left = centerX - diameter * 0.50f;
            float width = diameter;
            float height = diameter * 1.12f;

            GraphicsPath path = new GraphicsPath(
                FillMode.Winding);

            // Upper wings from the native field-boss icon.
            path.AddPolygon(new[]
            {
                Point(left, top, width, height, 0.46f, 0.43f),
                Point(left, top, width, height, 0.08f, 0.03f),
                Point(left, top, width, height, 0.16f, 0.30f),
                Point(left, top, width, height, 0.02f, 0.22f),
                Point(left, top, width, height, 0.17f, 0.47f),
                Point(left, top, width, height, 0.35f, 0.58f)
            });
            path.AddPolygon(new[]
            {
                Point(left, top, width, height, 0.54f, 0.43f),
                Point(left, top, width, height, 0.92f, 0.03f),
                Point(left, top, width, height, 0.84f, 0.30f),
                Point(left, top, width, height, 0.98f, 0.22f),
                Point(left, top, width, height, 0.83f, 0.47f),
                Point(left, top, width, height, 0.65f, 0.58f)
            });

            // Lower hooked wings/arms.
            path.AddPolygon(new[]
            {
                Point(left, top, width, height, 0.37f, 0.53f),
                Point(left, top, width, height, 0.09f, 0.52f),
                Point(left, top, width, height, 0.00f, 0.63f),
                Point(left, top, width, height, 0.18f, 0.66f),
                Point(left, top, width, height, 0.13f, 0.92f),
                Point(left, top, width, height, 0.29f, 0.78f),
                Point(left, top, width, height, 0.34f, 0.63f)
            });
            path.AddPolygon(new[]
            {
                Point(left, top, width, height, 0.63f, 0.53f),
                Point(left, top, width, height, 0.91f, 0.52f),
                Point(left, top, width, height, 1.00f, 0.63f),
                Point(left, top, width, height, 0.82f, 0.66f),
                Point(left, top, width, height, 0.87f, 0.92f),
                Point(left, top, width, height, 0.71f, 0.78f),
                Point(left, top, width, height, 0.66f, 0.63f)
            });

            // Crown, head, tapered torso and base.
            path.AddPolygon(new[]
            {
                Point(left, top, width, height, 0.39f, 0.29f),
                Point(left, top, width, height, 0.39f, 0.18f),
                Point(left, top, width, height, 0.45f, 0.30f),
                Point(left, top, width, height, 0.47f, 0.16f),
                Point(left, top, width, height, 0.50f, 0.31f),
                Point(left, top, width, height, 0.53f, 0.16f),
                Point(left, top, width, height, 0.55f, 0.30f),
                Point(left, top, width, height, 0.61f, 0.18f),
                Point(left, top, width, height, 0.61f, 0.29f),
                Point(left, top, width, height, 0.67f, 0.42f),
                Point(left, top, width, height, 0.61f, 0.58f),
                Point(left, top, width, height, 0.58f, 0.94f),
                Point(left, top, width, height, 0.68f, 0.94f),
                Point(left, top, width, height, 0.68f, 0.99f),
                Point(left, top, width, height, 0.32f, 0.99f),
                Point(left, top, width, height, 0.32f, 0.94f),
                Point(left, top, width, height, 0.42f, 0.94f),
                Point(left, top, width, height, 0.39f, 0.58f),
                Point(left, top, width, height, 0.33f, 0.42f)
            });

            return path;
        }

        private static PointF Point(
            float left,
            float top,
            float width,
            float height,
            float x,
            float y)
        {
            return new PointF(
                left + width * x,
                top + height * y);
        }
    }
}
