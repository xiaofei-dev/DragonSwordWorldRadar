using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;

namespace DragonSwordWorldRadar
{
    internal sealed class SaveDatabaseLocator
    {
        private static readonly TimeSpan DiscoveryInterval =
            TimeSpan.FromSeconds(5);

        private DateTime _nextDiscoveryUtc;
        private string _selectedDatabasePath;
        private string _candidateSummary;

        public string FindNewest(
            Process game,
            out string candidateSummary)
        {
            DateTime now = DateTime.UtcNow;

            // The game alternates writes between the active slot's .db and
            // .bak files. Check only those siblings during the discovery
            // cooldown instead of recursively walking SaveGames every poll.
            if (_selectedDatabasePath != null)
            {
                string active = FindNewestActiveSlotDatabase(
                    _selectedDatabasePath);
                if (active != null)
                {
                    _selectedDatabasePath = active;
                    if (now < _nextDiscoveryUtc)
                    {
                        candidateSummary = _candidateSummary;
                        return _selectedDatabasePath;
                    }
                }
            }

            _selectedDatabasePath = FindNewestSaveDatabase(
                game,
                out _candidateSummary);
            _nextDiscoveryUtc = now.Add(DiscoveryInterval);
            candidateSummary = _candidateSummary;
            return _selectedDatabasePath;
        }

        public void Reset()
        {
            _nextDiscoveryUtc = DateTime.MinValue;
            _selectedDatabasePath = null;
            _candidateSummary = null;
        }

        internal static string SafeSlotName(string path)
        {
            if (String.IsNullOrEmpty(path))
            {
                return "none";
            }

            string filename = Path.GetFileName(path);
            string token = SafeSlotToken(filename);
            return token == null
                ? filename
                : token + Path.GetExtension(filename);
        }

        private static string FindNewestActiveSlotDatabase(
            string selectedDatabasePath)
        {
            string directory = Path.GetDirectoryName(
                selectedDatabasePath);
            string stem = Path.GetFileNameWithoutExtension(
                selectedDatabasePath);
            if (String.IsNullOrEmpty(directory)
                || String.IsNullOrEmpty(stem))
            {
                return null;
            }

            string database = Path.Combine(directory, stem + ".db");
            string backup = Path.Combine(directory, stem + ".bak");
            bool hasDatabase = File.Exists(database);
            bool hasBackup = File.Exists(backup);
            if (!hasDatabase)
            {
                return hasBackup ? backup : null;
            }
            if (!hasBackup)
            {
                return database;
            }
            return File.GetLastWriteTimeUtc(backup)
                > File.GetLastWriteTimeUtc(database)
                    ? backup
                    : database;
        }

        private static string FindNewestSaveDatabase(
            Process game,
            out string candidateSummary)
        {
            string win64 = Path.GetDirectoryName(
                game.MainModule.FileName);
            string saveRoot = Path.GetFullPath(Path.Combine(
                win64,
                "..",
                "..",
                "Saved",
                "SaveGames"));
            List<string> ordered = Directory.GetFiles(
                    saveRoot,
                    "*_Slot*.bak",
                    SearchOption.AllDirectories)
                .Concat(Directory.GetFiles(
                    saveRoot,
                    "*_Slot*.db",
                    SearchOption.AllDirectories))
                .OrderByDescending(File.GetLastWriteTimeUtc)
                .ToList();

            candidateSummary = BuildCandidateSummary(
                ordered,
                win64,
                saveRoot);
            if (ordered.Count == 0)
            {
                throw new FileNotFoundException(
                    "No slot database was found.",
                    saveRoot);
            }

            IGrouping<string, string> activeSlot = ordered
                .GroupBy(
                    path => Path.Combine(
                        Path.GetDirectoryName(path),
                        Path.GetFileNameWithoutExtension(path)),
                    StringComparer.OrdinalIgnoreCase)
                .OrderByDescending(group =>
                    GetNewestWriteTimeUtc(group))
                .First();

            // The live state may be in either sibling. Always choose the most
            // recently written file in the active slot.
            return activeSlot
                .OrderByDescending(File.GetLastWriteTimeUtc)
                .First();
        }

        private static DateTime GetNewestWriteTimeUtc(
            IEnumerable<string> paths)
        {
            DateTime newest = DateTime.MinValue;
            foreach (string path in paths)
            {
                DateTime writeTime = File.GetLastWriteTimeUtc(path);
                if (writeTime > newest)
                {
                    newest = writeTime;
                }
            }
            return newest;
        }

        private static string BuildCandidateSummary(
            IEnumerable<string> candidates,
            string win64,
            string saveRoot)
        {
            string databases = string.Join(
                ",",
                candidates.Select(path =>
                    SafeSlotName(path) + "@" +
                    File.GetLastWriteTimeUtc(path).ToString(
                        "O",
                        CultureInfo.InvariantCulture))
                .ToArray());
            return databases + "; " +
                BuildSlotHints(win64, saveRoot);
        }

        private static string BuildSlotHints(
            string win64,
            string saveRoot)
        {
            string spackSummary = string.Join(
                ",",
                Directory.GetFiles(
                    saveRoot,
                    "SPack_Slot*.sav",
                    SearchOption.AllDirectories)
                .OrderByDescending(File.GetLastWriteTimeUtc)
                .Select(path =>
                    SafeSlotName(path) + "@" +
                    File.GetLastWriteTimeUtc(path).ToString(
                        "O",
                        CultureInfo.InvariantCulture))
                .ToArray());

            string configPath = Path.GetFullPath(Path.Combine(
                win64,
                "..",
                "..",
                "Saved",
                "Config",
                "Windows",
                "Game.ini"));
            string configSummary = ReadConfigSlotSummary(configPath);

            return "spack=" +
                (spackSummary.Length == 0
                    ? "none"
                    : spackSummary) +
                "; configSections=" + configSummary;
        }

        private static string ReadConfigSlotSummary(string configPath)
        {
            if (!File.Exists(configPath))
            {
                return "none";
            }

            string[] slots = File.ReadLines(configPath)
                .Select(SafeSlotToken)
                .Where(slot => slot != null)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray();
            return slots.Length == 0
                ? "none"
                : string.Join(",", slots);
        }

        private static string SafeSlotToken(string value)
        {
            if (String.IsNullOrEmpty(value))
            {
                return null;
            }

            int slot = value.IndexOf(
                "_Slot",
                StringComparison.OrdinalIgnoreCase);
            if (slot < 0)
            {
                return null;
            }

            int numberStart = slot + 5;
            int end = numberStart;
            if (end < value.Length && value[end] == '-')
            {
                end++;
            }
            int digitsStart = end;
            while (end < value.Length && char.IsDigit(value[end]))
            {
                end++;
            }

            return end > digitsStart
                ? "Slot" + value.Substring(
                    numberStart,
                    end - numberStart)
                : null;
        }
    }
}
