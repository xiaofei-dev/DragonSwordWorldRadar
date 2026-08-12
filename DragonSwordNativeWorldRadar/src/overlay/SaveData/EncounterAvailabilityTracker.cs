using System;
using System.Collections.Generic;

namespace DragonSwordWorldRadar
{
    internal sealed class EncounterAvailabilityTracker
    {
        private readonly Dictionary<int, DateTime> _nextAvailable =
            new Dictionary<int, DateTime>();
        private readonly HashSet<int> _seen = new HashSet<int>();
        private readonly List<int> _removed = new List<int>();

        public bool Refresh(
            IList<WorldEncounter> encounters,
            TreasureSaveState saveState)
        {
            bool changed = false;
            _seen.Clear();
            if (encounters != null)
            {
                foreach (WorldEncounter encounter in encounters)
                {
                    if (encounter == null || !_seen.Add(encounter.Id))
                    {
                        continue;
                    }
                    DateTime next =
                        saveState.GetEncounterNextAvailableUtc(
                            encounter.Id);
                    DateTime previous;
                    if (!_nextAvailable.TryGetValue(
                            encounter.Id,
                            out previous)
                        || previous != next)
                    {
                        _nextAvailable[encounter.Id] = next;
                        changed = true;
                    }
                }
            }

            _removed.Clear();
            foreach (int id in _nextAvailable.Keys)
            {
                if (!_seen.Contains(id))
                {
                    _removed.Add(id);
                }
            }
            foreach (int id in _removed)
            {
                _nextAvailable.Remove(id);
                changed = true;
            }
            return changed;
        }

        public bool IsAvailable(
            WorldEncounter encounter,
            bool timeAvailable,
            int timeSeconds)
        {
            if (encounter == null)
            {
                return false;
            }
            DateTime next;
            if (_nextAvailable.TryGetValue(encounter.Id, out next)
                && next != DateTime.MinValue
                && DateTime.UtcNow < next)
            {
                return false;
            }
            return AreConditionsSatisfied(
                encounter,
                timeAvailable,
                timeSeconds);
        }

        internal static bool AreConditionsSatisfied(
            WorldEncounter encounter,
            bool timeAvailable,
            int timeSeconds)
        {
            if (encounter == null)
            {
                return false;
            }
            foreach (WorldEncounterCondition condition
                in encounter.Conditions)
            {
                if (!IsConditionSatisfied(
                    condition,
                    timeAvailable,
                    timeSeconds))
                {
                    return false;
                }
            }
            return true;
        }

        public void Reset()
        {
            _nextAvailable.Clear();
            _seen.Clear();
            _removed.Clear();
        }

        internal static bool IsConditionSatisfied(
            WorldEncounterCondition condition,
            bool timeAvailable,
            int timeSeconds)
        {
            if (condition == null)
            {
                return false;
            }
            if (!String.Equals(
                condition.Type,
                "world_time_window",
                StringComparison.Ordinal))
            {
                // Future weather or other condition types remain fail-closed
                // until both an install-time mapping and a trustworthy scalar
                // runtime source exist.
                return false;
            }
            if (!timeAvailable
                || timeSeconds < 0
                || timeSeconds >= 86400)
            {
                return false;
            }
            int hour = timeSeconds / 3600;
            return condition.VisibleFromHour
                    < condition.HiddenFromHour
                ? hour >= condition.VisibleFromHour
                    && hour < condition.HiddenFromHour
                : hour >= condition.VisibleFromHour
                    || hour < condition.HiddenFromHour;
        }
    }
}
