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
using System.Web.Script.Serialization;
using Microsoft.Win32;

namespace DragonSwordNativeWorldRadarPostRender.Installer
{
    internal sealed class InstallerHotkeys
    {
        internal string Settings { get; set; }
        internal string Enable { get; set; }
        internal string Disable { get; set; }
        internal string Description
        {
            get { return "Settings: " + Settings + " / Enable: " + Enable + " / Disable: " + Disable; }
        }
    }

    internal sealed class InstallerResult
    {
        internal string LayoutDescription { get; set; }
        internal string BackupDirectory { get; set; }
        internal string ModDirectory { get; set; }
        internal string ModsTxtPath { get; set; }
        internal bool UpdatedExistingRadar { get; set; }
        internal InstallerHotkeys Hotkeys { get; set; }
    }

    internal sealed class InstallerPlan
    {
        internal string IdentityToken { get; set; }
        internal string LayoutDescription { get; set; }
        internal string ActionDescription { get; set; }
        internal string PluginDescription { get; set; }
        internal string LoaderSha256 { get; set; }
        internal string ProxySha256 { get; set; }
        internal string UE4SSDirectory { get; set; }
        internal string ModDirectory { get; set; }
        internal string ModsTxtPath { get; set; }
        internal bool BootstrapsUE4SS { get; set; }
        internal bool ConvertsUE4SS { get; set; }
        internal bool UpdatesExistingRadar { get; set; }
        internal InstallerHotkeys Hotkeys { get; set; }
    }

    internal sealed class InstallerInstallationState
    {
        internal bool IsInstalled { get; set; }
        internal bool IsOwned { get; set; }
        internal bool CanInstall { get; set; }
        internal bool CanUpdate { get; set; }
        internal bool CanUninstall { get; set; }
        internal string InstalledVersion { get; set; }
        internal string StatusDescription { get; set; }
        internal string ModDirectory { get; set; }
        internal InstallerHotkeys Hotkeys { get; set; }
    }

    internal sealed class UninstallerPlan
    {
        internal string IdentityToken { get; set; }
        internal string LayoutDescription { get; set; }
        internal string ModDirectory { get; set; }
        internal string ModsTxtPath { get; set; }
    }

    internal sealed class UninstallerResult
    {
        internal string ModDirectory { get; set; }
        internal string ModsTxtPath { get; set; }
    }

    internal static class InstallerEngine
    {
        private const string ProductVersion = "2.3.0";
        private const string RuntimeLabel = "DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_2_3_0";
        private const string GameFileName = "DSClient-Win64-Shipping.exe";
        private const string ModName = "DragonSwordNativeWorldRadarPostRender";
        private const string LegacyRadarName = "DragonSwordWorldRadarObjectState";
        private const string LegacyExternalRadarName = "DragonSwordWorldRadar";
        private const string GameCompatibilityPolicy =
            "pe32plus-x64-runtime-unique-owner-pointer-pattern";
        private const string ExperimentalUE4SSHash =
            "F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1";
        private const string KnownStableRootUE4SS301HashForBackupLabel =
            "8AC18FBFFC1EF96B0662D4A2D537B3F224C26D65CAABA7989A9404C566102B26";
        private const string ExperimentalDwmapiHash =
            "30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B";
        private const string ExperimentalUsmapHash =
            "0F558E61A259245F65D8801F4D9F4B67CABC58423D5741CDE77B77F2D572AD47";
        private const string ExperimentalSettingsHash =
            "4E9BBDB5F50A6DCAFFE4046EDB2D78669255C7ADC079AF98E8D3E364C3593E74";
        private const string ExperimentalUsmapName = "DS-5.3.2-0+UE5-1c1a1497.usmap";
        private const string ManifestResource = "Payload.Manifest.ini";
        private const string ExperimentalRuntimeZipResource = "Payload.ExperimentalRuntime.zip";
        private const string ExperimentalUE4SSResource = "Payload.ExperimentalUE4SS.dll";
        private const string ExperimentalDwmapiResource = "Payload.ExperimentalDwmapi.dll";
        private const string ExperimentalUsmapResource = "Payload.Experimental.usmap";
        private const string ExperimentalSettingsResource = "Payload.ExperimentalUE4SS-settings.ini";
        private const string NoticesResource = "Payload.ThirdPartyNotices.txt";
        private const string TreasureOverridesPath = "data/defaults/treasure_overrides.txt";
        private const int MaximumPayloadFiles = 128;
        private const long MaximumPayloadBytes = 256L * 1024L * 1024L;

        // Reflection-only integration seam. Production callers never set this field.
        private static string IntegrationTestFailurePoint = null;

        private enum UE4SSLayout
        {
            ExperimentalNested
        }

        private sealed class InstallContext
        {
            internal string GameExecutable;
            internal string GameRootDirectory;
            internal string Win64Directory;
            internal string StableUE4SSDirectory;
            internal string NestedUE4SSDirectory;
            internal string UE4SSDirectory;
            internal string ModsDirectory;
            internal string ModsTxtPath;
            internal List<string> AdditionalModsDirectories = new List<string>();
            internal UE4SSLayout Layout;
            internal bool BootstrappedUE4SS;
            internal bool ConvertsUE4SS;
            internal bool UpdatesExistingRadar;
            internal string ExistingLayoutDescription;
            internal string MigrationSettingsPath;
            internal string MigrationModsTxtPath;
            internal List<string> MigrationModsDirectories = new List<string>();
        }

        private sealed class ConversionSnapshot
        {
            internal string SnapshotWin64Directory;
            internal List<string> MigrationModsDirectories = new List<string>();
        }

        private sealed class PayloadEntry
        {
            internal string Path;
            internal long Size;
            internal string Sha256;
            internal byte[] Bytes;
        }

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
                // Discovery is a convenience only. Explicit file selection remains authoritative.
            }
            return string.Empty;
        }

        internal static InstallerPlan Inspect(string selectedGameExecutable)
        {
            return InspectCore(selectedGameExecutable, null);
        }

        internal static InstallerPlan InspectWithHotkeys(
            string selectedGameExecutable, string settings, string enable, string disable)
        {
            return InspectCore(selectedGameExecutable, NormalizeRequestedHotkeys(settings, enable, disable));
        }

        private static InstallerPlan InspectCore(
            string selectedGameExecutable, InstallerHotkeys requestedHotkeys)
        {
            EnsureGameIsClosed();
            var gameExecutable = ValidateGameExecutableLocation(selectedGameExecutable);
            var gameHash = HashFile(gameExecutable);

            var manifest = ReadManifest();
            ValidateManifest(manifest);
            var experimentalPayload = LoadAndValidatePayload(
                manifest,
                "experimental",
                ExperimentalRuntimeZipResource);
            ValidatePayloadContract(experimentalPayload);
            ValidateExperimentalLoaderResources();

            var context = DetectLayout(gameExecutable);
            ValidateExistingUE4SS(context);
            PrepareExperimentalTarget(context);
            var modDirectory = Path.Combine(context.ModsDirectory, ModName);
            ValidateMigrationConflicts(context, modDirectory);

            byte[] visibilityBytes, diagnosticsBytes, hotkeyBytes, treasureOverrideBytes;
            var configurationSource = FindExistingRadarDirectory(context, modDirectory);
            PrepareUserConfiguration(configurationSource, experimentalPayload,
                out visibilityBytes, out diagnosticsBytes, out hotkeyBytes, out treasureOverrideBytes);
            return BuildInstallerPlan(gameExecutable, gameHash, context, modDirectory,
                configurationSource, hotkeyBytes, ApplyRequestedHotkeys(hotkeyBytes, requestedHotkeys));
        }

        internal static InstallerInstallationState InspectInstallationState(
            string selectedGameExecutable)
        {
            var gameExecutable = ValidateGameExecutableLocation(selectedGameExecutable);
            var context = DetectLayout(gameExecutable);
            ValidateExistingUE4SS(context);

            var state = new InstallerInstallationState
            {
                InstalledVersion = string.Empty,
                StatusDescription = "Native World Radar installation state is unavailable.",
                ModDirectory = Path.Combine(context.ModsDirectory, ModName),
                Hotkeys = NormalizeRequestedHotkeys("F6", "F7", "F8")
            };
            try
            {
                PrepareExperimentalTarget(context);
                var target = Path.Combine(context.ModsDirectory, ModName);
                state.ModDirectory = target;
                ValidateMigrationConflicts(context, target);

                var candidates = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                var roots = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
                {
                    context.ModsDirectory
                };
                foreach (var root in context.MigrationModsDirectories)
                {
                    roots.Add(root);
                }
                foreach (var root in context.AdditionalModsDirectories)
                {
                    roots.Add(root);
                }
                foreach (var root in roots)
                {
                    var candidate = Path.Combine(root, ModName);
                    if (Directory.Exists(candidate) &&
                        Directory.GetFiles(candidate, "*", SearchOption.AllDirectories).Length != 0)
                    {
                        candidates.Add(Path.GetFullPath(candidate));
                    }
                    else if (File.Exists(candidate))
                    {
                        state.IsInstalled = true;
                        state.StatusDescription =
                            "The Native World Radar target path is a file. Setup will not overwrite or delete it: " +
                            candidate;
                        return state;
                    }
                }

                state.IsInstalled = candidates.Count != 0;
                if (candidates.Count == 0)
                {
                    state.CanInstall = true;
                    state.StatusDescription =
                        "Native World Radar is not installed. Install is available. " +
                        "Game compatibility is checked structurally without a game-version or EXE-hash allowlist.";
                    return state;
                }
                if (candidates.Count != 1)
                {
                    state.StatusDescription =
                        "Multiple Native World Radar directories were found. Setup will not modify an ambiguous installation.";
                    return state;
                }

                var installedDirectory = candidates.Single();
                state.ModDirectory = installedDirectory;
                if (!TryValidateOwnedTargetMetadata(
                        context.GameRootDirectory,
                        installedDirectory))
                {
                    state.StatusDescription =
                        "A Native World Radar directory exists, but strict release and package ownership could not be verified. " +
                        "Setup will not overwrite or uninstall it.";
                    return state;
                }

                string installedVersion;
                if (!TryReadOwnedMetadata(
                        context.GameRootDirectory,
                        Path.Combine(installedDirectory, "metadata", "release.json"),
                        ModName,
                        out installedVersion))
                {
                    state.StatusDescription =
                        "Native World Radar ownership was detected, but its installed version could not be read safely.";
                    return state;
                }

                // Hotkey validation controls Update / Repair, not ownership or
                // safe Uninstall availability for an otherwise owned target.
                state.IsOwned = true;
                state.InstalledVersion = installedVersion;
                state.CanUninstall = !context.BootstrappedUE4SS &&
                    !context.ConvertsUE4SS &&
                    string.Equals(
                        Path.GetFullPath(installedDirectory),
                        Path.GetFullPath(target),
                        StringComparison.OrdinalIgnoreCase) &&
                    context.UpdatesExistingRadar;
                var installedHotkeyPath = Path.Combine(installedDirectory, "config", "hotkeys.ini");
                InstallerPathSafety.Validate(context.GameRootDirectory, installedHotkeyPath,
                    "installed Radar hotkeys");
                if (Directory.Exists(installedHotkeyPath))
                    throw new InvalidDataException("hotkeys.ini is a directory.");
                if (File.Exists(installedHotkeyPath))
                {
                    if (new FileInfo(installedHotkeyPath).Length > 4096)
                        throw new InvalidDataException("hotkeys.ini exceeds the strict 4 KiB size limit.");
                    state.Hotkeys = ReadHotkeys(File.ReadAllBytes(installedHotkeyPath));
                }
                state.CanUpdate = true;
                state.StatusDescription = string.Equals(
                        installedVersion,
                        ProductVersion,
                        StringComparison.Ordinal)
                    ? "Native World Radar " + ProductVersion +
                      " is installed and strictly owned. Repair is available."
                    : "Native World Radar " + installedVersion +
                      " is installed and strictly owned. Update to " + ProductVersion + " is available.";
                if (!state.CanUninstall)
                {
                    state.StatusDescription +=
                        " Automatic uninstall is unavailable until the owned installation is in the active compatible UE4SS layout.";
                }
                return state;
            }
            catch (Exception exception)
            {
                state.IsInstalled = Directory.Exists(state.ModDirectory) ||
                    File.Exists(state.ModDirectory);
                state.StatusDescription = exception.Message;
                return state;
            }
        }

        internal static InstallerResult InstallConfirmed(
            string selectedGameExecutable,
            string confirmedPlanIdentity)
        {
            if (string.IsNullOrWhiteSpace(confirmedPlanIdentity))
            {
                throw new InvalidOperationException(
                    "The installation plan was not confirmed. Run the compatibility check again.");
            }
            return InstallCore(selectedGameExecutable, confirmedPlanIdentity, null);
        }

        internal static InstallerResult InstallConfirmedWithHotkeys(
            string selectedGameExecutable, string settings, string enable, string disable,
            string confirmedPlanIdentity)
        {
            if (string.IsNullOrWhiteSpace(confirmedPlanIdentity))
                throw new InvalidOperationException("The installation plan was not confirmed.");
            return InstallCore(selectedGameExecutable, confirmedPlanIdentity,
                NormalizeRequestedHotkeys(settings, enable, disable));
        }

        internal static InstallerResult Install(string selectedGameExecutable)
        {
            return InstallCore(selectedGameExecutable, null, null);
        }

        internal static UninstallerPlan InspectUninstall(
            string selectedGameExecutable)
        {
            string gameHash;
            var context = PrepareUninstallContext(
                selectedGameExecutable,
                out gameHash);
            return BuildUninstallerPlan(context, gameHash);
        }

        internal static UninstallerResult UninstallConfirmed(
            string selectedGameExecutable,
            string confirmedPlanIdentity)
        {
            if (string.IsNullOrWhiteSpace(confirmedPlanIdentity))
            {
                throw new InvalidOperationException(
                    "The uninstall plan was not confirmed. Run the uninstall check again.");
            }

            string gameHash;
            var context = PrepareUninstallContext(
                selectedGameExecutable,
                out gameHash);
            var plan = BuildUninstallerPlan(context, gameHash);
            if (!string.Equals(
                    plan.IdentityToken,
                    confirmedPlanIdentity,
                    StringComparison.Ordinal))
            {
                throw new InvalidOperationException(
                    "The game, UE4SS layout, installed Radar, or mods.txt changed after the uninstall prompt. " +
                    "Setup stopped without changing files. Review the new plan and confirm it again.");
            }

            var modsTxtBytes = BuildUninstallModsTxt(context.ModsTxtPath);
            var originalUE4SSFingerprint = FingerprintUE4SSLoaderState(context);
            var backupRoot = Path.Combine(
                context.Win64Directory,
                ".dsnwr-uninstall-transaction-" + Guid.NewGuid().ToString("N"));
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                backupRoot,
                "Native World Radar uninstall rollback directory");

            EnsureGameIsClosed();
            using (var transaction = new InstallTransaction(
                context.Win64Directory,
                context.GameRootDirectory,
                backupRoot,
                false))
            {
                try
                {
                    transaction.DeleteDirectoryTree(
                        plan.ModDirectory,
                        "Remove strictly owned Native World Radar directory");
                    transaction.WriteBytes(
                        context.ModsTxtPath,
                        modsTxtBytes,
                        "Remove Native World Radar mods.txt authority");

                    ThrowIfIntegrationTestFailure("after-uninstall-mutations");

                    if (Directory.Exists(plan.ModDirectory) || File.Exists(plan.ModDirectory))
                    {
                        throw new InvalidOperationException(
                            "The strictly owned Native World Radar directory remained after uninstall.");
                    }
                    if (!File.ReadAllBytes(context.ModsTxtPath).SequenceEqual(modsTxtBytes) ||
                        CountAnyEntry(
                            ReadUtf8Document(context.ModsTxtPath).Text,
                            ModName) != 0)
                    {
                        throw new InvalidOperationException(
                            "The Native World Radar mods.txt authority remained after uninstall.");
                    }
                    if (!string.Equals(
                            FingerprintUE4SSLoaderState(context),
                            originalUE4SSFingerprint,
                            StringComparison.Ordinal))
                    {
                        throw new InvalidOperationException(
                            "The UE4SS loader state changed during uninstall. Setup will restore the Radar transaction.");
                    }

                    transaction.Commit(new[]
                    {
                        "DragonSword Native World Radar " + ProductVersion + " uninstall",
                        "UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture),
                        "Removed Mod directory: " + plan.ModDirectory,
                        "Updated mods.txt: " + context.ModsTxtPath,
                        "UE4SS, unrelated Mods, and game saves: preserved"
                    });
                    return new UninstallerResult
                    {
                        ModDirectory = plan.ModDirectory,
                        ModsTxtPath = context.ModsTxtPath
                    };
                }
                catch (Exception uninstallError)
                {
                    try
                    {
                        transaction.Rollback();
                    }
                    catch (Exception rollbackError)
                    {
                        throw new InvalidOperationException(
                            "Uninstall failed and automatic rollback was incomplete. Original error: " +
                            uninstallError.Message + " Rollback error: " + rollbackError.Message +
                            " Recovery directory: " + backupRoot,
                            uninstallError);
                    }
                    throw new InvalidOperationException(
                        "Uninstall failed; all recorded file mutations were restored. " +
                        uninstallError.Message + " Recovery directory: " + backupRoot,
                        uninstallError);
                }
            }
        }

        private static InstallerResult InstallCore(
            string selectedGameExecutable,
            string confirmedPlanIdentity,
            InstallerHotkeys requestedHotkeys)
        {
            EnsureGameIsClosed();
            var gameExecutable = ValidateGameExecutableLocation(selectedGameExecutable);
            var gameHash = HashFile(gameExecutable);

            var manifest = ReadManifest();
            ValidateManifest(manifest);
            var experimentalPayload = LoadAndValidatePayload(
                manifest,
                "experimental",
                ExperimentalRuntimeZipResource);
            ValidatePayloadContract(experimentalPayload);
            ValidateExperimentalLoaderResources();

            var context = DetectLayout(gameExecutable);
            ValidateExistingUE4SS(context);
            PrepareExperimentalTarget(context);
            var payload = experimentalPayload;

            var modDirectory = Path.Combine(context.ModsDirectory, ModName);
            ValidateMigrationConflicts(context, modDirectory);
            byte[] visibilityBytes = null;
            byte[] diagnosticsBytes = null;
            byte[] hotkeyBytes = null;
            byte[] treasureOverrideBytes = null;
            var configurationSource = FindExistingRadarDirectory(context, modDirectory);
            PrepareUserConfiguration(
                configurationSource,
                payload,
                out visibilityBytes,
                out diagnosticsBytes,
                out hotkeyBytes,
                out treasureOverrideBytes);
            var originalHotkeyBytes = hotkeyBytes;
            hotkeyBytes = ApplyRequestedHotkeys(originalHotkeyBytes, requestedHotkeys);
            var modsTxtBytes = BuildControlledModsTxt(
                string.IsNullOrEmpty(context.MigrationModsTxtPath)
                    ? context.ModsTxtPath
                    : context.MigrationModsTxtPath);
            var legacyMarkers = FindOwnedLegacyRadarMarkers(context);

            if (confirmedPlanIdentity != null)
            {
                var currentIdentity = BuildInstallerPlan(
                    gameExecutable,
                    gameHash,
                    context,
                    modDirectory, configurationSource, originalHotkeyBytes, hotkeyBytes).IdentityToken;
                if (!string.Equals(
                        currentIdentity,
                        confirmedPlanIdentity,
                        StringComparison.Ordinal))
                {
                    throw new InvalidOperationException(
                        "The game, UE4SS layout, or hotkey configuration changed after the compatibility prompt. " +
                        "Setup stopped without changing files. Review the new plan and confirm it again.");
                }
            }

            EnsureGameIsClosed();
            var retainBackup = context.ConvertsUE4SS;
            var backupRoot = Path.Combine(
                context.Win64Directory,
                retainBackup
                    ? "UE4SS-" + GetBackupVersionLabel(context) + "-" +
                        DateTime.UtcNow.ToString("yyyyMMdd-HHmmss-fff", CultureInfo.InvariantCulture) +
                        "-Backup"
                    : ".dsnwr-transaction-" + Guid.NewGuid().ToString("N"));
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                backupRoot,
                "Native World Radar backup directory");

            EnsureGameIsClosed();
            using (var transaction = new InstallTransaction(
                context.Win64Directory,
                context.GameRootDirectory,
                backupRoot,
                retainBackup))
            {
                try
                {
                    ConversionSnapshot conversionSnapshot = null;
                    if (context.ConvertsUE4SS)
                    {
                        conversionSnapshot = CreateCompleteConversionSnapshot(context, backupRoot);
                        RemoveConvertedUE4SSLayout(context, transaction);
                    }
                    InstallExperimentalUE4SS(context, transaction);
                    MigrateExistingMods(
                        context,
                        transaction,
                        conversionSnapshot == null
                            ? context.MigrationModsDirectories
                            : conversionSnapshot.MigrationModsDirectories);

                    foreach (var existing in EnumerateTargetFiles(modDirectory))
                    {
                        transaction.DeleteFile(existing, "Remove owned previous Native World Radar file");
                    }

                    foreach (var entry in payload.Values.OrderBy(value => value.Path, StringComparer.OrdinalIgnoreCase))
                    {
                        if (IsInstallerOnlyDefault(entry.Path) || IsPreservedUserPayload(entry.Path))
                        {
                            continue;
                        }
                        transaction.WriteBytes(
                            Path.Combine(modDirectory, entry.Path.Replace('/', Path.DirectorySeparatorChar)),
                            entry.Bytes,
                            "Install source-bound Native World Radar payload");
                    }

                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "config", "visibility.ini"),
                        visibilityBytes,
                        "Install or preserve user visibility configuration");
                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "config", "diagnostics.ini"),
                        diagnosticsBytes,
                        "Install or preserve user diagnostics configuration");
                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "config", "hotkeys.ini"),
                        hotkeyBytes,
                        "Install or preserve user hotkey configuration");
                    transaction.WriteBytes(
                        Path.Combine(
                            modDirectory,
                            TreasureOverridesPath.Replace('/', Path.DirectorySeparatorChar)),
                        treasureOverrideBytes,
                        "Install or preserve user treasure-ignore configuration");

                    foreach (var marker in legacyMarkers)
                    {
                        transaction.DeleteFile(marker, "Remove owned legacy Radar enabled.txt bypass");
                    }
                    transaction.WriteBytes(
                        context.ModsTxtPath,
                        modsTxtBytes,
                        "Set one authoritative Native World Radar mods.txt entry");

                    var installRecord = BuildInstallRecord(
                        context,
                        gameHash,
                        payload["dlls/main.dll"].Sha256,
                        retainBackup ? backupRoot : null,
                        visibilityBytes,
                        diagnosticsBytes,
                        hotkeyBytes,
                        treasureOverrideBytes);
                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "INSTALL-RECORD.txt"),
                        new UTF8Encoding(false).GetBytes(installRecord),
                        "Write Native World Radar install record");

                    ThrowIfIntegrationTestFailure("after-recorded-mutations");

                    VerifyInstalledState(
                        context,
                        modDirectory,
                        payload,
                        visibilityBytes,
                        diagnosticsBytes,
                        hotkeyBytes,
                        treasureOverrideBytes);
                    VerifyExperimentalLoader(context);
                    var layoutDescription = "structurally complete ExperimentalNested UE4SS";
                    var loaderHash = HashFile(Path.Combine(context.NestedUE4SSDirectory, "UE4SS.dll"));
                    var proxyHash = HashFile(Path.Combine(context.Win64Directory, "dwmapi.dll"));
                    transaction.Commit(new[]
                    {
                        "DragonSword Native World Radar " + ProductVersion,
                        "UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture),
                        "Game executable: " + context.GameExecutable,
                        "Game SHA-256: " + gameHash,
                        "UE4SS layout: " + layoutDescription,
                        "UE4SS bootstrapped: " + context.BootstrappedUE4SS,
                        "UE4SS converted: " + context.ConvertsUE4SS,
                        "Observed UE4SS SHA-256 (provenance only): " + loaderHash,
                        "Observed proxy SHA-256 (provenance only): " + proxyHash,
                        "Mods directory: " + context.ModsDirectory,
                        "Controlling mods.txt: " + context.ModsTxtPath,
                        "Public diagnostics default: false",
                        "Installer signature: unsigned"
                    });

                    return new InstallerResult
                    {
                        LayoutDescription = layoutDescription,
                        BackupDirectory = retainBackup ? backupRoot : string.Empty,
                        ModDirectory = modDirectory,
                        ModsTxtPath = context.ModsTxtPath,
                        UpdatedExistingRadar = context.UpdatesExistingRadar,
                        Hotkeys = ReadHotkeys(hotkeyBytes)
                    };
                }
                catch (Exception installError)
                {
                    try
                    {
                        transaction.Rollback();
                    }
                    catch (Exception rollbackError)
                    {
                        throw new InvalidOperationException(
                            "Installation failed and automatic rollback was incomplete. Original error: " +
                            installError.Message + " Rollback error: " + rollbackError.Message +
                            " Recovery directory: " + backupRoot,
                            installError);
                    }
                    throw new InvalidOperationException(
                        "Installation failed; all recorded file mutations were restored. " +
                        installError.Message + " Recovery directory: " + backupRoot,
                        installError);
                }
            }
        }

        private static InstallerPlan BuildInstallerPlan(
            string gameExecutable,
            string gameHash,
            InstallContext context,
            string modDirectory,
            string configurationSource,
            byte[] originalHotkeyBytes,
            byte[] selectedHotkeyBytes)
        {
            var layoutDescription = context.ExistingLayoutDescription;
            var pluginDescription = "ExperimentalNested-compatible Radar DLL";
            var loaderHash = ExperimentalUE4SSHash;
            var proxyHash = ExperimentalDwmapiHash;
            var actionDescription = context.ConvertsUE4SS
                ? "Create and verify a complete original-layout backup of the detected UE4SS installation, migrate existing Mods and configuration into the pinned ExperimentalNested layout, remove the old active UE4SS layout, then install Radar. The old loader, Mods, and settings remain only in the backup. Third-party native DLL Mods may require compatible builds."
                : context.BootstrappedUE4SS
                    ? "Install the embedded, hash-verified pinned ExperimentalNested UE4SS build, then install Radar."
                    : context.UpdatesExistingRadar
                        ? "Update or repair Radar by replacing installer-owned binaries and generated catalogs, applying the confirmed hotkeys, and preserving visibility, diagnostics, and treasure-ignore settings. Unchanged hotkeys are preserved byte-for-byte. Keep the structurally complete ExperimentalNested UE4SS loader and proxy unchanged. The temporary rollback journal is removed after success."
                        : "Keep the structurally complete ExperimentalNested UE4SS loader and proxy unchanged, then install Radar. The temporary rollback journal is removed after success.";
            var identityText = string.Join("\n", new[]
            {
                gameExecutable,
                gameHash,
                context.Layout.ToString(),
                context.BootstrappedUE4SS ? "true" : "false",
                context.ConvertsUE4SS ? "true" : "false",
                context.ExistingLayoutDescription ?? string.Empty,
                loaderHash,
                proxyHash,
                FingerprintExistingUE4SS(context),
                context.UE4SSDirectory,
                context.ModsDirectory,
                context.ModsTxtPath,
                modDirectory,
                configurationSource,
                File.Exists(Path.Combine(configurationSource, "config", "hotkeys.ini")) ? "hotkeys-present" : "hotkeys-missing",
                HashBytes(originalHotkeyBytes),
                HashBytes(selectedHotkeyBytes)
            });

            return new InstallerPlan
            {
                IdentityToken = HashBytes(new UTF8Encoding(false).GetBytes(identityText)),
                LayoutDescription = layoutDescription,
                ActionDescription = actionDescription,
                PluginDescription = pluginDescription,
                LoaderSha256 = loaderHash,
                ProxySha256 = proxyHash,
                UE4SSDirectory = context.UE4SSDirectory,
                ModDirectory = modDirectory,
                ModsTxtPath = context.ModsTxtPath,
                BootstrapsUE4SS = context.BootstrappedUE4SS,
                ConvertsUE4SS = context.ConvertsUE4SS,
                UpdatesExistingRadar = context.UpdatesExistingRadar,
                Hotkeys = ReadHotkeys(selectedHotkeyBytes)
            };
        }

        private static InstallContext PrepareUninstallContext(
            string selectedGameExecutable,
            out string gameHash)
        {
            EnsureGameIsClosed();
            var gameExecutable = ValidateGameExecutableLocation(
                selectedGameExecutable);
            gameHash = HashFile(gameExecutable);
            var context = DetectLayout(gameExecutable);
            ValidateExistingUE4SS(context);
            if (context.BootstrappedUE4SS || context.ConvertsUE4SS)
            {
                throw new InvalidOperationException(
                    "A structurally complete ExperimentalNested UE4SS installation is required for automatic uninstall. " +
                    "Setup did not modify the detected layout.");
            }
            PrepareExperimentalTarget(context);
            var modDirectory = Path.Combine(context.ModsDirectory, ModName);
            if (!context.UpdatesExistingRadar || !Directory.Exists(modDirectory))
            {
                throw new InvalidOperationException(
                    "A strictly owned installed DragonSword Native World Radar was not found at the selected game path.");
            }
            ValidateExistingTargetOwnership(context, modDirectory);
            return context;
        }

        private static UninstallerPlan BuildUninstallerPlan(
            InstallContext context,
            string gameHash)
        {
            var modDirectory = Path.Combine(context.ModsDirectory, ModName);
            var identityText = string.Join("\n", new[]
            {
                context.GameExecutable,
                gameHash,
                context.ExistingLayoutDescription ?? string.Empty,
                context.UE4SSDirectory,
                context.ModsDirectory,
                context.ModsTxtPath,
                modDirectory,
                FingerprintExistingUE4SS(context),
                FingerprintOwnedTarget(context.GameRootDirectory, modDirectory),
                HashFile(context.ModsTxtPath)
            });
            return new UninstallerPlan
            {
                IdentityToken = HashBytes(
                    new UTF8Encoding(false).GetBytes(identityText)),
                LayoutDescription = context.ExistingLayoutDescription,
                ModDirectory = modDirectory,
                ModsTxtPath = context.ModsTxtPath
            };
        }

        private static void EnsureGameIsClosed()
        {
            var processes = Process.GetProcessesByName("DSClient-Win64-Shipping");
            try
            {
                if (processes.Length != 0)
                {
                    throw new InvalidOperationException(
                        "DragonSword Awakening is running. Close the game completely and run Setup again.");
                }
            }
            finally
            {
                foreach (var process in processes)
                {
                    process.Dispose();
                }
            }
        }

        private static void ThrowIfIntegrationTestFailure(string point)
        {
            if (string.Equals(
                IntegrationTestFailurePoint,
                point,
                StringComparison.Ordinal))
            {
                throw new InvalidOperationException(
                    "Injected installer integration-test failure at " + point + ".");
            }
        }

        private static string ValidateGameExecutableLocation(string selectedPath)
        {
            if (string.IsNullOrWhiteSpace(selectedPath))
            {
                throw new InvalidOperationException("Select DSClient-Win64-Shipping.exe first.");
            }
            var fullPath = Path.GetFullPath(
                Environment.ExpandEnvironmentVariables(selectedPath.Trim().Trim('"')));
            if (!File.Exists(fullPath))
            {
                throw new FileNotFoundException("The selected game executable does not exist.", fullPath);
            }
            if (!string.Equals(Path.GetFileName(fullPath), GameFileName, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException("The selected file must be " + GameFileName + ".");
            }
            var win64 = new DirectoryInfo(Path.GetDirectoryName(fullPath));
            if (!string.Equals(win64.Name, "Win64", StringComparison.OrdinalIgnoreCase) ||
                win64.Parent == null ||
                !string.Equals(win64.Parent.Name, "Binaries", StringComparison.OrdinalIgnoreCase) ||
                win64.Parent.Parent == null ||
                !string.Equals(win64.Parent.Parent.Name, "DS", StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(
                    "The executable must be in DS\\Binaries\\Win64. The selected directory is not supported.");
            }
            InstallerPathSafety.Validate(
                win64.Parent.Parent.FullName,
                fullPath,
                "selected DragonSword executable");
            ValidateGameExecutableImage(fullPath);
            return fullPath;
        }

        private static void ValidateGameExecutableImage(string path)
        {
            const long maximumGameImageBytes = 512L * 1024L * 1024L;
            var item = new FileInfo(path);
            if (item.Length < 256 || item.Length > maximumGameImageBytes)
            {
                throw new InvalidDataException(
                    "The selected game executable is not a bounded x64 PE32+ image.");
            }
            using (var stream = new FileStream(
                path, FileMode.Open, FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete))
            {
                var dos = ReadExactly(stream, 64);
                if (dos[0] != 0x4D || dos[1] != 0x5A)
                {
                    throw new InvalidDataException(
                        "The selected game executable is not a valid x64 PE32+ image.");
                }
                var peOffset = BitConverter.ToInt32(dos, 0x3C);
                if (peOffset < 64 || peOffset > item.Length - 84)
                {
                    throw new InvalidDataException(
                        "The selected game executable is not a valid x64 PE32+ image.");
                }
                stream.Position = peOffset;
                var header = ReadExactly(stream, 84);
                var signature = BitConverter.ToUInt32(header, 0);
                var machine = BitConverter.ToUInt16(header, 4);
                var sectionCount = BitConverter.ToUInt16(header, 6);
                var optionalSize = BitConverter.ToUInt16(header, 20);
                var characteristics = BitConverter.ToUInt16(header, 22);
                var optionalMagic = BitConverter.ToUInt16(header, 24);
                var sizeOfImage = BitConverter.ToUInt32(header, 80);
                if (signature != 0x00004550 || machine != 0x8664
                    || sectionCount == 0 || sectionCount > 96
                    || optionalSize < 60 || optionalMagic != 0x020B
                    || sizeOfImage == 0
                    || peOffset + 24L + optionalSize
                        + sectionCount * 40L > item.Length
                    || (characteristics & 0x0002) == 0
                    || (characteristics & 0x2000) != 0)
                {
                    throw new InvalidDataException(
                        "The selected game executable is not a valid x64 PE32+ game image.");
                }
            }
        }

        private static byte[] ReadExactly(Stream stream, int count)
        {
            var bytes = new byte[count];
            var offset = 0;
            while (offset < count)
            {
                var read = stream.Read(bytes, offset, count - offset);
                if (read <= 0)
                {
                    throw new InvalidDataException(
                        "The selected game executable has a truncated PE header.");
                }
                offset += read;
            }
            return bytes;
        }

        private static InstallContext DetectLayout(string gameExecutable)
        {
            var win64 = Path.GetDirectoryName(gameExecutable);
            var stableDll = Path.Combine(win64, "UE4SS.dll");
            var nestedDirectory = Path.Combine(win64, "ue4ss");
            var nestedDll = Path.Combine(nestedDirectory, "UE4SS.dll");
            var stableExists = File.Exists(stableDll);
            var nestedExists = File.Exists(nestedDll);
            var proxy = Path.Combine(win64, "dwmapi.dll");
            var proxyExists = File.Exists(proxy);
            var nestedHasAnyEntry = Directory.Exists(nestedDirectory) &&
                Directory.EnumerateFileSystemEntries(nestedDirectory).Any();
            var legacyRootFiles = Directory.GetFiles(
                    win64,
                    "UE4SS*",
                    SearchOption.TopDirectoryOnly)
                .Any();
            var legacyRootLayout = legacyRootFiles ||
                Directory.Exists(Path.Combine(win64, "Mods")) ||
                Directory.Exists(Path.Combine(win64, "CXXHeaderDump"));

            var context = new InstallContext
            {
                GameExecutable = gameExecutable,
                GameRootDirectory = Path.GetFullPath(Path.Combine(win64, "..", "..")),
                Win64Directory = win64,
                StableUE4SSDirectory = win64,
                NestedUE4SSDirectory = nestedDirectory,
                UE4SSDirectory = nestedDirectory,
                ModsDirectory = Path.Combine(nestedDirectory, "Mods"),
                ModsTxtPath = Path.Combine(nestedDirectory, "Mods", "mods.txt"),
                Layout = UE4SSLayout.ExperimentalNested
            };

            var nestedSettings = Path.Combine(nestedDirectory, "UE4SS-settings.ini");
            var nestedMods = Path.Combine(nestedDirectory, "Mods");
            var nestedModsTxt = Path.Combine(nestedMods, "mods.txt");
            var structuralExperimental = nestedExists && proxyExists &&
                File.Exists(nestedSettings) && Directory.Exists(nestedMods) &&
                File.Exists(nestedModsTxt) &&
                IsStructurallyValidX64Dll(nestedDll) &&
                IsStructurallyValidX64Dll(proxy);
            var anyUE4SS = stableExists || nestedExists || nestedHasAnyEntry || proxyExists ||
                Directory.Exists(Path.Combine(win64, "Mods")) ||
                File.Exists(Path.Combine(win64, "UE4SS-settings.ini"));

            context.BootstrappedUE4SS = !anyUE4SS;
            context.ConvertsUE4SS = anyUE4SS && (!structuralExperimental || legacyRootLayout);
            if (structuralExperimental && !legacyRootLayout)
            {
                context.ExistingLayoutDescription =
                    "structurally complete ExperimentalNested UE4SS";
            }
            else if (!anyUE4SS)
            {
                context.ExistingLayoutDescription = "no UE4SS installation detected";
            }
            else
            {
                var parts = new List<string>();
                if (stableExists) parts.Add("root UE4SS.dll");
                if (nestedExists) parts.Add("nested UE4SS.dll");
                if (proxyExists) parts.Add("Win64 proxy");
                if (legacyRootLayout) parts.Add("legacy root UE4SS or Mods files");
                if (nestedHasAnyEntry && !nestedExists) parts.Add("incomplete nested ue4ss directory");
                context.ExistingLayoutDescription =
                    "different or incomplete UE4SS installation (" + string.Join(", ", parts) + ")";
            }

            AddMigrationDirectory(context, Path.Combine(win64, "Mods"));
            AddMigrationDirectory(context, Path.Combine(nestedDirectory, "Mods"));
            context.MigrationModsTxtPath = FirstExistingFile(new[]
            {
                Path.Combine(nestedDirectory, "Mods", "mods.txt"),
                Path.Combine(win64, "Mods", "mods.txt")
            });
            context.MigrationSettingsPath = FirstExistingFile(new[]
            {
                Path.Combine(nestedDirectory, "UE4SS-settings.ini"),
                Path.Combine(win64, "UE4SS-settings.ini")
            });
            return context;
        }

        private static bool IsStructurallyValidX64Dll(string path)
        {
            const long maximumDllBytes = 256L * 1024L * 1024L;
            try
            {
                var item = new FileInfo(path);
                if (!item.Exists || item.Length < 256 || item.Length > maximumDllBytes)
                {
                    return false;
                }
                using (var stream = new FileStream(
                    path, FileMode.Open, FileAccess.Read,
                    FileShare.ReadWrite | FileShare.Delete))
                {
                    var dos = ReadExactly(stream, 64);
                    if (dos[0] != 0x4D || dos[1] != 0x5A)
                    {
                        return false;
                    }
                    var peOffset = BitConverter.ToInt32(dos, 0x3C);
                    if (peOffset < 64 || peOffset > item.Length - 84)
                    {
                        return false;
                    }
                    stream.Position = peOffset;
                    var header = ReadExactly(stream, 84);
                    var signature = BitConverter.ToUInt32(header, 0);
                    var machine = BitConverter.ToUInt16(header, 4);
                    var sectionCount = BitConverter.ToUInt16(header, 6);
                    var optionalSize = BitConverter.ToUInt16(header, 20);
                    var characteristics = BitConverter.ToUInt16(header, 22);
                    var optionalMagic = BitConverter.ToUInt16(header, 24);
                    var sizeOfImage = BitConverter.ToUInt32(header, 80);
                    return signature == 0x00004550 && machine == 0x8664 &&
                        sectionCount > 0 && sectionCount <= 96 &&
                        optionalSize >= 60 && optionalMagic == 0x020B &&
                        sizeOfImage != 0 &&
                        peOffset + 24L + optionalSize + sectionCount * 40L <= item.Length &&
                        (characteristics & 0x0002) != 0 &&
                        (characteristics & 0x2000) != 0;
                }
            }
            catch
            {
                return false;
            }
        }

        private static void ValidateExistingUE4SS(InstallContext context)
        {
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                context.Win64Directory,
                "DragonSword Win64 directory");

            if (Directory.Exists(context.NestedUE4SSDirectory) &&
                (new DirectoryInfo(context.NestedUE4SSDirectory).Attributes & FileAttributes.ReparsePoint) != 0)
            {
                throw new InvalidOperationException(
                    "The nested ue4ss directory is a reparse point. Setup will not inspect or modify it.");
            }
            foreach (var source in context.MigrationModsDirectories)
            {
                InstallerPathSafety.ValidateTree(
                    context.GameRootDirectory,
                    source,
                    "existing UE4SS Mods directory selected for migration");
            }
        }

        private static void PrepareExperimentalTarget(InstallContext context)
        {
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                context.UE4SSDirectory,
                "ExperimentalNested UE4SS directory");
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                context.ModsDirectory,
                "ExperimentalNested Mods directory");
            var settingsPath = Path.Combine(context.NestedUE4SSDirectory, "UE4SS-settings.ini");
            if (!context.ConvertsUE4SS && !context.BootstrappedUE4SS && File.Exists(settingsPath))
            {
                ResolveConfiguredModPaths(context);
                ValidateCompetingLoadRoots(context);
                context.MigrationModsTxtPath = context.ModsTxtPath;
            }
            var modDirectory = Path.Combine(context.ModsDirectory, ModName);
            if (Directory.Exists(modDirectory))
            {
                ValidateExistingTargetOwnership(context, modDirectory);
                context.UpdatesExistingRadar =
                    !context.ConvertsUE4SS && !context.BootstrappedUE4SS &&
                    Directory.GetFiles(modDirectory, "*", SearchOption.AllDirectories).Length != 0;
            }
        }

        private static void ValidateExperimentalLoaderResources()
        {
            ValidateResourceHash(ExperimentalUE4SSResource, ExperimentalUE4SSHash);
            ValidateResourceHash(ExperimentalDwmapiResource, ExperimentalDwmapiHash);
            ValidateResourceHash(ExperimentalUsmapResource, ExperimentalUsmapHash);
            ValidateResourceHash(ExperimentalSettingsResource, ExperimentalSettingsHash);
        }

        private static void ValidateResourceHash(string resource, string expected)
        {
            var actual = HashBytes(ReadResource(resource));
            if (!string.Equals(actual, expected, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException(
                    "The embedded Experimental UE4SS resource failed its Setup integrity check: " + resource);
            }
        }

        private static void InstallExperimentalUE4SS(
            InstallContext context,
            InstallTransaction transaction)
        {
            if (context.ConvertsUE4SS || context.BootstrappedUE4SS)
            {
                transaction.WriteBytes(
                    Path.Combine(context.NestedUE4SSDirectory, "UE4SS.dll"),
                    ReadResource(ExperimentalUE4SSResource),
                    "Install bundled ExperimentalNested UE4SS.dll");
                transaction.WriteBytes(
                    Path.Combine(context.Win64Directory, "dwmapi.dll"),
                    ReadResource(ExperimentalDwmapiResource),
                    "Install bundled ExperimentalNested proxy");
            }
            transaction.WriteBytes(
                Path.Combine(context.NestedUE4SSDirectory, ExperimentalUsmapName),
                ReadResource(ExperimentalUsmapResource),
                "Install bundled DragonSword mapping file");
            var settingsPath = Path.Combine(context.NestedUE4SSDirectory, "UE4SS-settings.ini");
            if (context.ConvertsUE4SS || context.BootstrappedUE4SS || !File.Exists(settingsPath))
            {
                transaction.WriteBytes(
                    settingsPath,
                    ReadResource(ExperimentalSettingsResource),
                    "Install tested ExperimentalNested UE4SS settings");
            }
            transaction.DeleteFile(
                Path.Combine(context.Win64Directory, "UE4SS.dll"),
                "Deactivate incompatible root UE4SS.dll after backup");
        }

        private static void VerifyExperimentalLoader(InstallContext context)
        {
            var loader = Path.Combine(context.NestedUE4SSDirectory, "UE4SS.dll");
            var proxy = Path.Combine(context.Win64Directory, "dwmapi.dll");
            if (!IsStructurallyValidX64Dll(loader) || !IsStructurallyValidX64Dll(proxy))
            {
                throw new InvalidOperationException(
                    "The installed ExperimentalNested UE4SS structure is incomplete or is not x64 PE32+.");
            }
            var usmap = Path.Combine(context.NestedUE4SSDirectory, ExperimentalUsmapName);
            if (!File.Exists(usmap) ||
                !string.Equals(HashFile(usmap), ExperimentalUsmapHash, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(
                    "The bundled DragonSword mapping file failed installation verification.");
            }
            if (File.Exists(Path.Combine(context.Win64Directory, "UE4SS.dll")))
            {
                throw new InvalidOperationException("The incompatible root UE4SS.dll remained active after conversion.");
            }
        }

        private static void AddMigrationDirectory(InstallContext context, string path)
        {
            if (!Directory.Exists(path))
            {
                return;
            }
            var full = Path.GetFullPath(path);
            if (!context.MigrationModsDirectories.Contains(full, StringComparer.OrdinalIgnoreCase))
            {
                context.MigrationModsDirectories.Add(full);
            }
        }

        private static string FirstExistingFile(IEnumerable<string> candidates)
        {
            return candidates.FirstOrDefault(File.Exists);
        }

        private static string FindExistingRadarDirectory(InstallContext context, string target)
        {
            if (Directory.Exists(target))
            {
                return target;
            }
            foreach (var source in context.MigrationModsDirectories)
            {
                var candidate = Path.Combine(source, ModName);
                if (Directory.Exists(candidate))
                {
                    ValidateExistingTargetOwnership(context, candidate);
                    return candidate;
                }
            }
            return target;
        }

        private static void ValidateMigrationConflicts(InstallContext context, string radarTarget)
        {
            var selected = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (var source in context.MigrationModsDirectories)
            {
                if (!Directory.Exists(source))
                {
                    continue;
                }
                foreach (var file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
                {
                    var relative = file.Substring(source.TrimEnd(Path.DirectorySeparatorChar).Length + 1);
                    if (string.Equals(relative, "mods.txt", StringComparison.OrdinalIgnoreCase) ||
                        relative.StartsWith(ModName + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }
                    string existingSource;
                    if (selected.TryGetValue(relative, out existingSource) &&
                        !string.Equals(HashFile(existingSource), HashFile(file), StringComparison.OrdinalIgnoreCase))
                    {
                        throw new InvalidOperationException(
                            "UE4SS conversion found two different files for the same Mod path: " + relative +
                            ". Setup stopped before changing files so it does not guess which copy to keep.");
                    }
                    selected[relative] = file;

                    var destination = Path.Combine(context.ModsDirectory, relative);
                    if (!string.Equals(Path.GetFullPath(file), Path.GetFullPath(destination), StringComparison.OrdinalIgnoreCase) &&
                        File.Exists(destination) &&
                        !string.Equals(HashFile(file), HashFile(destination), StringComparison.OrdinalIgnoreCase))
                    {
                        throw new InvalidOperationException(
                            "UE4SS conversion would overwrite a different existing Mod file: " + destination +
                            ". Setup stopped before changing files.");
                    }
                }
            }
        }

        private static void MigrateExistingMods(
            InstallContext context,
            InstallTransaction transaction,
            IEnumerable<string> migrationSources)
        {
            foreach (var source in migrationSources)
            {
                if (!Directory.Exists(source) ||
                    string.Equals(Path.GetFullPath(source), Path.GetFullPath(context.ModsDirectory), StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
                foreach (var file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
                {
                    var relative = file.Substring(source.TrimEnd(Path.DirectorySeparatorChar).Length + 1);
                    if (string.Equals(relative, "mods.txt", StringComparison.OrdinalIgnoreCase) ||
                        relative.StartsWith(ModName + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }
                    var destination = Path.Combine(context.ModsDirectory, relative);
                    if (File.Exists(destination) &&
                        string.Equals(HashFile(file), HashFile(destination), StringComparison.OrdinalIgnoreCase))
                    {
                        continue;
                    }
                    transaction.WriteBytes(
                        destination,
                        File.ReadAllBytes(file),
                        "Migrate existing UE4SS Mod file into ExperimentalNested layout");
                }
            }
        }

        private static ConversionSnapshot CreateCompleteConversionSnapshot(
            InstallContext context,
            string backupRoot)
        {
            var snapshot = new ConversionSnapshot
            {
                SnapshotWin64Directory = backupRoot
            };
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                snapshot.SnapshotWin64Directory,
                "complete original-layout UE4SS snapshot");

            var copiedPairs = new List<KeyValuePair<string, string>>();
            var rootFiles = Directory.GetFiles(
                    context.Win64Directory,
                    "UE4SS*",
                    SearchOption.TopDirectoryOnly)
                .Concat(new[]
                {
                    Path.Combine(context.Win64Directory, "dwmapi.dll"),
                    Path.Combine(context.Win64Directory, ExperimentalUsmapName)
                })
                .Where(File.Exists)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .OrderBy(value => value, StringComparer.OrdinalIgnoreCase)
                .ToArray();
            foreach (var source in rootFiles)
            {
                var destination = Path.Combine(
                    snapshot.SnapshotWin64Directory,
                    Path.GetFileName(source));
                CopySnapshotFile(context, source, destination, copiedPairs);
            }

            var rootDirectories = new[]
            {
                Path.Combine(context.Win64Directory, "Mods"),
                Path.Combine(context.Win64Directory, "CXXHeaderDump"),
                context.NestedUE4SSDirectory
            };
            foreach (var source in rootDirectories
                .Where(Directory.Exists)
                .Distinct(StringComparer.OrdinalIgnoreCase))
            {
                var destination = Path.Combine(
                    snapshot.SnapshotWin64Directory,
                    Path.GetFileName(source.TrimEnd(Path.DirectorySeparatorChar)));
                CopySnapshotDirectory(context, source, destination, copiedPairs);
            }

            if (copiedPairs.Count == 0)
            {
                throw new InvalidOperationException(
                    "UE4SS conversion was requested, but no active UE4SS files were available for the required complete backup.");
            }
            foreach (var pair in copiedPairs)
            {
                if (!File.Exists(pair.Value) ||
                    new FileInfo(pair.Value).Length != new FileInfo(pair.Key).Length ||
                    !string.Equals(HashFile(pair.Key), HashFile(pair.Value), StringComparison.OrdinalIgnoreCase))
                {
                    throw new IOException(
                        "The complete UE4SS backup failed byte verification: " + pair.Key);
                }
            }

            var rootModsSnapshot = Path.Combine(snapshot.SnapshotWin64Directory, "Mods");
            if (Directory.Exists(rootModsSnapshot))
            {
                snapshot.MigrationModsDirectories.Add(rootModsSnapshot);
            }
            var nestedModsSnapshot = Path.Combine(snapshot.SnapshotWin64Directory, "ue4ss", "Mods");
            if (Directory.Exists(nestedModsSnapshot))
            {
                snapshot.MigrationModsDirectories.Add(nestedModsSnapshot);
            }

            var manifest = new List<string>
            {
                "Complete UE4SS conversion backup",
                "Created UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture),
                "Original Win64 directory: " + context.Win64Directory,
                "Snapshot Win64 directory: " + snapshot.SnapshotWin64Directory,
                "Verified file count: " + copiedPairs.Count.ToString(CultureInfo.InvariantCulture),
                string.Empty,
                "Every listed file was copied and SHA-256 verified before the old active UE4SS layout was removed."
            };
            manifest.AddRange(copiedPairs
                .OrderBy(pair => pair.Key, StringComparer.OrdinalIgnoreCase)
                .Select(pair =>
                    pair.Key.Substring(
                        context.Win64Directory.TrimEnd(Path.DirectorySeparatorChar).Length + 1) +
                    " | " + HashFile(pair.Value)));
            var manifestPath = Path.Combine(backupRoot, "COMPLETE-UE4SS-BACKUP.txt");
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                manifestPath,
                "complete UE4SS backup manifest");
            File.WriteAllLines(manifestPath, manifest, new UTF8Encoding(false));
            return snapshot;
        }

        private static string GetBackupVersionLabel(InstallContext context)
        {
            var rootLoader = Path.Combine(context.Win64Directory, "UE4SS.dll");
            var nestedLoader = Path.Combine(context.NestedUE4SSDirectory, "UE4SS.dll");
            if (File.Exists(rootLoader))
            {
                var rootHash = HashFile(rootLoader);
                return string.Equals(
                        rootHash,
                        KnownStableRootUE4SS301HashForBackupLabel,
                        StringComparison.OrdinalIgnoreCase)
                    ? "v3.0.1-StableRoot"
                    : "Root-" + rootHash.Substring(0, 8);
            }
            if (File.Exists(nestedLoader))
            {
                var nestedHash = HashFile(nestedLoader);
                return string.Equals(
                        nestedHash,
                        ExperimentalUE4SSHash,
                        StringComparison.OrdinalIgnoreCase)
                    ? "v3.0.1-ExperimentalNested-1c1a149"
                    : "Nested-" + nestedHash.Substring(0, 8);
            }
            return "DetectedLayout";
        }

        private static void CopySnapshotFile(
            InstallContext context,
            string source,
            string destination,
            ICollection<KeyValuePair<string, string>> copiedPairs)
        {
            InstallerPathSafety.Validate(context.GameRootDirectory, source, "UE4SS backup source file");
            InstallerPathSafety.Validate(context.GameRootDirectory, destination, "UE4SS backup destination file");
            Directory.CreateDirectory(Path.GetDirectoryName(destination));
            InstallerPathSafety.Validate(context.GameRootDirectory, destination, "UE4SS backup destination file");
            File.Copy(source, destination, false);
            copiedPairs.Add(new KeyValuePair<string, string>(source, destination));
        }

        private static void CopySnapshotDirectory(
            InstallContext context,
            string source,
            string destination,
            ICollection<KeyValuePair<string, string>> copiedPairs)
        {
            InstallerPathSafety.ValidateTree(
                context.GameRootDirectory,
                source,
                "UE4SS directory selected for complete conversion backup");
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                destination,
                "UE4SS backup destination directory");
            Directory.CreateDirectory(destination);
            foreach (var directory in Directory.GetDirectories(source, "*", SearchOption.AllDirectories))
            {
                var relative = directory.Substring(source.TrimEnd(Path.DirectorySeparatorChar).Length + 1);
                var target = Path.Combine(destination, relative);
                InstallerPathSafety.Validate(
                    context.GameRootDirectory,
                    target,
                    "UE4SS backup destination directory");
                Directory.CreateDirectory(target);
            }
            foreach (var file in Directory.GetFiles(source, "*", SearchOption.AllDirectories))
            {
                var relative = file.Substring(source.TrimEnd(Path.DirectorySeparatorChar).Length + 1);
                CopySnapshotFile(context, file, Path.Combine(destination, relative), copiedPairs);
            }
        }

        private static void RemoveConvertedUE4SSLayout(
            InstallContext context,
            InstallTransaction transaction)
        {
            foreach (var file in Directory.GetFiles(
                    context.Win64Directory,
                    "UE4SS*",
                    SearchOption.TopDirectoryOnly)
                .Concat(new[] { Path.Combine(context.Win64Directory, ExperimentalUsmapName) })
                .Where(File.Exists)
                .Distinct(StringComparer.OrdinalIgnoreCase))
            {
                transaction.DeleteFile(file, "Remove backed-up old root UE4SS file");
            }
            transaction.DeleteDirectoryTree(
                Path.Combine(context.Win64Directory, "Mods"),
                "Remove backed-up old root Mods layout");
            transaction.DeleteDirectoryTree(
                Path.Combine(context.Win64Directory, "CXXHeaderDump"),
                "Remove backed-up old root CXXHeaderDump layout");
            transaction.DeleteDirectoryTree(
                context.NestedUE4SSDirectory,
                "Remove backed-up old nested UE4SS layout");
        }

        private static string FingerprintExistingUE4SS(InstallContext context)
        {
            var paths = new List<string>
            {
                Path.Combine(context.Win64Directory, "UE4SS.dll"),
                Path.Combine(context.Win64Directory, "dwmapi.dll"),
                Path.Combine(context.NestedUE4SSDirectory, "UE4SS.dll"),
                Path.Combine(context.NestedUE4SSDirectory, "UE4SS-settings.ini"),
                context.MigrationModsTxtPath ?? string.Empty
            };
            var values = paths.Select(path => string.IsNullOrEmpty(path)
                ? "missing"
                : path + "=" + (File.Exists(path) ? HashFile(path) : "missing"));
            return HashBytes(new UTF8Encoding(false).GetBytes(string.Join("\n", values)));
        }

        private static string FingerprintUE4SSLoaderState(
            InstallContext context)
        {
            var paths = new[]
            {
                Path.Combine(context.Win64Directory, "UE4SS.dll"),
                Path.Combine(context.Win64Directory, "dwmapi.dll"),
                Path.Combine(context.NestedUE4SSDirectory, "UE4SS.dll"),
                Path.Combine(context.NestedUE4SSDirectory, "UE4SS-settings.ini")
            };
            var values = paths.Select(path =>
                path + "=" + (File.Exists(path) ? HashFile(path) : "missing"));
            return HashBytes(
                new UTF8Encoding(false).GetBytes(string.Join("\n", values)));
        }

        private static string FingerprintOwnedTarget(
            string gameRootDirectory,
            string modDirectory)
        {
            InstallerPathSafety.ValidateTree(
                gameRootDirectory,
                modDirectory,
                "owned Native World Radar uninstall target");
            var root = Path.GetFullPath(modDirectory)
                .TrimEnd(Path.DirectorySeparatorChar);
            var prefix = root + Path.DirectorySeparatorChar;
            var values = Directory.GetFiles(
                    root,
                    "*",
                    SearchOption.AllDirectories)
                .OrderBy(value => value, StringComparer.OrdinalIgnoreCase)
                .Select(path =>
                {
                    InstallerPathSafety.Validate(
                        gameRootDirectory,
                        path,
                        "owned Native World Radar uninstall file");
                    var item = new FileInfo(path);
                    return path.Substring(prefix.Length).Replace('\\', '/') +
                        "=" + item.Length.ToString(CultureInfo.InvariantCulture) +
                        ":" + HashFile(path);
                });
            return HashBytes(
                new UTF8Encoding(false).GetBytes(string.Join("\n", values)));
        }

        private static Dictionary<string, string> ReadManifest()
        {
            var text = new UTF8Encoding(false, true).GetString(ReadResource(ManifestResource));
            var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (var original in text.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
            {
                var line = original.Trim();
                if (line.Length == 0 || line.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }
                var separator = line.IndexOf('=');
                if (separator <= 0)
                {
                    throw new InvalidDataException("The embedded installer manifest contains a malformed line.");
                }
                var key = line.Substring(0, separator).Trim();
                var value = line.Substring(separator + 1).Trim();
                if (key.Length == 0 || value.Length == 0 || result.ContainsKey(key))
                {
                    throw new InvalidDataException("The embedded installer manifest contains an empty or duplicate key: " + key);
                }
                result.Add(key, value);
            }
            return result;
        }

        private static void ValidateManifest(IDictionary<string, string> manifest)
        {
            var required = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                "version",
                "runtime_label",
                "game_compatibility_policy",
                "experimental_ue4ss_sha256",
                "experimental_dwmapi_sha256",
                "experimental_runtime_zip_sha256",
                "experimental_runtime_file_count",
                "third_party_notices_sha256"
            };
            RequireManifestValue(manifest, "version", ProductVersion);
            RequireManifestValue(manifest, "runtime_label", RuntimeLabel);
            RequireManifestValue(
                manifest, "game_compatibility_policy", GameCompatibilityPolicy);
            RequireManifestValue(manifest, "experimental_ue4ss_sha256", ExperimentalUE4SSHash);
            RequireManifestValue(manifest, "experimental_dwmapi_sha256", ExperimentalDwmapiHash);
            RequireSha256(manifest, "experimental_runtime_zip_sha256");
            RequireSha256(manifest, "third_party_notices_sha256");

            ValidateRuntimeManifest(manifest, required, "experimental");
            var unexpected = manifest.Keys.Where(key => !required.Contains(key)).ToArray();
            if (unexpected.Length != 0 || manifest.Count != required.Count)
            {
                throw new InvalidDataException(
                    "The embedded installer manifest contains an unexpected key set: " +
                    string.Join(", ", unexpected));
            }
        }

        private static void ValidateRuntimeManifest(
            IDictionary<string, string> manifest,
            ISet<string> required,
            string variant)
        {
            var fileCountKey = variant + "_runtime_file_count";
            int fileCount;
            if (!int.TryParse(manifest[fileCountKey], NumberStyles.None, CultureInfo.InvariantCulture, out fileCount) ||
                fileCount <= 0 || fileCount > MaximumPayloadFiles)
            {
                throw new InvalidDataException(
                    "The embedded " + variant + " runtime file count is invalid.");
            }
            var paths = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            long totalSize = 0;
            for (var index = 0; index < fileCount; ++index)
            {
                var prefix = variant + "_file_" +
                    index.ToString("D3", CultureInfo.InvariantCulture) + "_";
                var pathKey = prefix + "path";
                var sizeKey = prefix + "size";
                var hashKey = prefix + "sha256";
                required.Add(pathKey);
                required.Add(sizeKey);
                required.Add(hashKey);
                if (!manifest.ContainsKey(pathKey) || !manifest.ContainsKey(sizeKey) || !manifest.ContainsKey(hashKey))
                {
                    throw new InvalidDataException("The embedded runtime file manifest is incomplete at index " + index + ".");
                }
                var path = NormalizePayloadPath(manifest[pathKey]);
                if (!paths.Add(path))
                {
                    throw new InvalidDataException("The embedded runtime file manifest repeats a path: " + path);
                }
                long size;
                if (!long.TryParse(manifest[sizeKey], NumberStyles.None, CultureInfo.InvariantCulture, out size) ||
                    size < 0 || size > MaximumPayloadBytes)
                {
                    throw new InvalidDataException("The embedded runtime file size is invalid: " + path);
                }
                checked { totalSize += size; }
                RequireSha256(manifest, hashKey);
            }
            if (totalSize > MaximumPayloadBytes)
            {
                throw new InvalidDataException(
                    "The embedded " + variant + " runtime payload exceeds the bounded release size.");
            }
        }

        private static Dictionary<string, PayloadEntry> LoadAndValidatePayload(
            IDictionary<string, string> manifest,
            string variant,
            string resourceName)
        {
            var zipBytes = ReadResource(resourceName);
            var zipHashKey = variant + "_runtime_zip_sha256";
            if (!string.Equals(HashBytes(zipBytes), manifest[zipHashKey], StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException(
                    "The embedded " + variant + " runtime ZIP does not match its release manifest.");
            }
            var notices = ReadResource(NoticesResource);
            if (!string.Equals(HashBytes(notices), manifest["third_party_notices_sha256"], StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException("The embedded third-party notices do not match the release manifest.");
            }

            var expected = new Dictionary<string, PayloadEntry>(StringComparer.OrdinalIgnoreCase);
            var fileCount = int.Parse(
                manifest[variant + "_runtime_file_count"],
                CultureInfo.InvariantCulture);
            for (var index = 0; index < fileCount; ++index)
            {
                var prefix = variant + "_file_" +
                    index.ToString("D3", CultureInfo.InvariantCulture) + "_";
                var path = NormalizePayloadPath(manifest[prefix + "path"]);
                expected.Add(path, new PayloadEntry
                {
                    Path = path,
                    Size = long.Parse(manifest[prefix + "size"], CultureInfo.InvariantCulture),
                    Sha256 = manifest[prefix + "sha256"].ToUpperInvariant()
                });
            }

            var actual = new Dictionary<string, PayloadEntry>(StringComparer.OrdinalIgnoreCase);
            using (var memory = new MemoryStream(zipBytes, false))
            using (var archive = new ZipArchive(memory, ZipArchiveMode.Read, false))
            {
                foreach (var zipEntry in archive.Entries)
                {
                    if (string.IsNullOrEmpty(zipEntry.Name))
                    {
                        throw new InvalidDataException(
                            "The embedded runtime ZIP contains an unexpected directory entry: " + zipEntry.FullName);
                    }
                    var path = NormalizePayloadPath(zipEntry.FullName);
                    if (actual.ContainsKey(path))
                    {
                        throw new InvalidDataException("The embedded runtime ZIP repeats a path: " + path);
                    }
                    PayloadEntry contract;
                    if (!expected.TryGetValue(path, out contract))
                    {
                        throw new InvalidDataException("The embedded runtime ZIP contains an unexpected file: " + path);
                    }
                    if (zipEntry.Length != contract.Size)
                    {
                        throw new InvalidDataException("The embedded runtime ZIP size does not match: " + path);
                    }
                    byte[] bytes;
                    using (var stream = zipEntry.Open())
                    using (var output = new MemoryStream())
                    {
                        stream.CopyTo(output);
                        bytes = output.ToArray();
                    }
                    if (bytes.LongLength != contract.Size ||
                        !string.Equals(HashBytes(bytes), contract.Sha256, StringComparison.OrdinalIgnoreCase))
                    {
                        throw new InvalidDataException("The embedded runtime ZIP hash does not match: " + path);
                    }
                    contract.Bytes = bytes;
                    actual.Add(path, contract);
                }
            }
            if (actual.Count != expected.Count || expected.Keys.Any(path => !actual.ContainsKey(path)))
            {
                throw new InvalidDataException("The embedded runtime ZIP is missing one or more manifest files.");
            }
            return actual;
        }

        private static void ValidatePayloadContract(IDictionary<string, PayloadEntry> payload)
        {
            var required = new[]
            {
                "dlls/main.dll",
                "config/visibility.example.ini",
                "config/hotkeys.example.ini",
                "config/diagnostics.example.ini",
                "metadata/release.json",
                "metadata/native-build-lock.json",
                "metadata/native-build-receipt.json",
                "metadata/package-manifest.json",
                "THIRD_PARTY_NOTICES.txt",
                "README.txt"
            };
            foreach (var path in required)
            {
                if (!payload.ContainsKey(path))
                {
                    throw new InvalidDataException("The runtime payload is missing a required release file: " + path);
                }
            }
            foreach (var path in payload.Keys)
            {
                var lower = path.ToLowerInvariant();
                if (lower.EndsWith("/enabled.txt", StringComparison.Ordinal) ||
                    string.Equals(lower, "enabled.txt", StringComparison.Ordinal) ||
                    string.Equals(lower, "config/visibility.ini", StringComparison.Ordinal) ||
                    string.Equals(lower, "config/hotkeys.ini", StringComparison.Ordinal) ||
                    string.Equals(lower, "config/diagnostics.ini", StringComparison.Ordinal) ||
                    lower.StartsWith("runtime/logs/", StringComparison.Ordinal) ||
                    lower.StartsWith("runtime/backups/", StringComparison.Ordinal) ||
                    lower.StartsWith("runtime/diagnostics/", StringComparison.Ordinal))
                {
                    throw new InvalidDataException("A forbidden local-state path entered the public runtime payload: " + path);
                }
            }

            AssertX64Plugin(payload["dlls/main.dll"].Bytes);
            var releaseText = new UTF8Encoding(false, true).GetString(payload["metadata/release.json"].Bytes);
            var releaseRoot = DeserializeJsonObject(
                releaseText,
                "embedded release metadata");
            string releaseName;
            string releaseVersion;
            string releaseRuntimeLabel;
            int releaseSchema;
            if (!TryGetJsonString(releaseRoot, "name", out releaseName) ||
                !TryGetJsonString(releaseRoot, "version", out releaseVersion) ||
                !TryGetJsonString(releaseRoot, "runtime_label", out releaseRuntimeLabel) ||
                !TryGetJsonInt32(releaseRoot, "schema_version", out releaseSchema) ||
                releaseSchema != 5 ||
                !string.Equals(releaseName, ModName, StringComparison.Ordinal) ||
                !string.Equals(releaseVersion, ProductVersion, StringComparison.Ordinal) ||
                !string.Equals(releaseRuntimeLabel, RuntimeLabel, StringComparison.Ordinal))
            {
                throw new InvalidDataException("The embedded release metadata does not match the installer product identity.");
            }
            var packageManifestText = new UTF8Encoding(false, true).GetString(
                payload["metadata/package-manifest.json"].Bytes);
            var packageRoot = DeserializeJsonObject(
                packageManifestText,
                "embedded package manifest");
            string packageName;
            string packageVersion;
            int packageSchema;
            if (!TryGetJsonString(packageRoot, "name", out packageName) ||
                !TryGetJsonString(packageRoot, "version", out packageVersion) ||
                !TryGetJsonInt32(packageRoot, "schema_version", out packageSchema) ||
                packageSchema != 1 ||
                !string.Equals(packageName, ModName, StringComparison.Ordinal) ||
                !string.Equals(packageVersion, ProductVersion, StringComparison.Ordinal))
            {
                throw new InvalidDataException("The embedded package manifest does not match the installer product identity.");
            }
            ValidateVisibilityConfig(payload["config/visibility.example.ini"].Bytes, true);
            ValidateDiagnosticsConfig(payload["config/diagnostics.example.ini"].Bytes, true);
            ValidateHotkeyConfig(payload["config/hotkeys.example.ini"].Bytes, true);
        }

        private static bool IsInstallerOnlyDefault(string path)
        {
            return string.Equals(
                    path,
                    "config/visibility.example.ini",
                    StringComparison.OrdinalIgnoreCase) ||
                string.Equals(
                    path,
                    "config/diagnostics.example.ini",
                    StringComparison.OrdinalIgnoreCase) ||
                string.Equals(
                    path,
                    "config/hotkeys.example.ini",
                    StringComparison.OrdinalIgnoreCase);
        }

        private static bool IsPreservedUserPayload(string path)
        {
            return string.Equals(
                path,
                TreasureOverridesPath,
                StringComparison.OrdinalIgnoreCase);
        }

        private static void AssertX64Plugin(byte[] bytes)
        {
            if (bytes == null || bytes.Length < 256 || bytes[0] != 0x4D || bytes[1] != 0x5A)
            {
                throw new InvalidDataException("The embedded native radar plugin is not a PE image.");
            }
            var peOffset = BitConverter.ToInt32(bytes, 0x3C);
            if (peOffset < 0 || peOffset + 24 > bytes.Length ||
                bytes[peOffset] != 0x50 || bytes[peOffset + 1] != 0x45 ||
                bytes[peOffset + 2] != 0 || bytes[peOffset + 3] != 0)
            {
                throw new InvalidDataException("The embedded native radar plugin has an invalid PE header.");
            }
            var machine = BitConverter.ToUInt16(bytes, peOffset + 4);
            var characteristics = BitConverter.ToUInt16(bytes, peOffset + 22);
            if (machine != 0x8664 || (characteristics & 0x2000) == 0)
            {
                throw new InvalidDataException("The embedded native radar plugin must be an x64 DLL.");
            }
            var ascii = Encoding.ASCII.GetString(bytes);
            var unicode = Encoding.Unicode.GetString(bytes);
            if ((!ascii.Contains(ProductVersion) && !unicode.Contains(ProductVersion)) ||
                (!ascii.Contains(RuntimeLabel) && !unicode.Contains(RuntimeLabel)))
            {
                throw new InvalidDataException("The embedded native radar plugin lacks the 2.3.0 release identity marker.");
            }
        }

        private static void RequireManifestValue(
            IDictionary<string, string> manifest,
            string key,
            string expected)
        {
            string actual;
            if (!manifest.TryGetValue(key, out actual) ||
                !string.Equals(actual, expected, StringComparison.Ordinal))
            {
                throw new InvalidDataException("The embedded installer manifest is stale for " + key + ".");
            }
        }

        private static void RequireSha256(IDictionary<string, string> manifest, string key)
        {
            string value;
            if (!manifest.TryGetValue(key, out value) ||
                !Regex.IsMatch(value, "^[0-9A-Fa-f]{64}$", RegexOptions.CultureInvariant))
            {
                throw new InvalidDataException("The embedded installer manifest has an invalid SHA-256 for " + key + ".");
            }
        }

        private static string NormalizePayloadPath(string value)
        {
            var path = (value ?? string.Empty).Replace('\\', '/');
            if (path.Length == 0 || path.StartsWith("/", StringComparison.Ordinal) ||
                path.EndsWith("/", StringComparison.Ordinal) || path.Contains(":"))
            {
                throw new InvalidDataException("The runtime payload contains an unsafe path: " + value);
            }
            var segments = path.Split('/');
            if (segments.Any(segment => segment.Length == 0 || segment == "." || segment == ".."))
            {
                throw new InvalidDataException("The runtime payload contains an unsafe path: " + value);
            }
            return string.Join("/", segments);
        }

        private static byte[] ReadResource(string name)
        {
            var assembly = Assembly.GetExecutingAssembly();
            using (var stream = assembly.GetManifestResourceStream(name))
            {
                if (stream == null)
                {
                    throw new InvalidDataException("The installer resource is missing: " + name);
                }
                using (var output = new MemoryStream())
                {
                    stream.CopyTo(output);
                    return output.ToArray();
                }
            }
        }

        private static void ResolveConfiguredModPaths(InstallContext context)
        {
            var settingsPath = Path.Combine(context.UE4SSDirectory, "UE4SS-settings.ini");
            if (!File.Exists(settingsPath))
            {
                var legacySettings = Path.Combine(context.Win64Directory, "UE4SS-settings.ini");
                if (File.Exists(legacySettings))
                {
                    settingsPath = legacySettings;
                }
            }
            if (!File.Exists(settingsPath))
            {
                throw new InvalidOperationException(
                    "UE4SS-settings.ini is missing from the supported ExperimentalNested layout.");
            }
            InstallerPathSafety.Validate(context.GameRootDirectory, settingsPath, "active UE4SS settings file");
            var overrides = ReadOverrides(settingsPath);
            string modsOverride;
            if (overrides.TryGetValue("ModsFolderPath", out modsOverride) &&
                !string.IsNullOrWhiteSpace(modsOverride))
            {
                context.ModsDirectory = ResolveUE4SSPath(modsOverride, context.Win64Directory);
            }
            else
            {
                context.ModsDirectory = Path.Combine(context.UE4SSDirectory, "Mods");
            }
            if (!Directory.Exists(context.ModsDirectory))
            {
                throw new InvalidOperationException(
                    "The configured ExperimentalNested UE4SS Mods directory is missing or incomplete: " +
                    context.ModsDirectory);
            }
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                context.ModsDirectory,
                "configured UE4SS Mods directory");

            string controllingOverride;
            var hasControllingOverride = overrides.TryGetValue(
                "ControllingModsTxt", out controllingOverride) &&
                !string.IsNullOrWhiteSpace(controllingOverride);
            context.ModsTxtPath = hasControllingOverride
                ? ResolveUE4SSPath(controllingOverride, context.UE4SSDirectory)
                : Path.Combine(context.ModsDirectory, "mods.txt");
            if (!File.Exists(context.ModsTxtPath))
            {
                throw new InvalidOperationException(
                    "The controlling mods.txt is missing or incomplete: " + context.ModsTxtPath);
            }
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                context.ModsTxtPath,
                "configured controlling mods.txt");

            context.AdditionalModsDirectories = ReadAdditionalModsDirectories(
                settingsPath,
                context.UE4SSDirectory);
            foreach (var additional in context.AdditionalModsDirectories)
            {
                if (!Directory.Exists(additional))
                {
                    throw new InvalidOperationException(
                        "A configured additional UE4SS Mods directory is missing: " + additional);
                }
                InstallerPathSafety.Validate(
                    context.GameRootDirectory,
                    additional,
                    "additional UE4SS Mods directory");
            }
        }

        private static Dictionary<string, string> ReadOverrides(string settingsPath)
        {
            var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            var section = string.Empty;
            foreach (var original in File.ReadAllLines(settingsPath, new UTF8Encoding(false, true)))
            {
                var line = original.Trim();
                if (line.Length == 0 || line.StartsWith(";", StringComparison.Ordinal) ||
                    line.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }
                if (line.StartsWith("[", StringComparison.Ordinal) &&
                    line.EndsWith("]", StringComparison.Ordinal))
                {
                    section = line.Substring(1, line.Length - 2).Trim();
                    continue;
                }
                if (!string.Equals(section, "Overrides", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
                var separator = line.IndexOf('=');
                if (separator <= 0)
                {
                    continue;
                }
                var key = line.Substring(0, separator).Trim();
                if (!string.Equals(key, "ModsFolderPath", StringComparison.OrdinalIgnoreCase) &&
                    !string.Equals(key, "ControllingModsTxt", StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
                if (result.ContainsKey(key))
                {
                    throw new InvalidDataException(
                        "UE4SS-settings.ini contains a duplicate " + key + " override.");
                }
                result.Add(key, line.Substring(separator + 1).Trim());
            }
            return result;
        }

        private static List<string> ReadAdditionalModsDirectories(
            string settingsPath,
            string workingDirectory)
        {
            var added = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var removed = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            var section = string.Empty;
            foreach (var original in File.ReadAllLines(settingsPath, new UTF8Encoding(false, true)))
            {
                var line = original.Trim();
                if (line.Length == 0 || line.StartsWith(";", StringComparison.Ordinal) ||
                    line.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }
                if (line.StartsWith("[", StringComparison.Ordinal) &&
                    line.EndsWith("]", StringComparison.Ordinal))
                {
                    section = line.Substring(1, line.Length - 2).Trim();
                    continue;
                }
                if (!string.Equals(section, "Overrides", StringComparison.OrdinalIgnoreCase) ||
                    (line[0] != '+' && line[0] != '-'))
                {
                    continue;
                }
                var separator = line.IndexOf('=');
                if (separator <= 1 ||
                    !string.Equals(
                        line.Substring(1, separator - 1).Trim(),
                        "ModsFolderPaths",
                        StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
                var configured = line.Substring(separator + 1).Trim();
                if (configured.Length == 0)
                {
                    continue;
                }
                var resolved = ResolveUE4SSPath(configured, workingDirectory);
                if (line[0] == '+')
                {
                    removed.Remove(resolved);
                    added.Add(resolved);
                }
                else
                {
                    removed.Add(resolved);
                }
            }
            added.ExceptWith(removed);
            return added.OrderBy(value => value, StringComparer.OrdinalIgnoreCase).ToList();
        }

        private static string ResolveUE4SSPath(string configured, string workingDirectory)
        {
            var value = configured.Trim().Trim('"');
            if (value.Length == 0)
            {
                throw new InvalidDataException("UE4SS contains an empty configured path.");
            }
            return Path.GetFullPath(
                Path.IsPathRooted(value) ? value : Path.Combine(workingDirectory, value));
        }

        private static void ValidateCompetingLoadRoots(InstallContext context)
        {
            var roots = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            roots.Add(Path.GetFullPath(context.ModsDirectory));
            foreach (var additional in context.AdditionalModsDirectories)
            {
                roots.Add(Path.GetFullPath(additional));
            }
            var legacyRoot = Path.Combine(context.Win64Directory, "Mods");
            if (Directory.Exists(legacyRoot))
            {
                roots.Add(Path.GetFullPath(legacyRoot));
            }

            var primary = Path.GetFullPath(context.ModsDirectory);
            foreach (var root in roots)
            {
                InstallerPathSafety.Validate(
                    context.GameRootDirectory,
                    root,
                    "approved UE4SS Mods root");
                ValidateNoEnabledExternalRadarAuthority(
                    context.GameRootDirectory,
                    root);
                if (string.Equals(root, primary, StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }
                var duplicate = Path.Combine(root, ModName);
                if (Directory.Exists(duplicate) || File.Exists(duplicate))
                {
                    throw new InvalidOperationException(
                        "Another active UE4SS Mods root contains " + ModName + ": " + root +
                        ". Setup refuses a competing Mod copy or enabled.txt authority.");
                }
                var alternateModsTxt = Path.Combine(root, "mods.txt");
                if (File.Exists(alternateModsTxt) && ContainsEntry(alternateModsTxt, ModName))
                {
                    throw new InvalidOperationException(
                        "Another active UE4SS mods.txt contains " + ModName + ": " + alternateModsTxt +
                        ". Resolve the competing load authority before installation.");
                }
            }
        }

        private static void ValidateNoEnabledExternalRadarAuthority(
            string gameRootDirectory,
            string modsRoot)
        {
            var externalMarker = Path.Combine(
                modsRoot,
                LegacyExternalRadarName,
                "enabled.txt");
            if (File.Exists(externalMarker) || Directory.Exists(externalMarker))
            {
                InstallerPathSafety.Validate(
                    gameRootDirectory,
                    externalMarker,
                    "legacy external Radar enabled.txt authority");
                throw new InvalidOperationException(
                    "The legacy external DragonSwordWorldRadar enabled.txt authority is active. " +
                    "Disable it before installing Native World Radar: " + externalMarker);
            }
            var modsTxt = Path.Combine(modsRoot, "mods.txt");
            if (!File.Exists(modsTxt))
            {
                return;
            }
            ValidateExternalRadarModsText(
                ReadUtf8Document(modsTxt).Text,
                modsTxt);
        }

        private static void ValidateExistingTargetOwnership(
            InstallContext context,
            string modDirectory)
        {
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                modDirectory,
                "Native World Radar target directory");
            if (!Directory.Exists(modDirectory))
            {
                if (File.Exists(modDirectory))
                {
                    throw new InvalidOperationException(
                        "The Native World Radar target path is a file, not a directory.");
                }
                return;
            }
            var files = Directory.GetFiles(modDirectory, "*", SearchOption.AllDirectories);
            if (files.Length == 0)
            {
                return;
            }
            if (TryValidateOwnedTargetMetadata(
                    context.GameRootDirectory,
                    modDirectory))
            {
                return;
            }
            throw new InvalidOperationException(
                "An existing same-name target cannot be proven by matching top-level release and package identities to belong to Native World Radar. " +
                "Setup will not overwrite or delete it: " + modDirectory);
        }

        private static bool TryValidateOwnedTargetMetadata(
            string gameRootDirectory,
            string modDirectory)
        {
            try
            {
                var releasePath = Path.Combine(modDirectory, "metadata", "release.json");
                var manifestPath = Path.Combine(modDirectory, "metadata", "package-manifest.json");
                IDictionary<string, object> release;
                IDictionary<string, object> manifest;
                if (!TryReadJsonObject(gameRootDirectory, releasePath, out release) ||
                    !TryReadJsonObject(gameRootDirectory, manifestPath, out manifest))
                {
                    return false;
                }

                string releaseName;
                string releaseVersion;
                string runtimeLabel;
                int releaseSchema;
                string manifestName;
                string manifestVersion;
                int manifestSchema;
                if (!TryGetJsonString(release, "name", out releaseName) ||
                    !TryGetJsonString(release, "version", out releaseVersion) ||
                    !TryGetJsonString(release, "runtime_label", out runtimeLabel) ||
                    !TryGetJsonInt32(release, "schema_version", out releaseSchema) ||
                    !TryGetJsonString(manifest, "name", out manifestName) ||
                    !TryGetJsonString(manifest, "version", out manifestVersion) ||
                    !TryGetJsonInt32(manifest, "schema_version", out manifestSchema) ||
                    releaseSchema != 5 || manifestSchema != 1 ||
                    !string.Equals(releaseName, ModName, StringComparison.Ordinal) ||
                    !string.Equals(manifestName, ModName, StringComparison.Ordinal) ||
                    !string.Equals(releaseVersion, manifestVersion, StringComparison.Ordinal) ||
                    string.IsNullOrWhiteSpace(releaseVersion) || releaseVersion.Length > 64 ||
                    string.IsNullOrWhiteSpace(runtimeLabel) || runtimeLabel.Length > 160 ||
                    !runtimeLabel.StartsWith(
                        "DRAGONSWORD_NATIVE_WORLD_RADAR_POSTRENDER_",
                        StringComparison.Ordinal))
                {
                    return false;
                }

                object filesValue;
                int fileCount;
                var files = manifest.TryGetValue("files", out filesValue)
                    ? filesValue as object[]
                    : null;
                if (files == null ||
                    !TryGetJsonInt32(manifest, "file_count", out fileCount) ||
                    fileCount != files.Length || fileCount < 2 || fileCount > 256)
                {
                    return false;
                }

                var rootFull = Path.GetFullPath(modDirectory).TrimEnd(Path.DirectorySeparatorChar);
                var rootPrefix = rootFull + Path.DirectorySeparatorChar;
                var seen = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                var releaseProven = false;
                var pluginProven = false;
                long totalOwnedBytes = 0;
                foreach (var rawEntry in files)
                {
                    var entry = rawEntry as IDictionary<string, object>;
                    string relative;
                    string sha256;
                    long size;
                    if (entry == null ||
                        !TryGetJsonString(entry, "path", out relative) ||
                        !TryGetJsonString(entry, "sha256", out sha256) ||
                        !TryGetJsonInt64(entry, "size", out size) ||
                        size < 0 || size > MaximumPayloadBytes ||
                        !Regex.IsMatch(sha256, "^[0-9A-Fa-f]{64}$", RegexOptions.CultureInvariant))
                    {
                        return false;
                    }
                    try
                    {
                        totalOwnedBytes = checked(totalOwnedBytes + size);
                    }
                    catch (OverflowException)
                    {
                        return false;
                    }
                    if (totalOwnedBytes > MaximumPayloadBytes)
                    {
                        return false;
                    }
                    relative = NormalizePayloadPath(relative);
                    if (!seen.Add(relative))
                    {
                        return false;
                    }
                    var full = Path.GetFullPath(Path.Combine(
                        rootFull,
                        relative.Replace('/', Path.DirectorySeparatorChar)));
                    if (!full.StartsWith(rootPrefix, StringComparison.OrdinalIgnoreCase) ||
                        !File.Exists(full))
                    {
                        return false;
                    }
                    InstallerPathSafety.Validate(
                        gameRootDirectory,
                        full,
                        "owned Native World Radar manifest file");
                    var item = new FileInfo(full);
                    if (IsPreservedUserPayload(relative))
                    {
                        if (item.Length <= 0 || item.Length > 64 * 1024)
                        {
                            return false;
                        }
                        ValidateTreasureOverrides(File.ReadAllBytes(full), false);
                    }
                    else if (item.Length != size ||
                        !string.Equals(HashFile(full), sha256, StringComparison.OrdinalIgnoreCase))
                    {
                        return false;
                    }
                    if (string.Equals(relative, "metadata/release.json", StringComparison.OrdinalIgnoreCase))
                    {
                        releaseProven = true;
                    }
                    else if (string.Equals(relative, "dlls/main.dll", StringComparison.OrdinalIgnoreCase))
                    {
                        var bytes = File.ReadAllBytes(full);
                        var ascii = Encoding.ASCII.GetString(bytes);
                        var unicode = Encoding.Unicode.GetString(bytes);
                        pluginProven =
                            ascii.Contains(runtimeLabel) || unicode.Contains(runtimeLabel);
                    }
                }
                if (!releaseProven || !pluginProven)
                {
                    return false;
                }
                const string packageManifestRelative = "metadata/package-manifest.json";
                if (seen.Contains(packageManifestRelative))
                {
                    return false;
                }
                var permitted = new HashSet<string>(
                    seen,
                    StringComparer.OrdinalIgnoreCase)
                {
                    packageManifestRelative,
                    "config/visibility.ini",
                    "config/hotkeys.ini",
                    "config/diagnostics.ini",
                    "INSTALL-RECORD.txt",
                    "enabled.txt",
                    "config/visibility.example.ini",
                    "config/hotkeys.example.ini",
                    "config/diagnostics.example.ini",
                    "runtime/cache/world-map-treasure-atlas.tga",
                    "runtime/cache/world-map-encounter-atlas.tga",
                    "runtime/logs/DragonSwordNativeWorldRadarPostRender.Native.log",
                    "runtime/logs/DragonSwordNativeWorldRadarPostRender.Native.previous.log"
                };
                var permittedDirectories = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach (var permittedFile in permitted)
                {
                    var separator = permittedFile.LastIndexOf('/');
                    while (separator > 0)
                    {
                        var directory = permittedFile.Substring(0, separator);
                        permittedDirectories.Add(directory);
                        separator = directory.LastIndexOf('/');
                    }
                }
                foreach (var directory in Directory.GetDirectories(
                    modDirectory,
                    "*",
                    SearchOption.AllDirectories))
                {
                    var relative = directory.Substring(rootPrefix.Length).Replace('\\', '/');
                    if (!permittedDirectories.Contains(relative))
                    {
                        return false;
                    }
                }
                var actual = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach (var file in Directory.GetFiles(
                    modDirectory,
                    "*",
                    SearchOption.AllDirectories))
                {
                    var relative = file.Substring(rootPrefix.Length).Replace('\\', '/');
                    if (!actual.Add(relative) || !permitted.Contains(relative))
                    {
                        return false;
                    }
                }
                if (!actual.Contains(packageManifestRelative) ||
                    seen.Any(path => !actual.Contains(path)))
                {
                    return false;
                }
                foreach (var legacyRelative in new[]
                {
                    "enabled.txt",
                    "config/visibility.example.ini",
                    "config/hotkeys.example.ini",
                    "config/diagnostics.example.ini"
                })
                {
                    var legacyPath = Path.Combine(
                        rootFull,
                        legacyRelative.Replace('/', Path.DirectorySeparatorChar));
                    if (File.Exists(legacyPath) && new FileInfo(legacyPath).Length > 64L * 1024L)
                    {
                        return false;
                    }
                }
                foreach (var logRelative in new[]
                {
                    "runtime/logs/DragonSwordNativeWorldRadarPostRender.Native.log",
                    "runtime/logs/DragonSwordNativeWorldRadarPostRender.Native.previous.log"
                })
                {
                    var logPath = Path.Combine(
                        rootFull,
                        logRelative.Replace('/', Path.DirectorySeparatorChar));
                    if (File.Exists(logPath) && new FileInfo(logPath).Length > 2L * 1024L * 1024L)
                    {
                        return false;
                    }
                }
                foreach (var cacheRelative in new[]
                {
                    "runtime/cache/world-map-treasure-atlas.tga",
                    "runtime/cache/world-map-encounter-atlas.tga"
                })
                {
                    var cachePath = Path.Combine(
                        rootFull,
                        cacheRelative.Replace('/', Path.DirectorySeparatorChar));
                    if (File.Exists(cachePath) && new FileInfo(cachePath).Length > 16L * 1024L * 1024L)
                    {
                        return false;
                    }
                }
                var installRecordPath = Path.Combine(modDirectory, "INSTALL-RECORD.txt");
                if (File.Exists(installRecordPath))
                {
                    InstallerPathSafety.Validate(
                        gameRootDirectory,
                        installRecordPath,
                        "Native World Radar install record");
                    var recordItem = new FileInfo(installRecordPath);
                    if (recordItem.Length <= 0 || recordItem.Length > 64 * 1024)
                    {
                        return false;
                    }
                    var record = File.ReadAllText(
                        installRecordPath,
                        new UTF8Encoding(false, true));
                    if (!record.StartsWith(
                            "DragonSword Native World Radar install record",
                            StringComparison.Ordinal) ||
                        !record.Contains("Version: " + releaseVersion) ||
                        !record.Contains("Runtime label: " + runtimeLabel))
                    {
                        return false;
                    }
                }
                return true;
            }
            catch
            {
                return false;
            }
        }

        private static bool TryReadJsonObject(
            string gameRootDirectory,
            string path,
            out IDictionary<string, object> root)
        {
            root = null;
            InstallerPathSafety.Validate(
                gameRootDirectory,
                path,
                "product ownership metadata");
            if (!File.Exists(path))
            {
                return false;
            }
            var item = new FileInfo(path);
            if (item.Length <= 0 || item.Length > 4L * 1024L * 1024L)
            {
                return false;
            }
            try
            {
                var text = File.ReadAllText(path, new UTF8Encoding(false, true));
                root = DeserializeJsonObject(text, "product ownership metadata");
                return true;
            }
            catch
            {
                root = null;
                return false;
            }
        }

        private static IDictionary<string, object> DeserializeJsonObject(
            string text,
            string description)
        {
            if (!string.IsNullOrEmpty(text) && text[0] == '\uFEFF')
            {
                text = text.Substring(1);
            }
            var serializer = new JavaScriptSerializer
            {
                MaxJsonLength = 4 * 1024 * 1024,
                RecursionLimit = 16
            };
            var root = serializer.DeserializeObject(text) as IDictionary<string, object>;
            if (root == null)
            {
                throw new InvalidDataException(description + " must be a top-level JSON object.");
            }
            return root;
        }

        private static bool TryGetJsonString(
            IDictionary<string, object> root,
            string key,
            out string value)
        {
            object raw;
            value = null;
            if (!root.TryGetValue(key, out raw))
            {
                return false;
            }
            value = raw as string;
            return value != null;
        }

        private static bool TryGetJsonInt32(
            IDictionary<string, object> root,
            string key,
            out int value)
        {
            object raw;
            value = 0;
            return root.TryGetValue(key, out raw) && raw is int &&
                ((value = (int)raw) >= 0);
        }

        private static bool TryGetJsonInt64(
            IDictionary<string, object> root,
            string key,
            out long value)
        {
            object raw;
            value = 0;
            if (!root.TryGetValue(key, out raw))
            {
                return false;
            }
            if (raw is int)
            {
                value = (int)raw;
                return true;
            }
            if (raw is long)
            {
                value = (long)raw;
                return true;
            }
            return false;
        }

        private static bool IsOwnedMetadata(
            string gameRootDirectory,
            string path,
            string expectedName)
        {
            string version;
            return TryReadOwnedMetadata(
                gameRootDirectory,
                path,
                expectedName,
                out version);
        }

        private static bool TryReadOwnedMetadata(
            string gameRootDirectory,
            string path,
            string expectedName,
            out string version)
        {
            version = null;
            InstallerPathSafety.Validate(
                gameRootDirectory,
                path,
                "product ownership metadata");
            var item = new FileInfo(path);
            if (item.Length <= 0 || item.Length > 4L * 1024L * 1024L)
            {
                return false;
            }
            try
            {
                var text = File.ReadAllText(path, new UTF8Encoding(false, true));
                var serializer = new JavaScriptSerializer
                {
                    MaxJsonLength = 4 * 1024 * 1024,
                    RecursionLimit = 16
                };
                var root = serializer.DeserializeObject(text) as IDictionary<string, object>;
                object nameValue;
                object versionValue;
                object schemaValue;
                if (root == null ||
                    !root.TryGetValue("name", out nameValue) ||
                    !root.TryGetValue("version", out versionValue) ||
                    !root.TryGetValue("schema_version", out schemaValue) ||
                    !(schemaValue is int))
                {
                    return false;
                }
                var name = nameValue as string;
                version = versionValue as string;
                return string.Equals(name, expectedName, StringComparison.Ordinal) &&
                    !string.IsNullOrWhiteSpace(version) &&
                    version.Length <= 64 &&
                    (int)schemaValue > 0 &&
                    (int)schemaValue <= 1024;
            }
            catch
            {
                version = null;
                return false;
            }
        }

        private static string[] EnumerateTargetFiles(string modDirectory)
        {
            if (!Directory.Exists(modDirectory))
            {
                return new string[0];
            }
            return Directory.GetFiles(modDirectory, "*", SearchOption.AllDirectories)
                .OrderByDescending(path => path.Length)
                .ToArray();
        }

        private static List<string> FindOwnedLegacyRadarMarkers(InstallContext context)
        {
            var markers = new List<string>();
            var roots = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                context.ModsDirectory,
                Path.Combine(context.Win64Directory, "Mods"),
                Path.Combine(context.UE4SSDirectory, "Mods")
            };
            foreach (var additional in context.AdditionalModsDirectories)
            {
                roots.Add(additional);
            }
            foreach (var root in roots)
            {
                var legacyDirectory = Path.Combine(root, LegacyRadarName);
                var marker = Path.Combine(legacyDirectory, "enabled.txt");
                if (!File.Exists(marker))
                {
                    continue;
                }
                InstallerPathSafety.Validate(
                    context.GameRootDirectory,
                    marker,
                    "legacy Radar enabled.txt marker");
                var metadata = Path.Combine(legacyDirectory, "metadata", "release.json");
                if (!File.Exists(metadata) ||
                    !IsOwnedMetadata(context.GameRootDirectory, metadata, LegacyRadarName))
                {
                    throw new InvalidOperationException(
                        "A legacy Radar enabled.txt marker exists but product ownership is not proven: " +
                        marker + ". Setup will not delete it automatically.");
                }
                markers.Add(marker);
            }
            return markers.Distinct(StringComparer.OrdinalIgnoreCase).ToList();
        }

        private static void PrepareUserConfiguration(
            string modDirectory,
            IDictionary<string, PayloadEntry> payload,
            out byte[] visibilityBytes,
            out byte[] diagnosticsBytes,
            out byte[] hotkeyBytes,
            out byte[] treasureOverrideBytes)
        {
            var visibilityDefault = payload["config/visibility.example.ini"].Bytes;
            var diagnosticsDefault = payload["config/diagnostics.example.ini"].Bytes;
            var hotkeyDefault = payload["config/hotkeys.example.ini"].Bytes;
            var treasureOverrideDefault = payload[TreasureOverridesPath].Bytes;
            ValidateVisibilityConfig(visibilityDefault, true);
            ValidateDiagnosticsConfig(diagnosticsDefault, true);
            ValidateHotkeyConfig(hotkeyDefault, true);
            ValidateTreasureOverrides(treasureOverrideDefault, true);

            var visibilityPath = Path.Combine(modDirectory, "config", "visibility.ini");
            var diagnosticsPath = Path.Combine(modDirectory, "config", "diagnostics.ini");
            var hotkeyPath = Path.Combine(modDirectory, "config", "hotkeys.ini");
            var treasureOverridePath = Path.Combine(
                modDirectory,
                TreasureOverridesPath.Replace('/', Path.DirectorySeparatorChar));
            if (Directory.Exists(visibilityPath) || Directory.Exists(diagnosticsPath) || Directory.Exists(hotkeyPath) ||
                Directory.Exists(treasureOverridePath))
            {
                throw new InvalidOperationException(
                    "A live Native World Radar configuration path is a directory. Setup stopped before mutation.");
            }
            visibilityBytes = File.Exists(visibilityPath)
                ? File.ReadAllBytes(visibilityPath)
                : (byte[])visibilityDefault.Clone();
            diagnosticsBytes = File.Exists(diagnosticsPath)
                ? File.ReadAllBytes(diagnosticsPath)
                : (byte[])diagnosticsDefault.Clone();
            if (File.Exists(hotkeyPath) && new FileInfo(hotkeyPath).Length > 4096)
                throw new InvalidDataException("hotkeys.ini exceeds the strict 4 KiB size limit.");
            hotkeyBytes = File.Exists(hotkeyPath)
                ? File.ReadAllBytes(hotkeyPath)
                : (byte[])hotkeyDefault.Clone();
            treasureOverrideBytes = File.Exists(treasureOverridePath)
                ? File.ReadAllBytes(treasureOverridePath)
                : (byte[])treasureOverrideDefault.Clone();
            ValidateVisibilityConfig(visibilityBytes, false);
            ValidateDiagnosticsConfig(diagnosticsBytes, false);
            ValidateHotkeyConfig(hotkeyBytes, false);
            ValidateTreasureOverrides(treasureOverrideBytes, false);
        }

        private static InstallerHotkeys NormalizeRequestedHotkeys(string settings, string enable, string disable)
        {
            var names = new[] { settings, enable, disable };
            if (names.Any(name => string.IsNullOrEmpty(name) || name.Length > 32 ||
                    name.Any(c => c < ' ' && c != '\t') || ParseHotkeyName(name) < 0))
                throw new InvalidDataException("Choose valid Settings, Enable and Disable keys. Examples: F6, INSERT, HOME, PAGEUP. Modifier combinations are not supported.");
            if (names.Select(ParseHotkeyName).Distinct().Count() != 3)
                throw new InvalidDataException("Settings, Enable and Disable must use three different keys.");
            return new InstallerHotkeys
            {
                Settings = settings.Trim(' ', '\t').ToUpperInvariant(),
                Enable = enable.Trim(' ', '\t').ToUpperInvariant(),
                Disable = disable.Trim(' ', '\t').ToUpperInvariant()
            };
        }

        private static InstallerHotkeys ReadHotkeys(byte[] bytes)
        {
            ValidateHotkeyConfig(bytes, false);
            var text = new UTF8Encoding(false, true).GetString(bytes).TrimStart('\uFEFF');
            var values = new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (var raw in text.Split('\n'))
            {
                var line = raw.TrimEnd('\r').Trim(' ', '\t');
                if (line.Length == 0 || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
                int equal = line.IndexOf('=');
                values.Add(line.Substring(0, equal).Trim(' ', '\t'), line.Substring(equal + 1).Trim(' ', '\t'));
            }
            return NormalizeRequestedHotkeys(values["settings_hotkey"], values["enable_hotkey"], values["disable_hotkey"]);
        }

        private static byte[] ApplyRequestedHotkeys(byte[] original, InstallerHotkeys requested)
        {
            var current = ReadHotkeys(original);
            if (requested == null || (current.Settings == requested.Settings &&
                    current.Enable == requested.Enable && current.Disable == requested.Disable))
                return original;

            // Keep comments, BOM, spacing and line endings. Only changed key values
            // enter the existing transaction; no write occurs during inspection.
            bool bom = original.Length >= 3 && original[0] == 0xEF && original[1] == 0xBB && original[2] == 0xBF;
            var text = new UTF8Encoding(false, true).GetString(original, bom ? 3 : 0, original.Length - (bom ? 3 : 0));
            var names = new[] { "settings_hotkey", "enable_hotkey", "disable_hotkey" };
            var before = new[] { current.Settings, current.Enable, current.Disable };
            var after = new[] { requested.Settings, requested.Enable, requested.Disable };
            for (int i = 0; i < names.Length; ++i)
            {
                if (before[i] == after[i]) continue;
                var pattern = "(?m)^([ \\t]*" + names[i] + "[ \\t]*=[ \\t]*)([^ \\t\\r\\n]+)([ \\t]*)(\\r?)$";
                var matches = Regex.Matches(text, pattern, RegexOptions.CultureInvariant);
                if (matches.Count != 1) throw new InvalidDataException("hotkeys.ini cannot be updated unambiguously.");
                var replacement = after[i];
                text = Regex.Replace(text, pattern,
                    match => match.Groups[1].Value + replacement + match.Groups[3].Value + match.Groups[4].Value,
                    RegexOptions.CultureInvariant);
            }
            var result = new UTF8Encoding(false).GetBytes((bom ? "\uFEFF" : string.Empty) + text);
            ValidateHotkeyConfig(result, false);
            return result;
        }

        private static int ParseHotkeyName(string value)
        {
            if (value.Any(c => c > 127)) return -1;
            var key = value.Trim(' ', '\t').ToUpperInvariant();
            if (key.Length == 1 && ((key[0] >= 'A' && key[0] <= 'Z') || (key[0] >= '0' && key[0] <= '9')))
                return key[0];
            int number;
            if (Regex.IsMatch(key, "^F[1-9][0-9]?$") && int.TryParse(key.Substring(1), out number) && number <= 24)
                return 0x6F + number;
            if (Regex.IsMatch(key, "^NUM[0-9]$")) return 0x60 + key[3] - '0';
            var names = new[] { "HOME", "END", "PAGEUP", "PAGEDOWN", "INSERT", "DELETE", "SPACE" };
            var codes = new[] { 0x24, 0x23, 0x21, 0x22, 0x2D, 0x2E, 0x20 };
            var index = Array.IndexOf(names, key);
            return index >= 0 ? codes[index] : -1;
        }

        private static void ValidateHotkeyConfig(byte[] bytes, bool requirePublicDefault)
        {
            if (bytes == null || bytes.Length == 0 || bytes.Length > 4096)
                throw new InvalidDataException("hotkeys.ini is empty or exceeds the strict 4 KiB size limit.");
            int offset = bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF ? 3 : 0;
            var text = new UTF8Encoding(false, true).GetString(bytes, offset, bytes.Length - offset);
            if (text.IndexOf('\0') >= 0 || text.Replace("\r\n", "\n").IndexOf('\r') >= 0)
                throw new InvalidDataException("hotkeys.ini contains invalid text or line endings.");
            bool sectionSeen = false;
            var values = new Dictionary<string, int>(StringComparer.Ordinal);
            foreach (var raw in text.Split('\n'))
            {
                var line = raw.TrimEnd('\r').Trim(' ', '\t');
                if (line.Length == 0 || line.StartsWith("#") || line.StartsWith(";")) continue;
                if (line == "[hotkeys]" && !sectionSeen) { sectionSeen = true; continue; }
                int equal = line.IndexOf('=');
                if (!sectionSeen || equal < 0) throw new InvalidDataException("hotkeys.ini has an invalid section or line.");
                var name = line.Substring(0, equal).Trim(' ', '\t');
                int key = ParseHotkeyName(line.Substring(equal + 1));
                if ((name != "settings_hotkey" && name != "enable_hotkey" && name != "disable_hotkey") ||
                    key < 0 || values.ContainsKey(name))
                    throw new InvalidDataException("hotkeys.ini has an unknown, duplicate, or invalid setting.");
                values.Add(name, key);
            }
            if (values.Count != 3 || values.Values.Distinct().Count() != 3)
                throw new InvalidDataException("hotkeys.ini requires three different valid keys.");
            if (requirePublicDefault && (values["settings_hotkey"] != 0x75 || values["enable_hotkey"] != 0x76 || values["disable_hotkey"] != 0x77))
                throw new InvalidDataException("Public hotkey defaults must be F6/F7/F8.");
        }

        private static void ValidateTreasureOverrides(byte[] bytes, bool requirePublicDefault)
        {
            const long confirmedAbsentTreasureId = 11230106;
            if (bytes == null || bytes.Length == 0 || bytes.Length > 64 * 1024)
            {
                throw new InvalidDataException(
                    "treasure_overrides.txt is empty or exceeds the strict size limit.");
            }
            var offset = bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF
                ? 3
                : 0;
            var text = new UTF8Encoding(false, true).GetString(bytes, offset, bytes.Length - offset);
            RejectMixedOrBareCarriageReturns(text, "treasure_overrides.txt");
            var ignored = new HashSet<long>();
            foreach (var original in text.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
            {
                var line = original.Trim();
                if (line.Length == 0 || line.StartsWith("#", StringComparison.Ordinal) ||
                    line.StartsWith(";", StringComparison.Ordinal))
                {
                    continue;
                }
                var fields = line.Split((char[])null, StringSplitOptions.RemoveEmptyEntries);
                long id;
                if (fields.Length != 2 ||
                    !string.Equals(fields[0], "ignore", StringComparison.Ordinal) ||
                    !long.TryParse(fields[1], NumberStyles.None, CultureInfo.InvariantCulture, out id) ||
                    id <= 0 || !ignored.Add(id))
                {
                    throw new InvalidDataException(
                        "treasure_overrides.txt must contain unique 'ignore <positive save ID>' rows.");
                }
            }
            if (requirePublicDefault &&
                (ignored.Count != 1 || !ignored.Contains(confirmedAbsentTreasureId)))
            {
                throw new InvalidDataException(
                    "The embedded treasure override must ignore exactly save ID 11230106.");
            }
        }

        private static void ValidateVisibilityConfig(byte[] bytes, bool requirePublicDefault)
        {
            if (bytes == null || bytes.Length == 0 || bytes.Length > 4096)
            {
                throw new InvalidDataException(
                    "visibility.ini is empty or exceeds the strict 4 KiB size limit.");
            }
            var offset = bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF
                ? 3
                : 0;
            var text = new UTF8Encoding(false, true).GetString(
                bytes, offset, bytes.Length - offset);
            RejectMixedOrBareCarriageReturns(text, "visibility.ini");
            var sectioned = text.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None)
                .Select(line => line.Trim(' ', '\t'))
                .Any(line => line.StartsWith("[", StringComparison.Ordinal));
            if (sectioned)
            {
                ValidateSectionedVisibilityConfig(text, requirePublicDefault);
                return;
            }
            if (requirePublicDefault)
            {
                throw new InvalidDataException(
                    "The embedded public visibility default must use the readable sectioned format.");
            }

            var values = ParseStrictConfig(bytes, "visibility.ini");
            var allowedKeys = new HashSet<string>(StringComparer.Ordinal)
            {
                "schema_version",
                "compact_mask",
                "world_mask",
                "area_quest_mode",
                "assault_mode"
            };
            if (!values.ContainsKey("compact_mask") ||
                !values.ContainsKey("world_mask") ||
                values.Keys.Any(key => !allowedKeys.Contains(key)))
            {
                throw new InvalidDataException(
                    "Visibility configuration contains a missing or unsupported setting.");
            }
            int compact;
            int world;
            int schema = 1;
            if (!int.TryParse(values["compact_mask"], NumberStyles.None, CultureInfo.InvariantCulture, out compact) ||
                !int.TryParse(values["world_mask"], NumberStyles.None, CultureInfo.InvariantCulture, out world) ||
                (values.ContainsKey("schema_version") &&
                    !int.TryParse(values["schema_version"], NumberStyles.None, CultureInfo.InvariantCulture, out schema)) ||
                compact < 0 || compact > 127 || world < 0 || world > 62 ||
                (world & ~62) != 0 ||
                schema < 1 || schema > 4)
            {
                throw new InvalidDataException("Visibility configuration values are outside their supported ranges.");
            }
            var hasAreaQuestMode = values.ContainsKey("area_quest_mode");
            if ((schema >= 3 && !hasAreaQuestMode) ||
                (schema < 3 && hasAreaQuestMode))
            {
                throw new InvalidDataException(
                    "Visibility schemas 3 and 4 require exactly one area_quest_mode; older schemas must not contain it.");
            }
            if (hasAreaQuestMode &&
                !string.Equals(values["area_quest_mode"], "available", StringComparison.Ordinal) &&
                !string.Equals(values["area_quest_mode"], "all", StringComparison.Ordinal))
            {
                throw new InvalidDataException(
                    "area_quest_mode must be exactly 'available' or 'all'.");
            }
            var hasAssaultMode = values.ContainsKey("assault_mode");
            if ((schema >= 4 && !hasAssaultMode) ||
                (schema < 4 && hasAssaultMode))
            {
                throw new InvalidDataException(
                    "Visibility schema 4 requires exactly one assault_mode; older schemas must not contain it.");
            }
            if (hasAssaultMode &&
                !string.Equals(values["assault_mode"], "available", StringComparison.Ordinal) &&
                !string.Equals(values["assault_mode"], "current", StringComparison.Ordinal) &&
                !string.Equals(values["assault_mode"], "all", StringComparison.Ordinal))
            {
                throw new InvalidDataException(
                    "assault_mode must be exactly 'available' or 'all'; legacy 'current' remains accepted for upgrade compatibility.");
            }
        }

        private static void ValidateSectionedVisibilityConfig(
            string text,
            bool requirePublicDefault)
        {
            var required = new Dictionary<string, HashSet<string>>(StringComparer.Ordinal)
            {
                { "radar", new HashSet<string>(new[] {
                    "clock", "treasure", "boss", "assault", "mini_games",
                    "area_quests", "bird_eggs" }, StringComparer.Ordinal) },
                { "map", new HashSet<string>(new[] {
                    "treasure", "boss", "assault", "mini_games", "area_quests"
                }, StringComparer.Ordinal) },
                { "modes", new HashSet<string>(new[] {
                    "area_quests", "assault" }, StringComparer.Ordinal) },
                { "height_arrows", new HashSet<string>(new[] {
                    "treasure", "area_quests", "mole" }, StringComparer.Ordinal) },
                { "interface", new HashSet<string>(new[] {
                    "language" }, StringComparer.Ordinal) }
            };
            var seenSections = new HashSet<string>(StringComparer.Ordinal);
            var values = new Dictionary<string, string>(StringComparer.Ordinal);
            string section = null;
            foreach (var original in text.Split(
                new[] { "\r\n", "\n" }, StringSplitOptions.None))
            {
                var line = original.Trim(' ', '\t');
                if (line.Length == 0 || line.StartsWith("#", StringComparison.Ordinal) ||
                    line.StartsWith(";", StringComparison.Ordinal))
                {
                    continue;
                }
                if (line.StartsWith("[", StringComparison.Ordinal))
                {
                    if (line.Length < 3 || !line.EndsWith("]", StringComparison.Ordinal))
                    {
                        throw new InvalidDataException(
                            "visibility.ini contains a malformed section.");
                    }
                    section = line.Substring(1, line.Length - 2);
                    if (!required.ContainsKey(section) || !seenSections.Add(section))
                    {
                        throw new InvalidDataException(
                            "visibility.ini contains an unknown or duplicate section: " + section);
                    }
                    continue;
                }
                var separator = line.IndexOf('=');
                if (section == null || separator <= 0 || separator == line.Length - 1 ||
                    line.IndexOf('=', separator + 1) >= 0)
                {
                    throw new InvalidDataException(
                        "visibility.ini contains a setting outside a section or a malformed value.");
                }
                var key = line.Substring(0, separator).Trim(' ', '\t');
                var value = line.Substring(separator + 1).Trim(' ', '\t');
                if (!required[section].Contains(key))
                {
                    throw new InvalidDataException(
                        "visibility.ini contains an unsupported setting: " + section + "." + key);
                }
                var qualified = section + "." + key;
                if (values.ContainsKey(qualified))
                {
                    throw new InvalidDataException(
                        "visibility.ini contains a duplicate setting: " + qualified);
                }
                if (string.Equals(section, "modes", StringComparison.Ordinal))
                {
                    if (!string.Equals(value, "available", StringComparison.Ordinal) &&
                        !string.Equals(value, "all", StringComparison.Ordinal))
                    {
                        throw new InvalidDataException(
                            qualified + " must be exactly 'available' or 'all'.");
                    }
                }
                else if (string.Equals(section, "interface", StringComparison.Ordinal))
                {
                    var supportedLanguages = new HashSet<string>(new[] {
                        "auto", "en", "ja", "ko", "zh-hans", "zh-hant",
                        "fr", "de", "es-es", "ru", "th", "pt-br"
                    }, StringComparer.Ordinal);
                    if (!supportedLanguages.Contains(value))
                    {
                        throw new InvalidDataException(
                            qualified + " contains an unsupported language ID.");
                    }
                }
                else if (!string.Equals(value, "true", StringComparison.Ordinal) &&
                         !string.Equals(value, "false", StringComparison.Ordinal))
                {
                    throw new InvalidDataException(
                        qualified + " must be exactly 'true' or 'false'.");
                }
                values.Add(qualified, value);
            }
            var legacySections = new HashSet<string>(new[] {
                "radar", "map", "modes"
            }, StringComparer.Ordinal);
            var legacyLayout = seenSections.SetEquals(legacySections);
            var currentLayout = seenSections.SetEquals(required.Keys);
            IEnumerable<KeyValuePair<string, HashSet<string>>> requiredForLayout = currentLayout
                ? required
                : required.Where(pair => legacySections.Contains(pair.Key));
            if ((!legacyLayout && !currentLayout) ||
                requiredForLayout.Any(pair => pair.Value.Any(
                    key => !values.ContainsKey(pair.Key + "." + key))))
            {
                throw new InvalidDataException(
                    "visibility.ini is missing a required section or setting.");
            }
            if (requirePublicDefault &&
                (!currentLayout ||
                 values.Any(pair =>
                    (pair.Key.StartsWith("radar.", StringComparison.Ordinal) ||
                     pair.Key.StartsWith("map.", StringComparison.Ordinal))
                        ? !string.Equals(pair.Value, "true", StringComparison.Ordinal)
                        : pair.Key.StartsWith("modes.", StringComparison.Ordinal)
                            ? !string.Equals(pair.Value, "available", StringComparison.Ordinal)
                            : pair.Key.StartsWith("height_arrows.", StringComparison.Ordinal)
                                ? !string.Equals(pair.Value, "true", StringComparison.Ordinal)
                                    : !string.Equals(pair.Value, "auto", StringComparison.Ordinal))))
            {
                throw new InvalidDataException(
                    "The embedded public visibility default must enable every display category and height indicator, use available modes, and follow the game language.");
            }
        }

        private static void ValidateDiagnosticsConfig(byte[] bytes, bool requirePublicDefault)
        {
            bool enabled;
            if (!TryParseDiagnosticsConfig(bytes, out enabled))
            {
                throw new InvalidDataException(
                    "Diagnostics configuration must contain one [diagnostics] section with exactly debug_logging=true or false; the exact legacy event_log_enabled=true or false line is also accepted.");
            }
            if (requirePublicDefault && enabled)
            {
                throw new InvalidDataException(
                    "The embedded public diagnostics default must set debug_logging=false.");
            }
        }

        private static bool TryParseDiagnosticsConfig(byte[] bytes, out bool enabled)
        {
            enabled = false;
            if (bytes == null || bytes.Length == 0 || bytes.Length > 2048 ||
                (bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF))
            {
                return false;
            }
            string text;
            try
            {
                text = new UTF8Encoding(false, true).GetString(bytes);
                RejectMixedOrBareCarriageReturns(text, "diagnostics.ini");
            }
            catch
            {
                return false;
            }

            var legacy = text;
            if (legacy.EndsWith("\r\n", StringComparison.Ordinal))
            {
                legacy = legacy.Substring(0, legacy.Length - 2);
            }
            else if (legacy.EndsWith("\n", StringComparison.Ordinal))
            {
                legacy = legacy.Substring(0, legacy.Length - 1);
            }
            if (string.Equals(legacy, "event_log_enabled=true", StringComparison.Ordinal))
            {
                enabled = true;
                return true;
            }
            if (string.Equals(legacy, "event_log_enabled=false", StringComparison.Ordinal))
            {
                return true;
            }

            var sectionSeen = false;
            var valueSeen = false;
            foreach (var original in text.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
            {
                var line = original.Trim(' ', '\t');
                if (line.Length == 0 || line.StartsWith("#", StringComparison.Ordinal) ||
                    line.StartsWith(";", StringComparison.Ordinal))
                {
                    continue;
                }
                if (string.Equals(line, "[diagnostics]", StringComparison.Ordinal))
                {
                    if (sectionSeen || valueSeen)
                    {
                        return false;
                    }
                    sectionSeen = true;
                    continue;
                }
                var separator = line.IndexOf('=');
                if (!sectionSeen || valueSeen || separator < 0 ||
                    !string.Equals(
                        line.Substring(0, separator).Trim(' ', '\t'),
                        "debug_logging",
                        StringComparison.Ordinal))
                {
                    return false;
                }
                var value = line.Substring(separator + 1).Trim(' ', '\t');
                if (string.Equals(value, "true", StringComparison.Ordinal))
                {
                    enabled = true;
                }
                else if (!string.Equals(value, "false", StringComparison.Ordinal))
                {
                    return false;
                }
                valueSeen = true;
            }
            return sectionSeen && valueSeen;
        }

        private static Dictionary<string, string> ParseStrictConfig(byte[] bytes, string description)
        {
            if (bytes == null || bytes.Length == 0 || bytes.Length > 64 * 1024)
            {
                throw new InvalidDataException(description + " is empty or exceeds the strict size limit.");
            }
            var offset = bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF
                ? 3
                : 0;
            var text = new UTF8Encoding(false, true).GetString(bytes, offset, bytes.Length - offset);
            RejectMixedOrBareCarriageReturns(text, description);
            // Legacy visibility keys are part of the same exact, lowercase
            // contract as the native parser. Case-insensitive acceptance here
            // would preserve a file that the runtime then rejects.
            var values = new Dictionary<string, string>(StringComparer.Ordinal);
            foreach (var original in text.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
            {
                var line = original.Trim();
                if (line.Length == 0 || line.StartsWith("#", StringComparison.Ordinal) ||
                    line.StartsWith(";", StringComparison.Ordinal))
                {
                    continue;
                }
                var separator = line.IndexOf('=');
                if (separator <= 0 || separator == line.Length - 1)
                {
                    throw new InvalidDataException(description + " contains a malformed setting.");
                }
                var key = line.Substring(0, separator).Trim();
                var value = line.Substring(separator + 1).Trim();
                if (key.Length == 0 || value.Length == 0 || values.ContainsKey(key))
                {
                    throw new InvalidDataException(description + " contains an empty or duplicate setting: " + key);
                }
                values.Add(key, value);
            }
            return values;
        }

        private sealed class Utf8Document
        {
            internal string Text;
            internal bool HasBom;
            internal string NewLine;
        }

        private static Utf8Document ReadUtf8Document(string path)
        {
            if (!File.Exists(path))
            {
                throw new InvalidDataException("The required UTF-8 file is missing: " + path);
            }
            var bytes = File.ReadAllBytes(path);
            var hasBom = bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF;
            var offset = hasBom ? 3 : 0;
            var text = new UTF8Encoding(false, true).GetString(bytes, offset, bytes.Length - offset);
            RejectMixedOrBareCarriageReturns(text, "controlling mods.txt");
            return new Utf8Document
            {
                Text = text,
                HasBom = hasBom,
                NewLine = text.Contains("\r\n") ? "\r\n" : "\n"
            };
        }

        private static void RejectMixedOrBareCarriageReturns(string text, string description)
        {
            if (text.Replace("\r\n", string.Empty).Contains("\r"))
            {
                throw new InvalidDataException(description + " contains a bare carriage return.");
            }
            var withoutCrLf = text.Replace("\r\n", string.Empty);
            if (text.Contains("\r\n") && withoutCrLf.Contains("\n"))
            {
                throw new InvalidDataException(description + " mixes CRLF and LF line endings.");
            }
        }

        private static byte[] BuildControlledModsTxt(string path)
        {
            var document = File.Exists(path)
                ? ReadUtf8Document(path)
                : new Utf8Document { Text = string.Empty, HasBom = false, NewLine = "\r\n" };
            ValidateExternalRadarModsText(document.Text, path);
            var updated = NormalizeModsEntry(document.Text, document.NewLine, ModName, true, true);
            if (ContainsEntryText(updated, LegacyRadarName))
            {
                updated = RemoveModsEntries(updated, LegacyRadarName);
            }
            if (CountValidEntry(updated, ModName, true) != 1 ||
                CountAnyEntry(updated, ModName) != 1)
            {
                throw new InvalidDataException("The controlling mods.txt did not produce one native Radar authority.");
            }
            if (ContainsEntryText(updated, LegacyRadarName))
            {
                throw new InvalidDataException("The legacy Radar mods.txt removal did not verify.");
            }
            return EncodeUtf8(updated, document.HasBom);
        }

        private static byte[] BuildUninstallModsTxt(string path)
        {
            var document = ReadUtf8Document(path);
            var any = CountAnyEntry(document.Text, ModName);
            var valid = CountValidEntry(document.Text, ModName, true) +
                CountValidEntry(document.Text, ModName, false);
            if (any != valid || any > 1)
            {
                throw new InvalidDataException(
                    "The controlling mods.txt contains malformed or duplicate Native World Radar authority. " +
                    "Setup will not guess which entry to remove.");
            }
            var updated = RemoveModsEntries(document.Text, ModName);
            if (CountAnyEntry(updated, ModName) != 0)
            {
                throw new InvalidDataException(
                    "The Native World Radar mods.txt authority could not be removed exactly.");
            }
            return EncodeUtf8(updated, document.HasBom);
        }

        private static void ValidateExternalRadarModsText(string text, string path)
        {
            var any = CountAnyEntry(text, LegacyExternalRadarName);
            var enabled = CountValidEntry(text, LegacyExternalRadarName, true);
            var disabled = CountValidEntry(text, LegacyExternalRadarName, false);
            if (any != enabled + disabled)
            {
                throw new InvalidDataException(
                    "The mods.txt authority contains a malformed " +
                    LegacyExternalRadarName + " entry: " + path);
            }
            if (enabled != 0)
            {
                throw new InvalidOperationException(
                    "The legacy external DragonSwordWorldRadar renderer is enabled in mods.txt. " +
                    "Disable it before installing Native World Radar: " + path);
            }
        }

        private static string RemoveModsEntries(string text, string name)
        {
            var linePattern = "(?m)^[\\t ]*" + Regex.Escape(name) +
                "[\\t ]*:[^\\r\\n]*(?:\\r\\n|\\n|$)";
            var matches = Regex.Matches(text, linePattern, RegexOptions.CultureInvariant);
            foreach (Match match in matches)
            {
                var line = match.Value.TrimEnd('\r', '\n');
                if (!Regex.IsMatch(
                    line,
                    "^[\\t ]*" + Regex.Escape(name) + "[\\t ]*:[\\t ]*[01][\\t ]*$",
                    RegexOptions.CultureInvariant))
                {
                    throw new InvalidDataException(
                        "The controlling mods.txt contains a malformed " + name + " entry.");
                }
            }
            var updated = text;
            for (var index = matches.Count - 1; index >= 0; --index)
            {
                updated = updated.Remove(matches[index].Index, matches[index].Length);
            }
            return updated;
        }

        private static string NormalizeModsEntry(
            string text,
            string newLine,
            string name,
            bool enabled,
            bool addWhenMissing)
        {
            var anyPattern = "(?m)^[\\t ]*" + Regex.Escape(name) +
                "[\\t ]*:[^\\r\\n]*(?=\\r?$)";
            var matches = Regex.Matches(text, anyPattern, RegexOptions.CultureInvariant);
            foreach (Match match in matches)
            {
                if (!Regex.IsMatch(
                    match.Value,
                    "^[\\t ]*" + Regex.Escape(name) + "[\\t ]*:[\\t ]*[01][\\t ]*$",
                    RegexOptions.CultureInvariant))
                {
                    throw new InvalidDataException(
                        "The controlling mods.txt contains a malformed " + name + " entry.");
                }
            }
            var desired = name + (enabled ? " : 1" : " : 0");
            if (matches.Count == 0)
            {
                if (!addWhenMissing)
                {
                    return text;
                }
                var separator = text.Length == 0 || text.EndsWith("\n", StringComparison.Ordinal)
                    ? string.Empty
                    : newLine;
                return text + separator + desired + newLine;
            }
            var updated = text;
            for (var index = matches.Count - 1; index >= 1; --index)
            {
                updated = updated.Remove(matches[index].Index, matches[index].Length);
            }
            var first = Regex.Match(updated, anyPattern, RegexOptions.CultureInvariant);
            if (!first.Success)
            {
                throw new InvalidDataException("The first " + name + " entry could not be normalized.");
            }
            return updated.Remove(first.Index, first.Length).Insert(first.Index, desired);
        }

        private static bool ContainsEntry(string path, string name)
        {
            return ContainsEntryText(ReadUtf8Document(path).Text, name);
        }

        private static bool ContainsEntryText(string text, string name)
        {
            return CountAnyEntry(text, name) != 0;
        }

        private static int CountAnyEntry(string text, string name)
        {
            return Regex.Matches(
                text,
                "(?m)^[\\t ]*" + Regex.Escape(name) + "[\\t ]*:[^\\r\\n]*(?=\\r?$)",
                RegexOptions.CultureInvariant).Count;
        }

        private static int CountValidEntry(string text, string name, bool enabled)
        {
            return Regex.Matches(
                text,
                "(?m)^[\\t ]*" + Regex.Escape(name) + "[\\t ]*:[\\t ]*" +
                (enabled ? "1" : "0") + "[\\t ]*(?=\\r?$)",
                RegexOptions.CultureInvariant).Count;
        }

        private static byte[] EncodeUtf8(string text, bool bom)
        {
            var body = new UTF8Encoding(false).GetBytes(text);
            if (!bom)
            {
                return body;
            }
            var preamble = Encoding.UTF8.GetPreamble();
            var result = new byte[preamble.Length + body.Length];
            Buffer.BlockCopy(preamble, 0, result, 0, preamble.Length);
            Buffer.BlockCopy(body, 0, result, preamble.Length, body.Length);
            return result;
        }

        private static string BuildInstallRecord(
            InstallContext context,
            string gameHash,
            string pluginHash,
            string backupRoot,
            byte[] visibilityBytes,
            byte[] diagnosticsBytes,
            byte[] hotkeyBytes,
            byte[] treasureOverrideBytes)
        {
            var builder = new StringBuilder();
            builder.AppendLine("DragonSword Native World Radar install record");
            builder.AppendLine("Version: " + ProductVersion);
            builder.AppendLine("Runtime label: " + RuntimeLabel);
            builder.AppendLine("Installed UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture));
            builder.AppendLine("Game SHA-256: " + gameHash);
            var layoutDescription = "structurally complete ExperimentalNested UE4SS";
            builder.AppendLine("UE4SS layout: " + layoutDescription);
            builder.AppendLine("UE4SS bootstrapped: " + context.BootstrappedUE4SS);
            builder.AppendLine("UE4SS converted: " + context.ConvertsUE4SS);
            builder.AppendLine("Update or repair: " + context.UpdatesExistingRadar);
            builder.AppendLine("Observed UE4SS SHA-256 (provenance only): " + HashFile(
                Path.Combine(context.NestedUE4SSDirectory, "UE4SS.dll")));
            builder.AppendLine("Observed proxy SHA-256 (provenance only): " + HashFile(
                Path.Combine(context.Win64Directory, "dwmapi.dll")));
            builder.AppendLine("Plugin SHA-256: " + pluginHash);
            builder.AppendLine("Visibility config SHA-256: " + HashBytes(visibilityBytes));
            builder.AppendLine("Diagnostics config SHA-256: " + HashBytes(diagnosticsBytes));
            builder.AppendLine("Hotkey config SHA-256: " + HashBytes(hotkeyBytes));
            builder.AppendLine("Treasure overrides SHA-256: " + HashBytes(treasureOverrideBytes));
            builder.AppendLine("Public diagnostics default: false");
            builder.AppendLine("Load authority: " + context.ModsTxtPath);
            builder.AppendLine("Legacy enabled.txt authority: absent");
            builder.AppendLine("Persistent backup directory: " +
                (string.IsNullOrEmpty(backupRoot) ? "not retained" : backupRoot));
            builder.AppendLine("Installer signature: unsigned");
            builder.AppendLine("Gameplay acceptance: NOT_VALIDATED_FOR_EXACT_ARTIFACT");
            return builder.ToString();
        }

        private static void VerifyInstalledState(
            InstallContext context,
            string modDirectory,
            IDictionary<string, PayloadEntry> payload,
            byte[] visibilityBytes,
            byte[] diagnosticsBytes,
            byte[] hotkeyBytes,
            byte[] treasureOverrideBytes)
        {
            InstallerPathSafety.ValidateTree(
                context.GameRootDirectory,
                modDirectory,
                "installed Native World Radar target");
            var expected = new HashSet<string>(
                payload.Keys.Where(path => !IsInstallerOnlyDefault(path)),
                StringComparer.OrdinalIgnoreCase)
            {
                "config/visibility.ini",
                "config/hotkeys.ini",
                "config/diagnostics.ini",
                "INSTALL-RECORD.txt"
            };
            var actual = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (var file in Directory.GetFiles(modDirectory, "*", SearchOption.AllDirectories))
            {
                var relative = file.Substring(
                    modDirectory.TrimEnd(Path.DirectorySeparatorChar).Length + 1).Replace('\\', '/');
                if (!actual.Add(relative))
                {
                    throw new InvalidDataException("The installed payload repeats a case-insensitive path: " + relative);
                }
            }
            if (!actual.SetEquals(expected))
            {
                throw new InvalidDataException(
                    "The installed runtime file set differs from the embedded release contract.");
            }
            foreach (var entry in payload.Values)
            {
                if (IsInstallerOnlyDefault(entry.Path) || IsPreservedUserPayload(entry.Path))
                {
                    continue;
                }
                var path = Path.Combine(modDirectory, entry.Path.Replace('/', Path.DirectorySeparatorChar));
                if (!string.Equals(HashFile(path), entry.Sha256, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException("Installed payload hash verification failed: " + entry.Path);
                }
            }
            if (!string.Equals(
                    HashFile(Path.Combine(modDirectory, "config", "visibility.ini")),
                    HashBytes(visibilityBytes),
                    StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(
                    HashFile(Path.Combine(modDirectory, "config", "diagnostics.ini")),
                    HashBytes(diagnosticsBytes),
                    StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(
                    HashFile(Path.Combine(modDirectory, "config", "hotkeys.ini")),
                    HashBytes(hotkeyBytes),
                    StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(
                    HashFile(Path.Combine(
                        modDirectory,
                        TreasureOverridesPath.Replace('/', Path.DirectorySeparatorChar))),
                    HashBytes(treasureOverrideBytes),
                    StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException("Installed user configuration verification failed.");
            }
            if (Directory.GetFiles(modDirectory, "enabled.txt", SearchOption.AllDirectories).Length != 0)
            {
                throw new InvalidDataException("Legacy enabled.txt remained inside the installed runtime.");
            }
            var document = ReadUtf8Document(context.ModsTxtPath);
            if (CountAnyEntry(document.Text, ModName) != 1 ||
                CountValidEntry(document.Text, ModName, true) != 1)
            {
                throw new InvalidDataException("The installed mods.txt authority did not verify.");
            }
            if (ContainsEntryText(document.Text, LegacyRadarName))
            {
                throw new InvalidDataException("The legacy Radar mods.txt entry was not removed.");
            }
            ValidateExternalRadarModsText(document.Text, context.ModsTxtPath);
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
            using (var stream = File.OpenRead(path))
            using (var sha = SHA256.Create())
            {
                return ToHex(sha.ComputeHash(stream));
            }
        }

        private static string HashBytes(byte[] bytes)
        {
            using (var sha = SHA256.Create())
            {
                return ToHex(sha.ComputeHash(bytes));
            }
        }

        private static string HashZipEntry(ZipArchiveEntry entry)
        {
            using (var stream = entry.Open())
            using (var sha = SHA256.Create())
            {
                return ToHex(sha.ComputeHash(stream));
            }
        }

        private static string AppendDirectorySeparator(string path)
        {
            return path.EndsWith(Path.DirectorySeparatorChar.ToString(), StringComparison.Ordinal)
                ? path
                : path + Path.DirectorySeparatorChar;
        }

        private static string ToHex(byte[] value)
        {
            var builder = new StringBuilder(value.Length * 2);
            foreach (var item in value)
            {
                builder.Append(item.ToString("X2", CultureInfo.InvariantCulture));
            }
            return builder.ToString();
        }
    }

    internal static class InstallerPathSafety
    {
        internal static string Validate(string gameRoot, string candidatePath, string description)
        {
            var root = Path.GetFullPath(gameRoot).TrimEnd(
                Path.DirectorySeparatorChar,
                Path.AltDirectorySeparatorChar);
            var candidate = Path.GetFullPath(candidatePath);
            var prefix = root + Path.DirectorySeparatorChar;
            if (!string.Equals(candidate, root, StringComparison.OrdinalIgnoreCase) &&
                !candidate.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(
                    "The " + description + " resolves outside the selected DS game directory: " + candidate);
            }
            RejectReparsePoint(root, description);
            if (!string.Equals(candidate, root, StringComparison.OrdinalIgnoreCase))
            {
                var current = root;
                foreach (var segment in candidate.Substring(prefix.Length).Split(new[]
                {
                    Path.DirectorySeparatorChar,
                    Path.AltDirectorySeparatorChar
                }, StringSplitOptions.RemoveEmptyEntries))
                {
                    current = Path.Combine(current, segment);
                    if (!File.Exists(current) && !Directory.Exists(current))
                    {
                        break;
                    }
                    RejectReparsePoint(current, description);
                }
            }
            return candidate;
        }

        internal static void ValidateTree(string gameRoot, string path, string description)
        {
            var root = Validate(gameRoot, path, description);
            if (!File.Exists(root) && !Directory.Exists(root))
            {
                return;
            }
            var pending = new Stack<string>();
            pending.Push(root);
            while (pending.Count != 0)
            {
                var current = pending.Pop();
                RejectReparsePoint(current, description);
                if (!Directory.Exists(current))
                {
                    continue;
                }
                foreach (var child in Directory.GetFileSystemEntries(current))
                {
                    RejectReparsePoint(child, description);
                    if (Directory.Exists(child))
                    {
                        pending.Push(child);
                    }
                }
            }
        }

        private static void RejectReparsePoint(string path, string description)
        {
            if (!File.Exists(path) && !Directory.Exists(path))
            {
                return;
            }
            if ((File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0)
            {
                throw new InvalidOperationException(
                    "The " + description + " crosses a symbolic link, junction, or other reparse point: " + path);
            }
        }
    }

    internal sealed class InstallTransaction : IDisposable
    {
        private sealed class OriginalFile
        {
            internal string Target;
            internal string Backup;
            internal bool Existed;
        }

        private readonly string _scopeRoot;
        private readonly string _gameRoot;
        private readonly string _backupRoot;
        private readonly bool _retainBackupAfterCommit;
        private readonly Dictionary<string, OriginalFile> _originals =
            new Dictionary<string, OriginalFile>(StringComparer.OrdinalIgnoreCase);
        private readonly List<string> _order = new List<string>();
        private readonly List<string> _actions = new List<string>();
        private readonly List<string> _createdDirectories = new List<string>();
        private readonly List<string> _removedDirectories = new List<string>();
        private bool _committed;
        private bool _rolledBack;

        internal InstallTransaction(
            string scopeRoot,
            string gameRoot,
            string backupRoot,
            bool retainBackupAfterCommit)
        {
            _scopeRoot = AppendSeparator(Path.GetFullPath(scopeRoot));
            _gameRoot = Path.GetFullPath(gameRoot);
            _backupRoot = InstallerPathSafety.Validate(
                _gameRoot,
                backupRoot,
                "Native World Radar backup directory");
            _retainBackupAfterCommit = retainBackupAfterCommit;
            Directory.CreateDirectory(_backupRoot);
            InstallerPathSafety.Validate(
                _gameRoot,
                _backupRoot,
                "Native World Radar backup directory");
        }

        internal void WriteBytes(string targetPath, byte[] bytes, string action)
        {
            if (bytes == null)
            {
                throw new ArgumentNullException("bytes");
            }
            var target = Path.GetFullPath(targetPath);
            Prepare(target);
            var parent = Path.GetDirectoryName(target);
            EnsureDirectory(parent);
            InstallerPathSafety.Validate(_gameRoot, target, "installer write target");
            var temporary = Path.Combine(
                parent,
                ".dsnwr-install-" + Guid.NewGuid().ToString("N") + ".tmp");
            try
            {
                InstallerPathSafety.Validate(_gameRoot, temporary, "installer temporary file");
                File.WriteAllBytes(temporary, bytes);
                InstallerPathSafety.Validate(_gameRoot, target, "installer write target");
                ReplaceFromSameDirectoryTemporary(temporary, target);
            }
            finally
            {
                if (File.Exists(temporary))
                {
                    File.Delete(temporary);
                }
            }
            _actions.Add(action + ": " + target);
        }

        internal void DeleteFile(string targetPath, string action)
        {
            var target = Path.GetFullPath(targetPath);
            if (!File.Exists(target))
            {
                return;
            }
            Prepare(target);
            InstallerPathSafety.Validate(_gameRoot, target, "installer delete target");
            File.Delete(target);
            _actions.Add(action + ": " + target);
        }

        internal void DeleteDirectoryTree(string targetPath, string action)
        {
            var target = Path.GetFullPath(targetPath);
            if (!Directory.Exists(target))
            {
                return;
            }
            if (!target.StartsWith(_scopeRoot, StringComparison.OrdinalIgnoreCase) ||
                string.Equals(
                    target.TrimEnd(Path.DirectorySeparatorChar),
                    _scopeRoot.TrimEnd(Path.DirectorySeparatorChar),
                    StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(
                    "Installer refused to remove a directory outside the bounded Win64 scope: " + target);
            }
            InstallerPathSafety.ValidateTree(_gameRoot, target, "installer directory-tree delete target");
            foreach (var file in Directory.GetFiles(target, "*", SearchOption.AllDirectories))
            {
                DeleteFile(file, action + " file");
            }
            foreach (var directory in Directory.GetDirectories(target, "*", SearchOption.AllDirectories)
                .Concat(new[] { target })
                .OrderByDescending(value => value.Length))
            {
                InstallerPathSafety.Validate(_gameRoot, directory, "installer directory-tree delete target");
                Directory.Delete(directory, false);
                _removedDirectories.Add(directory);
            }
            _actions.Add(action + ": " + target);
        }

        internal void Commit(IEnumerable<string> summary)
        {
            if (_retainBackupAfterCommit)
            {
                var lines = new List<string>(summary);
                lines.Add(string.Empty);
                lines.Add("Recorded file operations:");
                lines.AddRange(_actions.Select(value => "- " + value));
                lines.Add(string.Empty);
                lines.Add("The complete original Win64-relative UE4SS layout is stored directly in this backup directory.");
                lines.Add("Transaction rollback copies use short internal names under .rollback to avoid Windows path-length failures.");
                var installLog = Path.Combine(_backupRoot, "INSTALL-LOG.txt");
                InstallerPathSafety.Validate(_gameRoot, installLog, "installer log file");
                File.WriteAllLines(installLog, lines, new UTF8Encoding(false));
            }
            else
            {
                _committed = true;
                try
                {
                    InstallerPathSafety.ValidateTree(
                        _gameRoot,
                        _backupRoot,
                        "temporary installer rollback directory");
                    Directory.Delete(_backupRoot, true);
                }
                catch
                {
                    // Installation is already verified and committed. A cleanup failure may
                    // leave only the bounded temporary rollback directory; it must not turn
                    // a successful update into an unsafe partial rollback.
                }
                return;
            }
            _committed = true;
        }

        internal void Rollback()
        {
            if (_rolledBack)
            {
                return;
            }
            var failures = new List<string>();
            for (var index = _order.Count - 1; index >= 0; --index)
            {
                var original = _originals[_order[index]];
                try
                {
                    if (original.Existed)
                    {
                        RestoreOriginal(original);
                    }
                    else if (File.Exists(original.Target))
                    {
                        InstallerPathSafety.Validate(
                            _gameRoot,
                            original.Target,
                            "rollback delete target");
                        File.Delete(original.Target);
                    }
                }
                catch (Exception exception)
                {
                    failures.Add(original.Target + ": " + exception.Message);
                }
            }
            foreach (var directory in _createdDirectories
                .OrderByDescending(value => value.Length)
                .Distinct(StringComparer.OrdinalIgnoreCase))
            {
                try
                {
                    if (Directory.Exists(directory) && !Directory.EnumerateFileSystemEntries(directory).Any())
                    {
                        InstallerPathSafety.Validate(
                            _gameRoot,
                            directory,
                            "rollback empty directory");
                        Directory.Delete(directory, false);
                    }
                }
                catch (Exception exception)
                {
                    failures.Add(directory + ": " + exception.Message);
                }
            }
            foreach (var directory in _removedDirectories
                .OrderBy(value => value.Length)
                .Distinct(StringComparer.OrdinalIgnoreCase))
            {
                try
                {
                    if (!Directory.Exists(directory))
                    {
                        InstallerPathSafety.Validate(
                            _gameRoot,
                            directory,
                            "rollback restore original directory");
                        Directory.CreateDirectory(directory);
                    }
                }
                catch (Exception exception)
                {
                    failures.Add(directory + ": " + exception.Message);
                }
            }
            _rolledBack = true;
            var rollbackLog = Path.Combine(_backupRoot, "ROLLBACK-LOG.txt");
            InstallerPathSafety.Validate(_gameRoot, rollbackLog, "rollback log file");
            File.WriteAllLines(
                rollbackLog,
                failures.Count == 0
                    ? new[] { "Automatic rollback completed." }
                    : failures.ToArray(),
                new UTF8Encoding(false));
            if (failures.Count != 0)
            {
                throw new IOException(string.Join(" | ", failures));
            }
        }

        private void Prepare(string target)
        {
            InstallerPathSafety.Validate(_gameRoot, target, "installer transaction target");
            if (_originals.ContainsKey(target))
            {
                return;
            }
            var existed = File.Exists(target);
            string backup = null;
            if (existed)
            {
                backup = Path.Combine(
                    _backupRoot,
                    ".rollback",
                    _order.Count.ToString("D5", CultureInfo.InvariantCulture) + ".bak");
                InstallerPathSafety.Validate(_gameRoot, backup, "installer backup target");
                Directory.CreateDirectory(Path.GetDirectoryName(backup));
                InstallerPathSafety.Validate(_gameRoot, backup, "installer backup target");
                File.Copy(target, backup, false);
            }
            _originals.Add(target, new OriginalFile
            {
                Target = target,
                Backup = backup,
                Existed = existed
            });
            _order.Add(target);
        }

        private void EnsureDirectory(string path)
        {
            var full = Path.GetFullPath(path);
            InstallerPathSafety.Validate(_gameRoot, full, "installer target directory");
            if (Directory.Exists(full))
            {
                return;
            }
            var missing = new Stack<string>();
            var current = full;
            while (!Directory.Exists(current))
            {
                if (File.Exists(current))
                {
                    throw new IOException("A required target directory is occupied by a file: " + current);
                }
                missing.Push(current);
                current = Path.GetDirectoryName(current);
                if (string.IsNullOrEmpty(current))
                {
                    throw new IOException("A target directory has no existing ancestor: " + full);
                }
            }
            while (missing.Count != 0)
            {
                var create = missing.Pop();
                InstallerPathSafety.Validate(_gameRoot, create, "installer target directory");
                Directory.CreateDirectory(create);
                InstallerPathSafety.Validate(_gameRoot, create, "installer target directory");
                _createdDirectories.Add(create);
            }
        }

        private void RestoreOriginal(OriginalFile original)
        {
            EnsureDirectory(Path.GetDirectoryName(original.Target));
            InstallerPathSafety.Validate(_gameRoot, original.Target, "rollback restore target");
            var temporary = Path.Combine(
                Path.GetDirectoryName(original.Target),
                ".dsnwr-rollback-" + Guid.NewGuid().ToString("N") + ".tmp");
            try
            {
                InstallerPathSafety.Validate(_gameRoot, temporary, "rollback temporary file");
                File.Copy(original.Backup, temporary, false);
                InstallerPathSafety.Validate(_gameRoot, original.Target, "rollback restore target");
                ReplaceFromSameDirectoryTemporary(temporary, original.Target);
            }
            finally
            {
                if (File.Exists(temporary))
                {
                    File.Delete(temporary);
                }
            }
        }

        private static void ReplaceFromSameDirectoryTemporary(string temporary, string target)
        {
            if (File.Exists(target))
            {
                File.Replace(temporary, target, null, true);
            }
            else
            {
                File.Move(temporary, target);
            }
        }

        private static string ShortPathIdentity(string path)
        {
            using (var sha = SHA256.Create())
            {
                var hash = sha.ComputeHash(Encoding.UTF8.GetBytes(path.ToUpperInvariant()));
                var builder = new StringBuilder(16);
                for (var index = 0; index < 8; ++index)
                {
                    builder.Append(hash[index].ToString("X2", CultureInfo.InvariantCulture));
                }
                return builder.ToString();
            }
        }

        private static string AppendSeparator(string value)
        {
            return value.EndsWith(Path.DirectorySeparatorChar.ToString(), StringComparison.Ordinal)
                ? value
                : value + Path.DirectorySeparatorChar;
        }

        public void Dispose()
        {
            if (!_committed && !_rolledBack && _order.Count != 0)
            {
                Rollback();
            }
        }
    }
}
