using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace DragonSwordWorldRadar
{
    internal sealed class BossRespawnRuleResolver
    {
        private static readonly TimeSpan ScanInterval =
            TimeSpan.FromSeconds(30);

        private readonly object _sync = new object();
        private DateTime _nextScanUtc;
        private BossRespawnRule _rule =
            BossRespawnRule.Daily(9, 0, "built-in-original");
        private string _lastSummary;

        public void Refresh()
        {
            lock (_sync)
            {
                if (DateTime.UtcNow < _nextScanUtc)
                {
                    return;
                }
                _nextScanUtc = DateTime.UtcNow.Add(ScanInterval);
            }

            BossRespawnRule detected;
            if (!TryDetectReadableOverride(out detected))
            {
                detected = BossRespawnRule.Daily(
                    9,
                    0,
                    "built-in-original");
            }

            string summary = detected.ToString();
            bool changed;
            lock (_sync)
            {
                _rule = detected;
                changed = summary != _lastSummary;
                _lastSummary = summary;
            }
            if (changed)
            {
                ErrorLog.WriteMessage(
                    "Boss respawn rule: " + summary);
            }
        }

        public DateTime NextAvailableUtc(
            DateTime destroyTimeUtc)
        {
            BossRespawnRule rule;
            lock (_sync)
            {
                rule = _rule;
            }
            return rule.NextAvailableUtc(destroyTimeUtc);
        }

        public bool IsAvailable(DateTime destroyTimeUtc)
        {
            return DateTime.UtcNow >=
                NextAvailableUtc(destroyTimeUtc);
        }

        public string Describe(DateTime destroyTimeUtc)
        {
            BossRespawnRule rule;
            lock (_sync)
            {
                rule = _rule;
            }
            DateTime next = rule.NextAvailableUtc(
                destroyTimeUtc);
            return String.Format(
                CultureInfo.InvariantCulture,
                "{0}; destroyUtc={1:O}; nextUtc={2:O}; available={3}",
                rule,
                destroyTimeUtc,
                next,
                DateTime.UtcNow >= next);
        }

        public string RuleSummary
        {
            get
            {
                lock (_sync)
                {
                    return _rule.ToString();
                }
            }
        }

        private static bool TryDetectReadableOverride(
            out BossRespawnRule rule)
        {
            rule = null;
            string pakRoot = Path.GetFullPath(Path.Combine(
                ModPath.BaseDirectory,
                "..",
                "..",
                "..",
                "..",
                "Content",
                "Paks"));
            if (!Directory.Exists(pakRoot))
            {
                return false;
            }

            IEnumerable<string> candidates = Directory
                .GetFiles(
                    pakRoot,
                    "*.pak",
                    SearchOption.AllDirectories)
                .Where(path =>
                {
                    try
                    {
                        return new FileInfo(path).Length <=
                            8L * 1024L * 1024L;
                    }
                    catch
                    {
                        return false;
                    }
                })
                .OrderByDescending(
                    Path.GetFileName,
                    StringComparer.OrdinalIgnoreCase);

            foreach (string path in candidates)
            {
                try
                {
                    byte[] bytes = File.ReadAllBytes(path);
                    string text = Encoding.UTF8.GetString(bytes);
                    BossRespawnRule parsed;
                    if (TryParseRule106(
                        text,
                        Path.GetFileName(path),
                        out parsed))
                    {
                        rule = parsed;
                        return true;
                    }
                }
                catch
                {
                }
            }
            return false;
        }

        private static bool TryParseRule106(
            string text,
            string source,
            out BossRespawnRule rule)
        {
            rule = null;
            if (String.IsNullOrEmpty(text))
            {
                return false;
            }

            MatchCollection jsonBlocks = Regex.Matches(
                text,
                "[\"']?106[\"']?\\s*:\\s*\\{(?<body>.{0,4096}?)\\}",
                RegexOptions.IgnoreCase |
                RegexOptions.Singleline);
            foreach (Match block in jsonBlocks)
            {
                if (TryParseRuleBody(
                    block.Groups["body"].Value,
                    source,
                    out rule))
                {
                    return true;
                }
            }

            MatchCollection xmlRows = Regex.Matches(
                text,
                "<[^>]*\\bID\\s*=\\s*[\"']106[\"'][^>]*>",
                RegexOptions.IgnoreCase |
                RegexOptions.Singleline);
            foreach (Match row in xmlRows)
            {
                if (TryParseRuleBody(
                    row.Value,
                    source,
                    out rule))
                {
                    return true;
                }
            }

            int offset = 0;
            while (offset < text.Length)
            {
                int id = text.IndexOf(
                    "\"106\"",
                    offset,
                    StringComparison.Ordinal);
                if (id < 0)
                {
                    break;
                }
                int start = Math.Max(0, id - 128);
                int length = Math.Min(
                    text.Length - start,
                    4096);
                if (TryParseRuleBody(
                    text.Substring(start, length),
                    source,
                    out rule))
                {
                    return true;
                }
                offset = id + 5;
            }
            return false;
        }

        private static bool TryParseRuleBody(
            string body,
            string source,
            out BossRespawnRule rule)
        {
            rule = null;
            Match type = Regex.Match(
                body,
                "RespawnType[\\\"'=:\\s]+(?<v>[A-Z_]+)",
                RegexOptions.IgnoreCase);
            if (!type.Success)
            {
                return false;
            }

            string value = type.Groups["v"].Value
                .ToUpperInvariant();
            if (value == "RELATIVETIME")
            {
                Match minutes = Regex.Match(
                    body,
                    "RespawnRealTime[\\\"'=:\\s]+(?<v>-?\\d+)",
                    RegexOptions.IgnoreCase);
                int amount;
                if (!minutes.Success
                    || !Int32.TryParse(
                        minutes.Groups["v"].Value,
                        NumberStyles.Integer,
                        CultureInfo.InvariantCulture,
                        out amount))
                {
                    return false;
                }
                amount = Math.Max(0, amount);
                rule = BossRespawnRule.Relative(
                    TimeSpan.FromMinutes(amount),
                    source);
                return true;
            }

            if (value == "DAILY")
            {
                int hour = 9;
                int minute = 0;
                TryParseDailyTime(body, ref hour, ref minute);
                rule = BossRespawnRule.Daily(
                    hour,
                    minute,
                    source);
                return true;
            }
            return false;
        }

        private static void TryParseDailyTime(
            string body,
            ref int hour,
            ref int minute)
        {
            string[] names =
            {
                "RespawnDailyTime",
                "RespawnDayTime",
                "RespawnTime",
                "DailyTime"
            };
            foreach (string name in names)
            {
                Match clock = Regex.Match(
                    body,
                    name +
                    "[\\\"'=:\\s]+(?<h>\\d{1,2})[:](?<m>\\d{1,2})",
                    RegexOptions.IgnoreCase);
                int parsedHour;
                int parsedMinute;
                if (clock.Success
                    && Int32.TryParse(
                        clock.Groups["h"].Value,
                        out parsedHour)
                    && Int32.TryParse(
                        clock.Groups["m"].Value,
                        out parsedMinute)
                    && parsedHour >= 0
                    && parsedHour <= 23
                    && parsedMinute >= 0
                    && parsedMinute <= 59)
                {
                    hour = parsedHour;
                    minute = parsedMinute;
                    return;
                }
            }

            Match hourMatch = Regex.Match(
                body,
                "RespawnHour[\\\"'=:\\s]+(?<v>\\d{1,2})",
                RegexOptions.IgnoreCase);
            Match minuteMatch = Regex.Match(
                body,
                "RespawnMinute[\\\"'=:\\s]+(?<v>\\d{1,2})",
                RegexOptions.IgnoreCase);
            int h;
            int m;
            if (hourMatch.Success
                && Int32.TryParse(
                    hourMatch.Groups["v"].Value,
                    out h)
                && h >= 0
                && h <= 23)
            {
                hour = h;
            }
            if (minuteMatch.Success
                && Int32.TryParse(
                    minuteMatch.Groups["v"].Value,
                    out m)
                && m >= 0
                && m <= 59)
            {
                minute = m;
            }
        }

        private sealed class BossRespawnRule
        {
            private readonly string _type;
            private readonly TimeSpan _relative;
            private readonly int _hour;
            private readonly int _minute;
            private readonly string _source;

            private BossRespawnRule(
                string type,
                TimeSpan relative,
                int hour,
                int minute,
                string source)
            {
                _type = type;
                _relative = relative;
                _hour = hour;
                _minute = minute;
                _source = source;
            }

            public static BossRespawnRule Relative(
                TimeSpan value,
                string source)
            {
                return new BossRespawnRule(
                    "RELATIVETIME",
                    value,
                    0,
                    0,
                    source);
            }

            public static BossRespawnRule Daily(
                int hour,
                int minute,
                string source)
            {
                return new BossRespawnRule(
                    "DAILY",
                    TimeSpan.Zero,
                    hour,
                    minute,
                    source);
            }

            public DateTime NextAvailableUtc(
                DateTime destroyUtc)
            {
                if (_type == "RELATIVETIME")
                {
                    return destroyUtc.Add(_relative);
                }
                DateTime localDestroy =
                    destroyUtc.ToLocalTime();
                DateTime reset = new DateTime(
                    localDestroy.Year,
                    localDestroy.Month,
                    localDestroy.Day,
                    _hour,
                    _minute,
                    0,
                    DateTimeKind.Local);
                if (reset <= localDestroy)
                {
                    reset = reset.AddDays(1);
                }
                return reset.ToUniversalTime();
            }

            public override string ToString()
            {
                return _type == "RELATIVETIME"
                    ? String.Format(
                        CultureInfo.InvariantCulture,
                        "type={0}; minutes={1}; source={2}",
                        _type,
                        _relative.TotalMinutes,
                        _source)
                    : String.Format(
                        CultureInfo.InvariantCulture,
                        "type={0}; localReset={1:00}:{2:00}; source={3}",
                        _type,
                        _hour,
                        _minute,
                        _source);
            }
        }
    }
}
