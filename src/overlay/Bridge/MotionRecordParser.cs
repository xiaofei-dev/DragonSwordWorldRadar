using System;

namespace DragonSwordWorldRadar
{
    internal static class MotionRecordParser
    {
        // Parses the fixed 21-field compact ASCII motion record directly from
        // a reusable byte buffer. The leading and trailing sequence values
        // must match before a frame is published.
        public static bool TryParse(
            byte[] buffer,
            int length,
            MotionFrame frame,
            WorldMapState worldMap)
        {
            if (buffer == null
                || length <= 0
                || length > buffer.Length
                || frame == null
                || worldMap == null)
            {
                return false;
            }
            FieldReader reader = new FieldReader(buffer, length);

            long sequence;
            int generation;
            int enabled;
            string mode;
            double playerX;
            double playerY;
            double playerZ;
            int hasPlayerZ;
            double radius;
            int mapId;
            double dimensions;
            double uiSize;
            double left;
            double top;
            double zoom;
            double viewportWidth;
            double viewportHeight;
            double viewportScale;
            double playerMapX;
            double playerMapY;
            long trailingSequence;

            if (!reader.TryReadInt64(out sequence)
                || !reader.TryReadInt32(out generation)
                || !reader.TryReadInt32(out enabled)
                || !reader.TryReadMode(out mode)
                || !reader.TryReadDouble(out playerX)
                || !reader.TryReadDouble(out playerY)
                || !reader.TryReadDouble(out playerZ)
                || !reader.TryReadInt32(out hasPlayerZ)
                || !reader.TryReadDouble(out radius)
                || !reader.TryReadInt32(out mapId)
                || !reader.TryReadDouble(out dimensions)
                || !reader.TryReadDouble(out uiSize)
                || !reader.TryReadDouble(out left)
                || !reader.TryReadDouble(out top)
                || !reader.TryReadDouble(out zoom)
                || !reader.TryReadDouble(out viewportWidth)
                || !reader.TryReadDouble(out viewportHeight)
                || !reader.TryReadDouble(out viewportScale)
                || !reader.TryReadDouble(out playerMapX)
                || !reader.TryReadDouble(out playerMapY)
                || !reader.TryReadInt64(out trailingSequence)
                || !reader.AtEnd
                || sequence != trailingSequence
                || sequence < 0
                || generation < 0)
            {
                return false;
            }

            WorldMapState map = null;
            if (String.Equals(mode, "world", StringComparison.Ordinal))
            {
                if (mapId <= 0
                    || dimensions <= 0
                    || uiSize <= 0
                    || zoom <= 0
                    || viewportWidth <= 0
                    || viewportHeight <= 0
                    || viewportScale <= 0)
                {
                    return false;
                }

                worldMap.mapId = mapId;
                worldMap.dimensions = dimensions;
                worldMap.uiSize = uiSize;
                worldMap.left = left;
                worldMap.top = top;
                worldMap.zoom = zoom;
                worldMap.viewportWidth = viewportWidth;
                worldMap.viewportHeight = viewportHeight;
                worldMap.viewportScale = viewportScale;
                worldMap.playerWorldX = playerX;
                worldMap.playerWorldY = playerY;
                worldMap.playerMapX = playerMapX;
                worldMap.playerMapY = playerMapY;
                map = worldMap;
            }

            frame.Sequence = sequence;
            frame.Generation = generation;
            frame.Enabled = enabled != 0;
            frame.Mode = mode;
            frame.PlayerX = playerX;
            frame.PlayerY = playerY;
            frame.PlayerZ = playerZ;
            frame.HasPlayerZ = hasPlayerZ != 0;
            frame.Radius = radius;
            frame.WorldMap = map;
            return true;
        }

        private struct FieldReader
        {
            private readonly byte[] _buffer;
            private readonly int _length;
            private int _position;
            private bool _lastTokenHadDelimiter;

            public FieldReader(byte[] buffer, int length)
            {
                _buffer = buffer;
                _position = 0;
                _length = TrimmedLength(buffer, length);
                _lastTokenHadDelimiter = false;
            }

            public bool AtEnd
            {
                get
                {
                    return _position == _length
                        && !_lastTokenHadDelimiter;
                }
            }

            public bool TryReadInt32(out int value)
            {
                long parsed;
                if (!TryReadInteger(out parsed)
                    || parsed < Int32.MinValue
                    || parsed > Int32.MaxValue)
                {
                    value = 0;
                    return false;
                }
                value = (int)parsed;
                return true;
            }

            public bool TryReadInt64(out long value)
            {
                return TryReadInteger(out value);
            }

            public bool TryReadMode(out string mode)
            {
                int start;
                int count;
                if (!TryReadToken(out start, out count))
                {
                    mode = null;
                    return false;
                }
                if (Matches(start, count, "radar"))
                {
                    mode = "radar";
                    return true;
                }
                if (Matches(start, count, "world"))
                {
                    mode = "world";
                    return true;
                }
                if (Matches(start, count, "disabled"))
                {
                    mode = "disabled";
                    return true;
                }
                mode = null;
                return false;
            }

            public bool TryReadDouble(out double value)
            {
                int start;
                int count;
                if (!TryReadToken(out start, out count)
                    || count == 0)
                {
                    value = 0.0;
                    return false;
                }

                int index = start;
                int end = start + count;
                bool negative = false;
                if (_buffer[index] == (byte)'-'
                    || _buffer[index] == (byte)'+')
                {
                    negative = _buffer[index] == (byte)'-';
                    index++;
                }

                bool hasDigit = false;
                double result = 0.0;
                while (index < end && IsDigit(_buffer[index]))
                {
                    hasDigit = true;
                    result = result * 10.0 +
                        (_buffer[index] - (byte)'0');
                    index++;
                }

                if (index < end && _buffer[index] == (byte)'.')
                {
                    index++;
                    double scale = 0.1;
                    while (index < end && IsDigit(_buffer[index]))
                    {
                        hasDigit = true;
                        result += (_buffer[index] - (byte)'0') * scale;
                        scale *= 0.1;
                        index++;
                    }
                }

                int exponent = 0;
                bool exponentNegative = false;
                if (index < end
                    && (_buffer[index] == (byte)'e'
                        || _buffer[index] == (byte)'E'))
                {
                    index++;
                    if (index < end
                        && (_buffer[index] == (byte)'-'
                            || _buffer[index] == (byte)'+'))
                    {
                        exponentNegative =
                            _buffer[index] == (byte)'-';
                        index++;
                    }
                    int exponentDigits = 0;
                    while (index < end && IsDigit(_buffer[index]))
                    {
                        exponentDigits++;
                        exponent = exponent * 10 +
                            (_buffer[index] - (byte)'0');
                        if (exponent > 308)
                        {
                            value = 0.0;
                            return false;
                        }
                        index++;
                    }
                    if (exponentDigits == 0)
                    {
                        value = 0.0;
                        return false;
                    }
                }

                if (!hasDigit || index != end)
                {
                    value = 0.0;
                    return false;
                }
                if (exponent != 0)
                {
                    result *= Math.Pow(
                        10.0,
                        exponentNegative ? -exponent : exponent);
                }
                value = negative ? -result : result;
                return !Double.IsNaN(value)
                    && !Double.IsInfinity(value);
            }

            private bool TryReadInteger(out long value)
            {
                int start;
                int count;
                if (!TryReadToken(out start, out count)
                    || count == 0)
                {
                    value = 0;
                    return false;
                }

                int index = start;
                int end = start + count;
                bool negative = false;
                if (_buffer[index] == (byte)'-'
                    || _buffer[index] == (byte)'+')
                {
                    negative = _buffer[index] == (byte)'-';
                    index++;
                }
                if (index == end)
                {
                    value = 0;
                    return false;
                }

                long result = 0;
                while (index < end)
                {
                    byte current = _buffer[index++];
                    if (!IsDigit(current))
                    {
                        value = 0;
                        return false;
                    }
                    int digit = current - (byte)'0';
                    if (result > (Int64.MaxValue - digit) / 10)
                    {
                        value = 0;
                        return false;
                    }
                    result = result * 10 + digit;
                }
                value = negative ? -result : result;
                return true;
            }

            private bool TryReadToken(out int start, out int count)
            {
                if (_position >= _length)
                {
                    start = 0;
                    count = 0;
                    return false;
                }

                start = _position;
                while (_position < _length
                    && _buffer[_position] != (byte)'|')
                {
                    _position++;
                }
                count = _position - start;
                _lastTokenHadDelimiter = _position < _length;
                if (_lastTokenHadDelimiter)
                {
                    _position++;
                }
                return true;
            }

            private bool Matches(int start, int count, string value)
            {
                if (count != value.Length)
                {
                    return false;
                }
                for (int index = 0; index < count; index++)
                {
                    if (_buffer[start + index] != (byte)value[index])
                    {
                        return false;
                    }
                }
                return true;
            }

            private static int TrimmedLength(byte[] buffer, int length)
            {
                if (buffer == null || length <= 0)
                {
                    return 0;
                }
                int end = length;
                while (end > 0)
                {
                    byte current = buffer[end - 1];
                    if (current != (byte)'\r'
                        && current != (byte)'\n'
                        && current != (byte)' '
                        && current != (byte)'\t')
                    {
                        break;
                    }
                    end--;
                }
                return end;
            }

            private static bool IsDigit(byte value)
            {
                return value >= (byte)'0' && value <= (byte)'9';
            }
        }
    }
}
