using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    internal static class BossMarkerRenderer
    {
        // The overlay cannot ask Unreal to paint a PaperSprite into an external
        // WinForms surface. This vector silhouette reproduces the game's
        // Icon_Mark_FieldBoss_Sprite role and is shared by all nine bosses.
        public static void DrawMarker(
            Graphics graphics,
            float centerX,
            float centerY,
            float diameter,
            float displayScale)
        {
            float half = diameter * 0.50f;
            float inset = Math.Max(3f, 4f * displayScale);
            float borderWidth = RadarMarkerStyle.GetBossOutlineWidth(displayScale);
            float shadowOffset = Math.Max(1.2f, 1.8f * displayScale);

            PointF[] diamond = new[]
            {
                new PointF(centerX, centerY - half),
                new PointF(centerX + half, centerY),
                new PointF(centerX, centerY + half),
                new PointF(centerX - half, centerY),
            };
            PointF[] shadowDiamond = new[]
            {
                new PointF(centerX + shadowOffset, centerY - half + shadowOffset),
                new PointF(centerX + half + shadowOffset, centerY + shadowOffset),
                new PointF(centerX + shadowOffset, centerY + half + shadowOffset),
                new PointF(centerX - half + shadowOffset, centerY + shadowOffset),
            };
            PointF[] innerDiamond = new[]
            {
                new PointF(centerX, centerY - half + inset),
                new PointF(centerX + half - inset, centerY),
                new PointF(centerX, centerY + half - inset),
                new PointF(centerX - half + inset, centerY),
            };

            using (Brush shadow = new SolidBrush(RadarMarkerStyle.Shadow))
            using (Brush backing = new SolidBrush(RadarMarkerStyle.BossBacking))
            using (Pen outer = new Pen(RadarMarkerStyle.Outline, borderWidth))
            using (Pen inner = new Pen(RadarMarkerStyle.BossInner, RadarMarkerStyle.GetBossInnerWidth(displayScale)))
            using (GraphicsPath silhouette = BuildSilhouette(centerX, centerY, diameter * 0.50f))
            using (Brush iconShadow = new SolidBrush(RadarMarkerStyle.Shadow))
            using (Brush icon = new SolidBrush(RadarMarkerStyle.BossIcon))
            using (Matrix iconShadowTransform = new Matrix())
            using (GraphicsPath iconShadowPath = (GraphicsPath)silhouette.Clone())
            {
                outer.LineJoin = LineJoin.Round;
                inner.LineJoin = LineJoin.Round;
                graphics.FillPolygon(shadow, shadowDiamond);
                graphics.FillPolygon(backing, diamond);
                graphics.DrawPolygon(outer, diamond);
                graphics.DrawPolygon(inner, innerDiamond);

                iconShadowTransform.Translate(shadowOffset, shadowOffset);
                iconShadowPath.Transform(iconShadowTransform);
                graphics.FillPath(iconShadow, iconShadowPath);
                graphics.FillPath(icon, silhouette);
            }
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
                Point(left, top, width, height, 0.35f, 0.58f),
            });
            path.AddPolygon(new[]
            {
                Point(left, top, width, height, 0.54f, 0.43f),
                Point(left, top, width, height, 0.92f, 0.03f),
                Point(left, top, width, height, 0.84f, 0.30f),
                Point(left, top, width, height, 0.98f, 0.22f),
                Point(left, top, width, height, 0.83f, 0.47f),
                Point(left, top, width, height, 0.65f, 0.58f),
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
                Point(left, top, width, height, 0.34f, 0.63f),
            });
            path.AddPolygon(new[]
            {
                Point(left, top, width, height, 0.63f, 0.53f),
                Point(left, top, width, height, 0.91f, 0.52f),
                Point(left, top, width, height, 1.00f, 0.63f),
                Point(left, top, width, height, 0.82f, 0.66f),
                Point(left, top, width, height, 0.87f, 0.92f),
                Point(left, top, width, height, 0.71f, 0.78f),
                Point(left, top, width, height, 0.66f, 0.63f),
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
                Point(left, top, width, height, 0.33f, 0.42f),
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

        public static void DrawDebugLabel(
            Graphics graphics,
            BossPoint boss,
            float x,
            float y,
            float diameter,
            float textScale,
            float displayScale)
        {
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
            using (Brush outline = new SolidBrush(
                Color.FromArgb(235, 0, 0, 0)))
            using (Brush foreground = new SolidBrush(
                Color.FromArgb(255, 210, 210, 200)))
            {
                float left = x + diameter * 0.55f;
                float top = y - size * 0.55f;
                graphics.DrawString(text, font, outline, left + 1, top + 1);
                graphics.DrawString(text, font, foreground, left, top);
            }
        }
    }
}
