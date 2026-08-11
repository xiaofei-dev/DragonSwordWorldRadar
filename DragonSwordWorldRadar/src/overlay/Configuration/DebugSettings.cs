using System;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal static class DebugSettings
    {
        private const string SettingName = "debug_logging";
        private const string LegacySettingName = "diagnostic_verbose";
        private static readonly bool StartupVerboseLabelsEnabled =
            LoadBooleanAtStartup(LegacySettingName, false);
        private const string HighResolutionTimerSettingName =
            "high_resolution_timer";
        private static readonly bool StartupEnabled = LoadAtStartup();
        private static readonly bool StartupHighResolutionTimerEnabled =
            LoadBooleanAtStartup(
                HighResolutionTimerSettingName,
                false);
        private static readonly bool StartupShowAssaults =
            LoadBooleanAtStartup("show_assaults", true);
        public static bool ShowAssaults { get { return StartupShowAssaults; } }

        public static bool Enabled
        {
            get { return StartupEnabled; }
        }

        public static bool VerboseLabelsEnabled
        {
            get { return StartupVerboseLabelsEnabled; }
        }

        public static bool HighResolutionTimerEnabled
        {
            get { return StartupHighResolutionTimerEnabled; }
        }

        private static bool LoadAtStartup()
        {
            try
            {
                string path = Path.Combine(
                    ModPath.BaseDirectory,
                    "scripts",
                    "config.lua");
                if (!File.Exists(path))
                {
                    return false;
                }

                bool primaryEnabled = false;
                bool legacyEnabled = false;
                foreach (string sourceLine in File.ReadLines(path))
                {
                    string line = RemoveComment(sourceLine).Trim();
                    int equals = line.IndexOf('=');
                    if (equals < 0)
                    {
                        continue;
                    }

                    string name = line.Substring(0, equals).Trim();
                    bool isPrimary = string.Equals(
                        name,
                        SettingName,
                        StringComparison.OrdinalIgnoreCase);
                    bool isLegacy = string.Equals(
                        name,
                        LegacySettingName,
                        StringComparison.OrdinalIgnoreCase);
                    if (!isPrimary && !isLegacy)
                    {
                        continue;
                    }

                    string value = line.Substring(equals + 1)
                        .Trim()
                        .TrimEnd(',');
                    bool valueEnabled = string.Equals(
                        value,
                        "true",
                        StringComparison.OrdinalIgnoreCase);
                    if (isPrimary)
                    {
                        primaryEnabled = valueEnabled;
                    }
                    else
                    {
                        legacyEnabled = valueEnabled;
                    }
                }

                return primaryEnabled || legacyEnabled;
            }
            catch (IOException)
            {
                return false;
            }
            catch (UnauthorizedAccessException)
            {
                return false;
            }
            catch
            {
                // Debug mode is optional. A malformed or temporarily locked
                // config must never prevent the Overlay from starting.
                return false;
            }
        }

        private static string RemoveComment(string line)
        {
            int comment = line.IndexOf(
                "--",
                StringComparison.Ordinal);
            return comment < 0
                ? line
                : line.Substring(0, comment);
        }

        private static bool LoadBooleanAtStartup(
            string settingName,
            bool defaultValue)
        {
            try
            {
                string path = Path.Combine(
                    ModPath.BaseDirectory,
                    "scripts",
                    "config.lua");
                if (!File.Exists(path))
                {
                    return defaultValue;
                }

                bool result = defaultValue;
                foreach (string sourceLine in File.ReadLines(path))
                {
                    string line = RemoveComment(sourceLine).Trim();
                    int equals = line.IndexOf('=');
                    if (equals < 0
                        || !String.Equals(
                            line.Substring(0, equals).Trim(),
                            settingName,
                            StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }

                    string value = line.Substring(equals + 1)
                        .Trim()
                        .TrimEnd(',');
                    if (String.Equals(
                        value,
                        "true",
                        StringComparison.OrdinalIgnoreCase))
                    {
                        result = true;
                    }
                    else if (String.Equals(
                        value,
                        "false",
                        StringComparison.OrdinalIgnoreCase))
                    {
                        result = false;
                    }
                }
                return result;
            }
            catch
            {
                // Optional timer tuning must never prevent Overlay startup.
                return defaultValue;
            }
        }
    }
}
