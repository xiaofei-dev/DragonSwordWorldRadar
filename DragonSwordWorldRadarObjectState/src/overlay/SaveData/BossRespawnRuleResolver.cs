using System;
using System.Globalization;

namespace DragonSwordWorldRadar
{
    internal sealed class BossRespawnRuleResolver
    {
        private const int RespawnMinutes = 120;
        private const string FixedRuleSummary =
            "type=ELAPSED; cooldownMinutes=120; source=user-validated-runtime";

        public DateTime NextAvailableUtc(
            DateTime destroyTimeUtc)
        {
            DateTime normalizedDestroyUtc =
                destroyTimeUtc.Kind == DateTimeKind.Utc
                    ? destroyTimeUtc
                    : destroyTimeUtc.ToUniversalTime();
            return normalizedDestroyUtc.AddMinutes(RespawnMinutes);
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
