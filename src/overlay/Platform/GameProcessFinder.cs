using System;
using System.Diagnostics;

namespace DragonSwordWorldRadar
{
    internal static class GameProcessFinder
    {
        private const string ProcessName =
            "DSClient-Win64-Shipping";

        public static Process OpenTracked(int processId)
        {
            if (processId > 0)
            {
                Process tracked = null;
                try
                {
                    tracked = Process.GetProcessById(processId);
                    if (!tracked.HasExited
                        && String.Equals(
                            tracked.ProcessName,
                            ProcessName,
                            StringComparison.OrdinalIgnoreCase))
                    {
                        return tracked;
                    }
                }
                catch
                {
                    // The process may have exited between timer ticks.
                }

                if (tracked != null)
                {
                    tracked.Dispose();
                }
            }

            return FindNewest();
        }

        public static Process FindNewest()
        {
            Process selected = null;
            DateTime selectedStartTime = DateTime.MinValue;

            foreach (Process candidate in
                Process.GetProcessesByName(ProcessName))
            {
                try
                {
                    DateTime startTime = candidate.StartTime;
                    if (selected == null
                        || startTime > selectedStartTime)
                    {
                        if (selected != null)
                        {
                            selected.Dispose();
                        }
                        selected = candidate;
                        selectedStartTime = startTime;
                    }
                    else
                    {
                        candidate.Dispose();
                    }
                }
                catch
                {
                    candidate.Dispose();
                }
            }

            return selected;
        }
    }
}
