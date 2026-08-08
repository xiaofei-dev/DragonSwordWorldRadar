using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;

namespace DragonSwordWorldDataProbe
{
    public static class WorldBossSaveMonitor
    {
        private const string Version = "0.5.7";
        private const uint ProcessReadAccess = 0x0010 | 0x1000;
        private const int SqliteOpenReadOnly = 0x00000001;
        private const long MaximumRuleFileBytes = 8L * 1024L * 1024L;
        private const int MaximumRuleCandidates = 256;
        private const int MaximumRuleTraversalFiles = 4000;

        private static readonly int[] BossIds =
        {
            9000005, 9000007, 9000010,
            9000011, 9000012, 9000019,
            9000022, 9000023, 9000025
        };

        private static readonly Dictionary<int, string> BossUidNames =
            new Dictionary<int, string>
            {
                { 9000005, "DEnt_RB_1070401" },
                { 9000007, "DCommon_Dragon_RB_1070901" },
                { 9000010, "DOgre_RB_1031401_Pat" },
                { 9000011, "DSpiderHunterBoss_RB_050601" },
                { 9000012, "Lich_FieldBoss" },
                { 9000019, "DOgreRedchain_RB_1060801_Roam" },
                { 9000022, "TrollBarbarian_Nam_1112302" },
                { 9000023, "DsMon_Red_Harpy_FieldBoss_1111101" },
                { 9000025, "DWraith_FB_1101801" }
            };


        private static readonly BossCatalogKey[] BossCatalogKeys =
        {
                new BossCatalogKey
                {
                    BossId = 9000005,
                    UidName = "DEnt_RB_1070401",
                    Uid = "17764099721625934119",
                    SectionUid = "2191990000100",
                    GroupId = "110706",
                    SwitchWeekId = "90002",
                    LevelCid = "3013",
                    WorldMapSectionId = "101701"
                },
                new BossCatalogKey
                {
                    BossId = 9000007,
                    UidName = "DCommon_Dragon_RB_1070901",
                    Uid = "1373813664261435816",
                    SectionUid = "2171950000100",
                    GroupId = "0",
                    SwitchWeekId = "90004",
                    LevelCid = "3015",
                    WorldMapSectionId = "101703"
                },
                new BossCatalogKey
                {
                    BossId = 9000010,
                    UidName = "DOgre_RB_1031401_Pat",
                    Uid = "17221195949474716040",
                    SectionUid = "2052140000100",
                    GroupId = "9000010",
                    SwitchWeekId = "90000",
                    LevelCid = "3011",
                    WorldMapSectionId = "101300"
                },
                new BossCatalogKey
                {
                    BossId = 9000011,
                    UidName = "DSpiderHunterBoss_RB_050601",
                    Uid = "7205131353416783309",
                    SectionUid = "2142060000100",
                    GroupId = "0",
                    SwitchWeekId = "90001",
                    LevelCid = "3012",
                    WorldMapSectionId = "101501"
                },
                new BossCatalogKey
                {
                    BossId = 9000012,
                    UidName = "Lich_FieldBoss",
                    Uid = "13329372305523848115",
                    SectionUid = "2111940000100",
                    GroupId = "0",
                    SwitchWeekId = "90003",
                    LevelCid = "3014",
                    WorldMapSectionId = "101800"
                },
                new BossCatalogKey
                {
                    BossId = 9000019,
                    UidName = "DOgreRedchain_RB_1060801_Roam",
                    Uid = "14523313456005002760",
                    SectionUid = "2032010000100",
                    GroupId = "0",
                    SwitchWeekId = "90005",
                    LevelCid = "3016",
                    WorldMapSectionId = "101601"
                },
                new BossCatalogKey
                {
                    BossId = 9000022,
                    UidName = "TrollBarbarian_Nam_1112302",
                    Uid = "1927377281794824538",
                    SectionUid = "2222100000100",
                    GroupId = "0",
                    SwitchWeekId = "90006",
                    LevelCid = "3018",
                    WorldMapSectionId = "102100"
                },
                new BossCatalogKey
                {
                    BossId = 9000023,
                    UidName = "DsMon_Red_Harpy_FieldBoss_1111101",
                    Uid = "14780634162364795516",
                    SectionUid = "2232060000100",
                    GroupId = "0",
                    SwitchWeekId = "90007",
                    LevelCid = "3017",
                    WorldMapSectionId = "102103"
                },
                new BossCatalogKey
                {
                    BossId = 9000025,
                    UidName = "DWraith_FB_1101801",
                    Uid = "13891057566040470946",
                    SectionUid = "2142150000100",
                    GroupId = "0",
                    SwitchWeekId = "90008",
                    LevelCid = "3019",
                    WorldMapSectionId = "102005"
                }
        };

        private static readonly byte[] OwnerReferencePattern =
        {
            0x48, 0x8B, 0x0D, 0, 0, 0, 0,
            0xE8, 0, 0, 0, 0,
            0x8B, 0xC7, 0x48, 0x8B, 0x5C, 0x24,
            0x40, 0x48, 0x8B, 0x6C, 0x24, 0x50
        };

        private const string OwnerReferenceMask =
            "xxx????x????xxxxxxxxxxxx";

        private static readonly ulong[] FallbackOwnerPointerRvas =
        {
            0x94DFAE8UL,
            0x94DEAE8UL,
            0x94DDB20UL
        };

        private static readonly object Sync = new object();
        private static readonly UTF8Encoding Utf8NoBom =
            new UTF8Encoding(false);
        private static readonly DateTime UnixEpochUtc =
            new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);

        private static string _sqlcipherPath;
        private static string _catalogPath;
        private static string _snapshotLogPath;
        private static string _changeLogPath;
        private static string _statusPath;
        private static string _root;
        private static bool _configured;
        private static bool _snapshotHeaderReady;

        private static string _databaseDiffRoot;
        private static string _spawnConditionReportRoot;
        private static Dictionary<string, LogicalTableSnapshot> _lastLogicalTables =
            new Dictionary<string, LogicalTableSnapshot>(StringComparer.OrdinalIgnoreCase);
        private static int _logicalSnapshotSequence;
        private const int MaximumLogicalRowsPerTable = 5000;
        private const int MaximumLogicalCharactersPerTable = 8 * 1024 * 1024;
        private const int MaximumLogicalCharactersTotal = 64 * 1024 * 1024;

        private static int _processId;
        private static string _cachedKey;
        private static ulong _cachedDetectedRva;
        private static string _cachedExecutablePath;
        private static DateTime _cachedExecutableWriteUtc;

        private static string _lastSlotSignature;
        private static string _lastSlotSummary;
        private static string _lastRuleSignature;
        private static DateTime _lastDatabaseReadUtc;
        private static DateTime _lastStatusWriteUtc;
        private static string _lastError;
        private static Dictionary<int, BossState> _states =
            new Dictionary<int, BossState>();

        private static DateTime _nextRuleScanUtc;
        private static RespawnRule _rule =
            RespawnRule.Daily(9, 0, "built-in-original-fallback");

        public static void Configure(
            string sqlcipherPath,
            string catalogPath,
            string snapshotLogPath,
            string changeLogPath,
            string statusPath,
            string root)
        {
            lock (Sync)
            {
                if (String.IsNullOrWhiteSpace(sqlcipherPath)
                    || !File.Exists(sqlcipherPath))
                {
                    throw new FileNotFoundException(
                        "e_sqlcipher.dll was not found.",
                        sqlcipherPath);
                }

                _sqlcipherPath = Path.GetFullPath(sqlcipherPath);
                _catalogPath = String.IsNullOrWhiteSpace(catalogPath)
                    ? String.Empty
                    : Path.GetFullPath(catalogPath);
                _snapshotLogPath = Path.GetFullPath(snapshotLogPath);
                _changeLogPath = Path.GetFullPath(changeLogPath);
                _statusPath = Path.GetFullPath(statusPath);
                _root = Path.GetFullPath(root);
                _databaseDiffRoot = Path.Combine(
                    _root,
                    "runtime",
                    "reports",
                    "save-database-diff");
                _spawnConditionReportRoot = Path.Combine(
                    _root,
                    "runtime",
                    "reports",
                    "world-boss-spawn-condition");

                EnsureParentDirectory(_snapshotLogPath);
                EnsureParentDirectory(_changeLogPath);
                EnsureParentDirectory(_statusPath);
                Directory.CreateDirectory(_databaseDiffRoot);
                Directory.CreateDirectory(_spawnConditionReportRoot);

                string nativeDirectory = Path.GetDirectoryName(
                    _sqlcipherPath);
                if (!NativeMethods.SetDllDirectory(nativeDirectory))
                {
                    throw new InvalidOperationException(
                        "SetDllDirectory failed: "
                        + Marshal.GetLastWin32Error().ToString(
                            CultureInfo.InvariantCulture));
                }

                _configured = true;
                _snapshotHeaderReady = File.Exists(_snapshotLogPath)
                    && new FileInfo(_snapshotLogPath).Length > 0;
                ResetInternal();
                AppendChange(
                    "MONITOR_CONFIGURED",
                    "sqlcipher=" + _sqlcipherPath
                    + "; catalog=" + _catalogPath);
            }
        }

        public static void Reset()
        {
            lock (Sync)
            {
                ResetInternal();
            }
        }

        public static string Poll(
            int processId,
            string executablePath,
            string saveRoot,
            string pakRoot)
        {
            lock (Sync)
            {
                DateTime nowUtc = DateTime.UtcNow;
                if (!_configured)
                {
                    return "state=disabled reason=not_configured";
                }

                try
                {
                    if (processId <= 0)
                    {
                        throw new InvalidOperationException(
                            "Game process is not available.");
                    }
                    if (processId != _processId)
                    {
                        ResetForProcess(processId);
                    }

                    bool ruleChanged = RefreshRespawnRule(
                        nowUtc,
                        pakRoot);
                    string key = ReadSaveKey(
                        processId,
                        executablePath);
                    SaveSlot slot = LocateActiveSlot(saveRoot);

                    bool needsDatabaseRead =
                        _states.Count == 0
                        || !String.Equals(
                            slot.Signature,
                            _lastSlotSignature,
                            StringComparison.Ordinal)
                        || ruleChanged;

                    if (needsDatabaseRead)
                    {
                        SaveSnapshot snapshot = ReadSaveSlot(slot, key);
                        PublishSnapshot(
                            snapshot,
                            slot,
                            nowUtc,
                            ruleChanged);
                        _lastSlotSignature = slot.Signature;
                        _lastSlotSummary = slot.Summary;
                        _lastDatabaseReadUtc = nowUtc;
                    }
                    else
                    {
                        RefreshTimeBasedAvailability(nowUtc);
                    }

                    _lastError = null;
                    if ((nowUtc - _lastStatusWriteUtc).TotalSeconds >= 2
                        || needsDatabaseRead)
                    {
                        WriteStatus(nowUtc, "ready");
                    }

                    int rows = 0;
                    int available = 0;
                    int cooldown = 0;
                    int unknown = 0;
                    foreach (int bossId in BossIds)
                    {
                        BossState state;
                        if (_states.TryGetValue(bossId, out state))
                        {
                            if (state.RowPresent) rows++;
                            if (!state.AvailabilityKnown) unknown++;
                            else if (state.Available) available++;
                            else cooldown++;
                        }
                    }
                    return String.Format(
                        CultureInfo.InvariantCulture,
                        "state=ready rows={0} available={1} cooldown={2} unknown={3} slot=\"{4}\" rule=\"{5}\"",
                        rows,
                        available,
                        cooldown,
                        unknown,
                        SafeInline(_lastSlotSummary),
                        SafeInline(_rule.Describe()));
                }
                catch (Exception exception)
                {
                    _lastError = exception.Message;
                    if (ShouldInvalidateCachedKey(exception))
                    {
                        _cachedKey = null;
                        _lastSlotSignature = null;
                        try
                        {
                            AppendChange(
                                "SAVE_KEY_CACHE_INVALIDATED",
                                "reason=" + exception.Message);
                        }
                        catch
                        {
                        }
                    }
                    WriteStatus(nowUtc, "error");
                    return "state=error error=\""
                        + SafeInline(exception.Message)
                        + "\"";
                }
            }
        }

        private static bool ShouldInvalidateCachedKey(
            Exception exception)
        {
            string message = exception == null
                ? String.Empty
                : exception.ToString();
            return message.IndexOf(
                    "SQLCipher error 26",
                    StringComparison.OrdinalIgnoreCase) >= 0
                || message.IndexOf(
                    "SQLCipher error 11",
                    StringComparison.OrdinalIgnoreCase) >= 0
                || message.IndexOf(
                    "not a database",
                    StringComparison.OrdinalIgnoreCase) >= 0
                || message.IndexOf(
                    "file is encrypted",
                    StringComparison.OrdinalIgnoreCase) >= 0
                || message.IndexOf(
                    "malformed",
                    StringComparison.OrdinalIgnoreCase) >= 0;
        }

        private static void ResetInternal()
        {
            _processId = 0;
            _cachedKey = null;
            _cachedDetectedRva = 0;
            _cachedExecutablePath = null;
            _cachedExecutableWriteUtc = DateTime.MinValue;
            _lastSlotSignature = null;
            _lastSlotSummary = null;
            _lastRuleSignature = null;
            _lastDatabaseReadUtc = DateTime.MinValue;
            _lastStatusWriteUtc = DateTime.MinValue;
            _lastError = null;
            _states = new Dictionary<int, BossState>();
            _lastLogicalTables = new Dictionary<string, LogicalTableSnapshot>(
                StringComparer.OrdinalIgnoreCase);
            _logicalSnapshotSequence = 0;
            _nextRuleScanUtc = DateTime.MinValue;
            _rule = RespawnRule.Daily(
                9,
                0,
                "built-in-original-fallback");
        }

        private static void ResetForProcess(int processId)
        {
            _processId = processId;
            _cachedKey = null;
            _cachedDetectedRva = 0;
            _cachedExecutablePath = null;
            _cachedExecutableWriteUtc = DateTime.MinValue;
            _lastSlotSignature = null;
            _lastSlotSummary = null;
            _lastDatabaseReadUtc = DateTime.MinValue;
            _states = new Dictionary<int, BossState>();
            _lastLogicalTables = new Dictionary<string, LogicalTableSnapshot>(
                StringComparer.OrdinalIgnoreCase);
            _logicalSnapshotSequence = 0;
            AppendChange(
                "GAME_PROCESS_CHANGED",
                "process_id=" + processId.ToString(
                    CultureInfo.InvariantCulture));
        }

        private static bool RefreshRespawnRule(
            DateTime nowUtc,
            string pakRoot)
        {
            if (nowUtc < _nextRuleScanUtc)
            {
                return false;
            }
            _nextRuleScanUtc = nowUtc.AddMinutes(5);

            RespawnRule detected;
            if (!TryDetectRespawnRule(pakRoot, out detected))
            {
                detected = RespawnRule.Daily(
                    9,
                    0,
                    "built-in-original-fallback");
            }

            string signature = detected.Signature;
            bool changed = !String.Equals(
                signature,
                _lastRuleSignature,
                StringComparison.Ordinal);
            if (changed)
            {
                _rule = detected;
                _lastRuleSignature = signature;
                AppendChange(
                    "RESPAWN_RULE_CHANGED",
                    detected.Describe());
            }
            return changed;
        }

        private static bool TryDetectRespawnRule(
            string pakRoot,
            out RespawnRule rule)
        {
            rule = null;
            List<FileInfo> candidates = EnumerateRuleCandidates(
                pakRoot);
            candidates.Sort(delegate(FileInfo left, FileInfo right)
            {
                return right.LastWriteTimeUtc.CompareTo(
                    left.LastWriteTimeUtc);
            });

            int inspected = 0;
            foreach (FileInfo candidate in candidates)
            {
                if (inspected >= MaximumRuleCandidates)
                {
                    break;
                }
                inspected++;
                try
                {
                    string text = ReadLooseText(candidate.FullName);
                    RespawnRule parsed;
                    if (TryParseRuleText(
                            text,
                            candidate.FullName,
                            out parsed))
                    {
                        rule = parsed;
                        return true;
                    }
                }
                catch
                {
                    // One unreadable override/extracted file must not disable
                    // the save monitor or prevent later candidates.
                }
            }
            return false;
        }

        private static List<FileInfo> EnumerateRuleCandidates(
            string pakRoot)
        {
            List<FileInfo> results = new List<FileInfo>();
            HashSet<string> seen = new HashSet<string>(
                StringComparer.OrdinalIgnoreCase);

            string modsRoot = null;
            try
            {
                modsRoot = Directory.GetParent(_root).FullName;
            }
            catch
            {
            }

            AddRuleCandidatesFromTree(
                modsRoot,
                results,
                seen);
            AddRuleCandidatesFromTree(
                pakRoot,
                results,
                seen);
            return results;
        }

        private static void AddRuleCandidatesFromTree(
            string root,
            IList<FileInfo> results,
            ISet<string> seen)
        {
            if (String.IsNullOrWhiteSpace(root)
                || !Directory.Exists(root))
            {
                return;
            }

            Stack<DirectoryInfo> pending = new Stack<DirectoryInfo>();
            pending.Push(new DirectoryInfo(root));
            int visitedFiles = 0;

            while (pending.Count > 0
                && visitedFiles < MaximumRuleTraversalFiles
                && results.Count < MaximumRuleCandidates * 4)
            {
                DirectoryInfo directory = pending.Pop();
                FileInfo[] files;
                try
                {
                    files = directory.GetFiles();
                }
                catch
                {
                    files = new FileInfo[0];
                }

                foreach (FileInfo file in files)
                {
                    visitedFiles++;
                    if (visitedFiles > MaximumRuleTraversalFiles)
                    {
                        break;
                    }
                    if (file.Length <= 0
                        || file.Length > MaximumRuleFileBytes)
                    {
                        continue;
                    }

                    string extension = file.Extension.ToLowerInvariant();
                    string name = file.Name.ToLowerInvariant();
                    bool isText = extension == ".xml"
                        || extension == ".json"
                        || extension == ".txt"
                        || extension == ".lua"
                        || extension == ".csv"
                        || extension == ".tsv";
                    if (!isText
                        || name.IndexOf(
                            "respawn",
                            StringComparison.OrdinalIgnoreCase) < 0)
                    {
                        continue;
                    }
                    if (seen.Add(file.FullName))
                    {
                        results.Add(file);
                    }
                }

                DirectoryInfo[] children;
                try
                {
                    children = directory.GetDirectories();
                }
                catch
                {
                    children = new DirectoryInfo[0];
                }
                foreach (DirectoryInfo child in children)
                {
                    string lower = child.Name.ToLowerInvariant();
                    if (lower == ".git"
                        || lower == "diagnostics"
                        || lower == "crash-files"
                        || lower == "saved"
                        || lower == "runtime")
                    {
                        continue;
                    }
                    pending.Push(child);
                }
            }
        }

        private static string ReadLooseText(string path)
        {
            byte[] bytes = File.ReadAllBytes(path);
            string utf8 = Encoding.UTF8.GetString(bytes);
            string unicode = String.Empty;
            if (bytes.Length >= 2)
            {
                unicode = Encoding.Unicode.GetString(bytes);
            }
            return utf8 + "\n" + unicode;
        }

        private static bool TryParseRuleText(
            string text,
            string source,
            out RespawnRule rule)
        {
            rule = null;
            if (String.IsNullOrEmpty(text))
            {
                return false;
            }

            Match idMatch = Regex.Match(
                text,
                "(?:ID|RespawnCycleID)\\s*[\\\"'=:\\s]+106(?:[^0-9]|$)",
                RegexOptions.IgnoreCase);
            if (!idMatch.Success)
            {
                idMatch = Regex.Match(
                    text,
                    "[\\\"']106[\\\"']",
                    RegexOptions.IgnoreCase);
            }
            if (!idMatch.Success)
            {
                return false;
            }

            int start = Math.Max(0, idMatch.Index - 512);
            int length = Math.Min(
                text.Length - start,
                4096);
            string window = text.Substring(start, length);

            Match typeMatch = Regex.Match(
                window,
                "RespawnType\\s*[\\\"'=:\\s]+(?<v>[A-Z_]+)",
                RegexOptions.IgnoreCase);
            if (!typeMatch.Success)
            {
                return false;
            }

            string type = typeMatch.Groups["v"].Value
                .ToUpperInvariant();
            Match amountMatch = Regex.Match(
                window,
                "RespawnRealTime\\s*[\\\"'=:\\s]+(?<v>\\d+)",
                RegexOptions.IgnoreCase);
            int amount = 0;
            if (amountMatch.Success)
            {
                Int32.TryParse(
                    amountMatch.Groups["v"].Value,
                    NumberStyles.Integer,
                    CultureInfo.InvariantCulture,
                    out amount);
            }

            if (type.IndexOf(
                    "RELATIVE",
                    StringComparison.OrdinalIgnoreCase) >= 0
                && amount > 0)
            {
                rule = RespawnRule.Relative(
                    amount,
                    source);
                return true;
            }

            if (type.IndexOf(
                    "DAILY",
                    StringComparison.OrdinalIgnoreCase) >= 0)
            {
                int hour = ParseOptionalInt(
                    window,
                    "RespawnHour",
                    9,
                    0,
                    23);
                int minute = ParseOptionalInt(
                    window,
                    "RespawnMinute",
                    0,
                    0,
                    59);
                rule = RespawnRule.Daily(
                    hour,
                    minute,
                    source);
                return true;
            }
            return false;
        }

        private static int ParseOptionalInt(
            string text,
            string field,
            int fallback,
            int minimum,
            int maximum)
        {
            Match match = Regex.Match(
                text,
                Regex.Escape(field)
                    + "\\s*[\\\"'=:\\s]+(?<v>\\d+)",
                RegexOptions.IgnoreCase);
            int value;
            if (!match.Success
                || !Int32.TryParse(
                    match.Groups["v"].Value,
                    NumberStyles.Integer,
                    CultureInfo.InvariantCulture,
                    out value)
                || value < minimum
                || value > maximum)
            {
                return fallback;
            }
            return value;
        }

        private static string ReadSaveKey(
            int processId,
            string executablePath)
        {
            if (_processId == processId
                && !String.IsNullOrEmpty(_cachedKey))
            {
                return _cachedKey;
            }
            if (String.IsNullOrWhiteSpace(executablePath)
                || !File.Exists(executablePath))
            {
                throw new FileNotFoundException(
                    "Game executable was not found.",
                    executablePath);
            }

            IntPtr process = NativeMethods.OpenProcess(
                ProcessReadAccess,
                false,
                processId);
            if (process == IntPtr.Zero)
            {
                throw new InvalidOperationException(
                    "OpenProcess failed: "
                    + Marshal.GetLastWin32Error().ToString(
                        CultureInfo.InvariantCulture));
            }

            try
            {
                ulong moduleBase = GetModuleBaseAddress(processId);
                List<ulong> candidates = GetOwnerPointerCandidates(
                    executablePath);
                Exception lastError = null;
                foreach (ulong candidate in candidates)
                {
                    try
                    {
                        string key = ReadKeyAtRva(
                            process,
                            moduleBase,
                            candidate);
                        _cachedKey = key;
                        _cachedDetectedRva = candidate;
                        return key;
                    }
                    catch (Exception exception)
                    {
                        lastError = exception;
                    }
                }
                string lastDetail = lastError == null
                    ? "none"
                    : lastError.Message;
                throw new InvalidOperationException(
                    "Save database key could not be located. "
                    + "process_id="
                    + processId.ToString(CultureInfo.InvariantCulture)
                    + " candidate_count="
                    + candidates.Count.ToString(CultureInfo.InvariantCulture)
                    + " detected_rva=0x"
                    + _cachedDetectedRva.ToString("X", CultureInfo.InvariantCulture)
                    + " last_error="
                    + lastDetail,
                    lastError);
            }
            finally
            {
                NativeMethods.CloseHandle(process);
            }
        }

        private static ulong GetModuleBaseAddress(int processId)
        {
            System.Diagnostics.Process process =
                System.Diagnostics.Process.GetProcessById(processId);
            try
            {
                return unchecked((ulong)
                    process.MainModule.BaseAddress.ToInt64());
            }
            finally
            {
                process.Dispose();
            }
        }

        private static List<ulong> GetOwnerPointerCandidates(
            string executablePath)
        {
            DateTime executableWriteUtc =
                File.GetLastWriteTimeUtc(executablePath);
            if (!String.Equals(
                    _cachedExecutablePath,
                    executablePath,
                    StringComparison.OrdinalIgnoreCase)
                || _cachedExecutableWriteUtc != executableWriteUtc)
            {
                _cachedExecutablePath = executablePath;
                _cachedExecutableWriteUtc = executableWriteUtc;
                _cachedDetectedRva = DetectOwnerPointerRva(
                    executablePath);
            }

            List<ulong> result = new List<ulong>();
            if (_cachedDetectedRva != 0)
            {
                result.Add(_cachedDetectedRva);
            }
            foreach (ulong fallback in FallbackOwnerPointerRvas)
            {
                if (!result.Contains(fallback))
                {
                    result.Add(fallback);
                }
            }
            return result;
        }

        private static string ReadKeyAtRva(
            IntPtr process,
            ulong moduleBase,
            ulong ownerPointerRva)
        {
            ulong owner = ReadUInt64(
                process,
                moduleBase + ownerPointerRva);
            if (owner == 0)
            {
                throw new InvalidOperationException(
                    "Save database owner is not ready.");
            }

            ulong keyPointer = ReadUInt64(
                process,
                owner + 0x120UL);
            int keyLength = ReadInt32(
                process,
                owner + 0x128UL);
            if (keyPointer == 0
                || keyLength <= 1
                || keyLength > 256)
            {
                throw new InvalidOperationException(
                    "Save database key is not ready.");
            }

            string key = Encoding.Unicode.GetString(
                ReadBytes(
                    process,
                    keyPointer,
                    checked(keyLength * 2)))
                .TrimEnd('\0');
            if (key.Length == 0)
            {
                throw new InvalidOperationException(
                    "Save database key is empty.");
            }
            foreach (char character in key)
            {
                if (character < 0x20 || character > 0x7E)
                {
                    throw new InvalidOperationException(
                        "Save database key contains invalid characters.");
                }
            }
            return key;
        }

        private static ulong DetectOwnerPointerRva(
            string executablePath)
        {
            try
            {
                byte[] image = File.ReadAllBytes(executablePath);
                if (image.Length < 0x100)
                {
                    return 0;
                }
                int peOffset = BitConverter.ToInt32(image, 0x3C);
                if (peOffset <= 0
                    || peOffset + 24 > image.Length
                    || BitConverter.ToUInt32(image, peOffset)
                        != 0x00004550)
                {
                    return 0;
                }

                int sectionCount = BitConverter.ToUInt16(
                    image,
                    peOffset + 6);
                int optionalHeaderSize = BitConverter.ToUInt16(
                    image,
                    peOffset + 20);
                int sectionTable = peOffset + 24 + optionalHeaderSize;

                for (int index = 0; index < sectionCount; index++)
                {
                    int header = sectionTable + index * 40;
                    if (header < 0 || header + 40 > image.Length)
                    {
                        break;
                    }
                    uint rawSize = BitConverter.ToUInt32(
                        image,
                        header + 16);
                    uint rawOffset = BitConverter.ToUInt32(
                        image,
                        header + 20);
                    uint virtualAddress = BitConverter.ToUInt32(
                        image,
                        header + 12);
                    if (rawOffset > Int32.MaxValue
                        || rawSize > Int32.MaxValue)
                    {
                        continue;
                    }
                    int match = FindPattern(
                        image,
                        (int)rawOffset,
                        (int)rawSize);
                    if (match < 0)
                    {
                        continue;
                    }
                    int displacement = BitConverter.ToInt32(
                        image,
                        match + 3);
                    long instructionRva =
                        (long)virtualAddress + match - rawOffset;
                    long result = instructionRva + 7L + displacement;
                    return result > 0
                        ? unchecked((ulong)result)
                        : 0;
                }
            }
            catch
            {
            }
            return 0;
        }

        private static int FindPattern(
            byte[] data,
            int start,
            int size)
        {
            if (start < 0
                || size <= 0
                || start >= data.Length)
            {
                return -1;
            }
            long rawEnd = (long)start + size;
            int end = (int)Math.Min(
                data.Length,
                rawEnd);
            end -= OwnerReferencePattern.Length;
            for (int offset = start; offset <= end; offset++)
            {
                bool matches = true;
                for (int index = 0;
                    index < OwnerReferencePattern.Length;
                    index++)
                {
                    if (OwnerReferenceMask[index] != '?'
                        && data[offset + index]
                            != OwnerReferencePattern[index])
                    {
                        matches = false;
                        break;
                    }
                }
                if (matches)
                {
                    return offset;
                }
            }
            return -1;
        }

        private static ulong ReadUInt64(
            IntPtr process,
            ulong address)
        {
            return BitConverter.ToUInt64(
                ReadBytes(process, address, 8),
                0);
        }

        private static int ReadInt32(
            IntPtr process,
            ulong address)
        {
            return BitConverter.ToInt32(
                ReadBytes(process, address, 4),
                0);
        }

        private static byte[] ReadBytes(
            IntPtr process,
            ulong address,
            int size)
        {
            byte[] bytes = new byte[size];
            IntPtr read;
            bool success = NativeMethods.ReadProcessMemory(
                process,
                new IntPtr(unchecked((long)address)),
                bytes,
                new IntPtr(size),
                out read);
            if (!success || read.ToInt64() != size)
            {
                throw new InvalidOperationException(
                    "ReadProcessMemory failed at 0x"
                    + address.ToString("X", CultureInfo.InvariantCulture)
                    + ": "
                    + Marshal.GetLastWin32Error().ToString(
                        CultureInfo.InvariantCulture));
            }
            return bytes;
        }

        private static SaveSlot LocateActiveSlot(string saveRoot)
        {
            if (String.IsNullOrWhiteSpace(saveRoot)
                || !Directory.Exists(saveRoot))
            {
                throw new DirectoryNotFoundException(
                    "SaveGames directory was not found: "
                    + saveRoot);
            }

            Dictionary<string, List<string>> groups =
                new Dictionary<string, List<string>>(
                    StringComparer.OrdinalIgnoreCase);
            Stack<string> pending = new Stack<string>();
            pending.Push(saveRoot);

            while (pending.Count > 0)
            {
                string directory = pending.Pop();
                string[] files;
                try
                {
                    files = Directory.GetFiles(directory);
                }
                catch
                {
                    files = new string[0];
                }
                foreach (string path in files)
                {
                    string extension = Path.GetExtension(path);
                    string name = Path.GetFileName(path);
                    if ((!extension.Equals(
                                ".db",
                                StringComparison.OrdinalIgnoreCase)
                            && !extension.Equals(
                                ".bak",
                                StringComparison.OrdinalIgnoreCase))
                        || name.IndexOf(
                            "slot",
                            StringComparison.OrdinalIgnoreCase) < 0)
                    {
                        continue;
                    }
                    string stem = Path.Combine(
                        Path.GetDirectoryName(path),
                        Path.GetFileNameWithoutExtension(path));
                    List<string> group;
                    if (!groups.TryGetValue(stem, out group))
                    {
                        group = new List<string>();
                        groups[stem] = group;
                    }
                    group.Add(path);
                }

                string[] children;
                try
                {
                    children = Directory.GetDirectories(directory);
                }
                catch
                {
                    children = new string[0];
                }
                foreach (string child in children)
                {
                    pending.Push(child);
                }
            }

            string selectedStem = null;
            DateTime selectedWriteUtc = DateTime.MinValue;
            foreach (KeyValuePair<string, List<string>> pair in groups)
            {
                string authoritative = SelectAuthoritativeCandidate(
                    pair.Value);
                DateTime newest = authoritative == null
                    ? DateTime.MinValue
                    : GetRelatedNewestWriteUtc(authoritative);
                if (selectedStem == null || newest > selectedWriteUtc)
                {
                    selectedStem = pair.Key;
                    selectedWriteUtc = newest;
                }
            }

            if (selectedStem == null)
            {
                throw new FileNotFoundException(
                    "No slot .db/.bak database was found.",
                    saveRoot);
            }

            List<string> selectedFiles = groups[selectedStem];
            selectedFiles.Sort(delegate(string left, string right)
            {
                int priority = DatabaseCandidatePriority(left).CompareTo(
                    DatabaseCandidatePriority(right));
                if (priority != 0)
                {
                    return priority;
                }
                return GetRelatedNewestWriteUtc(right).CompareTo(
                    GetRelatedNewestWriteUtc(left));
            });

            StringBuilder signature = new StringBuilder();
            foreach (string path in selectedFiles)
            {
                AppendFileSignature(signature, path);
                AppendFileSignature(signature, path + "-wal");
                AppendFileSignature(signature, path + "-shm");
                AppendFileSignature(signature, path + "-journal");
            }

            return new SaveSlot
            {
                Stem = selectedStem,
                MainFiles = selectedFiles,
                Signature = signature.ToString(),
                Summary = Path.GetFileName(selectedStem)
                    + " files="
                    + selectedFiles.Count.ToString(
                        CultureInfo.InvariantCulture)
                    + " newest="
                    + selectedWriteUtc.ToString(
                        "O",
                        CultureInfo.InvariantCulture)
            };
        }

        private static string SelectAuthoritativeCandidate(
            IList<string> paths)
        {
            string selected = null;
            int selectedPriority = Int32.MaxValue;
            DateTime selectedWriteUtc = DateTime.MinValue;
            foreach (string path in paths)
            {
                int priority = DatabaseCandidatePriority(path);
                DateTime writeUtc = GetRelatedNewestWriteUtc(path);
                if (selected == null
                    || priority < selectedPriority
                    || (priority == selectedPriority
                        && writeUtc > selectedWriteUtc))
                {
                    selected = path;
                    selectedPriority = priority;
                    selectedWriteUtc = writeUtc;
                }
            }
            return selected;
        }

        private static int DatabaseCandidatePriority(string path)
        {
            string extension = Path.GetExtension(path);
            if (extension.Equals(
                    ".db",
                    StringComparison.OrdinalIgnoreCase))
            {
                return 0;
            }
            if (extension.Equals(
                    ".bak",
                    StringComparison.OrdinalIgnoreCase))
            {
                return 1;
            }
            return 2;
        }

        private static DateTime GetRelatedNewestWriteUtc(string path)
        {
            DateTime newest = File.Exists(path)
                ? File.GetLastWriteTimeUtc(path)
                : DateTime.MinValue;
            string[] suffixes =
            {
                "-wal", "-shm", "-journal"
            };
            foreach (string suffix in suffixes)
            {
                string related = path + suffix;
                if (File.Exists(related))
                {
                    DateTime write = File.GetLastWriteTimeUtc(
                        related);
                    if (write > newest)
                    {
                        newest = write;
                    }
                }
            }
            return newest;
        }

        private static void AppendFileSignature(
            StringBuilder signature,
            string path)
        {
            if (!File.Exists(path))
            {
                return;
            }
            FileInfo file = new FileInfo(path);
            signature.Append(path.ToLowerInvariant());
            signature.Append('|');
            signature.Append(file.Length.ToString(
                CultureInfo.InvariantCulture));
            signature.Append('|');
            signature.Append(file.LastWriteTimeUtc.Ticks.ToString(
                CultureInfo.InvariantCulture));
            signature.Append(';');
        }

        private static SaveSnapshot ReadSaveSlot(
            SaveSlot slot,
            string key)
        {
            string temporaryRoot = Path.Combine(
                Path.GetTempPath(),
                "DragonSwordWorldDataProbe",
                Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(temporaryRoot);

            List<string> attemptedFiles = new List<string>();
            Exception lastError = null;

            try
            {
                foreach (string sourceMain in slot.MainFiles)
                {
                    attemptedFiles.Add(Path.GetFileName(sourceMain));
                    try
                    {
                        string destinationMain = Path.Combine(
                            temporaryRoot,
                            Path.GetFileName(sourceMain));
                        CopyDatabaseSet(
                            sourceMain,
                            destinationMain);
                        Dictionary<int, BossRow> rows =
                            QueryBossRows(destinationMain, key);
                        TryCaptureLogicalDatabaseSnapshot(
                            destinationMain,
                            key,
                            slot.Stem,
                            Path.GetFileName(sourceMain));
                        return new SaveSnapshot
                        {
                            Rows = rows,
                            LoadedFiles = new List<string>
                            {
                                Path.GetFileName(sourceMain)
                            },
                            AttemptedFiles = attemptedFiles
                        };
                    }
                    catch (Exception exception)
                    {
                        lastError = exception;
                        AppendChange(
                            "SAVE_DATABASE_READ_SKIPPED",
                            "source=" + sourceMain
                            + "; error=" + exception.Message);
                    }
                }
            }
            finally
            {
                try
                {
                    Directory.Delete(temporaryRoot, true);
                }
                catch
                {
                }
            }

            throw new InvalidOperationException(
                "No authoritative save database snapshot could be read. Attempted: "
                + String.Join(",", attemptedFiles.ToArray()),
                lastError);
        }

        private static void CopyDatabaseSet(
            string sourceMain,
            string destinationMain)
        {
            CopyFileShared(sourceMain, destinationMain);
            string[] suffixes =
            {
                "-wal", "-shm", "-journal"
            };
            foreach (string suffix in suffixes)
            {
                string source = sourceMain + suffix;
                if (!File.Exists(source))
                {
                    continue;
                }
                CopyFileShared(
                    source,
                    destinationMain + suffix);
            }
        }

        private static void CopyFileShared(
            string source,
            string destination)
        {
            using (FileStream input = new FileStream(
                source,
                FileMode.Open,
                FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete))
            using (FileStream output = new FileStream(
                destination,
                FileMode.Create,
                FileAccess.Write,
                FileShare.Read))
            {
                byte[] buffer = new byte[1024 * 1024];
                int read;
                while ((read = input.Read(
                    buffer,
                    0,
                    buffer.Length)) > 0)
                {
                    output.Write(buffer, 0, read);
                }
                output.Flush();
            }
        }

        private static Dictionary<int, BossRow> QueryBossRows(
            string databasePath,
            string key)
        {
            IntPtr database = IntPtr.Zero;
            try
            {
                int openResult = SqliteMethods.sqlite3_open_v2(
                    Utf8(databasePath),
                    out database,
                    SqliteOpenReadOnly,
                    IntPtr.Zero);
                if (openResult != 0)
                {
                    throw SqliteError(
                        database,
                        openResult,
                        IntPtr.Zero);
                }

                Dictionary<int, BossRow> rows =
                    new Dictionary<int, BossRow>();
                string escapedKey = key.Replace("'", "''");
                string sql =
                    "PRAGMA key = '" + escapedKey + "';"
                    + "PRAGMA cipher_compatibility = 4;"
                    + "SELECT ACTOR_CID,RESPAWN_TYPE,DESTROY_TIME "
                    + "FROM tb_actor_respawn WHERE ACTOR_CID IN "
                    + "(9000005,9000007,9000010,9000011,9000012,"
                    + "9000019,9000022,9000023,9000025);";

                Exception callbackError = null;
                int invalidRows = 0;
                SqliteMethods.ExecCallback callback = delegate(
                    IntPtr context,
                    int count,
                    IntPtr values,
                    IntPtr names)
                {
                    try
                    {
                        if (count < 3)
                        {
                            invalidRows++;
                            return 0;
                        }
                        int bossId = 0;
                        int respawnType = 0;
                        long destroyRaw = 0;
                        bool parsed = Int32.TryParse(
                                PointerString(Marshal.ReadIntPtr(
                                    values,
                                    0)),
                                NumberStyles.Integer,
                                CultureInfo.InvariantCulture,
                                out bossId)
                            && Int32.TryParse(
                                PointerString(Marshal.ReadIntPtr(
                                    values,
                                    IntPtr.Size)),
                                NumberStyles.Integer,
                                CultureInfo.InvariantCulture,
                                out respawnType)
                            && Int64.TryParse(
                                PointerString(Marshal.ReadIntPtr(
                                    values,
                                    IntPtr.Size * 2)),
                                NumberStyles.Integer,
                                CultureInfo.InvariantCulture,
                                out destroyRaw);
                        if (!parsed)
                        {
                            invalidRows++;
                            return 0;
                        }

                        long destroySeconds;
                        DateTime destroyUtc;
                        string destroyTimeStatus;
                        bool destroyTimeNormalized =
                            TryNormalizeDestroyTime(
                                destroyRaw,
                                out destroySeconds,
                                out destroyUtc,
                                out destroyTimeStatus);
                        BossRow candidate = new BossRow
                        {
                            BossId = bossId,
                            RespawnType = respawnType,
                            DestroyRaw = destroyRaw,
                            DestroyTimeNormalized =
                                destroyTimeNormalized,
                            DestroyTimeStatus = destroyTimeStatus,
                            DestroyUnixSeconds = destroySeconds,
                            DestroyTimeUtc = destroyUtc,
                            SourceDatabase = databasePath
                        };
                        BossRow existing;
                        if (!rows.TryGetValue(bossId, out existing)
                            || IsNewerBossRow(candidate, existing))
                        {
                            rows[bossId] = candidate;
                        }
                        return 0;
                    }
                    catch (Exception callbackException)
                    {
                        callbackError = callbackException;
                        return 1;
                    }
                };

                try
                {
                    ExecuteSql(database, sql, callback);
                }
                catch (Exception)
                {
                    if (callbackError != null)
                    {
                        throw new InvalidDataException(
                            "Boss row callback failed without crossing the native SQLCipher boundary.",
                            callbackError);
                    }
                    throw;
                }
                if (callbackError != null)
                {
                    throw new InvalidDataException(
                        "Boss row callback failed without crossing the native SQLCipher boundary.",
                        callbackError);
                }
                if (invalidRows > 0)
                {
                    AppendChange(
                        "SAVE_DATABASE_ROWS_SKIPPED",
                        "source=" + databasePath
                        + "; invalid_rows=" + invalidRows.ToString(
                            CultureInfo.InvariantCulture));
                }
                return rows;
            }
            finally
            {
                if (database != IntPtr.Zero)
                {
                    SqliteMethods.sqlite3_close_v2(database);
                }
            }
        }

        private static void TryCaptureLogicalDatabaseSnapshot(
            string databasePath,
            string key,
            string slotStem,
            string sourceName)
        {
            try
            {
                Dictionary<string, LogicalTableSnapshot> current =
                    CaptureLogicalDatabase(databasePath, key);
                DateTime nowUtc = DateTime.UtcNow;
                _logicalSnapshotSequence++;
                string stamp = nowUtc.ToString(
                    "yyyyMMdd-HHmmssfff",
                    CultureInfo.InvariantCulture);
                string snapshotName = String.Format(
                    CultureInfo.InvariantCulture,
                    "{0:D4}-{1}",
                    _logicalSnapshotSequence,
                    stamp);
                string snapshotRoot = Path.Combine(
                    _databaseDiffRoot,
                    "snapshots",
                    snapshotName);
                Directory.CreateDirectory(snapshotRoot);
                WriteLogicalSnapshot(
                    snapshotRoot,
                    current,
                    slotStem,
                    sourceName,
                    nowUtc);

                string diffRoot = Path.Combine(
                    _databaseDiffRoot,
                    "diffs");
                Directory.CreateDirectory(diffRoot);
                string diffPath = Path.Combine(
                    diffRoot,
                    snapshotName + "-changes.txt");
                WriteLogicalDiff(
                    diffPath,
                    _lastLogicalTables,
                    current,
                    slotStem,
                    sourceName,
                    nowUtc);
                string spawnConditionReport = WriteSpawnConditionCorrelationReport(
                    current,
                    slotStem,
                    sourceName,
                    nowUtc,
                    snapshotName);
                _lastLogicalTables = current;
                AppendChange(
                    "SAVE_DATABASE_LOGICAL_SNAPSHOT",
                    "sequence=" + _logicalSnapshotSequence.ToString(
                        CultureInfo.InvariantCulture)
                    + "; tables=" + current.Count.ToString(
                        CultureInfo.InvariantCulture)
                    + "; snapshot=" + snapshotRoot
                    + "; diff=" + diffPath
                    + "; spawn_condition_report=" + spawnConditionReport);
            }
            catch (Exception exception)
            {
                AppendChange(
                    "SAVE_DATABASE_LOGICAL_SNAPSHOT_ERROR",
                    "source=" + databasePath
                    + "; error=" + exception.Message);
            }
        }

        private static Dictionary<string, LogicalTableSnapshot>
            CaptureLogicalDatabase(string databasePath, string key)
        {
            IntPtr database = IntPtr.Zero;
            try
            {
                int openResult = SqliteMethods.sqlite3_open_v2(
                    Utf8(databasePath),
                    out database,
                    SqliteOpenReadOnly,
                    IntPtr.Zero);
                if (openResult != 0)
                {
                    throw SqliteError(database, openResult, IntPtr.Zero);
                }

                string escapedKey = key.Replace("'", "''");
                SqliteMethods.ExecCallback pragmaCallback = delegate(
                    IntPtr context,
                    int count,
                    IntPtr values,
                    IntPtr names)
                {
                    return 0;
                };
                ExecuteSql(
                    database,
                    "PRAGMA key = '" + escapedKey + "';"
                    + "PRAGMA cipher_compatibility = 4;",
                    pragmaCallback);

                List<string> tableNames = new List<string>();
                Exception callbackError = null;
                SqliteMethods.ExecCallback tableCallback = delegate(
                    IntPtr context,
                    int count,
                    IntPtr values,
                    IntPtr names)
                {
                    try
                    {
                        if (count > 0)
                        {
                            string name = PointerString(
                                Marshal.ReadIntPtr(values, 0));
                            if (!String.IsNullOrWhiteSpace(name))
                            {
                                tableNames.Add(name);
                            }
                        }
                        return 0;
                    }
                    catch (Exception exception)
                    {
                        callbackError = exception;
                        return 1;
                    }
                };
                ExecuteSql(
                    database,
                    "SELECT name FROM sqlite_master "
                    + "WHERE type='table' AND name NOT LIKE 'sqlite_%' "
                    + "ORDER BY name;",
                    tableCallback);
                if (callbackError != null)
                {
                    throw callbackError;
                }

                Dictionary<string, LogicalTableSnapshot> result =
                    new Dictionary<string, LogicalTableSnapshot>(
                        StringComparer.OrdinalIgnoreCase);
                int totalCharacters = 0;
                foreach (string tableName in tableNames)
                {
                    if (totalCharacters >= MaximumLogicalCharactersTotal)
                    {
                        break;
                    }
                    LogicalTableSnapshot table = CaptureLogicalTable(
                        database,
                        tableName,
                        MaximumLogicalCharactersTotal - totalCharacters);
                    result[tableName] = table;
                    totalCharacters += table.CharacterCount;
                }
                return result;
            }
            finally
            {
                if (database != IntPtr.Zero)
                {
                    SqliteMethods.sqlite3_close_v2(database);
                }
            }
        }

        private static LogicalTableSnapshot CaptureLogicalTable(
            IntPtr database,
            string tableName,
            int remainingCharacterBudget)
        {
            List<string> columns = new List<string>();
            Exception callbackError = null;
            SqliteMethods.ExecCallback columnCallback = delegate(
                IntPtr context,
                int count,
                IntPtr values,
                IntPtr names)
            {
                try
                {
                    if (count >= 2)
                    {
                        columns.Add(PointerString(
                            Marshal.ReadIntPtr(values, IntPtr.Size)));
                    }
                    return 0;
                }
                catch (Exception exception)
                {
                    callbackError = exception;
                    return 1;
                }
            };
            ExecuteSql(
                database,
                "PRAGMA table_info(" + QuoteIdentifier(tableName) + ");",
                columnCallback);
            if (callbackError != null)
            {
                throw callbackError;
            }

            List<string> rows = new List<string>();
            int characterCount = 0;
            bool truncated = false;
            callbackError = null;
            SqliteMethods.ExecCallback rowCallback = delegate(
                IntPtr context,
                int count,
                IntPtr values,
                IntPtr names)
            {
                try
                {
                    if (rows.Count >= MaximumLogicalRowsPerTable
                        || characterCount >= MaximumLogicalCharactersPerTable
                        || characterCount >= remainingCharacterBudget)
                    {
                        truncated = true;
                        return 1;
                    }
                    StringBuilder row = new StringBuilder();
                    for (int index = 0; index < count; index++)
                    {
                        if (index > 0) row.Append('\t');
                        string value = PointerString(Marshal.ReadIntPtr(
                            values,
                            index * IntPtr.Size));
                        row.Append(EscapeTsv(value));
                    }
                    string rowText = row.ToString();
                    rows.Add(rowText);
                    characterCount += rowText.Length + 1;
                    return 0;
                }
                catch (Exception exception)
                {
                    callbackError = exception;
                    return 1;
                }
            };
            try
            {
                ExecuteSql(
                    database,
                    "SELECT * FROM " + QuoteIdentifier(tableName)
                    + " LIMIT "
                    + (MaximumLogicalRowsPerTable + 1).ToString(
                        CultureInfo.InvariantCulture)
                    + ";",
                    rowCallback);
            }
            catch (InvalidOperationException exception)
            {
                if (!truncated || exception.Message.IndexOf(
                    "SQLCipher error 4",
                    StringComparison.OrdinalIgnoreCase) < 0)
                {
                    throw;
                }
            }
            if (callbackError != null)
            {
                throw callbackError;
            }
            rows.Sort(StringComparer.Ordinal);
            string canonical = String.Join("\n", rows.ToArray());
            return new LogicalTableSnapshot
            {
                Name = tableName,
                Columns = columns,
                Rows = rows,
                Hash = ComputeSha256(canonical),
                CharacterCount = characterCount,
                Truncated = truncated
            };
        }


        private static string WriteSpawnConditionCorrelationReport(
            Dictionary<string, LogicalTableSnapshot> tables,
            string slotStem,
            string sourceName,
            DateTime nowUtc,
            string snapshotName)
        {
            Directory.CreateDirectory(_spawnConditionReportRoot);
            string reportPath = Path.Combine(
                _spawnConditionReportRoot,
                snapshotName + "-spawn-condition-report.txt");
            StringBuilder output = new StringBuilder();
            output.AppendLine("DragonSword world boss spawn-condition correlation");
            output.AppendLine("version=" + Version);
            output.AppendLine("captured_utc=" + nowUtc.ToString("o"));
            output.AppendLine("slot=" + slotStem);
            output.AppendLine("source=" + sourceName);
            output.AppendLine("purpose=correlate nine static world bosses with spawn-condition and runtime-state tables");
            output.AppendLine("quest_completion_policy=non_authoritative; tb_quest_complete and switch_week_id must not be used to infer alive, killed, respawning, or cooldown state");
            output.AppendLine("required_authoritative_evidence=global spawn enable/state, next-spawn time, destroy time, timer, or equivalent generation-manager state");
            output.AppendLine();

            List<string> tableNames = new List<string>(tables.Keys);
            tableNames.Sort(StringComparer.OrdinalIgnoreCase);
            output.AppendLine("CANDIDATE_TABLES");
            int candidateTableCount = 0;
            foreach (string tableName in tableNames)
            {
                LogicalTableSnapshot table = tables[tableName];
                string schemaText = (tableName + " "
                    + String.Join(" ", table.Columns.ToArray())).ToLowerInvariant();
                if (!tableName.Equals("tb_quest_complete", StringComparison.OrdinalIgnoreCase)
                    && ContainsStateKeyword(schemaText))
                {
                    candidateTableCount++;
                    output.AppendLine(
                        "table=" + tableName
                        + "\tcolumns=" + String.Join(",", table.Columns.ToArray())
                        + "\trows=" + table.Rows.Count.ToString(CultureInfo.InvariantCulture)
                        + "\ttruncated=" + table.Truncated.ToString());
                }
            }
            output.AppendLine("candidate_table_count="
                + candidateTableCount.ToString(CultureInfo.InvariantCulture));
            output.AppendLine();

            SortedSet<string> discoveredConditionIds = new SortedSet<string>(StringComparer.Ordinal);
            int bossesWithReferences = 0;
            foreach (BossCatalogKey boss in BossCatalogKeys)
            {
                List<string> tokens = BuildBossCorrelationTokens(boss);
                List<string> references = new List<string>();
                foreach (string tableName in tableNames)
                {
                    if (tableName.Equals("tb_quest_complete", StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }
                    LogicalTableSnapshot table = tables[tableName];
                    int conditionColumnIndex = FindColumnIndex(
                        table.Columns,
                        "SPAWN_CONDITION_ID");
                    for (int rowIndex = 0;
                        rowIndex < table.Rows.Count && references.Count < 120;
                        rowIndex++)
                    {
                        string row = table.Rows[rowIndex];
                        string matchedToken;
                        if (!RowContainsAnyToken(row, tokens, out matchedToken))
                        {
                            continue;
                        }
                        references.Add(
                            "table=" + tableName
                            + "\tmatch=" + matchedToken
                            + "\trow=" + SafeInline(row));
                        if (conditionColumnIndex >= 0)
                        {
                            string conditionId = GetTsvField(row, conditionColumnIndex);
                            if (!String.IsNullOrWhiteSpace(conditionId))
                            {
                                discoveredConditionIds.Add(conditionId);
                            }
                        }
                    }
                }

                output.AppendLine("BOSS " + boss.BossId.ToString(CultureInfo.InvariantCulture));
                output.AppendLine("uid_name=" + boss.UidName);
                output.AppendLine("uid=" + boss.Uid);
                output.AppendLine("section_uid=" + boss.SectionUid);
                output.AppendLine("group_id=" + boss.GroupId);
                output.AppendLine("switch_week_id=" + boss.SwitchWeekId);
                output.AppendLine("level_cid=" + boss.LevelCid);
                output.AppendLine("world_map_section_id=" + boss.WorldMapSectionId);
                output.AppendLine("reference_count="
                    + references.Count.ToString(CultureInfo.InvariantCulture));
                if (references.Count > 0)
                {
                    bossesWithReferences++;
                    foreach (string reference in references)
                    {
                        output.AppendLine("  " + reference);
                    }
                }
                else
                {
                    output.AppendLine("  no_database_reference_found=true");
                }
                output.AppendLine();
            }

            output.AppendLine("GLOBAL_GENERATION_STATE_CANDIDATES");
            int authoritativeCandidateCount = 0;
            foreach (string tableName in tableNames)
            {
                if (tableName.Equals("tb_quest_complete", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
                LogicalTableSnapshot table = tables[tableName];
                string signature = (tableName + " " + String.Join(" ", table.Columns.ToArray())).ToLowerInvariant();
                bool hasIdentity = signature.IndexOf("actor", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("spawn", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("section", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("monster", StringComparison.Ordinal) >= 0;
                bool hasState = signature.IndexOf("state", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("enable", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("respawn", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("refresh", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("destroy", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("cooldown", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("timer", StringComparison.Ordinal) >= 0
                    || signature.IndexOf("next", StringComparison.Ordinal) >= 0;
                if (hasIdentity && hasState)
                {
                    authoritativeCandidateCount++;
                    output.AppendLine("table=" + tableName
                        + "\tcolumns=" + String.Join(",", table.Columns.ToArray())
                        + "\trows=" + table.Rows.Count.ToString(CultureInfo.InvariantCulture)
                        + "\ttruncated=" + table.Truncated.ToString());
                }
            }
            output.AppendLine("authoritative_candidate_count=" + authoritativeCandidateCount.ToString(CultureInfo.InvariantCulture));
            output.AppendLine();

            output.AppendLine("DISCOVERED_SPAWN_CONDITION_IDS");
            if (discoveredConditionIds.Count == 0)
            {
                output.AppendLine("count=0");
                output.AppendLine("note=no row containing a known boss anchor also exposed SPAWN_CONDITION_ID");
            }
            else
            {
                output.AppendLine("count="
                    + discoveredConditionIds.Count.ToString(CultureInfo.InvariantCulture));
                foreach (string conditionId in discoveredConditionIds)
                {
                    output.AppendLine("id=" + conditionId);
                    int emitted = 0;
                    foreach (string tableName in tableNames)
                    {
                        LogicalTableSnapshot table = tables[tableName];
                        for (int rowIndex = 0;
                            rowIndex < table.Rows.Count && emitted < 100;
                            rowIndex++)
                        {
                            if (ContainsToken(table.Rows[rowIndex], conditionId))
                            {
                                output.AppendLine(
                                    "  table=" + tableName
                                    + "\trow=" + SafeInline(table.Rows[rowIndex]));
                                emitted++;
                            }
                        }
                    }
                }
            }
            output.AppendLine();
            output.AppendLine("SUMMARY");
            output.AppendLine("bosses=" + BossCatalogKeys.Length.ToString(CultureInfo.InvariantCulture));
            output.AppendLine("bosses_with_database_references="
                + bossesWithReferences.ToString(CultureInfo.InvariantCulture));
            output.AppendLine("spawn_condition_ids="
                + discoveredConditionIds.Count.ToString(CultureInfo.InvariantCulture));
            output.AppendLine("quest_completion_evidence=non_authoritative");
            output.AppendLine("interpretation=absence of a reference is evidence only for this save snapshot, not proof that no runtime manager exists");

            File.WriteAllText(reportPath, output.ToString(), Utf8NoBom);
            File.WriteAllText(
                Path.Combine(_spawnConditionReportRoot, "latest-report.txt"),
                output.ToString(),
                Utf8NoBom);
            AppendChange(
                "WORLD_BOSS_SPAWN_CONDITION_CORRELATION",
                "bosses_with_references="
                + bossesWithReferences.ToString(CultureInfo.InvariantCulture)
                + "; condition_ids="
                + discoveredConditionIds.Count.ToString(CultureInfo.InvariantCulture)
                + "; candidate_tables="
                + candidateTableCount.ToString(CultureInfo.InvariantCulture)
                + "; report=" + reportPath);
            return reportPath;
        }

        private static bool ContainsStateKeyword(string value)
        {
            if (String.IsNullOrWhiteSpace(value)) return false;
            string[] keywords =
            {
                "boss", "field", "world", "actor", "spawn", "condition",
                "respawn", "refresh", "reset", "cooldown", "destroy",
                "alive", "state", "cycle", "event", "monster", "switch",
                "enable", "shouldspawn", "canspawn", "nextspawn",
                "lastdestroy", "timer", "manager", "subsystem", "sectionactor"
            };
            foreach (string keyword in keywords)
            {
                if (value.IndexOf(keyword, StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    return true;
                }
            }
            return false;
        }

        private static List<string> BuildBossCorrelationTokens(BossCatalogKey boss)
        {
            List<string> result = new List<string>();
            AddCorrelationToken(result, boss.BossId.ToString(CultureInfo.InvariantCulture));
            AddCorrelationToken(result, boss.UidName);
            AddCorrelationToken(result, boss.Uid);
            AddCorrelationToken(result, boss.SectionUid);
            AddCorrelationToken(result, boss.GroupId);
            AddCorrelationToken(result, boss.LevelCid);
            AddCorrelationToken(result, boss.WorldMapSectionId);
            return result;
        }

        private static void AddCorrelationToken(List<string> tokens, string token)
        {
            if (String.IsNullOrWhiteSpace(token) || token == "0") return;
            if (!tokens.Contains(token)) tokens.Add(token);
        }

        private static bool RowContainsAnyToken(
            string row,
            List<string> tokens,
            out string matchedToken)
        {
            foreach (string token in tokens)
            {
                if (ContainsToken(row, token))
                {
                    matchedToken = token;
                    return true;
                }
            }
            matchedToken = String.Empty;
            return false;
        }

        private static bool ContainsToken(string row, string token)
        {
            if (String.IsNullOrWhiteSpace(row)
                || String.IsNullOrWhiteSpace(token))
            {
                return false;
            }
            int start = 0;
            while (true)
            {
                int index = row.IndexOf(token, start, StringComparison.OrdinalIgnoreCase);
                if (index < 0) return false;
                bool leftBoundary = index == 0
                    || row[index - 1] == '\t'
                    || row[index - 1] == ' '
                    || row[index - 1] == '='
                    || row[index - 1] == ':'
                    || row[index - 1] == ',';
                int end = index + token.Length;
                bool rightBoundary = end >= row.Length
                    || row[end] == '\t'
                    || row[end] == ' '
                    || row[end] == '='
                    || row[end] == ':'
                    || row[end] == ',';
                if (leftBoundary && rightBoundary) return true;
                start = index + 1;
            }
        }

        private static int FindColumnIndex(List<string> columns, string expected)
        {
            for (int index = 0; index < columns.Count; index++)
            {
                if (String.Equals(
                    columns[index],
                    expected,
                    StringComparison.OrdinalIgnoreCase))
                {
                    return index;
                }
            }
            return -1;
        }

        private static string GetTsvField(string row, int index)
        {
            string[] fields = row.Split('\t');
            return index >= 0 && index < fields.Length
                ? fields[index]
                : String.Empty;
        }

        private static void WriteLogicalSnapshot(
            string snapshotRoot,
            Dictionary<string, LogicalTableSnapshot> tables,
            string slotStem,
            string sourceName,
            DateTime nowUtc)
        {
            StringBuilder schema = new StringBuilder();
            schema.AppendLine("DragonSword save logical snapshot");
            schema.AppendLine("version=" + Version);
            schema.AppendLine("captured_utc=" + nowUtc.ToString("o"));
            schema.AppendLine("slot=" + slotStem);
            schema.AppendLine("source=" + sourceName);
            schema.AppendLine("table_count=" + tables.Count.ToString(
                CultureInfo.InvariantCulture));
            schema.AppendLine();

            string tablesRoot = Path.Combine(snapshotRoot, "tables");
            Directory.CreateDirectory(tablesRoot);
            List<string> names = new List<string>(tables.Keys);
            names.Sort(StringComparer.OrdinalIgnoreCase);
            foreach (string name in names)
            {
                LogicalTableSnapshot table = tables[name];
                schema.AppendLine(
                    "table=" + name
                    + "\tcolumns=" + String.Join(",", table.Columns.ToArray())
                    + "\trows=" + table.Rows.Count.ToString(
                        CultureInfo.InvariantCulture)
                    + "\tsha256=" + table.Hash
                    + "\ttruncated=" + table.Truncated.ToString());
                string safeName = SafeFileName(name) + ".tsv";
                StringBuilder content = new StringBuilder();
                content.AppendLine(String.Join("\t", table.Columns.ToArray()));
                foreach (string row in table.Rows)
                {
                    content.AppendLine(row);
                }
                File.WriteAllText(
                    Path.Combine(tablesRoot, safeName),
                    content.ToString(),
                    Utf8NoBom);
            }
            File.WriteAllText(
                Path.Combine(snapshotRoot, "schema-and-counts.txt"),
                schema.ToString(),
                Utf8NoBom);
        }

        private static void WriteLogicalDiff(
            string diffPath,
            Dictionary<string, LogicalTableSnapshot> previous,
            Dictionary<string, LogicalTableSnapshot> current,
            string slotStem,
            string sourceName,
            DateTime nowUtc)
        {
            StringBuilder output = new StringBuilder();
            output.AppendLine("DragonSword save database logical diff");
            output.AppendLine("version=" + Version);
            output.AppendLine("captured_utc=" + nowUtc.ToString("o"));
            output.AppendLine("slot=" + slotStem);
            output.AppendLine("source=" + sourceName);
            output.AppendLine("baseline=" + (previous.Count == 0).ToString());
            output.AppendLine();

            SortedSet<string> names = new SortedSet<string>(
                StringComparer.OrdinalIgnoreCase);
            foreach (string name in previous.Keys) names.Add(name);
            foreach (string name in current.Keys) names.Add(name);
            int changedTables = 0;
            foreach (string name in names)
            {
                LogicalTableSnapshot before;
                LogicalTableSnapshot after;
                bool hadBefore = previous.TryGetValue(name, out before);
                bool hasAfter = current.TryGetValue(name, out after);
                if (hadBefore && hasAfter
                    && String.Equals(before.Hash, after.Hash,
                        StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
                changedTables++;
                output.AppendLine("TABLE " + name);
                if (!hadBefore)
                {
                    output.AppendLine("  operation=TABLE_ADDED");
                }
                else if (!hasAfter)
                {
                    output.AppendLine("  operation=TABLE_REMOVED");
                }
                else
                {
                    output.AppendLine("  operation=ROWS_CHANGED");
                }
                output.AppendLine("  before_rows=" + (hadBefore
                    ? before.Rows.Count.ToString(CultureInfo.InvariantCulture)
                    : "0"));
                output.AppendLine("  after_rows=" + (hasAfter
                    ? after.Rows.Count.ToString(CultureInfo.InvariantCulture)
                    : "0"));

                HashSet<string> beforeRows = new HashSet<string>(
                    hadBefore ? before.Rows : new List<string>(),
                    StringComparer.Ordinal);
                HashSet<string> afterRows = new HashSet<string>(
                    hasAfter ? after.Rows : new List<string>(),
                    StringComparer.Ordinal);
                int emitted = 0;
                foreach (string row in beforeRows)
                {
                    if (!afterRows.Contains(row) && emitted < 200)
                    {
                        output.AppendLine("  - " + row);
                        emitted++;
                    }
                }
                foreach (string row in afterRows)
                {
                    if (!beforeRows.Contains(row) && emitted < 400)
                    {
                        output.AppendLine("  + " + row);
                        emitted++;
                    }
                }
                if (emitted >= 400)
                {
                    output.AppendLine("  detail_truncated=true");
                }
                output.AppendLine();
            }
            output.Insert(
                output.ToString().IndexOf(Environment.NewLine + Environment.NewLine,
                    StringComparison.Ordinal) + Environment.NewLine.Length,
                "changed_tables=" + changedTables.ToString(
                    CultureInfo.InvariantCulture) + Environment.NewLine);
            File.WriteAllText(diffPath, output.ToString(), Utf8NoBom);
        }

        private static string QuoteIdentifier(string value)
        {
            return "\"" + value.Replace("\"", "\"\"") + "\"";
        }

        private static string EscapeTsv(string value)
        {
            if (value == null) return "<NULL>";
            return value.Replace("\\", "\\\\")
                .Replace("\t", "\\t")
                .Replace("\r", "\\r")
                .Replace("\n", "\\n");
        }

        private static string SafeFileName(string value)
        {
            string safe = Regex.Replace(value, "[^A-Za-z0-9_.-]", "_");
            if (safe.Length > 100) safe = safe.Substring(0, 100);
            return safe;
        }

        private static string ComputeSha256(string value)
        {
            using (SHA256 sha = SHA256.Create())
            {
                byte[] bytes = Utf8NoBom.GetBytes(value ?? String.Empty);
                byte[] hash = sha.ComputeHash(bytes);
                StringBuilder text = new StringBuilder(hash.Length * 2);
                foreach (byte item in hash)
                {
                    text.Append(item.ToString("x2", CultureInfo.InvariantCulture));
                }
                return text.ToString();
            }
        }

        private static bool IsNewerBossRow(
            BossRow candidate,
            BossRow existing)
        {
            if (candidate.DestroyTimeNormalized
                && existing.DestroyTimeNormalized)
            {
                return candidate.DestroyUnixSeconds
                    > existing.DestroyUnixSeconds;
            }
            if (candidate.DestroyTimeNormalized
                != existing.DestroyTimeNormalized)
            {
                return candidate.DestroyTimeNormalized;
            }
            return candidate.DestroyRaw > existing.DestroyRaw;
        }

        private static bool TryNormalizeDestroyTime(
            long raw,
            out long seconds,
            out DateTime utc,
            out string status)
        {
            seconds = 0;
            utc = DateTime.MinValue;
            status = "unsupported_raw_unit_or_range";
            if (raw < 0)
            {
                status = "negative_raw_value";
                return false;
            }

            long candidate;
            if (raw <= 32503680000L)
            {
                candidate = raw;
                status = "unix_seconds";
            }
            else if (raw <= 32503680000000L)
            {
                candidate = raw / 1000L;
                status = "unix_milliseconds";
            }
            else
            {
                return false;
            }

            try
            {
                utc = UnixEpochUtc.AddSeconds(candidate);
                seconds = candidate;
                return true;
            }
            catch
            {
                seconds = 0;
                utc = DateTime.MinValue;
                status = "normalized_datetime_out_of_range";
                return false;
            }
        }

        private static void ExecuteSql(
            IntPtr database,
            string sql,
            SqliteMethods.ExecCallback callback)
        {
            IntPtr error;
            int result = SqliteMethods.sqlite3_exec(
                database,
                Utf8(sql),
                callback,
                IntPtr.Zero,
                out error);
            GC.KeepAlive(callback);
            if (result != 0)
            {
                Exception exception = SqliteError(
                    database,
                    result,
                    error);
                if (error != IntPtr.Zero)
                {
                    SqliteMethods.sqlite3_free(error);
                }
                throw exception;
            }
        }

        private static Exception SqliteError(
            IntPtr database,
            int result,
            IntPtr error)
        {
            string message = error != IntPtr.Zero
                ? PointerString(error)
                : database != IntPtr.Zero
                    ? PointerString(
                        SqliteMethods.sqlite3_errmsg(database))
                    : "database unavailable";
            return new InvalidOperationException(
                "SQLCipher error "
                + result.ToString(CultureInfo.InvariantCulture)
                + ": "
                + message);
        }

        private static void PublishSnapshot(
            SaveSnapshot snapshot,
            SaveSlot slot,
            DateTime nowUtc,
            bool ruleChanged)
        {
            Dictionary<int, BossState> next =
                new Dictionary<int, BossState>();
            foreach (int bossId in BossIds)
            {
                BossRow row;
                bool rowPresent = snapshot.Rows.TryGetValue(
                    bossId,
                    out row);
                BossState state = ComputeState(
                    bossId,
                    rowPresent ? row : null,
                    nowUtc,
                    String.Join(",", snapshot.LoadedFiles.ToArray()));
                next[bossId] = state;

                BossState previous;
                bool hadPrevious = _states.TryGetValue(
                    bossId,
                    out previous);
                if (!hadPrevious
                    || previous.RowPresent != state.RowPresent
                    || previous.RespawnType != state.RespawnType
                    || previous.DestroyRaw != state.DestroyRaw
                    || previous.DestroyTimeNormalized
                        != state.DestroyTimeNormalized
                    || !String.Equals(
                        previous.DestroyTimeStatus,
                        state.DestroyTimeStatus,
                        StringComparison.Ordinal)
                    || previous.DestroyUnixSeconds
                        != state.DestroyUnixSeconds)
                {
                    AppendChange(
                        "BOSS_RESPAWN_ROW_CHANGED",
                        StateChangeText(previous, state, hadPrevious));
                }
                if (hadPrevious
                    && previous.Available != state.Available)
                {
                    AppendChange(
                        "BOSS_AVAILABILITY_CHANGED",
                        StateChangeText(previous, state, true));
                }
                AppendSnapshotRow(state, slot.Signature, nowUtc);
            }
            _states = next;

            AppendChange(
                "SAVE_SNAPSHOT_LOADED",
                "slot=" + slot.Summary
                + "; authoritative_database="
                + String.Join(",", snapshot.LoadedFiles.ToArray())
                + "; attempted_databases="
                + String.Join(",", snapshot.AttemptedFiles.ToArray())
                + "; rows="
                + snapshot.Rows.Count.ToString(
                    CultureInfo.InvariantCulture)
                + "; rule_changed="
                + ruleChanged.ToString());
        }

        private static BossState ComputeState(
            int bossId,
            BossRow row,
            DateTime nowUtc,
            string sourceDatabase)
        {
            if (row == null)
            {
                return new BossState
                {
                    BossId = bossId,
                    UidName = BossUidNames[bossId],
                    RowPresent = false,
                    RespawnType = 0,
                    DestroyRaw = 0,
                    DestroyTimeNormalized = true,
                    DestroyTimeStatus = "row_absent",
                    DestroyUnixSeconds = 0,
                    DestroyTimeUtc = DateTime.MinValue,
                    NextAvailableUtc = DateTime.MinValue,
                    AvailabilityKnown = true,
                    Available = true,
                    SourceDatabase = sourceDatabase
                };
            }

            DateTime nextAvailableUtc = DateTime.MinValue;
            bool availabilityKnown = row.DestroyTimeNormalized;
            bool available = false;
            if (availabilityKnown)
            {
                nextAvailableUtc = _rule.NextAvailableUtc(
                    row.DestroyTimeUtc);
                available = nowUtc >= nextAvailableUtc;
            }
            return new BossState
            {
                BossId = bossId,
                UidName = BossUidNames[bossId],
                RowPresent = true,
                RespawnType = row.RespawnType,
                DestroyRaw = row.DestroyRaw,
                DestroyTimeNormalized = row.DestroyTimeNormalized,
                DestroyTimeStatus = row.DestroyTimeStatus,
                DestroyUnixSeconds = row.DestroyUnixSeconds,
                DestroyTimeUtc = row.DestroyTimeUtc,
                NextAvailableUtc = nextAvailableUtc,
                AvailabilityKnown = availabilityKnown,
                Available = available,
                SourceDatabase = sourceDatabase
            };
        }

        private static void RefreshTimeBasedAvailability(
            DateTime nowUtc)
        {
            foreach (int bossId in BossIds)
            {
                BossState state;
                if (!_states.TryGetValue(bossId, out state)
                    || !state.RowPresent
                    || !state.DestroyTimeNormalized)
                {
                    continue;
                }
                DateTime next = _rule.NextAvailableUtc(
                    state.DestroyTimeUtc);
                bool available = nowUtc >= next;
                if (state.Available != available
                    || state.NextAvailableUtc != next)
                {
                    BossState previous = state.Clone();
                    state.NextAvailableUtc = next;
                    state.Available = available;
                    _states[bossId] = state;
                    AppendChange(
                        "BOSS_AVAILABILITY_CHANGED",
                        StateChangeText(previous, state, true));
                }
            }
        }

        private static string StateChangeText(
            BossState previous,
            BossState current,
            bool hadPrevious)
        {
            return String.Format(
                CultureInfo.InvariantCulture,
                "boss_id={0}; uid_name={1}; previous={2}; current={3}",
                current.BossId,
                current.UidName,
                hadPrevious && previous != null
                    ? previous.Describe()
                    : "none",
                current.Describe());
        }

        private static void AppendSnapshotRow(
            BossState state,
            string slotSignature,
            DateTime nowUtc)
        {
            EnsureSnapshotHeader();
            long remaining = 0;
            if (state.RowPresent
                && state.AvailabilityKnown
                && !state.Available)
            {
                remaining = (long)Math.Ceiling(
                    (state.NextAvailableUtc - nowUtc).TotalSeconds);
                if (remaining < 0)
                {
                    remaining = 0;
                }
            }

            string[] values =
            {
                nowUtc.ToString("O", CultureInfo.InvariantCulture),
                state.BossId.ToString(CultureInfo.InvariantCulture),
                state.UidName,
                state.RowPresent.ToString(),
                state.RespawnType.ToString(CultureInfo.InvariantCulture),
                state.RowPresent
                    ? state.DestroyRaw.ToString(
                        CultureInfo.InvariantCulture)
                    : String.Empty,
                state.DestroyTimeStatus,
                state.RowPresent
                    && state.DestroyTimeNormalized
                    ? state.DestroyUnixSeconds.ToString(
                        CultureInfo.InvariantCulture)
                    : String.Empty,
                state.RowPresent
                    && state.DestroyTimeNormalized
                    ? state.DestroyTimeUtc.ToString(
                        "O",
                        CultureInfo.InvariantCulture)
                    : String.Empty,
                state.RowPresent
                    && state.DestroyTimeNormalized
                    ? state.NextAvailableUtc.ToString(
                        "O",
                        CultureInfo.InvariantCulture)
                    : String.Empty,
                remaining.ToString(CultureInfo.InvariantCulture),
                state.AvailabilityKnown.ToString(),
                state.Available.ToString(),
                _rule.Type,
                _rule.Source,
                state.SourceDatabase,
                slotSignature
            };
            AppendLine(
                _snapshotLogPath,
                JoinTsv(values));
        }

        private static void EnsureSnapshotHeader()
        {
            if (_snapshotHeaderReady)
            {
                return;
            }
            string[] columns =
            {
                "utc", "boss_id", "uid_name", "row_present",
                "respawn_type", "destroy_raw", "destroy_time_status",
                "destroy_unix", "destroy_utc", "next_available_utc",
                "remaining_seconds", "availability_known", "available",
                "rule_type", "rule_source", "source_database",
                "slot_signature"
            };
            AppendLine(
                _snapshotLogPath,
                JoinTsv(columns));
            _snapshotHeaderReady = true;
        }

        private static void WriteStatus(
            DateTime nowUtc,
            string stateText)
        {
            StringBuilder text = new StringBuilder();
            text.AppendLine(
                "DragonSwordWorldDataProbe boss save monitor");
            text.AppendLine("version=" + Version);
            text.AppendLine("state=" + stateText);
            text.AppendLine(
                "process_id=" + _processId.ToString(
                    CultureInfo.InvariantCulture));
            text.AppendLine(
                "sqlcipher=" + (_sqlcipherPath ?? "none"));
            text.AppendLine(
                "catalog=" + (_catalogPath ?? "none"));
            text.AppendLine(
                "key_rva=" + (_cachedDetectedRva == 0
                    ? "unresolved"
                    : "0x" + _cachedDetectedRva.ToString(
                        "X",
                        CultureInfo.InvariantCulture)));
            text.AppendLine(
                "selected_slot=" + (_lastSlotSummary ?? "none"));
            text.AppendLine(
                "slot_signature=" + (_lastSlotSignature ?? "none"));
            text.AppendLine(
                "respawn_rule=" + _rule.Describe());
            text.AppendLine(
                "last_database_read_utc="
                + (_lastDatabaseReadUtc == DateTime.MinValue
                    ? "none"
                    : _lastDatabaseReadUtc.ToString(
                        "O",
                        CultureInfo.InvariantCulture)));
            text.AppendLine(
                "last_error=" + (_lastError ?? "none"));

            foreach (int bossId in BossIds)
            {
                BossState boss;
                if (!_states.TryGetValue(bossId, out boss))
                {
                    text.AppendLine(
                        "boss="
                        + bossId.ToString(CultureInfo.InvariantCulture)
                        + " status=unknown");
                    continue;
                }
                long remaining = 0;
                if (boss.RowPresent
                    && boss.AvailabilityKnown
                    && !boss.Available)
                {
                    remaining = (long)Math.Ceiling(
                        (boss.NextAvailableUtc - nowUtc).TotalSeconds);
                    if (remaining < 0)
                    {
                        remaining = 0;
                    }
                }
                text.AppendLine(String.Format(
                    CultureInfo.InvariantCulture,
                    "boss={0} uid_name={1} row_present={2} respawn_type={3} destroy_raw={4} destroy_time_status={5} destroy_unix={6} destroy_utc={7} next_utc={8} remaining_seconds={9} availability_known={10} available={11}",
                    boss.BossId,
                    boss.UidName,
                    boss.RowPresent,
                    boss.RespawnType,
                    boss.RowPresent ? boss.DestroyRaw : 0,
                    boss.DestroyTimeStatus,
                    boss.RowPresent && boss.DestroyTimeNormalized
                        ? boss.DestroyUnixSeconds
                        : 0,
                    boss.RowPresent
                        && boss.DestroyTimeNormalized
                        ? boss.DestroyTimeUtc.ToString(
                            "O",
                            CultureInfo.InvariantCulture)
                        : "none",
                    boss.RowPresent
                        && boss.DestroyTimeNormalized
                        ? boss.NextAvailableUtc.ToString(
                            "O",
                            CultureInfo.InvariantCulture)
                        : "none",
                    remaining,
                    boss.AvailabilityKnown,
                    boss.Available));
            }
            text.AppendLine(
                "utc=" + nowUtc.ToString(
                    "O",
                    CultureInfo.InvariantCulture));

            WriteAtomic(_statusPath, text.ToString());
            _lastStatusWriteUtc = nowUtc;
        }

        private static void AppendChange(
            string eventName,
            string detail)
        {
            if (String.IsNullOrWhiteSpace(_changeLogPath))
            {
                return;
            }
            string line = String.Format(
                CultureInfo.InvariantCulture,
                "[{0}] [{1}] {2}",
                DateTime.UtcNow.ToString(
                    "O",
                    CultureInfo.InvariantCulture),
                SafeInline(eventName),
                SafeInline(detail));
            AppendLine(_changeLogPath, line);
        }

        private static void AppendLine(
            string path,
            string line)
        {
            EnsureParentDirectory(path);
            File.AppendAllText(
                path,
                line + Environment.NewLine,
                Utf8NoBom);
        }

        private static void WriteAtomic(
            string path,
            string text)
        {
            EnsureParentDirectory(path);
            string temporary = path + ".tmp";
            File.WriteAllText(temporary, text, Utf8NoBom);
            try
            {
                if (File.Exists(path))
                {
                    try
                    {
                        File.Replace(temporary, path, null);
                        return;
                    }
                    catch
                    {
                        File.Delete(path);
                    }
                }
                File.Move(temporary, path);
            }
            catch
            {
                File.WriteAllText(path, text, Utf8NoBom);
                try
                {
                    File.Delete(temporary);
                }
                catch
                {
                }
            }
        }

        private static void EnsureParentDirectory(string path)
        {
            string parent = Path.GetDirectoryName(path);
            if (!String.IsNullOrWhiteSpace(parent))
            {
                Directory.CreateDirectory(parent);
            }
        }

        private static string JoinTsv(string[] values)
        {
            string[] sanitized = new string[values.Length];
            for (int index = 0; index < values.Length; index++)
            {
                sanitized[index] = (values[index] ?? String.Empty)
                    .Replace('\t', ' ')
                    .Replace('\r', ' ')
                    .Replace('\n', ' ');
            }
            return String.Join("\t", sanitized);
        }

        private static string SafeInline(string value)
        {
            string text = value ?? String.Empty;
            text = text.Replace('\t', ' ')
                .Replace('\r', ' ')
                .Replace('\n', ' ')
                .Replace('"', '\'');
            if (text.Length > 1600)
            {
                text = text.Substring(0, 1600) + "...";
            }
            return text;
        }

        private static byte[] Utf8(string value)
        {
            return Encoding.UTF8.GetBytes(value + "\0");
        }

        private static string PointerString(IntPtr pointer)
        {
            return pointer == IntPtr.Zero
                ? String.Empty
                : Marshal.PtrToStringAnsi(pointer)
                    ?? String.Empty;
        }

        private sealed class BossCatalogKey
        {
            public int BossId;
            public string UidName;
            public string Uid;
            public string SectionUid;
            public string GroupId;
            public string SwitchWeekId;
            public string LevelCid;
            public string WorldMapSectionId;
        }

        private sealed class LogicalTableSnapshot
        {
            public string Name;
            public List<string> Columns;
            public List<string> Rows;
            public string Hash;
            public int CharacterCount;
            public bool Truncated;
        }

        private sealed class SaveSlot
        {
            public string Stem;
            public List<string> MainFiles;
            public string Signature;
            public string Summary;
        }

        private sealed class SaveSnapshot
        {
            public Dictionary<int, BossRow> Rows;
            public List<string> LoadedFiles;
            public List<string> AttemptedFiles;
        }

        private sealed class BossRow
        {
            public int BossId;
            public int RespawnType;
            public long DestroyRaw;
            public bool DestroyTimeNormalized;
            public string DestroyTimeStatus;
            public long DestroyUnixSeconds;
            public DateTime DestroyTimeUtc;
            public string SourceDatabase;
        }

        private sealed class BossState
        {
            public int BossId;
            public string UidName;
            public bool RowPresent;
            public int RespawnType;
            public long DestroyRaw;
            public bool DestroyTimeNormalized;
            public string DestroyTimeStatus;
            public long DestroyUnixSeconds;
            public DateTime DestroyTimeUtc;
            public DateTime NextAvailableUtc;
            public bool AvailabilityKnown;
            public bool Available;
            public string SourceDatabase;

            public BossState Clone()
            {
                return (BossState)MemberwiseClone();
            }

            public string Describe()
            {
                return String.Format(
                    CultureInfo.InvariantCulture,
                    "row_present={0},respawn_type={1},destroy_raw={2},destroy_time_status={3},destroy_unix={4},destroy_utc={5},next_utc={6},availability_known={7},available={8}",
                    RowPresent,
                    RespawnType,
                    DestroyRaw,
                    DestroyTimeStatus,
                    DestroyUnixSeconds,
                    RowPresent && DestroyTimeNormalized
                        ? DestroyTimeUtc.ToString(
                            "O",
                            CultureInfo.InvariantCulture)
                        : "none",
                    RowPresent && DestroyTimeNormalized
                        ? NextAvailableUtc.ToString(
                            "O",
                            CultureInfo.InvariantCulture)
                        : "none",
                    AvailabilityKnown,
                    Available);
            }
        }

        private sealed class RespawnRule
        {
            public readonly string Type;
            public readonly int RelativeMinutes;
            public readonly int LocalHour;
            public readonly int LocalMinute;
            public readonly string Source;

            private RespawnRule(
                string type,
                int relativeMinutes,
                int localHour,
                int localMinute,
                string source)
            {
                Type = type;
                RelativeMinutes = relativeMinutes;
                LocalHour = localHour;
                LocalMinute = localMinute;
                Source = source;
            }

            public string Signature
            {
                get
                {
                    return Type
                        + "|" + RelativeMinutes.ToString(
                            CultureInfo.InvariantCulture)
                        + "|" + LocalHour.ToString(
                            CultureInfo.InvariantCulture)
                        + "|" + LocalMinute.ToString(
                            CultureInfo.InvariantCulture)
                        + "|" + Source;
                }
            }

            public static RespawnRule Relative(
                int minutes,
                string source)
            {
                return new RespawnRule(
                    "RELATIVETIME",
                    minutes,
                    0,
                    0,
                    source);
            }

            public static RespawnRule Daily(
                int hour,
                int minute,
                string source)
            {
                return new RespawnRule(
                    "DAILY",
                    0,
                    hour,
                    minute,
                    source);
            }

            public DateTime NextAvailableUtc(DateTime destroyUtc)
            {
                if (Type == "RELATIVETIME")
                {
                    return destroyUtc.AddMinutes(
                        RelativeMinutes);
                }

                DateTime localDestroy = destroyUtc.ToLocalTime();
                DateTime reset = new DateTime(
                    localDestroy.Year,
                    localDestroy.Month,
                    localDestroy.Day,
                    LocalHour,
                    LocalMinute,
                    0,
                    DateTimeKind.Local);
                if (reset <= localDestroy)
                {
                    reset = reset.AddDays(1);
                }
                return reset.ToUniversalTime();
            }

            public string Describe()
            {
                if (Type == "RELATIVETIME")
                {
                    return String.Format(
                        CultureInfo.InvariantCulture,
                        "type={0}; minutes={1}; source={2}",
                        Type,
                        RelativeMinutes,
                        Source);
                }
                return String.Format(
                    CultureInfo.InvariantCulture,
                    "type={0}; local_reset={1:00}:{2:00}; source={3}",
                    Type,
                    LocalHour,
                    LocalMinute,
                    Source);
            }
        }

        private static class NativeMethods
        {
            [DllImport("kernel32.dll", SetLastError = true)]
            internal static extern IntPtr OpenProcess(
                uint desiredAccess,
                bool inheritHandle,
                int processId);

            [DllImport("kernel32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool ReadProcessMemory(
                IntPtr process,
                IntPtr baseAddress,
                [Out] byte[] buffer,
                IntPtr size,
                out IntPtr numberOfBytesRead);

            [DllImport("kernel32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool CloseHandle(IntPtr handle);

            [DllImport(
                "kernel32.dll",
                CharSet = CharSet.Unicode,
                SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            internal static extern bool SetDllDirectory(
                string pathName);
        }

        private static class SqliteMethods
        {
            [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
            internal delegate int ExecCallback(
                IntPtr context,
                int count,
                IntPtr values,
                IntPtr names);

            [DllImport(
                "e_sqlcipher",
                CallingConvention = CallingConvention.Cdecl)]
            internal static extern int sqlite3_open_v2(
                byte[] filename,
                out IntPtr database,
                int flags,
                IntPtr virtualFileSystem);

            [DllImport(
                "e_sqlcipher",
                CallingConvention = CallingConvention.Cdecl)]
            internal static extern int sqlite3_close_v2(
                IntPtr database);

            [DllImport(
                "e_sqlcipher",
                CallingConvention = CallingConvention.Cdecl)]
            internal static extern int sqlite3_exec(
                IntPtr database,
                byte[] sql,
                ExecCallback callback,
                IntPtr context,
                out IntPtr errorMessage);

            [DllImport(
                "e_sqlcipher",
                CallingConvention = CallingConvention.Cdecl)]
            internal static extern IntPtr sqlite3_errmsg(
                IntPtr database);

            [DllImport(
                "e_sqlcipher",
                CallingConvention = CallingConvention.Cdecl)]
            internal static extern void sqlite3_free(
                IntPtr pointer);
        }
    }
}
