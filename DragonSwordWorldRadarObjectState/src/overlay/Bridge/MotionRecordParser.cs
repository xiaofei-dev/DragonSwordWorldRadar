using System;

namespace DragonSwordWorldRadar
{
    internal static class MotionRecordParser
    {
        private const int SupportedProtocolVersion = 6;

        // Parses the fixed 38-field compact ASCII motion/control record directly
        // from a reusable byte buffer. The explicit protocol version prevents a
        // mixed old/new deployment from interpreting shifted fields, and the
        // leading/trailing sequence values reject partial slot rewrites.
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
            int protocolVersion;
            int generation;
            int worldEpoch;
            double sampleTimestampMs;
            int enabled;
            string mode;
            int diagnosticMode;
            int showHeight;
            int showTreasureTypes;
            int showTreasures;
            int showBosses;
            int showMoles;
            long moleMask;
            int showWorldStatus;
            int worldTimeAvailable;
            int worldTimeSeconds;
            int weatherAvailable;
            int weatherState;
            int weatherBtState;
            double textScale;
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
                || !reader.TryReadInt32(out protocolVersion)
                || !reader.TryReadInt32(out generation)
                || !reader.TryReadInt32(out worldEpoch)
                || !reader.TryReadDouble(out sampleTimestampMs)
                || !reader.TryReadInt32(out enabled)
                || !reader.TryReadMode(out mode)
                || !reader.TryReadInt32(out diagnosticMode)
                || !reader.TryReadInt32(out showHeight)
                || !reader.TryReadInt32(out showTreasureTypes)
                || !reader.TryReadInt32(out showTreasures)
                || !reader.TryReadInt32(out showBosses)
                || !reader.TryReadInt32(out showMoles)
                || !reader.TryReadInt64(out moleMask)
                || !reader.TryReadInt32(out showWorldStatus)
                || !reader.TryReadInt32(out worldTimeAvailable)
                || !reader.TryReadInt32(out worldTimeSeconds)
                || !reader.TryReadInt32(out weatherAvailable)
                || !reader.TryReadInt32(out weatherState)
                || !reader.TryReadInt32(out weatherBtState)
                || !reader.TryReadDouble(out textScale)
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
                || protocolVersion != SupportedProtocolVersion
                || generation < 0
                || worldEpoch < 0
                || Double.IsNaN(sampleTimestampMs)
                || Double.IsInfinity(sampleTimestampMs)
                || sampleTimestampMs < 0
                || (enabled != 0 && enabled != 1)
                || diagnosticMode < 0
                || diagnosticMode > 2
                || (enabled == 0 && diagnosticMode != 0)
                || (showHeight != 0 && showHeight != 1)
                || (showTreasureTypes != 0 && showTreasureTypes != 1)
                || (showTreasures != 0 && showTreasures != 1)
                || (showBosses != 0 && showBosses != 1)
                || (showMoles != 0 && showMoles != 1)
                || moleMask < 0
                || moleMask >= (1L << 34)
                || (showWorldStatus != 0 && showWorldStatus != 1)
                || (worldTimeAvailable != 0 && worldTimeAvailable != 1)
                || worldTimeSeconds < 0
                || worldTimeSeconds >= 86400
                || (worldTimeAvailable == 0 && worldTimeSeconds != 0)
                || (weatherAvailable != 0 && weatherAvailable != 1)
                || weatherState < -1000000
                || weatherState > 1000000
                || weatherBtState < -1000000
                || weatherBtState > 1000000
                || (weatherAvailable == 0
                    && (weatherState != 0 || weatherBtState != 0))
                || (hasPlayerZ != 0 && hasPlayerZ != 1)
                || (enabled == 0
                    && !String.Equals(
                        mode,
                        "disabled",
                        StringComparison.Ordinal))
                || (enabled != 0
                    && String.Equals(
                        mode,
                        "disabled",
                        StringComparison.Ordinal))
                || (enabled != 0 && radius <= 0)
                || Double.IsNaN(textScale)
                || Double.IsInfinity(textScale)
                || textScale < 0.5
                || textScale > 2.0)
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
            frame.ProtocolVersion = protocolVersion;
            frame.Generation = generation;
            frame.WorldEpoch = worldEpoch;
            frame.SampleTimestampMs = sampleTimestampMs;
            frame.Enabled = enabled != 0;
            frame.Mode = mode;
            frame.DiagnosticMode = diagnosticMode;
            frame.ShowHeight = showHeight != 0;
            frame.ShowTreasureTypes = showTreasureTypes != 0;
            frame.ShowTreasures = showTreasures != 0;
            frame.ShowBosses = showBosses != 0;
            frame.ShowMoles = showMoles != 0;
            frame.MoleMask = moleMask;
            frame.ShowWorldStatus = showWorldStatus != 0;
            frame.WorldTimeAvailable = worldTimeAvailable != 0;
            frame.WorldTimeSeconds = worldTimeSeconds;
            frame.WeatherAvailable = weatherAvailable != 0;
            frame.WeatherState = weatherState;
            frame.WeatherBtState = weatherBtState;
            frame.TextScale = textScale;
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
