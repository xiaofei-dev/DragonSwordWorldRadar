using System;
using System.Drawing;
using System.Drawing.Drawing2D;

namespace DragonSwordWorldRadar
{
    internal sealed class HeightIndicatorRenderer : IDisposable
    {
        private const double DeadZone = 100.0;
        private const double Sensitivity = 2500.0;
        private const double MaximumAngleDegrees = 85.0;

        private readonly Pen _outlinePen = new Pen(
            Color.FromArgb(235, 5, 12, 22),
            1f);
        private readonly Pen _indicatorPen = new Pen(Color.White, 1f);
        private readonly SolidBrush _outlineBrush = new SolidBrush(
            Color.FromArgb(235, 5, 12, 22));
        private readonly SolidBrush _indicatorBrush = new SolidBrush(
            Color.White);
        private readonly PointF[] _outlineHead = new PointF[3];
        private readonly PointF[] _colorHead = new PointF[3];
        private bool _disposed;

        public HeightIndicatorRenderer()
        {
            _outlinePen.StartCap = LineCap.Round;
            _outlinePen.EndCap = LineCap.Round;
            _indicatorPen.StartCap = LineCap.Round;
            _indicatorPen.EndCap = LineCap.Round;
        }

        public void Draw(
            Graphics graphics,
            float anchorX,
            float anchorY,
            float markerDiameter,
            double playerZ,
            bool hasPlayerZ,
            double treasureZ,
            bool hasTreasureZ,
            Color color,
            float displayScale,
            double comparablePlayerZOffset)
        {
            if (_disposed
                || graphics == null
                || !hasPlayerZ
                || !hasTreasureZ)
            {
                return;
            }

            double deltaZ = treasureZ -
                (playerZ + comparablePlayerZOffset);
            if (Math.Abs(deltaZ) <= DeadZone)
            {
                deltaZ = 0.0;
            }

            // 12 o'clock means above, 3 o'clock means near the same height,
            // and 6 o'clock means below. The pointer is anchored just left of
            // the marker, matching the stable6 geometry.
            double pointerAngle = -Math.Atan(deltaZ / Sensitivity);
            double maximumAngle =
                MaximumAngleDegrees * Math.PI / 180.0;
            pointerAngle = Math.Max(
                -maximumAngle,
                Math.Min(maximumAngle, pointerAngle));

            float directionX = (float)Math.Cos(pointerAngle);
            float directionY = (float)Math.Sin(pointerAngle);
            float perpendicularX = -directionY;
            float perpendicularY = directionX;

            float indicatorLength = Math.Min(
                36f,
                Math.Max(20f, 24f * displayScale));
            float markerGap = Math.Min(
                3f,
                Math.Max(1.5f, 2f * displayScale));
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
                Math.Max(8f, 10f * displayScale));
            float headHalfWidth = Math.Min(
                8f,
                Math.Max(4f, 5f * displayScale));
            float baseX = tipX - directionX * headLength;
            float baseY = tipY - directionY * headLength;

            _outlinePen.Width = Math.Min(
                11f,
                Math.Max(6f, 8f * displayScale));
            _indicatorPen.Width = Math.Min(
                8.5f,
                Math.Max(4.5f, 6f * displayScale));
            Color indicatorColor = Color.FromArgb(
                255,
                color.R,
                color.G,
                color.B);
            _indicatorPen.Color = indicatorColor;
            _indicatorBrush.Color = indicatorColor;

            graphics.DrawLine(
                _outlinePen,
                startX,
                startY,
                baseX,
                baseY);
            graphics.DrawLine(
                _indicatorPen,
                startX,
                startY,
                baseX,
                baseY);

            float headOutlineExpansion = Math.Min(
                3f,
                Math.Max(2f, 2f * displayScale));
            SetHead(
                _outlineHead,
                tipX,
                tipY,
                baseX,
                baseY,
                perpendicularX,
                perpendicularY,
                headHalfWidth + headOutlineExpansion);
            SetHead(
                _colorHead,
                tipX,
                tipY,
                baseX,
                baseY,
                perpendicularX,
                perpendicularY,
                headHalfWidth);
            graphics.FillPolygon(_outlineBrush, _outlineHead);
            graphics.FillPolygon(_indicatorBrush, _colorHead);
        }

        public void Dispose()
        {
            if (_disposed)
            {
                return;
            }
            _disposed = true;
            _outlinePen.Dispose();
            _indicatorPen.Dispose();
            _outlineBrush.Dispose();
            _indicatorBrush.Dispose();
        }

        private static void SetHead(
            PointF[] points,
            float tipX,
            float tipY,
            float baseX,
            float baseY,
            float perpendicularX,
            float perpendicularY,
            float halfWidth)
        {
            points[0] = new PointF(tipX, tipY);
            points[1] = new PointF(
                baseX + perpendicularX * halfWidth,
                baseY + perpendicularY * halfWidth);
            points[2] = new PointF(
                baseX - perpendicularX * halfWidth,
                baseY - perpendicularY * halfWidth);
        }
    }
}
