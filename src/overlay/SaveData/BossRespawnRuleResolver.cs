using System;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    internal sealed class BossRespawnRuleResolver
    {
        private const int ResetHour = 9;
        private const int ResetMinute = 0;
        private const string FixedRuleSummary =
            "type=DAILY; localReset=09:00; source=built-in-original";

        public DateTime NextAvailableUtc(
            DateTime destroyTimeUtc)
        {
            DateTime localDestroy =
                destroyTimeUtc.ToLocalTime();
            DateTime reset = new DateTime(
                localDestroy.Year,
                localDestroy.Month,
                localDestroy.Day,
                ResetHour,
                ResetMinute,
                0,
                DateTimeKind.Local);
            if (reset <= localDestroy)
            {
                reset = reset.AddDays(1);
            }
            return reset.ToUniversalTime();
        }

        public bool IsAvailable(DateTime destroyTimeUtc)
        {
            return DateTime.UtcNow >=
                NextAvailableUtc(destroyTimeUtc);
        }

        public string Describe(DateTime destroyTimeUtc)
        {
            DateTime next = NextAvailableUtc(
                destroyTimeUtc);
            return String.Format(
                CultureInfo.InvariantCulture,
                "{0}; destroyUtc={1:O}; nextUtc={2:O}; available={3}",
                FixedRuleSummary,
                destroyTimeUtc,
                next,
                DateTime.UtcNow >= next);
        }

        public string RuleSummary
        {
            get { return FixedRuleSummary; }
        }
    }
}
