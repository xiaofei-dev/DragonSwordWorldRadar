using System;
using System.IO;

namespace DragonSwordWorldRadar
{
    internal static class DebugSettings
    {
        private const string SettingName = "debug_logging";
        private const string LegacySettingName = "diagnostic_verbose";
        private static readonly bool StartupEnabled = LoadAtStartup();

        public static bool Enabled
        {
            get { return StartupEnabled; }
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
    }
}
