using System;
using System.Collections.Generic;

namespace DragonSwordWorldRadar
{
    internal sealed class BossAvailabilityTracker
    {
        private readonly Dictionary<int, BossAvailabilityEntry> _entries =
            new Dictionary<int, BossAvailabilityEntry>();
        private readonly HashSet<int> _seen = new HashSet<int>();
        private readonly List<int> _removed = new List<int>();

        public bool Refresh(
            IList<BossPoint> bosses,
            TreasureSaveState saveState)
        {
            bool changed = false;
            DateTime now = DateTime.UtcNow;
            _seen.Clear();

            if (bosses != null)
            {
                foreach (BossPoint boss in bosses)
                {
                    if (boss == null || !_seen.Add(boss.bossId))
                    {
                        continue;
                    }

                    DateTime nextAvailableUtc =
                        saveState.GetBossNextAvailableUtc(boss.bossId);
                    bool available = IsAvailable(
                        nextAvailableUtc,
                        now);
                    BossAvailabilityEntry previous;
                    if (!_entries.TryGetValue(
                            boss.bossId,
                            out previous)
                        || previous.NextAvailableUtc != nextAvailableUtc
                        || previous.Available != available)
                    {
                        _entries[boss.bossId] =
                            new BossAvailabilityEntry
                            {
                                NextAvailableUtc = nextAvailableUtc,
                                Available = available
                            };
                        changed = true;
                    }
                }
            }

            _removed.Clear();
            foreach (int bossId in _entries.Keys)
            {
                if (!_seen.Contains(bossId))
                {
                    _removed.Add(bossId);
                }
            }
            foreach (int bossId in _removed)
            {
                _entries.Remove(bossId);
                changed = true;
            }

            return changed;
        }

        public bool IsAvailable(int bossId)
        {
            BossAvailabilityEntry entry;
            return !_entries.TryGetValue(bossId, out entry)
                || IsAvailable(
                    entry.NextAvailableUtc,
                    DateTime.UtcNow);
        }

        public void Reset()
        {
            _entries.Clear();
            _seen.Clear();
            _removed.Clear();
        }

        private static bool IsAvailable(
            DateTime nextAvailableUtc,
            DateTime now)
        {
            return nextAvailableUtc == DateTime.MinValue
                || now >= nextAvailableUtc;
        }

        private struct BossAvailabilityEntry
        {
            public DateTime NextAvailableUtc;
            public bool Available;
        }
    }
}
