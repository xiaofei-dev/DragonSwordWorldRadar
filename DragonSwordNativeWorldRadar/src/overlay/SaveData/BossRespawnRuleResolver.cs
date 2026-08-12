using System;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    internal sealed class BossRespawnRuleResolver
    {
        private const int ResetUtcHour = 0;
        private const int ResetUtcMinute = 0;
        private const string FixedRuleSummary =
            "type=DAILY; resetKst=09:00; resetUtc=00:00; source=user-validated-runtime";

        public DateTime NextAvailableUtc(
            DateTime destroyTimeUtc)
        {
            DateTime normalizedDestroyUtc =
                destroyTimeUtc.Kind == DateTimeKind.Utc
                    ? destroyTimeUtc
                    : destroyTimeUtc.ToUniversalTime();
            DateTime reset = new DateTime(
                normalizedDestroyUtc.Year,
                normalizedDestroyUtc.Month,
                normalizedDestroyUtc.Day,
                ResetUtcHour,
                ResetUtcMinute,
                0,
                DateTimeKind.Utc);
            if (reset <= normalizedDestroyUtc)
            {
                reset = reset.AddDays(1);
            }
            return reset;
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
