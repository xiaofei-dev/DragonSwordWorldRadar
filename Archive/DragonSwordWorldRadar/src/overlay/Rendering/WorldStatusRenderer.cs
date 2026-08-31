using System;
using System.Drawing;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    // Transparent top-right game-time display. The four visual phases are
    // derived only from the normalized game clock; unproven weather scalars
    // are intentionally not sampled or exposed as user-facing weather.
    internal sealed class WorldStatusRenderer : IDisposable
    {
        internal const float StatusGroupReferenceWidth = 138f;
        internal const float StatusStripGapReference = -6f;
        internal const float PhaseFontReferencePixels = 13f;
        private const int MorningStartSeconds = 6 * 3600;
        private const int AfternoonStartSeconds = 12 * 3600;
        private const int EveningStartSeconds = 18 * 3600;
        private const int NightStartSeconds = 21 * 3600;

        private readonly Brush _dialBrush = new SolidBrush(
            Color.FromArgb(238, 29, 38, 49));
        private readonly Brush _morningBrush = new SolidBrush(
            Color.FromArgb(255, 255, 220, 111));
        private readonly Brush _afternoonBrush = new SolidBrush(
            Color.FromArgb(255, 255, 190, 62));
        private readonly Brush _eveningBrush = new SolidBrush(
            Color.FromArgb(255, 255, 132, 61));
        private readonly Brush _moonBrush = new SolidBrush(
            Color.FromArgb(255, 210, 228, 255));
        private readonly Brush _moonCutoutBrush = new SolidBrush(
            Color.FromArgb(255, 29, 38, 49));
        private readonly Brush _primaryTextBrush = new SolidBrush(
            Color.FromArgb(255, 245, 248, 252));
        private readonly Brush _secondaryTextBrush = new SolidBrush(
            Color.FromArgb(255, 205, 222, 235));
        private readonly Brush _textShadowBrush = new SolidBrush(
            Color.FromArgb(230, 5, 8, 12));
        private readonly Pen _borderPen = new Pen(
            Color.FromArgb(220, 7, 11, 17), 1f);
        private readonly Pen _sunRayPen = new Pen(
            Color.FromArgb(255, 255, 197, 74), 1.5f);

        private Font _timeFont;
        private Font _phaseFont;
        private float _fontScale;
        private int _cachedMinute = Int32.MinValue;
        private int _cachedPhase = Int32.MinValue;
        private bool _cachedTimeAvailable;
        private string _timeText = "--:--";
        private string _phaseText = "TIME";
        private bool _disposed;

        public void Draw(
            Graphics graphics,
            int clientWidth,
            int clientHeight,
            int minimapAreaSize,
            float displayScale,
            bool timeAvailable,
            int timeSeconds)
        {
            if (_disposed || graphics == null || clientWidth <= 0
                || clientHeight <= 0
                || minimapAreaSize <= 0)
            {
                return;
            }

            float scale = Math.Max(0.5f, displayScale);
            EnsureFonts(scale);
            UpdateText(timeAvailable, timeSeconds);

            // The minimap occupies the top square. Keep the complete status
            // group in the compact transparent strip immediately below it.
            RectangleF group = CalculateGroupBounds(
                clientWidth,
                clientHeight,
                minimapAreaSize,
                scale);
            float groupWidth = group.Width;
            float groupLeft = group.Left;
            float dialSize = 38f * scale;
            float dialLeft = groupLeft + groupWidth - dialSize;
            float dialTop = group.Top + 2f * scale;
            float textLeft = groupLeft;

            _borderPen.Width = Math.Max(1f, 1.35f * scale);
            graphics.FillEllipse(
                _dialBrush,
                dialLeft,
                dialTop,
                dialSize,
                dialSize);
            graphics.DrawEllipse(
                _borderPen,
                dialLeft,
                dialTop,
                dialSize,
                dialSize);
            DrawTimeDial(
                graphics,
                dialLeft + dialSize / 2f,
                dialTop + dialSize / 2f,
                dialSize,
                timeAvailable,
                timeSeconds,
                scale);

            DrawTextWithShadow(
                graphics,
                _timeText,
                _timeFont,
                _primaryTextBrush,
                textLeft,
                group.Top);
            DrawTextWithShadow(
                graphics,
                _phaseText,
                _phaseFont,
                _secondaryTextBrush,
                textLeft + 1f * scale,
                group.Top + 27f * scale);
        }

        internal static string FormatClock(int seconds)
        {
            if (seconds < 0 || seconds >= 86400)
            {
                return "--:--";
            }
            int hours = seconds / 3600;
            int minutes = (seconds % 3600) / 60;
            return hours.ToString("D2", CultureInfo.InvariantCulture)
                + ":"
                + minutes.ToString("D2", CultureInfo.InvariantCulture);
        }

        internal static string FormatPhase(int seconds)
        {
            int phase = GetPhase(seconds);
            if (phase == 0) return "MORNING";
            if (phase == 1) return "AFTERNOON";
            if (phase == 2) return "EVENING";
            if (phase == 3) return "NIGHT";
            return "TIME";
        }

        private static int GetPhase(int seconds)
        {
            if (seconds < 0 || seconds >= 86400)
            {
                return -1;
            }
            if (seconds >= MorningStartSeconds
                && seconds < AfternoonStartSeconds)
            {
                return 0;
            }
            if (seconds >= AfternoonStartSeconds
                && seconds < EveningStartSeconds)
            {
                return 1;
            }
            if (seconds >= EveningStartSeconds
                && seconds < NightStartSeconds)
            {
                return 2;
            }
            return 3;
        }

        private void UpdateText(bool timeAvailable, int timeSeconds)
        {
            int minute = timeAvailable ? timeSeconds / 60 : -1;
            int phase = timeAvailable ? GetPhase(timeSeconds) : -1;
            if (_cachedTimeAvailable == timeAvailable
                && _cachedMinute == minute
                && _cachedPhase == phase)
            {
                return;
            }
            _cachedTimeAvailable = timeAvailable;
            _cachedMinute = minute;
            _cachedPhase = phase;
            _timeText = timeAvailable ? FormatClock(timeSeconds) : "--:--";
            _phaseText = timeAvailable ? FormatPhase(timeSeconds) : "TIME";
        }

        private void EnsureFonts(float scale)
        {
            if (_timeFont != null
                && Math.Abs(_fontScale - scale) < 0.001f)
            {
                return;
            }
            if (_timeFont != null)
            {
                _timeFont.Dispose();
                _phaseFont.Dispose();
            }
            _fontScale = scale;
            _timeFont = new Font(
                FontFamily.GenericSansSerif,
                Math.Max(14f, 22f * scale),
                FontStyle.Bold,
                GraphicsUnit.Pixel);
            _phaseFont = new Font(
                FontFamily.GenericSansSerif,
                Math.Max(11f, PhaseFontReferencePixels * scale),
                FontStyle.Bold,
                GraphicsUnit.Pixel);
        }

        internal static RectangleF CalculateGroupBounds(
            int clientWidth,
            int clientHeight,
            int minimapAreaSize,
            float displayScale)
        {
            float scale = Math.Max(0.5f, displayScale);
            float padding = 3f * scale;
            float groupWidth = StatusGroupReferenceWidth * scale;
            float groupHeight = 42f * scale;
            float left = Math.Max(padding, Math.Min(
                (clientWidth - groupWidth) / 2f,
                clientWidth - groupWidth - padding));
            float top = minimapAreaSize
                + StatusStripGapReference * scale;
            top = Math.Max(minimapAreaSize - 8f * scale, Math.Min(
                top,
                clientHeight - groupHeight - padding));
            return new RectangleF(
                left,
                top,
                Math.Max(1f, Math.Min(groupWidth, clientWidth - left)),
                Math.Max(1f, Math.Min(groupHeight, clientHeight - top)));
        }

        private void DrawTimeDial(
            Graphics graphics,
            float centerX,
            float centerY,
            float diameter,
            bool available,
            int seconds,
            float scale)
        {
            int phase = available ? GetPhase(seconds) : -1;
            if (phase < 0)
            {
                graphics.DrawLine(
                    _borderPen,
                    centerX - 6f * scale,
                    centerY,
                    centerX + 6f * scale,
                    centerY);
                return;
            }
            if (phase == 3)
            {
                DrawMoon(graphics, centerX, centerY, diameter);
                return;
            }

            Brush sunBrush = phase == 0
                ? _morningBrush
                : (phase == 1 ? _afternoonBrush : _eveningBrush);
            Color rayColor = phase == 0
                ? Color.FromArgb(255, 255, 220, 111)
                : (phase == 1
                    ? Color.FromArgb(255, 255, 190, 62)
                    : Color.FromArgb(255, 255, 132, 61));
            float verticalOffset = phase == 0
                ? 2f * scale
                : (phase == 2 ? 5f * scale : 0f);
            DrawSun(
                graphics,
                centerX,
                centerY + verticalOffset,
                diameter,
                scale,
                sunBrush,
                rayColor,
                phase != 2);
        }

        private void DrawSun(
            Graphics graphics,
            float centerX,
            float centerY,
            float diameter,
            float scale,
            Brush brush,
            Color rayColor,
            bool fullRays)
        {
            float sunSize = diameter * 0.28f;
            graphics.FillEllipse(
                brush,
                centerX - sunSize / 2f,
                centerY - sunSize / 2f,
                sunSize,
                sunSize);
            _sunRayPen.Color = rayColor;
            _sunRayPen.Width = Math.Max(1f, 1.6f * scale);
            float inner = diameter * 0.21f;
            float outer = diameter * 0.33f;
            int first = fullRays ? 0 : 4;
            int last = fullRays ? 8 : 8;
            for (int index = first; index < last; index++)
            {
                double angle = index * Math.PI / 4.0;
                float cosine = (float)Math.Cos(angle);
                float sine = (float)Math.Sin(angle);
                graphics.DrawLine(
                    _sunRayPen,
                    centerX + cosine * inner,
                    centerY + sine * inner,
                    centerX + cosine * outer,
                    centerY + sine * outer);
            }
        }

        private void DrawMoon(
            Graphics graphics,
            float centerX,
            float centerY,
            float diameter)
        {
            float size = diameter * 0.45f;
            graphics.FillEllipse(
                _moonBrush,
                centerX - size / 2f,
                centerY - size / 2f,
                size,
                size);
            graphics.FillEllipse(
                _moonCutoutBrush,
                centerX - size * 0.04f,
                centerY - size * 0.57f,
                size,
                size);
        }

        private void DrawTextWithShadow(
            Graphics graphics,
            string text,
            Font font,
            Brush brush,
            float x,
            float y)
        {
            float offset = Math.Max(1f, _fontScale);
            graphics.DrawString(
                text,
                font,
                _textShadowBrush,
                x + offset,
                y + offset);
            graphics.DrawString(text, font, brush, x, y);
        }

        public void Dispose()
        {
            if (_disposed) return;
            _disposed = true;
            if (_timeFont != null)
            {
                _timeFont.Dispose();
                _phaseFont.Dispose();
            }
            _dialBrush.Dispose();
            _morningBrush.Dispose();
            _afternoonBrush.Dispose();
            _eveningBrush.Dispose();
            _moonBrush.Dispose();
            _moonCutoutBrush.Dispose();
            _primaryTextBrush.Dispose();
            _secondaryTextBrush.Dispose();
            _textShadowBrush.Dispose();
            _borderPen.Dispose();
            _sunRayPen.Dispose();
        }
    }
}
