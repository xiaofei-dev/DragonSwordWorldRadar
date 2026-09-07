using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using Microsoft.Win32;

namespace DragonSwordNativeAutoPickup.Installer
{
    internal sealed class AutoPickupInstallerPlan110
    {
        internal string IdentityToken { get; set; }
        internal string ExistingLayout { get; set; }
        internal string ActionDescription { get; set; }
        internal bool ConvertsUE4SS { get; set; }
        internal bool BootstrapsUE4SS { get; set; }
        internal bool UpdatesExistingAutoPickup { get; set; }
    }

    internal sealed class AutoPickupInstallerResult110
    {
        internal string LayoutDescription { get; set; }
        internal string ToggleHotkey { get; set; }
        internal string InteractionKeyFallback { get; set; }
        internal int RangeMultiplierInstalled { get; set; }
        internal string BackupDirectory { get; set; }
    }

    internal sealed class AutoPickupInstallationState110
    {
        internal bool IsInstalled { get; set; }
        internal bool IsOwned { get; set; }
        internal bool CanInstall { get; set; }
        internal bool CanUpgrade { get; set; }
        internal bool CanRepair { get; set; }
        internal bool CanUninstall { get; set; }
        internal string InstalledVersion { get; set; }
        internal string IdentityToken { get; set; }
        internal string StatusDescription { get; set; }
        internal string InstalledModDirectory { get; set; }
        internal string ToggleHotkey { get; set; }
        internal string InteractionKeyFallback { get; set; }
        internal InstallerEngine.OptionalRangePakState RangeState { get; set; }
    }

    internal sealed class AutoPickupUninstallResult110
    {
        internal bool RemovedMod { get; set; }
        internal bool RemovedRangePak { get; set; }
    }

    // Auto Pickup 1.3.1 intentionally supports one ABI only. Any other active
    // UE4SS layout is backed up, removed, and converted after explicit consent.
    internal static class InstallerEngine110
    {
        private const string ProductVersion = "1.3.1";
        private const string GameFileName = "DSClient-Win64-Shipping.exe";
        private const string ModName = "DragonSwordNativeAutoPickup";
        private const string OwnershipSchema = "2";
        private const string OwnershipId = "8F4282F9-25C4-4EFC-A150-1C4A812C85B4";
        private const string ExperimentalUE4SSHash =
            "F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1";
        private const string ExperimentalDwmapiHash =
            "30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B";
        private const string ExperimentalUsmapHash =
            "0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47";
        private const string ExperimentalSettingsHash =
            "4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74";
        private const string ExperimentalUsmapName = "DS-5.3.2-0+UE5-1c1a1497.usmap";
        private const string ObsoleteRangePakName = "DS_PickupRangeX3Canary_P.pak";
        private const string LegacyEnabledHash =
            "6B86B273FF34FCE19D6B804EFF5A3F5747ADA4EAA22F1D49C01E52DDB7875B4B";
        private const string NoticesHash =
            "8354A516D861629D32ED824D13ADD836913CEB6D8CDF2380DC19C7A5803AFE12";
        private const string RuntimeZipResource = "Payload.ExperimentalUE4SSRuntime.zip";
        private const string PluginResource = "Payload.ExperimentalPlugin.dll";
        private const string ConfigResource = "Payload.DefaultConfig.ini";
        private const string LuaResource = "Payload.Main.lua";
        private const string NoticesResource = "Payload.ThirdPartyNotices.txt";
        private const string RangeX3Resource = "Payload.PickupRangeX3.pak";
        private const string RangeX5Resource = "Payload.PickupRangeX5.pak";
        private const string RangeX10Resource = "Payload.PickupRangeX10.pak";
        private const string RangeX15Resource = "Payload.PickupRangeX15.pak";
        private const string RangeX20Resource = "Payload.PickupRangeX20.pak";
        private const string RangeX3Hash = "6BB99A1E35C06EB0284370B9D7BD2F34E90CB6DCA7479CF10A477C68EA0103E8";
        private const string RangeX5Hash = "DB9E129D8F8FCCA025864EC908C13C70F950AD779C37CF13A41164476587CECD";
        private const string RangeX10Hash = "6A1ADB7592BA0C70A17984DB3AC01348086AABE196F0FDAF914B3F52C7A395F1";
        private const string RangeX15Hash = "F8330CEA2F127319887FD3718BC66E2825D39DFE21D635404FC7535C7FAA37EC";
        private const string RangeX20Hash = "81214319100646CD5663940CACE3AFA8F3523F5E319AB7C4AE8D935443FB93D2";

        private sealed class OwnedPayloadContract
        {
            internal string Version;
            internal string PluginHash;
            internal string LuaHash;
        }

        private sealed class Utf8TextDocument
        {
            internal bool HasBom;
            internal string Newline;
            internal bool EndsWithNewline;
            internal List<string> Lines = new List<string>();
        }

        private sealed class Context
        {
            internal string GameExecutable;
            internal string GameRoot;
            internal string Win64;
            internal string NestedUE4SS;
            internal string Mods;
            internal string ModsTxt;
            internal string PakDirectory;
            internal bool Bootstraps;
            internal bool Converts;
            internal string ExistingLayout;
            internal string MigrationModsTxt;
            internal List<string> MigrationMods = new List<string>();
        }

        private sealed class Snapshot
        {
            internal List<string> MigrationMods = new List<string>();
        }

        private sealed class RangeInfo
        {
            internal InstallerEngine.RangeSelection Selection;
            internal string FileName;
            internal string Resource;
            internal string Hash;
        }

        private static readonly RangeInfo[] Ranges =
        {
            new RangeInfo { Selection = InstallerEngine.RangeSelection.X3, FileName = "DS_PickupRangeX3_P.pak", Resource = RangeX3Resource, Hash = RangeX3Hash },
            new RangeInfo { Selection = InstallerEngine.RangeSelection.X5, FileName = "DS_PickupRangeX5_P.pak", Resource = RangeX5Resource, Hash = RangeX5Hash },
            new RangeInfo { Selection = InstallerEngine.RangeSelection.X10, FileName = "DS_PickupRangeX10_P.pak", Resource = RangeX10Resource, Hash = RangeX10Hash },
            new RangeInfo { Selection = InstallerEngine.RangeSelection.X15, FileName = "DS_PickupRangeX15_P.pak", Resource = RangeX15Resource, Hash = RangeX15Hash },
            new RangeInfo { Selection = InstallerEngine.RangeSelection.X20, FileName = "DS_PickupRangeX20_P.pak", Resource = RangeX20Resource, Hash = RangeX20Hash }
        };

        internal static string TryDiscoverGameExecutable()
        {
            try
            {
                foreach (var steamRoot in DiscoverSteamRoots())
                {
                    foreach (var library in DiscoverSteamLibraries(steamRoot))
                    {
                        var manifest = Path.Combine(library, "steamapps", "appmanifest_4570720.acf");
                        if (!File.Exists(manifest))
                        {
                            continue;
                        }

                        var text = File.ReadAllText(manifest, new UTF8Encoding(false, true));
                        var match = Regex.Match(
                            text,
                            "\\\"installdir\\\"\\s*\\\"([^\\\"]+)\\\"",
                            RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
                        if (!match.Success)
                        {
                            continue;
                        }

                        var candidate = Path.Combine(
                            library,
                            "steamapps",
                            "common",
                            match.Groups[1].Value.Replace("\\\\", "\\"),
                            "DS",
                            "Binaries",
                            "Win64",
                            GameFileName);
                        if (File.Exists(candidate))
                        {
                            return Path.GetFullPath(candidate);
                        }
                    }
                }
            }
            catch
            {
                // Discovery is optional. Manual file selection remains authoritative.
            }

            return string.Empty;
        }

        internal static InstallerEngine.OptionalRangePakState InspectOptionalRangePak(string gameExecutable)
        {
            try
            {
                var executable = ValidateGameExecutable(gameExecutable, false);
                var win64 = Path.GetDirectoryName(executable);
                var pak = Path.GetFullPath(Path.Combine(win64, "..", "..", "Content", "Paks", "~mods"));
                var found = new List<InstallerEngine.RangeSelection>();
                var obsolete = Path.Combine(pak, ObsoleteRangePakName);
                if (File.Exists(obsolete))
                {
                    found.Add(InstallerEngine.RangeSelection.X3);
                }
                foreach (var range in Ranges)
                {
                    var path = Path.Combine(pak, range.FileName);
                    if (!File.Exists(path)) continue;
                    found.Add(range.Selection);
                }
                if (found.Count == 0) return InstallerEngine.OptionalRangePakState.Absent;
                if (found.Count > 1) return InstallerEngine.OptionalRangePakState.MultipleApproved;
                if (found[0] == InstallerEngine.RangeSelection.X3) return InstallerEngine.OptionalRangePakState.ApprovedX3;
                if (found[0] == InstallerEngine.RangeSelection.X5) return InstallerEngine.OptionalRangePakState.ApprovedX5;
                if (found[0] == InstallerEngine.RangeSelection.X10) return InstallerEngine.OptionalRangePakState.ApprovedX10;
                if (found[0] == InstallerEngine.RangeSelection.X15) return InstallerEngine.OptionalRangePakState.ApprovedX15;
                return InstallerEngine.OptionalRangePakState.ApprovedX20;
            }
            catch { return InstallerEngine.OptionalRangePakState.Absent; }
        }

        internal static AutoPickupInstallationState110 InspectInstallationState(string selectedGameExecutable)
        {
            var executable = ValidateGameExecutable(selectedGameExecutable, false);
            var context = Detect(executable);
            ValidateTrees(context);
            ValidateMigrationSources(context);
            var rangeState = InspectOptionalRangePak(executable);
            var candidates = ExistingModCandidates(context).Where(Directory.Exists).ToList();
            var owned = candidates.Where(path => IsOwnedModDirectory(path)).ToList();
            var unknown = candidates.Where(path => !IsOwnedModDirectory(path)).ToList();

            var state = new AutoPickupInstallationState110
            {
                IsInstalled = candidates.Count != 0,
                IsOwned = candidates.Count == 1 && owned.Count == 1 && unknown.Count == 0,
                CanInstall = candidates.Count == 0,
                CanUninstall = candidates.Count == 1 && owned.Count == 1 && unknown.Count == 0,
                InstalledVersion = string.Empty,
                RangeState = rangeState,
                ToggleHotkey = "F9",
                InteractionKeyFallback = "F",
                InstalledModDirectory = owned.Count == 1 ? owned[0] : string.Empty
            };

            if (state.IsOwned)
            {
                state.InstalledVersion = ReadOwnedInstalledVersion(state.InstalledModDirectory);
                state.CanRepair = string.Equals(
                    state.InstalledVersion, ProductVersion, StringComparison.Ordinal);
                state.CanUpgrade = !state.CanRepair;
                ReadInstalledKeys(Path.Combine(state.InstalledModDirectory, "config.ini"),
                    out var hotkey, out var fallback);
                state.ToggleHotkey = hotkey;
                state.InteractionKeyFallback = fallback;
                state.StatusDescription = state.CanRepair
                    ? "Auto Pickup " + ProductVersion +
                      " is installed and strictly owned. Repair is available."
                    : "Auto Pickup " + state.InstalledVersion +
                      " is installed and strictly owned. Upgrade to " + ProductVersion +
                      " is available.";
            }
            else if (state.IsInstalled)
            {
                state.StatusDescription = candidates.Count > 1
                    ? "Multiple Auto Pickup directories were found. Setup will not modify an ambiguous installation."
                    : "An Auto Pickup directory exists but strict ownership could not be verified.";
            }
            else
            {
                state.StatusDescription = "Auto Pickup is not installed.";
            }

            state.IdentityToken = BuildInstallationStateIdentity(context, state);
            return state;
        }

        internal static AutoPickupInstallerPlan110 Inspect(
            string selectedGameExecutable,
            string requestedHotkey,
            string requestedInteractionKeyFallback,
            InstallerEngine.RangeSelection rangeSelection)
        {
            EnsureGameIsClosed();
            var hotkey = NormalizeKey(requestedHotkey, "F9", false);
            var fallback = NormalizeKey(requestedInteractionKeyFallback, "F", true);
            var executable = ValidateGameExecutable(selectedGameExecutable, true);
            HashFile(executable);
            ValidateResources();
            var context = Detect(executable);
            ValidateTrees(context);
            var state = InspectInstallationState(executable);
            EnsureMutableInstallationState(state);
            if (state.IsOwned)
            {
                hotkey = state.ToggleHotkey;
                fallback = state.InteractionKeyFallback;
            }
            return BuildPlan(context, state, hotkey, fallback, rangeSelection);
        }

        internal static AutoPickupInstallerResult110 InstallConfirmed(
            string selectedGameExecutable,
            string requestedHotkey,
            string requestedInteractionKeyFallback,
            InstallerEngine.RangeSelection rangeSelection,
            string confirmedIdentity)
        {
            if (string.IsNullOrWhiteSpace(confirmedIdentity))
                throw new InvalidOperationException("The installation plan was not confirmed.");
            return InstallCore(selectedGameExecutable, requestedHotkey, requestedInteractionKeyFallback,
                rangeSelection, confirmedIdentity);
        }

        internal static AutoPickupUninstallResult110 UninstallConfirmed(
            string selectedGameExecutable,
            string confirmedIdentity)
        {
            EnsureGameIsClosed();
            if (string.IsNullOrWhiteSpace(confirmedIdentity))
                throw new InvalidOperationException("The uninstall plan was not confirmed.");

            var executable = ValidateGameExecutable(selectedGameExecutable, true);
            var context = Detect(executable);
            ValidateTrees(context);
            var state = InspectInstallationState(executable);
            if (!string.Equals(state.IdentityToken, confirmedIdentity, StringComparison.Ordinal))
                throw new InvalidOperationException("The Auto Pickup installation changed after confirmation. Check it again.");
            if (!state.CanUninstall || !state.IsOwned)
                throw new InvalidOperationException(state.IsInstalled
                    ? "Setup cannot verify ownership of the existing Auto Pickup files, so uninstall was refused."
                    : "Auto Pickup is not installed by this installer.");

            var ownedRanges = ValidateExistingRangeFiles(context);
            var ownsLegacyCanary = ValidateLegacyRangeCanary(context);
            var journal = CreateTransactionJournalDirectory(context);
            using (var transaction = new AutoInstallTransaction110(context.Win64, context.GameRoot, journal, false))
            {
                try
                {
                    transaction.DeleteDirectoryTree(state.InstalledModDirectory, "Remove owned Auto Pickup installation");
                    foreach (var modsTxtPath in ExistingModsTxtCandidates(context).Where(File.Exists))
                    {
                        var updated = BuildModsTxtWithoutAutoPickup(modsTxtPath);
                        transaction.WriteBytes(modsTxtPath, updated, "Remove Auto Pickup from mods.txt");
                    }
                    foreach (var range in Ranges.Where(value => ownedRanges.Contains(value.Selection)))
                        transaction.DeleteFile(Path.Combine(context.PakDirectory, range.FileName),
                            "Remove owned Auto Pickup range PAK");
                    if (ownsLegacyCanary)
                        transaction.DeleteFile(Path.Combine(context.PakDirectory, ObsoleteRangePakName),
                            "Remove owned legacy Auto Pickup range PAK");

                    if (Directory.Exists(state.InstalledModDirectory))
                        throw new IOException("Auto Pickup directory remained after uninstall.");
                    foreach (var range in Ranges.Where(value => ownedRanges.Contains(value.Selection)))
                        if (File.Exists(Path.Combine(context.PakDirectory, range.FileName)))
                            throw new IOException("Owned range PAK remained after uninstall: " + range.FileName);
                    if (File.Exists(Path.Combine(context.PakDirectory, ObsoleteRangePakName)))
                        throw new IOException("Owned legacy range PAK remained after uninstall: " + ObsoleteRangePakName);
                    foreach (var modsTxtPath in ExistingModsTxtCandidates(context).Where(File.Exists))
                        if (ContainsModEntry(modsTxtPath))
                            throw new IOException("Auto Pickup remained enabled in mods.txt: " + modsTxtPath);

                    transaction.Commit(new[]
                    {
                        "DragonSword Native Auto Pickup " + ProductVersion + " uninstall",
                        "UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture),
                        "Removed optional range PAK: " + (ownedRanges.Count != 0 || ownsLegacyCanary)
                    });
                    return new AutoPickupUninstallResult110
                    {
                        RemovedMod = true,
                        RemovedRangePak = ownedRanges.Count != 0 || ownsLegacyCanary
                    };
                }
                catch (Exception uninstallError)
                {
                    try { transaction.Rollback(); }
                    catch (Exception rollbackError)
                    {
                        throw new InvalidOperationException("Uninstall failed and rollback was incomplete. " +
                            uninstallError.Message + " Rollback error: " + rollbackError.Message +
                            " Transaction journal: " + journal, uninstallError);
                    }
                    throw new InvalidOperationException("Uninstall failed; recorded changes were restored. " +
                        uninstallError.Message + " Transaction journal: " + journal, uninstallError);
                }
            }
        }

        private static AutoPickupInstallerResult110 InstallCore(
            string selectedGameExecutable,
            string requestedHotkey,
            string requestedInteractionKeyFallback,
            InstallerEngine.RangeSelection rangeSelection,
            string confirmedIdentity)
        {
            EnsureGameIsClosed();
            if (!Enum.IsDefined(typeof(InstallerEngine.RangeSelection), rangeSelection))
                throw new ArgumentOutOfRangeException("rangeSelection");
            var hotkey = NormalizeKey(requestedHotkey, "F9", false);
            var fallback = NormalizeKey(requestedInteractionKeyFallback, "F", true);
            var executable = ValidateGameExecutable(selectedGameExecutable, true);
            var gameHash = HashFile(executable);
            ValidateResources();
            var context = Detect(executable);
            ValidateTrees(context);
            var installationState = InspectInstallationState(executable);
            EnsureMutableInstallationState(installationState);
            if (installationState.IsOwned)
            {
                hotkey = installationState.ToggleHotkey;
                fallback = installationState.InteractionKeyFallback;
            }
            var plan = BuildPlan(context, installationState, hotkey, fallback, rangeSelection);
            if (!string.Equals(plan.IdentityToken, confirmedIdentity, StringComparison.Ordinal))
                throw new InvalidOperationException("The game or UE4SS layout changed after confirmation. Run the check again.");

            var existingRange = ValidateExistingRangeFiles(context);
            var ownsLegacyCanary = ValidateLegacyRangeCanary(context);

            var preservedConfig = installationState.IsOwned
                ? File.ReadAllBytes(Path.Combine(installationState.InstalledModDirectory, "config.ini"))
                : null;
            var config = preservedConfig ?? BuildConfig(ReadResource(ConfigResource), hotkey, fallback);
            var sourceModsTxt = string.IsNullOrEmpty(context.MigrationModsTxt) ? context.ModsTxt : context.MigrationModsTxt;
            var modsTxt = File.Exists(sourceModsTxt)
                ? BuildModsTxt(sourceModsTxt)
                : BuildModsTxt(ReadRuntimeEntry("ue4ss/Mods/mods.txt"));
            var retainBackup = context.Converts;
            var backupRoot = retainBackup
                ? CreateUniqueBackupDirectory(context)
                : CreateTransactionJournalDirectory(context);

            using (var transaction = new AutoInstallTransaction110(context.Win64, context.GameRoot, backupRoot, retainBackup))
            {
                try
                {
                    Snapshot snapshot = null;
                    if (context.Converts)
                    {
                        snapshot = CreateSnapshot(context, backupRoot);
                        RemoveOldUE4SS(context, transaction);
                    }
                    if (context.Converts || context.Bootstraps)
                        InstallPinnedRuntime(context, transaction);
                    MigrateMods(context, transaction, snapshot == null ? context.MigrationMods : snapshot.MigrationMods);

                    var mod = Path.Combine(context.Mods, ModName);
                    if (Directory.Exists(mod))
                        transaction.DeleteDirectoryTree(mod, "Replace owned Auto Pickup installation");
                    transaction.WriteBytes(Path.Combine(mod, "dlls", "main.dll"), ReadResource(PluginResource), "Install Auto Pickup native plugin");
                    transaction.WriteBytes(Path.Combine(mod, "config.ini"), config, "Install Auto Pickup configuration");
                    transaction.WriteBytes(Path.Combine(mod, "Scripts", "main.lua"), ReadResource(LuaResource), "Install passive Lua entry point");
                    transaction.WriteBytes(Path.Combine(mod, "THIRD_PARTY_NOTICES.txt"), ReadResource(NoticesResource), "Install notices");
                    transaction.WriteBytes(context.ModsTxt, modsTxt, "Enable Auto Pickup in controlling mods.txt");

                    foreach (var range in Ranges)
                    {
                        var path = Path.Combine(context.PakDirectory, range.FileName);
                        if (range.Selection == rangeSelection)
                            transaction.WriteBytes(path, ReadResource(range.Resource), "Install optional " + (int)range.Selection + "x range PAK");
                        else if (existingRange.Contains(range.Selection))
                            transaction.DeleteFile(path, "Remove unselected owned range PAK");
                    }
                    if (ownsLegacyCanary)
                        transaction.DeleteFile(Path.Combine(context.PakDirectory, ObsoleteRangePakName),
                            "Remove owned legacy range PAK");

                    VerifyPinnedRuntime(context);
                    VerifyInstalledMod(context, config, modsTxt, rangeSelection);
                    var record = BuildRecord(context, gameHash, hotkey, fallback, rangeSelection,
                        retainBackup ? backupRoot : "none (transaction journal removed after success)");
                    transaction.WriteBytes(Path.Combine(mod, "INSTALL-RECORD.txt"), Encoding.UTF8.GetBytes(record), "Write install record");
                    transaction.Commit(new[]
                    {
                        "DragonSword Native Auto Pickup " + ProductVersion,
                        "UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture),
                        "UE4SS converted: " + context.Converts,
                        "UE4SS bootstrapped: " + context.Bootstraps,
                        "UE4SS runtime: v3.0.1 Beta #0 g1c1a1497 ExperimentalNested",
                        "Toggle key: " + hotkey,
                        "Interaction fallback: " + fallback,
                        "Range multiplier: " + (int)rangeSelection + "x"
                    });
                    return new AutoPickupInstallerResult110
                    {
                        LayoutDescription = "pinned ExperimentalNested UE4SS g1c1a1497",
                        ToggleHotkey = hotkey,
                        InteractionKeyFallback = fallback,
                        RangeMultiplierInstalled = (int)rangeSelection,
                        BackupDirectory = retainBackup ? backupRoot : string.Empty
                    };
                }
                catch (Exception installError)
                {
                    try { transaction.Rollback(); }
                    catch (Exception rollbackError)
                    {
                        throw new InvalidOperationException("Installation failed and rollback was incomplete. " +
                            installError.Message + " Rollback error: " + rollbackError.Message +
                            (retainBackup ? " Backup directory: " : " Transaction journal: ") + backupRoot, installError);
                    }
                    throw new InvalidOperationException("Installation failed; recorded changes were restored. " +
                        installError.Message + (retainBackup ? " Backup directory: " : " Transaction journal: ") + backupRoot, installError);
                }
            }
        }

        private static Context Detect(string executable)
        {
            var win64 = Path.GetDirectoryName(executable);
            var nested = Path.Combine(win64, "ue4ss");
            var rootLoader = Path.Combine(win64, "UE4SS.dll");
            var nestedLoader = Path.Combine(nested, "UE4SS.dll");
            var proxy = Path.Combine(win64, "dwmapi.dll");
            var rootArtifacts = File.Exists(rootLoader) || Directory.Exists(Path.Combine(win64, "Mods")) ||
                Directory.GetFiles(win64, "UE4SS*", SearchOption.TopDirectoryOnly).Length != 0;
            var any = rootArtifacts || File.Exists(nestedLoader) || Directory.Exists(nested) || File.Exists(proxy);
            var exact = File.Exists(nestedLoader) && File.Exists(proxy) &&
                string.Equals(HashFile(nestedLoader), ExperimentalUE4SSHash, StringComparison.OrdinalIgnoreCase) &&
                string.Equals(HashFile(proxy), ExperimentalDwmapiHash, StringComparison.OrdinalIgnoreCase) &&
                !rootArtifacts;
            var context = new Context
            {
                GameExecutable = executable,
                Win64 = win64,
                GameRoot = Path.GetFullPath(Path.Combine(win64, "..", "..")),
                NestedUE4SS = nested,
                Mods = Path.Combine(nested, "Mods"),
                ModsTxt = Path.Combine(nested, "Mods", "mods.txt"),
                PakDirectory = Path.GetFullPath(Path.Combine(win64, "..", "..", "Content", "Paks", "~mods")),
                Bootstraps = !any,
                Converts = any && !exact,
                ExistingLayout = !any ? "UE4SS not installed" : exact ? "pinned ExperimentalNested g1c1a1497" : "different or mixed UE4SS layout"
            };
            var rootMods = Path.Combine(win64, "Mods");
            var nestedMods = Path.Combine(nested, "Mods");
            if (Directory.Exists(rootMods)) context.MigrationMods.Add(rootMods);
            if (Directory.Exists(nestedMods)) context.MigrationMods.Add(nestedMods);
            context.MigrationModsTxt = new[] { Path.Combine(nestedMods, "mods.txt"), Path.Combine(rootMods, "mods.txt") }.FirstOrDefault(File.Exists);
            return context;
        }

        private static void ValidateTrees(Context context)
        {
            AutoPathSafety110.Validate(context.GameRoot, context.Win64, "Win64 directory");
            foreach (var path in new[] { Path.Combine(context.Win64, "Mods"), context.NestedUE4SS })
                if (Directory.Exists(path)) AutoPathSafety110.ValidateTree(context.GameRoot, path, "existing UE4SS tree");
            AutoPathSafety110.Validate(context.GameRoot, context.PakDirectory, "range PAK directory");
            if (Directory.Exists(context.PakDirectory))
                AutoPathSafety110.ValidateTree(context.GameRoot, context.PakDirectory, "range PAK directory");
        }

        private static void ValidateMigrationSources(Context context)
        {
            if (!context.Converts) return;

            var modsTxtFiles = context.MigrationMods
                .Select(path => Path.Combine(path, "mods.txt"))
                .Where(File.Exists)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToList();
            if (modsTxtFiles.Count > 1)
            {
                var expected = HashFile(modsTxtFiles[0]);
                foreach (var candidate in modsTxtFiles.Skip(1))
                    if (!string.Equals(HashFile(candidate), expected, StringComparison.OrdinalIgnoreCase))
                        throw new InvalidOperationException(
                            "Both UE4SS layouts contain different mods.txt files. Resolve the migration collision before conversion.");
            }

            var files = new Dictionary<string, KeyValuePair<string, string>>(StringComparer.OrdinalIgnoreCase);
            foreach (var source in context.MigrationMods.Distinct(StringComparer.OrdinalIgnoreCase))
            {
                if (!Directory.Exists(source)) continue;
                foreach (var file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
                {
                    var relative = file.Substring(source.TrimEnd(Path.DirectorySeparatorChar).Length + 1);
                    if (string.Equals(relative, "mods.txt", StringComparison.OrdinalIgnoreCase) ||
                        relative.StartsWith(ModName + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                        continue;

                    var hash = HashFile(file);
                    if (files.TryGetValue(relative, out var previous))
                    {
                        if (!string.Equals(previous.Value, hash, StringComparison.OrdinalIgnoreCase))
                            throw new InvalidOperationException(
                                "UE4SS Mod migration collision for '" + relative + "': " +
                                previous.Key + " conflicts with " + file + ". Resolve it before conversion.");
                        continue;
                    }
                    files.Add(relative, new KeyValuePair<string, string>(file, hash));
                }
            }
        }

        private static AutoPickupInstallerPlan110 BuildPlan(Context context, AutoPickupInstallationState110 state,
            string hotkey, string fallback,
            InstallerEngine.RangeSelection range)
        {
            var fingerprint = string.Join("\n", new[]
            {
                context.GameExecutable + "=" + HashFile(context.GameExecutable),
                FileFingerprint(Path.Combine(context.Win64, "UE4SS.dll")),
                FileFingerprint(Path.Combine(context.NestedUE4SS, "UE4SS.dll")),
                FileFingerprint(Path.Combine(context.Win64, "dwmapi.dll")),
                state.IdentityToken, hotkey, fallback, ((int)range).ToString(CultureInfo.InvariantCulture)
            });
            return new AutoPickupInstallerPlan110
            {
                IdentityToken = HashBytes(Encoding.UTF8.GetBytes(fingerprint)),
                ExistingLayout = context.ExistingLayout,
                ConvertsUE4SS = context.Converts,
                BootstrapsUE4SS = context.Bootstraps,
                UpdatesExistingAutoPickup = state.IsOwned,
                ActionDescription = context.Converts
                    ? "Setup will create a complete verified backup, replace the active UE4SS layout with the tested ExperimentalNested runtime, migrate existing Mods and settings, and install Auto Pickup."
                    : context.Bootstraps
                        ? "Setup will install the tested ExperimentalNested UE4SS runtime and Auto Pickup."
                        : state.IsOwned
                            ? "Setup will keep the tested UE4SS runtime, preserve config.ini, and update the owned Auto Pickup files in place."
                            : "Setup will keep the tested UE4SS runtime and install Auto Pickup."
            };
        }

        private static void EnsureMutableInstallationState(AutoPickupInstallationState110 state)
        {
            if (state.IsInstalled && !state.IsOwned)
                throw new InvalidOperationException(
                    "An Auto Pickup directory exists, but strict ownership could not be verified. Setup will not overwrite unknown files.");
        }

        private static IEnumerable<string> ExistingModCandidates(Context context)
        {
            return new[]
            {
                Path.Combine(context.NestedUE4SS, "Mods", ModName),
                Path.Combine(context.Win64, "Mods", ModName)
            }.Distinct(StringComparer.OrdinalIgnoreCase);
        }

        private static IEnumerable<string> ExistingModsTxtCandidates(Context context)
        {
            return new[]
            {
                Path.Combine(context.NestedUE4SS, "Mods", "mods.txt"),
                Path.Combine(context.Win64, "Mods", "mods.txt")
            }.Distinct(StringComparer.OrdinalIgnoreCase);
        }

        private static bool IsOwnedModDirectory(string modDirectory)
        {
            try
            {
                var recordPath = Path.Combine(modDirectory, "INSTALL-RECORD.txt");
                var pluginPath = Path.Combine(modDirectory, "dlls", "main.dll");
                var configPath = Path.Combine(modDirectory, "config.ini");
                var luaPath = Path.Combine(modDirectory, "Scripts", "main.lua");
                var noticesPath = Path.Combine(modDirectory, "THIRD_PARTY_NOTICES.txt");
                if (!File.Exists(recordPath) || !File.Exists(pluginPath) || !File.Exists(configPath) ||
                    !File.Exists(luaPath) || !File.Exists(noticesPath))
                    return false;

                var record = File.ReadAllText(recordPath, new UTF8Encoding(false, true));
                var product = ReadUniqueRecordValue(record, "Product");
                var version = ReadUniqueRecordValue(record, "Version");
                if (!string.Equals(product, "DragonSword Native Auto Pickup", StringComparison.Ordinal) ||
                    string.IsNullOrEmpty(version))
                    return false;

                var ownershipSchema = ReadUniqueRecordValue(record, "Ownership Schema");
                if (string.Equals(ownershipSchema, OwnershipSchema, StringComparison.Ordinal))
                {
                    if (!MatchesRecordedOwnedPayload(
                        record,
                        HashFile(pluginPath),
                        HashFile(luaPath),
                        HashFile(noticesPath)))
                        return false;
                }
                else if (!string.IsNullOrEmpty(ownershipSchema) ||
                    !MatchesKnownOwnedPayload(version, HashFile(pluginPath), HashFile(luaPath)) ||
                    !string.Equals(HashFile(noticesPath), NoticesHash, StringComparison.OrdinalIgnoreCase))
                {
                    return false;
                }

                ReadInstalledKeys(configPath, out _, out _);
                var legacyEnabled = Path.Combine(modDirectory, "enabled.txt");
                if (File.Exists(legacyEnabled) &&
                    !string.Equals(HashFile(legacyEnabled), LegacyEnabledHash, StringComparison.OrdinalIgnoreCase))
                    return false;
                foreach (var file in Directory.GetFiles(modDirectory, "*", SearchOption.AllDirectories))
                {
                    var relative = file.Substring(modDirectory.TrimEnd(Path.DirectorySeparatorChar).Length + 1)
                        .Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
                    if (!IsAllowedOwnedModFile(relative))
                        return false;
                }
                foreach (var directory in Directory.GetDirectories(modDirectory, "*", SearchOption.AllDirectories))
                {
                    var relative = directory.Substring(modDirectory.TrimEnd(Path.DirectorySeparatorChar).Length + 1)
                        .Replace(Path.AltDirectorySeparatorChar, Path.DirectorySeparatorChar);
                    if (!IsAllowedOwnedModDirectory(relative))
                        return false;
                }
                return true;
            }
            catch
            {
                return false;
            }
        }

        private static string ReadUniqueRecordValue(string record, string name)
        {
            var matches = Regex.Matches(
                record,
                "(?m)^" + Regex.Escape(name) + ":\\s*([^\\r\\n]*?)\\s*$",
                RegexOptions.CultureInvariant);
            return matches.Count == 1 ? matches[0].Groups[1].Value : string.Empty;
        }

        private static string ReadOwnedInstalledVersion(string modDirectory)
        {
            var record = File.ReadAllText(
                Path.Combine(modDirectory, "INSTALL-RECORD.txt"),
                new UTF8Encoding(false, true));
            var version = ReadUniqueRecordValue(record, "Version");
            if (string.IsNullOrWhiteSpace(version))
                throw new InvalidDataException(
                    "The owned Auto Pickup install record has no unique Version value.");
            return version;
        }

        private static bool MatchesKnownOwnedPayload(string version, string pluginHash, string luaHash)
        {
            var contracts = new[]
            {
                new OwnedPayloadContract { Version = "1.0.0", PluginHash = "B25CD88FE953EA6F9893DC82E6760E3B14B3125F52B6B71A69F2C0CDD9422E37", LuaHash = "5918498D0CC7FBE8BC6ACE00BECF4263335FD6622F162C203290E5631B6F09CB" },
                new OwnedPayloadContract { Version = "1.0.0", PluginHash = "FE78B221EAA234D564F996F96800049BCD7B2A856265FFC209E86660AE3837A1", LuaHash = "5918498D0CC7FBE8BC6ACE00BECF4263335FD6622F162C203290E5631B6F09CB" },
                new OwnedPayloadContract { Version = "1.1.0", PluginHash = "7156299A2E4A21ADF57819A2663C0D82C91490D1CDDCD025C21968D86BB716E6", LuaHash = "D700DCFBB3B0E37EF04733172A2363DFD88C7708FEA960536AD79B553843518F" },
                new OwnedPayloadContract { Version = "1.1.0", PluginHash = "A5CE724E1F04F40A5346BDD8ECD9C7D2FF4854FCE33942E66149256D89A9A90D", LuaHash = "D700DCFBB3B0E37EF04733172A2363DFD88C7708FEA960536AD79B553843518F" },
                new OwnedPayloadContract { Version = "1.1.1", PluginHash = "3A2BA3B251910439E7625863EC4981A34AA692F62B136B51054349CDC4E29A92", LuaHash = "292CD605CD417986887C2DBBF32A6DB0D56B38AD360AFC2A618B02AEC29A27FC" },
                new OwnedPayloadContract { Version = "1.2.0", PluginHash = "9AFFCD7CD29F7CEFED93B51F7A8BC1533E49FB0E5959517DF424A15F3A19FAD6", LuaHash = "62E47E954D771C9933E5CB02E8E4A8B48EFCFF5B4A3A54190E6CD270063821BF" },
                new OwnedPayloadContract { Version = "1.3.0", PluginHash = "BF6418A9570CCD3FD8FF58E734C38666D8E591504532A5C50A25E8EA5956B188", LuaHash = "0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A" },
                new OwnedPayloadContract { Version = "1.3.0", PluginHash = "6F64645B47A5ADC59FAAB4F663E79C1B0681BF72125E1123D686977C0A7FA5F1", LuaHash = "0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A" },
                new OwnedPayloadContract { Version = "1.3.0", PluginHash = "4404872210939EE88D3A186CF37741D2282D0E9CB227E73226C820DC039F276F", LuaHash = "0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A" },
                new OwnedPayloadContract { Version = "1.3.0", PluginHash = "01A6E1358FBFE0B9DB35B4ECAAB2F1F08425265F47D6B1873872AA56D9E002AC", LuaHash = "0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A" },
                new OwnedPayloadContract { Version = "1.3.0", PluginHash = "09288CCB1E5342BCC6A6BA406AFFE4AB103EC1ACE25D62CB20AAAC2418D2101B", LuaHash = "0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A" },
                new OwnedPayloadContract { Version = "1.3.0", PluginHash = "38DA6C417B68F702AF6DAA069F80A348187B55D28C22227537278CEE988D87A1", LuaHash = "0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A" },
                new OwnedPayloadContract { Version = "1.3.0", PluginHash = "5490BCC44612689C23F9E735C711FC760B721D79DA1125BF694E8686B92CDA08", LuaHash = "0B4A52FBA7912C7921816C665AFD872DFAB3458DEE8DE20C6460F2EAC96A7D6A" },
                new OwnedPayloadContract { Version = ProductVersion, PluginHash = HashBytes(ReadResource(PluginResource)), LuaHash = HashBytes(ReadResource(LuaResource)) }
            };
            return contracts.Any(contract =>
                string.Equals(version, contract.Version, StringComparison.Ordinal) &&
                string.Equals(pluginHash, contract.PluginHash, StringComparison.OrdinalIgnoreCase) &&
                string.Equals(luaHash, contract.LuaHash, StringComparison.OrdinalIgnoreCase));
        }

        private static bool MatchesRecordedOwnedPayload(
            string record,
            string pluginHash,
            string luaHash,
            string noticesHash)
        {
            var ownershipId = ReadUniqueRecordValue(record, "Ownership ID");
            var recordedPluginHash = ReadUniqueRecordValue(record, "Plugin SHA-256");
            var recordedLuaHash = ReadUniqueRecordValue(record, "Lua SHA-256");
            var recordedNoticesHash = ReadUniqueRecordValue(record, "Notices SHA-256");
            return string.Equals(ownershipId, OwnershipId, StringComparison.Ordinal) &&
                IsSha256(recordedPluginHash) &&
                IsSha256(recordedLuaHash) &&
                IsSha256(recordedNoticesHash) &&
                string.Equals(pluginHash, recordedPluginHash, StringComparison.OrdinalIgnoreCase) &&
                string.Equals(luaHash, recordedLuaHash, StringComparison.OrdinalIgnoreCase) &&
                string.Equals(noticesHash, recordedNoticesHash, StringComparison.OrdinalIgnoreCase);
        }

        private static bool IsSha256(string value)
        {
            return !string.IsNullOrEmpty(value) &&
                Regex.IsMatch(value, "^[0-9A-F]{64}$", RegexOptions.CultureInvariant);
        }

        private static bool IsAllowedOwnedModFile(string relative)
        {
            foreach (var exact in new[]
            {
                "config.ini", "enabled.txt", "INSTALL-RECORD.txt", "THIRD_PARTY_NOTICES.txt",
                Path.Combine("dlls", "main.dll"), Path.Combine("Scripts", "main.lua")
            })
                if (string.Equals(relative, exact, StringComparison.OrdinalIgnoreCase)) return true;

            var logPrefix = Path.Combine("runtime", "logs") + Path.DirectorySeparatorChar;
            if (!relative.StartsWith(logPrefix, StringComparison.OrdinalIgnoreCase)) return false;
            var logName = Path.GetFileName(relative);
            return logName.StartsWith(ModName, StringComparison.OrdinalIgnoreCase) &&
                (logName.EndsWith(".log", StringComparison.OrdinalIgnoreCase) ||
                 logName.EndsWith(".txt", StringComparison.OrdinalIgnoreCase));
        }

        private static bool IsAllowedOwnedModDirectory(string relative)
        {
            foreach (var allowed in new[]
            {
                "dlls", "Scripts", "runtime", Path.Combine("runtime", "logs")
            })
                if (string.Equals(relative, allowed, StringComparison.OrdinalIgnoreCase)) return true;
            return false;
        }

        private static void ReadInstalledKeys(string configPath, out string hotkey, out string fallback)
        {
            var text = File.ReadAllText(configPath, new UTF8Encoding(false, true));
            if (!Regex.IsMatch(text, "(?m)^\\s*\\[auto_pickup\\]\\s*$"))
                throw new InvalidDataException("Installed Auto Pickup config has no [auto_pickup] section.");
            hotkey = ReadConfigSetting(text, "toggle_hotkey");
            fallback = ReadConfigSetting(text, "interaction_key_fallback");
            hotkey = NormalizeKey(hotkey, "F9", false);
            fallback = NormalizeKey(fallback, "F", true);
        }

        private static string ReadConfigSetting(string text, string key)
        {
            var match = Regex.Match(text, "(?m)^\\s*" + Regex.Escape(key) + "\\s*=\\s*([^#;\\r\\n]+?)\\s*$");
            if (!match.Success) throw new InvalidDataException("Config setting is missing: " + key);
            return match.Groups[1].Value.Trim();
        }

        private static string BuildInstallationStateIdentity(Context context, AutoPickupInstallationState110 state)
        {
            var parts = new List<string>
            {
                FileFingerprint(context.GameExecutable),
                FileFingerprint(Path.Combine(context.Win64, "UE4SS.dll")),
                FileFingerprint(Path.Combine(context.NestedUE4SS, "UE4SS.dll")),
                FileFingerprint(Path.Combine(context.Win64, "dwmapi.dll")),
                state.IsInstalled.ToString(),
                state.IsOwned.ToString(),
                state.RangeState.ToString()
            };
            foreach (var modsTxt in ExistingModsTxtCandidates(context)) parts.Add(FileFingerprint(modsTxt));
            foreach (var candidate in ExistingModCandidates(context))
            {
                parts.Add(candidate + "=" + (Directory.Exists(candidate) ? "directory" : "missing"));
                if (!Directory.Exists(candidate)) continue;
                foreach (var file in Directory.GetFiles(candidate, "*", SearchOption.AllDirectories)
                    .OrderBy(value => value, StringComparer.OrdinalIgnoreCase))
                    parts.Add(FileFingerprint(file));
            }
            foreach (var range in Ranges)
                parts.Add(FileFingerprint(Path.Combine(context.PakDirectory, range.FileName)));
            parts.Add(FileFingerprint(Path.Combine(context.PakDirectory, ObsoleteRangePakName)));
            return HashBytes(Encoding.UTF8.GetBytes(string.Join("\n", parts)));
        }

        private static string CreateTransactionJournalDirectory(Context context)
        {
            var path = Path.Combine(context.Win64,
                ".DragonSwordNativeAutoPickup-transaction-" + Guid.NewGuid().ToString("N"));
            AutoPathSafety110.Validate(context.GameRoot, path, "transaction journal");
            return path;
        }

        private static HashSet<InstallerEngine.RangeSelection> ValidateExistingRangeFiles(Context context)
        {
            var result = new HashSet<InstallerEngine.RangeSelection>();
            foreach (var range in Ranges)
            {
                var path = Path.Combine(context.PakDirectory, range.FileName);
                if (!File.Exists(path)) continue;
                result.Add(range.Selection);
            }
            return result;
        }

        private static bool ValidateLegacyRangeCanary(Context context)
        {
            var path = Path.Combine(context.PakDirectory, ObsoleteRangePakName);
            return File.Exists(path);
        }

        private static string ExpectedRangeHash(RangeInfo range)
        {
            return string.IsNullOrWhiteSpace(range.Hash)
                ? HashBytes(ReadResource(range.Resource))
                : range.Hash;
        }

        private static string CreateUniqueBackupDirectory(Context context)
        {
            var label = GetBackupLabel(context);
            var basePath = Path.Combine(context.Win64, "UE4SS-" + label + "-" +
                DateTime.UtcNow.ToString("yyyyMMdd-HHmmss", CultureInfo.InvariantCulture) + "-Backup");
            var candidate = basePath;
            var suffix = 2;
            while (Directory.Exists(candidate) || File.Exists(candidate))
            {
                candidate = basePath + "_" + suffix.ToString(CultureInfo.InvariantCulture);
                suffix++;
            }
            AutoPathSafety110.Validate(context.GameRoot, candidate, "backup directory");
            return candidate;
        }

        private static string GetBackupLabel(Context context)
        {
            var root = Path.Combine(context.Win64, "UE4SS.dll");
            var nested = Path.Combine(context.NestedUE4SS, "UE4SS.dll");
            if (File.Exists(root)) return "Root-" + HashFile(root).Substring(0, 8);
            if (File.Exists(nested))
            {
                var hash = HashFile(nested);
                return string.Equals(hash, ExperimentalUE4SSHash, StringComparison.OrdinalIgnoreCase)
                    ? "v3.0.1-ExperimentalNested-1c1a149" : "Nested-" + hash.Substring(0, 8);
            }
            return "NewInstall";
        }

        private static Snapshot CreateSnapshot(Context context, string backupRoot)
        {
            Directory.CreateDirectory(backupRoot);
            var copied = new List<KeyValuePair<string, string>>();
            foreach (var source in Directory.GetFiles(context.Win64, "UE4SS*", SearchOption.TopDirectoryOnly)
                .Concat(new[] { Path.Combine(context.Win64, "dwmapi.dll") })
                .Where(File.Exists).Distinct(StringComparer.OrdinalIgnoreCase))
                CopySnapshotFile(context, source, Path.Combine(backupRoot, Path.GetFileName(source)), copied);
            foreach (var source in new[] { Path.Combine(context.Win64, "Mods"), Path.Combine(context.Win64, "CXXHeaderDump"), context.NestedUE4SS }
                .Where(Directory.Exists).Distinct(StringComparer.OrdinalIgnoreCase))
                CopySnapshotDirectory(context, source, Path.Combine(backupRoot, Path.GetFileName(source)), copied);
            if (copied.Count == 0) throw new InvalidOperationException("No UE4SS files were available for the required conversion backup.");
            foreach (var pair in copied)
                if (!File.Exists(pair.Value) || !string.Equals(HashFile(pair.Key), HashFile(pair.Value), StringComparison.OrdinalIgnoreCase))
                    throw new IOException("Backup verification failed: " + pair.Key);
            var snapshot = new Snapshot();
            foreach (var mods in new[] { Path.Combine(backupRoot, "Mods"), Path.Combine(backupRoot, "ue4ss", "Mods") })
                if (Directory.Exists(mods)) snapshot.MigrationMods.Add(mods);
            File.WriteAllLines(Path.Combine(backupRoot, "COMPLETE-UE4SS-BACKUP.txt"), new[]
            {
                "Complete UE4SS conversion backup",
                "Created UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture),
                "Original Win64 directory: " + context.Win64,
                "Verified file count: " + copied.Count.ToString(CultureInfo.InvariantCulture),
                "Restore by closing the game, removing the active UE4SS layout, then copying this folder's contents back into Win64."
            }, new UTF8Encoding(false));
            return snapshot;
        }

        private static void CopySnapshotFile(Context context, string source, string destination,
            ICollection<KeyValuePair<string, string>> copied)
        {
            AutoPathSafety110.Validate(context.GameRoot, source, "backup source");
            AutoPathSafety110.Validate(context.GameRoot, destination, "backup destination");
            Directory.CreateDirectory(Path.GetDirectoryName(destination));
            File.Copy(source, destination, false);
            copied.Add(new KeyValuePair<string, string>(source, destination));
        }

        private static void CopySnapshotDirectory(Context context, string source, string destination,
            ICollection<KeyValuePair<string, string>> copied)
        {
            AutoPathSafety110.ValidateTree(context.GameRoot, source, "backup source tree");
            foreach (var file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
            {
                var relative = file.Substring(source.TrimEnd(Path.DirectorySeparatorChar).Length + 1);
                CopySnapshotFile(context, file, Path.Combine(destination, relative), copied);
            }
        }

        private static void RemoveOldUE4SS(Context context, AutoInstallTransaction110 transaction)
        {
            foreach (var file in Directory.GetFiles(context.Win64, "UE4SS*", SearchOption.TopDirectoryOnly)
                .Where(File.Exists).Distinct(StringComparer.OrdinalIgnoreCase))
                transaction.DeleteFile(file, "Remove backed-up root UE4SS file");
            transaction.DeleteFile(Path.Combine(context.Win64, "dwmapi.dll"), "Remove backed-up proxy");
            transaction.DeleteDirectoryTree(Path.Combine(context.Win64, "Mods"), "Remove backed-up root Mods layout");
            transaction.DeleteDirectoryTree(Path.Combine(context.Win64, "CXXHeaderDump"), "Remove backed-up CXXHeaderDump");
            transaction.DeleteDirectoryTree(context.NestedUE4SS, "Remove backed-up nested UE4SS layout");
        }

        private static void InstallPinnedRuntime(Context context, AutoInstallTransaction110 transaction)
        {
            using (var memory = new MemoryStream(ReadResource(RuntimeZipResource), false))
            using (var archive = new ZipArchive(memory, ZipArchiveMode.Read, false))
            {
                foreach (var entry in archive.Entries)
                {
                    if (string.IsNullOrEmpty(entry.Name)) continue;
                    var name = entry.FullName.Replace('\\', '/');
                    if (!string.Equals(name, "dwmapi.dll", StringComparison.OrdinalIgnoreCase) &&
                        !name.StartsWith("ue4ss/", StringComparison.OrdinalIgnoreCase)) continue;
                    var target = Path.GetFullPath(Path.Combine(context.Win64, name.Replace('/', Path.DirectorySeparatorChar)));
                    AutoPathSafety110.Validate(context.GameRoot, target, "runtime extraction target");
                    using (var input = entry.Open())
                    using (var output = new MemoryStream())
                    {
                        input.CopyTo(output);
                        transaction.WriteBytes(target, output.ToArray(), "Install tested UE4SS runtime file");
                    }
                }
            }
        }

        private static byte[] ReadRuntimeEntry(string entryName)
        {
            using (var memory = new MemoryStream(ReadResource(RuntimeZipResource), false))
            using (var archive = new ZipArchive(memory, ZipArchiveMode.Read, false))
            {
                var entry = archive.Entries.FirstOrDefault(value =>
                    string.Equals(
                        value.FullName.Replace('\\', '/'),
                        entryName,
                        StringComparison.OrdinalIgnoreCase));
                if (entry == null) throw new InvalidDataException("Runtime archive entry is missing: " + entryName);
                using (var input = entry.Open())
                using (var output = new MemoryStream())
                {
                    input.CopyTo(output);
                    return output.ToArray();
                }
            }
        }

        private static void MigrateMods(Context context, AutoInstallTransaction110 transaction, IEnumerable<string> sources)
        {
            foreach (var source in sources.Distinct(StringComparer.OrdinalIgnoreCase))
            {
                if (!Directory.Exists(source)) continue;
                foreach (var file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
                {
                    var relative = file.Substring(source.TrimEnd(Path.DirectorySeparatorChar).Length + 1);
                    if (string.Equals(relative, "mods.txt", StringComparison.OrdinalIgnoreCase) ||
                        relative.StartsWith(ModName + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase)) continue;
                    transaction.WriteBytes(Path.Combine(context.Mods, relative), File.ReadAllBytes(file), "Migrate existing UE4SS Mod file");
                }
            }
        }

        private static byte[] BuildConfig(byte[] input, string hotkey, string fallback)
        {
            var text = new UTF8Encoding(false, true).GetString(input);
            text = ReplaceSetting(text, "enabled_on_launch", "false");
            text = ReplaceSetting(text, "debug_logging", "false");
            text = ReplaceSetting(text, "toggle_hotkey", hotkey);
            text = ReplaceSetting(text, "interaction_key", "AUTO");
            text = ReplaceSetting(text, "interaction_key_fallback", fallback);
            return new UTF8Encoding(false).GetBytes(text);
        }

        private static string ReplaceSetting(string text, string key, string value)
        {
            var pattern = "(?m)^(\\s*" + Regex.Escape(key) + "\\s*=\\s*).*$";
            if (!Regex.IsMatch(text, pattern)) throw new InvalidDataException("Config setting is missing: " + key);
            return new Regex(pattern).Replace(text, match => match.Groups[1].Value + value, 1);
        }

        private static byte[] BuildModsTxt(string source)
        {
            return BuildModsTxt(ReadUtf8TextDocument(source));
        }

        private static byte[] BuildModsTxt(byte[] source)
        {
            return BuildModsTxt(ReadUtf8TextDocument(source));
        }

        private static byte[] BuildModsTxt(Utf8TextDocument document)
        {
            var output = new List<string>();
            var wroteAuthority = false;
            foreach (var line in document.Lines)
            {
                if (!IsSameNameEntryCandidate(line))
                {
                    output.Add(line);
                    continue;
                }

                if (!TryParseValidModEntry(line, out var comment))
                    throw new InvalidDataException(
                        "The controlling mods.txt contains a malformed " + ModName + " entry. Resolve it manually.");
                if (wroteAuthority) continue;
                output.Add(ModName + " : 1" +
                    (string.IsNullOrEmpty(comment) ? string.Empty : " " + comment));
                wroteAuthority = true;
            }
            if (!wroteAuthority) output.Add(ModName + " : 1");
            return EncodeUtf8TextDocument(document, output);
        }

        private static byte[] BuildModsTxtWithoutAutoPickup(string source)
        {
            var document = ReadUtf8TextDocument(source);
            var output = new List<string>();
            foreach (var line in document.Lines)
            {
                if (!IsSameNameEntryCandidate(line))
                {
                    output.Add(line);
                    continue;
                }
                if (!TryParseValidModEntry(line, out _))
                    throw new InvalidDataException(
                        "The controlling mods.txt contains a malformed " + ModName + " entry. Resolve it manually.");
            }
            return EncodeUtf8TextDocument(document, output);
        }

        private static Utf8TextDocument ReadUtf8TextDocument(string source)
        {
            if (!File.Exists(source))
            {
                return new Utf8TextDocument
                {
                    HasBom = false,
                    Newline = "\r\n",
                    EndsWithNewline = true
                };
            }

            return ReadUtf8TextDocument(File.ReadAllBytes(source));
        }

        private static Utf8TextDocument ReadUtf8TextDocument(byte[] bytes)
        {
            var hasBom = bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF;
            var offset = hasBom ? 3 : 0;
            var text = new UTF8Encoding(false, true).GetString(bytes, offset, bytes.Length - offset);
            var newlineMatches = Regex.Matches(text, "\r\n|\r|\n")
                .Cast<Match>()
                .Select(match => match.Value)
                .Distinct(StringComparer.Ordinal)
                .ToList();
            if (newlineMatches.Count > 1)
                throw new InvalidDataException(
                    "The controlling mods.txt uses mixed newline styles. Normalize it before installation.");

            var newline = newlineMatches.Count == 0 ? "\r\n" : newlineMatches[0];
            var document = new Utf8TextDocument
            {
                HasBom = hasBom,
                Newline = newline,
                EndsWithNewline = text.EndsWith(newline, StringComparison.Ordinal)
            };
            if (text.Length == 0) return document;

            document.Lines.AddRange(Regex.Split(text, "\r\n|\r|\n"));
            if (document.EndsWithNewline && document.Lines.Count != 0)
                document.Lines.RemoveAt(document.Lines.Count - 1);
            return document;
        }

        private static byte[] EncodeUtf8TextDocument(Utf8TextDocument document, IList<string> lines)
        {
            var text = string.Join(document.Newline, lines);
            if (document.EndsWithNewline && lines.Count != 0) text += document.Newline;
            var body = new UTF8Encoding(false, true).GetBytes(text);
            if (!document.HasBom) return body;

            var preamble = new UTF8Encoding(true).GetPreamble();
            var output = new byte[preamble.Length + body.Length];
            Buffer.BlockCopy(preamble, 0, output, 0, preamble.Length);
            Buffer.BlockCopy(body, 0, output, preamble.Length, body.Length);
            return output;
        }

        private static bool IsSameNameEntryCandidate(string line)
        {
            return Regex.IsMatch(
                line,
                "^\\s*" + Regex.Escape(ModName) + "(?=\\s|:|$)",
                RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
        }

        private static bool TryParseValidModEntry(string line, out string comment)
        {
            var match = Regex.Match(
                line,
                "^\\s*" + Regex.Escape(ModName) + "\\s*:\\s*[01]\\s*((?:[#;].*)?)$",
                RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);
            comment = match.Success ? match.Groups[1].Value.TrimStart() : string.Empty;
            return match.Success;
        }

        private static bool ContainsModEntry(string source)
        {
            return File.ReadAllLines(source, new UTF8Encoding(false, true)).Any(line =>
                Regex.IsMatch(line, "^\\s*" + Regex.Escape(ModName) + "\\s*:", RegexOptions.IgnoreCase));
        }

        private static void VerifyInstalledMod(Context context, byte[] config, byte[] modsTxt,
            InstallerEngine.RangeSelection selected)
        {
            var mod = Path.Combine(context.Mods, ModName);
            var checks = new Dictionary<string, byte[]>
            {
                { Path.Combine(mod, "dlls", "main.dll"), ReadResource(PluginResource) },
                { Path.Combine(mod, "config.ini"), config },
                { Path.Combine(mod, "Scripts", "main.lua"), ReadResource(LuaResource) },
                { context.ModsTxt, modsTxt }
            };
            foreach (var check in checks)
                if (!File.Exists(check.Key) || !string.Equals(HashFile(check.Key), HashBytes(check.Value), StringComparison.OrdinalIgnoreCase))
                    throw new IOException("Installed file verification failed: " + check.Key);
            foreach (var range in Ranges)
            {
                var path = Path.Combine(context.PakDirectory, range.FileName);
                if (range.Selection == selected)
                {
                    if (!File.Exists(path) ||
                        !string.Equals(HashFile(path), ExpectedRangeHash(range), StringComparison.OrdinalIgnoreCase))
                        throw new IOException("Range PAK verification failed: " + path);
                }
                else if (File.Exists(path))
                {
                    throw new IOException("An unselected range PAK remained installed: " + path);
                }
            }
            if (File.Exists(Path.Combine(context.PakDirectory, ObsoleteRangePakName)))
                throw new IOException("The legacy range PAK remained installed.");
            if (File.Exists(Path.Combine(mod, "enabled.txt")))
                throw new IOException("Legacy enabled.txt remained installed.");
        }

        private static void VerifyPinnedRuntime(Context context)
        {
            var checks = new Dictionary<string, string>
            {
                { Path.Combine(context.NestedUE4SS, "UE4SS.dll"), ExperimentalUE4SSHash },
                { Path.Combine(context.Win64, "dwmapi.dll"), ExperimentalDwmapiHash },
                { Path.Combine(context.NestedUE4SS, ExperimentalUsmapName), ExperimentalUsmapHash }
            };
            foreach (var check in checks)
                if (!File.Exists(check.Key) || !string.Equals(HashFile(check.Key), check.Value, StringComparison.OrdinalIgnoreCase))
                    throw new IOException("Pinned UE4SS verification failed: " + check.Key);
            if (File.Exists(Path.Combine(context.Win64, "UE4SS.dll")))
                throw new IOException("The old root UE4SS.dll remained active after conversion.");
        }

        private static void ValidateResources()
        {
            var runtime = ReadResource(RuntimeZipResource);
            using (var memory = new MemoryStream(runtime, false))
            using (var archive = new ZipArchive(memory, ZipArchiveMode.Read, false))
            {
                ValidateZipEntry(archive, "dwmapi.dll", ExperimentalDwmapiHash);
                ValidateZipEntry(archive, "ue4ss/UE4SS.dll", ExperimentalUE4SSHash);
                ValidateZipEntry(archive, "ue4ss/" + ExperimentalUsmapName, ExperimentalUsmapHash);
                ValidateZipEntry(archive, "ue4ss/UE4SS-settings.ini", ExperimentalSettingsHash);
            }
            foreach (var range in Ranges)
                if (!string.Equals(HashBytes(ReadResource(range.Resource)), ExpectedRangeHash(range), StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("Embedded range PAK hash failed: " + range.FileName);
            var plugin = ReadResource(PluginResource);
            var ascii = Encoding.ASCII.GetString(plugin);
            if (!ascii.Contains("DRAGONSWORD_NATIVE_AUTO_PICKUP_1_3_1"))
                throw new InvalidDataException("Embedded Auto Pickup plugin is not the 1.3.1 ExperimentalNested build.");
        }

        private static void ValidateZipEntry(ZipArchive archive, string name, string hash)
        {
            var entry = archive.Entries.FirstOrDefault(value =>
                string.Equals(value.FullName.Replace('\\', '/'), name, StringComparison.OrdinalIgnoreCase));
            if (entry == null) throw new InvalidDataException("Runtime archive entry is missing: " + name);
            using (var input = entry.Open())
            using (var sha = SHA256.Create())
                if (!string.Equals(ToHex(sha.ComputeHash(input)), hash, StringComparison.OrdinalIgnoreCase))
                    throw new InvalidDataException("Runtime archive entry hash failed: " + name);
        }

        private static string BuildRecord(Context context, string gameHash, string hotkey, string fallback,
            InstallerEngine.RangeSelection range, string backup)
        {
            return "Product: DragonSword Native Auto Pickup\r\nVersion: " + ProductVersion +
                "\r\nOwnership Schema: " + OwnershipSchema +
                "\r\nOwnership ID: " + OwnershipId +
                "\r\nPlugin SHA-256: " + HashBytes(ReadResource(PluginResource)) +
                "\r\nLua SHA-256: " + HashBytes(ReadResource(LuaResource)) +
                "\r\nNotices SHA-256: " + HashBytes(ReadResource(NoticesResource)) +
                "\r\nInstalled UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture) +
                "\r\nGame SHA-256: " + gameHash +
                "\r\nUE4SS: v3.0.1 Beta #0 g1c1a1497 ExperimentalNested\r\nConverted: " + context.Converts +
                "\r\nToggle key: " + hotkey + "\r\nInteraction fallback: " + fallback +
                "\r\nRange: " + (int)range + "x\r\nBackup: " + backup + "\r\n";
        }

        private static string ValidateGameExecutable(string selected, bool requireClosed)
        {
            if (string.IsNullOrWhiteSpace(selected)) throw new ArgumentException("Select " + GameFileName + ".");
            var full = Path.GetFullPath(selected.Trim());
            if (!File.Exists(full) || !string.Equals(Path.GetFileName(full), GameFileName, StringComparison.OrdinalIgnoreCase))
                throw new FileNotFoundException("Select the installed " + GameFileName + ".", full);
            return full;
        }

        private static string NormalizeKey(string value, string fallback, bool allowGamepad)
        {
            var key = string.IsNullOrWhiteSpace(value) ? fallback : value.Trim();
            if (Regex.IsMatch(key, "^(F([1-9]|1[0-9]|2[0-4])|[A-Z0-9]|NUM[0-9]|HOME|END|PAGEUP|PAGEDOWN|INSERT|DELETE|SPACE)$", RegexOptions.IgnoreCase))
                return key.ToUpperInvariant();
            if (allowGamepad && Regex.IsMatch(key, "^[A-Za-z][A-Za-z0-9_]{1,63}$")) return key;
            throw new InvalidOperationException("Unsupported key name: " + key);
        }

        private static void EnsureGameIsClosed()
        {
            if (Process.GetProcessesByName("DSClient-Win64-Shipping").Length != 0)
                throw new InvalidOperationException("Close DragonSword: Awakening before installation.");
        }

        private static byte[] ReadResource(string name)
        {
            using (var input = Assembly.GetExecutingAssembly().GetManifestResourceStream(name))
            {
                if (input == null) throw new InvalidDataException("Installer resource is missing: " + name);
                using (var output = new MemoryStream()) { input.CopyTo(output); return output.ToArray(); }
            }
        }

        private static string FileFingerprint(string path)
        {
            return path + "=" + (File.Exists(path) ? HashFile(path) : "missing");
        }

        private static IEnumerable<string> DiscoverSteamRoots()
        {
            var roots = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var key in new[]
            {
                @"HKEY_CURRENT_USER\Software\Valve\Steam",
                @"HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Valve\Steam",
                @"HKEY_LOCAL_MACHINE\SOFTWARE\Valve\Steam"
            })
            {
                foreach (var valueName in new[] { "SteamPath", "InstallPath" })
                {
                    var value = Registry.GetValue(key, valueName, null) as string;
                    if (!string.IsNullOrWhiteSpace(value) && Directory.Exists(value))
                    {
                        roots.Add(Path.GetFullPath(value));
                    }
                }
            }

            return roots;
        }

        private static IEnumerable<string> DiscoverSteamLibraries(string steamRoot)
        {
            var libraries = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                Path.GetFullPath(steamRoot)
            };
            var vdf = Path.Combine(steamRoot, "steamapps", "libraryfolders.vdf");
            if (File.Exists(vdf))
            {
                var text = File.ReadAllText(vdf, new UTF8Encoding(false, true));
                foreach (Match match in Regex.Matches(
                    text,
                    "\\\"path\\\"\\s*\\\"([^\\\"]+)\\\"",
                    RegexOptions.IgnoreCase | RegexOptions.CultureInvariant))
                {
                    var value = match.Groups[1].Value.Replace("\\\\", "\\");
                    if (Directory.Exists(value))
                    {
                        libraries.Add(Path.GetFullPath(value));
                    }
                }
            }

            return libraries;
        }

        private static string HashFile(string path)
        {
            using (var input = File.OpenRead(path))
            using (var sha = SHA256.Create()) return ToHex(sha.ComputeHash(input));
        }

        private static string HashBytes(byte[] bytes)
        {
            using (var sha = SHA256.Create()) return ToHex(sha.ComputeHash(bytes));
        }

        private static string ToHex(byte[] bytes)
        {
            return BitConverter.ToString(bytes).Replace("-", string.Empty);
        }
    }

    internal static class AutoPathSafety110
    {
        internal static string Validate(string gameRoot, string path, string description)
        {
            var rootDirectory = Path.GetFullPath(gameRoot).TrimEnd(Path.DirectorySeparatorChar);
            var root = rootDirectory + Path.DirectorySeparatorChar;
            var full = Path.GetFullPath(path);
            if (!full.StartsWith(root, StringComparison.OrdinalIgnoreCase) &&
                !string.Equals(full.TrimEnd(Path.DirectorySeparatorChar), rootDirectory, StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException("The " + description + " is outside the game tree: " + full);

            var current = full.TrimEnd(Path.DirectorySeparatorChar);
            while (true)
            {
                RejectReparse(current, description);
                if (string.Equals(current, rootDirectory, StringComparison.OrdinalIgnoreCase)) break;
                current = Path.GetDirectoryName(current);
                if (string.IsNullOrEmpty(current) ||
                    (!current.StartsWith(root, StringComparison.OrdinalIgnoreCase) &&
                     !string.Equals(current, rootDirectory, StringComparison.OrdinalIgnoreCase)))
                    throw new InvalidOperationException("The " + description + " escaped the trusted game root.");
            }
            return full;
        }

        internal static void ValidateTree(string gameRoot, string root, string description)
        {
            Validate(gameRoot, root, description);
            var pending = new Stack<string>(); pending.Push(root);
            while (pending.Count != 0)
            {
                var current = pending.Pop(); RejectReparse(current, description);
                foreach (var file in Directory.GetFiles(current)) RejectReparse(file, description);
                foreach (var child in Directory.GetDirectories(current)) { RejectReparse(child, description); pending.Push(child); }
            }
        }

        private static void RejectReparse(string path, string description)
        {
            if ((File.Exists(path) || Directory.Exists(path)) && (File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0)
                throw new InvalidOperationException("The " + description + " crosses a reparse point: " + path);
        }
    }

    internal sealed class AutoInstallTransaction110 : IDisposable
    {
        private sealed class Original { internal string Target; internal string Backup; internal bool Existed; }
        private readonly string _scope;
        private readonly string _gameRoot;
        private readonly string _backup;
        private readonly bool _retainBackup;
        private readonly Dictionary<string, Original> _originals = new Dictionary<string, Original>(StringComparer.OrdinalIgnoreCase);
        private readonly List<string> _order = new List<string>();
        private readonly List<string> _created = new List<string>();
        private readonly List<string> _removedDirectories = new List<string>();
        private readonly List<string> _actions = new List<string>();
        private bool _committed;
        private bool _rolledBack;

        internal AutoInstallTransaction110(string scope, string gameRoot, string backup, bool retainBackup)
        {
            _scope = Path.GetFullPath(scope).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
            _gameRoot = Path.GetFullPath(gameRoot);
            _backup = AutoPathSafety110.Validate(_gameRoot, backup, "backup directory");
            _retainBackup = retainBackup;
            Directory.CreateDirectory(_backup);
        }

        internal void WriteBytes(string path, byte[] bytes, string action)
        {
            var target = Path.GetFullPath(path); Prepare(target); EnsureDirectory(Path.GetDirectoryName(target));
            var temp = Path.Combine(Path.GetDirectoryName(target), ".dsnap-" + Guid.NewGuid().ToString("N") + ".tmp");
            File.WriteAllBytes(temp, bytes);
            if (File.Exists(target)) File.Replace(temp, target, null, true); else File.Move(temp, target);
            _actions.Add(action + ": " + target);
        }

        internal void DeleteFile(string path, string action)
        {
            var target = Path.GetFullPath(path); if (!File.Exists(target)) return;
            Prepare(target); File.Delete(target); _actions.Add(action + ": " + target);
        }

        internal void DeleteDirectoryTree(string path, string action)
        {
            var target = Path.GetFullPath(path); if (!Directory.Exists(target)) return;
            if (!target.StartsWith(_scope, StringComparison.OrdinalIgnoreCase) ||
                string.Equals(target.TrimEnd(Path.DirectorySeparatorChar), _scope.TrimEnd(Path.DirectorySeparatorChar), StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException("Refused to remove directory outside Win64: " + target);
            AutoPathSafety110.ValidateTree(_gameRoot, target, "directory delete target");
            foreach (var file in Directory.GetFiles(target, "*", SearchOption.AllDirectories)) DeleteFile(file, action + " file");
            foreach (var directory in Directory.GetDirectories(target, "*", SearchOption.AllDirectories).Concat(new[] { target }).OrderByDescending(value => value.Length))
            { Directory.Delete(directory, false); _removedDirectories.Add(directory); }
            _actions.Add(action + ": " + target);
        }

        internal void Commit(IEnumerable<string> summary)
        {
            if (_retainBackup)
            {
                var lines = new List<string>(summary); lines.Add(""); lines.Add("Recorded operations:");
                lines.AddRange(_actions.Select(value => "- " + value));
                File.WriteAllLines(Path.Combine(_backup, "INSTALL-LOG.txt"), lines, new UTF8Encoding(false));
                _committed = true;
                return;
            }

            _committed = true;
            if (Directory.Exists(_backup)) Directory.Delete(_backup, true);
        }

        internal void Rollback()
        {
            if (_rolledBack) return;
            var failures = new List<string>();
            for (var i = _order.Count - 1; i >= 0; --i)
            {
                var original = _originals[_order[i]];
                try
                {
                    if (original.Existed)
                    {
                        EnsureDirectory(Path.GetDirectoryName(original.Target));
                        File.Copy(original.Backup, original.Target, true);
                    }
                    else if (File.Exists(original.Target)) File.Delete(original.Target);
                }
                catch (Exception error) { failures.Add(original.Target + ": " + error.Message); }
            }
            foreach (var directory in _removedDirectories.OrderBy(value => value.Length).Distinct(StringComparer.OrdinalIgnoreCase))
                if (!Directory.Exists(directory)) Directory.CreateDirectory(directory);
            foreach (var directory in _created.OrderByDescending(value => value.Length).Distinct(StringComparer.OrdinalIgnoreCase))
                if (Directory.Exists(directory) && !Directory.EnumerateFileSystemEntries(directory).Any()) Directory.Delete(directory);
            _rolledBack = true;
            File.WriteAllLines(Path.Combine(_backup, "ROLLBACK-LOG.txt"), failures.Count == 0 ? new[] { "Automatic rollback completed." } : failures, new UTF8Encoding(false));
            if (failures.Count != 0) throw new IOException(string.Join(" | ", failures));
        }

        private void Prepare(string target)
        {
            AutoPathSafety110.Validate(_gameRoot, target, "transaction target");
            if (_originals.ContainsKey(target)) return;
            var existed = File.Exists(target); string backup = null;
            if (existed)
            {
                backup = Path.Combine(_backup, ".rollback", _order.Count.ToString("D5", CultureInfo.InvariantCulture) + ".bak");
                Directory.CreateDirectory(Path.GetDirectoryName(backup)); File.Copy(target, backup, false);
            }
            _originals.Add(target, new Original { Target = target, Backup = backup, Existed = existed }); _order.Add(target);
        }

        private void EnsureDirectory(string path)
        {
            var missing = new Stack<string>(); var current = Path.GetFullPath(path);
            while (!Directory.Exists(current)) { missing.Push(current); current = Path.GetDirectoryName(current); }
            while (missing.Count != 0) { var item = missing.Pop(); AutoPathSafety110.Validate(_gameRoot, item, "target directory"); Directory.CreateDirectory(item); _created.Add(item); }
        }

        public void Dispose()
        {
            if (!_committed && !_rolledBack && _order.Count != 0) Rollback();
        }
    }
}
