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

namespace DragonSwordNativeAutoPickup.Installer
{
    internal sealed class InstallerResult
    {
        internal string LayoutDescription { get; set; }
        internal string ToggleHotkey { get; set; }
        internal string InteractionKeyFallback { get; set; }
        internal int RangeMultiplierInstalled { get; set; }
        internal string BackupDirectory { get; set; }
    }

    internal static class InstallerEngine
    {
        private const string ProductVersion = "1.0.1";
        private const string GameFileName = "DSClient-Win64-Shipping.exe";
        private const string ModName = "DragonSwordNativeAutoPickup";
        private const string ObsoleteRangePakName = "DS_PickupRangeX3Canary_P.pak";

        private const string StableUE4SSHash =
            "8AC18FBFFC1EF96B0662D4A2D537B3F224C26D65CAABA7989A9404C566102B26";
        private const string ExperimentalUE4SSHash =
            "F31188D59B34A812AFC32DB4B6FF0C74E1B44861D1ED7967452EE4B3B6635BE1";
        private const string ExperimentalDwmapiHash =
            "30122355CB2784E3BA89F6FB55EA4443467FF8EAE2747CBDEDBEEF49B03E669B";
        private const string OfficialUE4SSZipHash =
            "4B47D4BCEDDD2F561A4E395BFA00924CCFC945AF576A2D0C613E6537846C57EC";
        private const string OfficialDwmapiHash =
            "CE596412BEFA68C30B7F88F65BEB77D9BDAD55E9B96A276A5A9CF690C63F24BB";
        private const string RangePakX3Hash =
            "6130F8FDE15AA94D0140A39ADE81B787869DAE63DAE2DD3F230D255355FC3193";
        private const string RangePakX5Hash =
            "672B82AD41AA50F939674A41212629DB0F792FCF6B774E92214CE74FF5A13F27";
        private const string RangePakX10Hash =
            "F91454EEA895A81FC004E44A1E24011A53B06AD266D38EC473344DD79D184D56";

        private const string ManifestResource = "Payload.Manifest.ini";
        private const string StablePluginResource = "Payload.StablePlugin.dll";
        private const string ExperimentalPluginResource = "Payload.ExperimentalPlugin.dll";
        private const string ConfigResource = "Payload.DefaultConfig.ini";
        private const string LuaResource = "Payload.Main.lua";
        private const string UE4SSZipResource = "Payload.UE4SS.zip";
        private const string RangePakX3Resource = "Payload.PickupRangeX3.pak";
        private const string RangePakX5Resource = "Payload.PickupRangeX5.pak";
        private const string RangePakX10Resource = "Payload.PickupRangeX10.pak";
        private const string NoticesResource = "Payload.ThirdPartyNotices.txt";

        private enum UE4SSLayout
        {
            StableRoot,
            ExperimentalNested
        }

        internal enum RangeSelection
        {
            None = 0,
            X3 = 3,
            X5 = 5,
            X10 = 10
        }

        internal enum OptionalRangePakState
        {
            Absent,
            ApprovedX3,
            ApprovedX5,
            ApprovedX10,
            MultipleApproved,
            UnknownCollision
        }

        private sealed class RangeVariant
        {
            internal RangeSelection Selection;
            internal string FileName;
            internal string Hash;
            internal string Resource;
            internal string ManifestKey;
        }

        private static readonly RangeVariant[] RangeVariants =
        {
            new RangeVariant
            {
                Selection = RangeSelection.X3,
                FileName = "DS_PickupRangeX3_P.pak",
                Hash = RangePakX3Hash,
                Resource = RangePakX3Resource,
                ManifestKey = "pickup_range_x3_sha256"
            },
            new RangeVariant
            {
                Selection = RangeSelection.X5,
                FileName = "DS_PickupRangeX5_P.pak",
                Hash = RangePakX5Hash,
                Resource = RangePakX5Resource,
                ManifestKey = "pickup_range_x5_sha256"
            },
            new RangeVariant
            {
                Selection = RangeSelection.X10,
                FileName = "DS_PickupRangeX10_P.pak",
                Hash = RangePakX10Hash,
                Resource = RangePakX10Resource,
                ManifestKey = "pickup_range_x10_sha256"
            }
        };

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
            internal string PakDirectory;
            internal List<string> AdditionalModsDirectories = new List<string>();
            internal UE4SSLayout Layout;
            internal bool BootstrappedUE4SS;
        }

        internal static bool IsOptionalRangePakInstalled(string gameExecutable)
        {
            var state = InspectOptionalRangePak(gameExecutable);
            return state == OptionalRangePakState.ApprovedX3 ||
                   state == OptionalRangePakState.ApprovedX5 ||
                   state == OptionalRangePakState.ApprovedX10 ||
                   state == OptionalRangePakState.MultipleApproved;
        }

        internal static OptionalRangePakState InspectOptionalRangePak(string gameExecutable)
        {
            try
            {
                var fullPath = ValidateGameExecutableLocation(gameExecutable, false);
                var win64 = Path.GetDirectoryName(fullPath);
                var pakDirectory = Path.GetFullPath(Path.Combine(
                    win64, "..", "..", "Content", "Paks", "~mods"));
                var approved = new List<RangeSelection>();
                foreach (var variant in RangeVariants)
                {
                    var pak = Path.Combine(pakDirectory, variant.FileName);
                    if (!File.Exists(pak))
                    {
                        continue;
                    }
                    if (!string.Equals(HashFile(pak), variant.Hash, StringComparison.OrdinalIgnoreCase))
                    {
                        return OptionalRangePakState.UnknownCollision;
                    }
                    approved.Add(variant.Selection);
                }
                if (approved.Count == 0) return OptionalRangePakState.Absent;
                if (approved.Count > 1) return OptionalRangePakState.MultipleApproved;
                if (approved[0] == RangeSelection.X3) return OptionalRangePakState.ApprovedX3;
                if (approved[0] == RangeSelection.X5) return OptionalRangePakState.ApprovedX5;
                return OptionalRangePakState.ApprovedX10;
            }
            catch
            {
                return OptionalRangePakState.Absent;
            }
        }

        internal static InstallerResult Install(
            string selectedGameExecutable,
            string requestedHotkey,
            string requestedInteractionKeyFallback,
            RangeSelection rangeSelection)
        {
            EnsureGameIsClosed();

            if (!Enum.IsDefined(typeof(RangeSelection), rangeSelection))
            {
                throw new ArgumentOutOfRangeException("rangeSelection", "The requested range choice is invalid.");
            }

            var hotkey = NormalizeHotkey(requestedHotkey);
            var interactionKeyFallback = NormalizeInteractionKeyFallback(requestedInteractionKeyFallback);
            var gameExecutable = ValidateGameExecutableLocation(selectedGameExecutable, true);
            var manifest = ReadManifest();
            ValidateManifest(manifest);

            var gameHash = HashFile(gameExecutable);
            var supportedGameHashes = manifest["supported_game_sha256"]
                .Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries)
                .Select(value => value.Trim().ToUpperInvariant())
                .ToArray();
            if (!supportedGameHashes.Contains(gameHash, StringComparer.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(
                    "This game build is not supported by Auto Pickup 1.0.1. " +
                    "Selected executable SHA-256: " + gameHash);
            }

            var payloads = LoadAndValidatePayloads(manifest);
            var context = DetectLayout(gameExecutable);
            ValidateExistingUE4SS(context);

            var existingRangeVariants = new Dictionary<RangeSelection, string>();
            foreach (var variant in RangeVariants)
            {
                var path = Path.Combine(context.PakDirectory, variant.FileName);
                InstallerPathSafety.Validate(
                    context.GameRootDirectory,
                    path,
                    "optional " + (int)variant.Selection + "x PAK path");
                if (!File.Exists(path))
                {
                    continue;
                }
                if (!string.Equals(HashFile(path), variant.Hash, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidOperationException(
                        variant.FileName + " already exists but is not the approved Auto Pickup artifact. " +
                        "Setup will not overwrite or remove an unknown same-name PAK. Resolve the conflict manually.");
                }
                existingRangeVariants.Add(variant.Selection, path);
            }

            var obsoleteRangePakPath = Path.Combine(context.PakDirectory, ObsoleteRangePakName);
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                obsoleteRangePakPath,
                "obsolete 3x canary PAK path");
            if (rangeSelection != RangeSelection.None && File.Exists(obsoleteRangePakPath))
            {
                throw new InvalidOperationException(
                    "DS_PickupRangeX3Canary_P.pak is installed and would conflict with the selected range PAK. " +
                    "Setup cannot prove ownership of that older file, so it will not delete it automatically. " +
                    "Remove or archive the canary PAK manually, then run Setup again.");
            }

            var backupRoot = Path.Combine(
                context.Win64Directory,
                "DragonSwordNativeAutoPickup-Backups",
                DateTime.UtcNow.ToString("yyyyMMdd-HHmmss-fff", CultureInfo.InvariantCulture));

            using (var transaction = new InstallTransaction(
                context.Win64Directory,
                context.GameRootDirectory,
                backupRoot))
            {
                try
                {
                    if (context.BootstrappedUE4SS)
                    {
                        BootstrapStableUE4SS(context, payloads[UE4SSZipResource], transaction);
                    }

                    ResolveConfiguredModPaths(context);
                    var configured = BuildPublicConfig(
                        payloads[ConfigResource], hotkey, interactionKeyFallback);
                    var modsTxt = BuildControlledModsTxt(context.ModsTxtPath);

                    var modDirectory = Path.Combine(context.ModsDirectory, ModName);
                    var selectedPlugin = context.Layout == UE4SSLayout.StableRoot
                        ? payloads[StablePluginResource]
                        : payloads[ExperimentalPluginResource];

                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "dlls", "main.dll"),
                        selectedPlugin,
                        "Install ABI-specific native plugin");
                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "config.ini"),
                        configured,
                        "Install public startup-off configuration");
                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "Scripts", "main.lua"),
                        payloads[LuaResource],
                        "Install passive Lua entry point");
                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "THIRD_PARTY_NOTICES.txt"),
                        payloads[NoticesResource],
                        "Install third-party notices");

                    RemoveLegacyEnablementFiles(context, transaction);
                    transaction.WriteBytes(
                        context.ModsTxtPath,
                        modsTxt,
                        "Set exactly one controlling mods.txt entry");

                    foreach (var variant in RangeVariants)
                    {
                        var rangePakPath = Path.Combine(context.PakDirectory, variant.FileName);
                        var isSelected = variant.Selection == rangeSelection;
                        var isInstalled = existingRangeVariants.ContainsKey(variant.Selection);
                        if (isSelected && !isInstalled)
                        {
                            transaction.WriteBytes(
                                rangePakPath,
                                payloads[variant.Resource],
                                "Install optional " + (int)variant.Selection + "x native interaction range PAK");
                        }
                        else if (!isSelected && isInstalled)
                        {
                            transaction.DeleteFile(
                                rangePakPath,
                                "Remove unselected " + (int)variant.Selection + "x native interaction range PAK");
                        }
                    }

                    var layoutDescription = context.Layout == UE4SSLayout.StableRoot
                        ? "stable-root UE4SS ABI"
                        : "experimental-nested UE4SS ABI";
                    var record = BuildInstallRecord(
                        context,
                        layoutDescription,
                        hotkey,
                        interactionKeyFallback,
                        rangeSelection,
                        gameHash,
                        selectedPlugin,
                        backupRoot);
                    transaction.WriteBytes(
                        Path.Combine(modDirectory, "INSTALL-RECORD.txt"),
                        new UTF8Encoding(false).GetBytes(record),
                        "Write release install record");

                    transaction.Commit(new[]
                    {
                        "DragonSword Native Auto Pickup " + ProductVersion,
                        "UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture),
                        "Game executable: " + context.GameExecutable,
                        "Game SHA-256: " + gameHash,
                        "UE4SS layout: " + layoutDescription,
                        "UE4SS bootstrapped: " + context.BootstrappedUE4SS,
                        "Mods directory: " + context.ModsDirectory,
                        "Controlling mods.txt: " + context.ModsTxtPath,
                        "Toggle hotkey: " + hotkey,
                        "Interaction key mode: AUTO",
                        "Interaction key fallback: " + interactionKeyFallback,
                        "Optional range multiplier installed: " + (int)rangeSelection + "x"
                    });

                    return new InstallerResult
                    {
                        LayoutDescription = layoutDescription,
                        ToggleHotkey = hotkey,
                        InteractionKeyFallback = interactionKeyFallback,
                        RangeMultiplierInstalled = (int)rangeSelection,
                        BackupDirectory = backupRoot
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
                            "Installation failed and automatic rollback was incomplete. " +
                            "Original error: " + installError.Message + " Rollback error: " +
                            rollbackError.Message + " Backup directory: " + backupRoot,
                            installError);
                    }

                    throw new InvalidOperationException(
                        "Installation failed; all recorded file mutations were restored. " +
                        "Newly created empty directories may remain. " +
                        installError.Message + " Backup directory: " + backupRoot,
                        installError);
                }
            }
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

        private static string ValidateGameExecutableLocation(string selectedPath, bool requireSupportedLayout)
        {
            if (string.IsNullOrWhiteSpace(selectedPath))
            {
                throw new InvalidOperationException("Select DSClient-Win64-Shipping.exe first.");
            }

            var fullPath = Path.GetFullPath(Environment.ExpandEnvironmentVariables(selectedPath.Trim().Trim('"')));
            if (!File.Exists(fullPath))
            {
                throw new FileNotFoundException("The selected game executable does not exist.", fullPath);
            }

            if (!string.Equals(Path.GetFileName(fullPath), GameFileName, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException("The selected file must be " + GameFileName + ".");
            }

            if (requireSupportedLayout)
            {
                var win64 = new DirectoryInfo(Path.GetDirectoryName(fullPath));
                if (!string.Equals(win64.Name, "Win64", StringComparison.OrdinalIgnoreCase) ||
                    win64.Parent == null ||
                    !string.Equals(win64.Parent.Name, "Binaries", StringComparison.OrdinalIgnoreCase) ||
                    win64.Parent.Parent == null ||
                    !string.Equals(win64.Parent.Parent.Name, "DS", StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidOperationException(
                        "The executable must be in DS\\Binaries\\Win64. The selected directory is not a supported DragonSword installation.");
                }

                var gameRoot = win64.Parent.Parent.FullName;
                InstallerPathSafety.Validate(
                    gameRoot,
                    fullPath,
                    "selected DragonSword executable");
            }

            return fullPath;
        }

        private static string NormalizeHotkey(string requested)
        {
            var value = (requested ?? string.Empty).Trim().ToUpperInvariant();
            if (Regex.IsMatch(value, "^F(?:[1-9]|1[0-9]|2[0-4])$", RegexOptions.CultureInvariant) ||
                Regex.IsMatch(value, "^[A-Z0-9]$", RegexOptions.CultureInvariant) ||
                Regex.IsMatch(value, "^NUM[0-9]$", RegexOptions.CultureInvariant) ||
                new[] { "HOME", "END", "PAGEUP", "PAGEDOWN", "INSERT", "DELETE", "SPACE" }
                    .Contains(value, StringComparer.Ordinal))
            {
                return value;
            }

            throw new InvalidOperationException(
                "Unsupported toggle key. Use F1-F24, A-Z, 0-9, NUM0-NUM9, HOME, END, " +
                "PAGEUP, PAGEDOWN, INSERT, DELETE, or SPACE.");
        }

        private static string NormalizeInteractionKeyFallback(string requested)
        {
            var value = (requested ?? string.Empty).Trim();
            if (string.Equals(value, "AUTO", StringComparison.OrdinalIgnoreCase) ||
                !Regex.IsMatch(value, "^[A-Za-z0-9_]{1,64}$", RegexOptions.CultureInvariant))
            {
                throw new InvalidOperationException(
                    "Unsupported interaction fallback key. Enter one concrete Unreal FKey name, " +
                    "such as F, E, K, or Gamepad_FaceButton_Bottom. AUTO is reserved for primary detection.");
            }

            if (Regex.IsMatch(value, "^[A-Za-z]$", RegexOptions.CultureInvariant) ||
                Regex.IsMatch(value, "^F(?:[1-9]|1[0-9]|2[0-4])$", RegexOptions.IgnoreCase | RegexOptions.CultureInvariant))
            {
                return value.ToUpperInvariant();
            }
            return value;
        }

        private static Dictionary<string, string> ReadManifest()
        {
            var bytes = ReadResource(ManifestResource);
            var text = new UTF8Encoding(false, true).GetString(bytes);
            var values = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            foreach (var originalLine in text.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
            {
                var line = originalLine.Trim();
                if (line.Length == 0 || line.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }

                var separator = line.IndexOf('=');
                if (separator <= 0)
                {
                    throw new InvalidDataException("The embedded payload manifest is malformed.");
                }

                var key = line.Substring(0, separator).Trim();
                var value = line.Substring(separator + 1).Trim();
                if (values.ContainsKey(key) || value.Length == 0)
                {
                    throw new InvalidDataException("The embedded payload manifest contains a duplicate or empty value: " + key);
                }
                values.Add(key, value);
            }
            return values;
        }

        private static void ValidateManifest(IDictionary<string, string> manifest)
        {
            var required = new[]
            {
                "version",
                "supported_game_sha256",
                "stable_plugin_sha256",
                "experimental_plugin_sha256",
                "config_sha256",
                "lua_sha256",
                "stable_ue4ss_zip_sha256",
                "pickup_range_x3_sha256",
                "pickup_range_x5_sha256",
                "pickup_range_x10_sha256",
                "third_party_notices_sha256"
            };
            foreach (var key in required)
            {
                if (!manifest.ContainsKey(key))
                {
                    throw new InvalidDataException("The embedded payload manifest is missing: " + key);
                }
            }

            if (!string.Equals(manifest["version"], ProductVersion, StringComparison.Ordinal))
            {
                throw new InvalidDataException("The embedded payload version does not match this installer.");
            }
            if (!string.Equals(manifest["stable_ue4ss_zip_sha256"], OfficialUE4SSZipHash, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(manifest["pickup_range_x3_sha256"], RangePakX3Hash, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(manifest["pickup_range_x5_sha256"], RangePakX5Hash, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(manifest["pickup_range_x10_sha256"], RangePakX10Hash, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidDataException("The embedded third-party payload identity is not approved for this release.");
            }
        }

        private static Dictionary<string, byte[]> LoadAndValidatePayloads(IDictionary<string, string> manifest)
        {
            var keys = new Dictionary<string, string>(StringComparer.Ordinal)
            {
                { StablePluginResource, "stable_plugin_sha256" },
                { ExperimentalPluginResource, "experimental_plugin_sha256" },
                { ConfigResource, "config_sha256" },
                { LuaResource, "lua_sha256" },
                { UE4SSZipResource, "stable_ue4ss_zip_sha256" },
                { RangePakX3Resource, "pickup_range_x3_sha256" },
                { RangePakX5Resource, "pickup_range_x5_sha256" },
                { RangePakX10Resource, "pickup_range_x10_sha256" },
                { NoticesResource, "third_party_notices_sha256" }
            };
            var payloads = new Dictionary<string, byte[]>(StringComparer.Ordinal);
            foreach (var pair in keys)
            {
                var bytes = ReadResource(pair.Key);
                var actual = HashBytes(bytes);
                if (!string.Equals(actual, manifest[pair.Value], StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException("Embedded resource integrity check failed: " + pair.Key);
                }
                payloads.Add(pair.Key, bytes);
            }

            if (!string.Equals(HashBytes(payloads[UE4SSZipResource]), OfficialUE4SSZipHash, StringComparison.Ordinal) ||
                !string.Equals(HashBytes(payloads[RangePakX3Resource]), RangePakX3Hash, StringComparison.Ordinal) ||
                !string.Equals(HashBytes(payloads[RangePakX5Resource]), RangePakX5Hash, StringComparison.Ordinal) ||
                !string.Equals(HashBytes(payloads[RangePakX10Resource]), RangePakX10Hash, StringComparison.Ordinal))
            {
                throw new InvalidDataException("Embedded approved payload verification failed.");
            }
            return payloads;
        }

        private static byte[] ReadResource(string name)
        {
            var assembly = Assembly.GetExecutingAssembly();
            using (var stream = assembly.GetManifestResourceStream(name))
            {
                if (stream == null)
                {
                    throw new InvalidDataException("Installer resource is missing: " + name);
                }
                using (var memory = new MemoryStream())
                {
                    stream.CopyTo(memory);
                    return memory.ToArray();
                }
            }
        }

        private static InstallContext DetectLayout(string gameExecutable)
        {
            var win64 = Path.GetDirectoryName(gameExecutable);
            var stableDirectory = win64;
            var nestedDirectory = Path.Combine(win64, "ue4ss");
            var stableDll = Path.Combine(stableDirectory, "UE4SS.dll");
            var nestedDll = Path.Combine(nestedDirectory, "UE4SS.dll");
            var stableExists = File.Exists(stableDll);
            var nestedExists = File.Exists(nestedDll);

            if (stableExists && nestedExists)
            {
                throw new InvalidOperationException(
                    "Both stable-root and experimental-nested UE4SS.dll files exist. " +
                    "Setup will not guess which loader controls the game. Remove the unused layout manually and run Setup again.");
            }

            var context = new InstallContext
            {
                GameExecutable = gameExecutable,
                GameRootDirectory = Path.GetFullPath(Path.Combine(win64, "..", "..")),
                Win64Directory = win64,
                StableUE4SSDirectory = stableDirectory,
                NestedUE4SSDirectory = nestedDirectory,
                PakDirectory = Path.GetFullPath(Path.Combine(win64, "..", "..", "Content", "Paks", "~mods"))
            };

            if (stableExists)
            {
                context.Layout = UE4SSLayout.StableRoot;
                context.UE4SSDirectory = stableDirectory;
            }
            else if (nestedExists)
            {
                context.Layout = UE4SSLayout.ExperimentalNested;
                context.UE4SSDirectory = nestedDirectory;
            }
            else
            {
                context.Layout = UE4SSLayout.StableRoot;
                context.UE4SSDirectory = stableDirectory;
                context.BootstrappedUE4SS = true;
            }
            return context;
        }

        private static void ValidateExistingUE4SS(InstallContext context)
        {
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                context.Win64Directory,
                "DragonSword Win64 directory");

            if (context.BootstrappedUE4SS)
            {
                var proxy = Path.Combine(context.Win64Directory, "dwmapi.dll");
                InstallerPathSafety.Validate(
                    context.GameRootDirectory,
                    proxy,
                    "existing Win64 proxy loader");
                if (File.Exists(proxy) &&
                    !string.Equals(HashFile(proxy), OfficialDwmapiHash, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidOperationException(
                        "UE4SS is absent, but Win64\\dwmapi.dll belongs to an unknown loader or proxy. " +
                        "Setup will not overwrite it automatically.");
                }
                return;
            }

            var ue4ssDll = Path.Combine(context.UE4SSDirectory, "UE4SS.dll");
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                ue4ssDll,
                "existing UE4SS DLL");
            var actual = HashFile(ue4ssDll);
            var expected = context.Layout == UE4SSLayout.StableRoot
                ? StableUE4SSHash
                : ExperimentalUE4SSHash;
            if (!string.Equals(actual, expected, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(
                    "The installed UE4SS.dll is not a supported release for this Auto Pickup package. " +
                    "Setup stopped without changing files. SHA-256: " + actual);
            }

            var proxyPath = Path.Combine(context.Win64Directory, "dwmapi.dll");
            if (!File.Exists(proxyPath))
            {
                throw new InvalidOperationException(
                    "The supported UE4SS.dll exists, but Win64\\dwmapi.dll is missing. " +
                    "The loader installation is incomplete, so Setup will not report a usable installation.");
            }
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                proxyPath,
                "existing UE4SS proxy loader");
            var expectedProxy = context.Layout == UE4SSLayout.StableRoot
                ? OfficialDwmapiHash
                : ExperimentalDwmapiHash;
            var actualProxy = HashFile(proxyPath);
            if (!string.Equals(actualProxy, expectedProxy, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException(
                    "The installed Win64\\dwmapi.dll is not the approved proxy for the detected UE4SS layout. " +
                    "Setup stopped without changing files. SHA-256: " + actualProxy);
            }
        }

        private static void BootstrapStableUE4SS(
            InstallContext context,
            byte[] zipBytes,
            InstallTransaction transaction)
        {
            using (var memory = new MemoryStream(zipBytes, false))
            using (var archive = new ZipArchive(memory, ZipArchiveMode.Read, false))
            {
                var dllEntry = archive.GetEntry("UE4SS.dll");
                var proxyEntry = archive.GetEntry("dwmapi.dll");
                if (dllEntry == null || proxyEntry == null ||
                    !string.Equals(HashZipEntry(dllEntry), StableUE4SSHash, StringComparison.OrdinalIgnoreCase) ||
                    !string.Equals(HashZipEntry(proxyEntry), OfficialDwmapiHash, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidDataException("The embedded official UE4SS archive contents are not approved.");
                }

                var rootPrefix = AppendDirectorySeparator(Path.GetFullPath(context.Win64Directory));
                foreach (var entry in archive.Entries)
                {
                    if (string.IsNullOrEmpty(entry.Name))
                    {
                        continue;
                    }

                    var archivePath = entry.FullName.Replace('\\', '/');
                    var isCoreLoader =
                        string.Equals(archivePath, "UE4SS.dll", StringComparison.OrdinalIgnoreCase) ||
                        string.Equals(archivePath, "dwmapi.dll", StringComparison.OrdinalIgnoreCase);
                    var preserveWhenPresent =
                        string.Equals(archivePath, "UE4SS-settings.ini", StringComparison.OrdinalIgnoreCase) ||
                        string.Equals(archivePath, "Mods/mods.txt", StringComparison.OrdinalIgnoreCase) ||
                        archivePath.StartsWith("Mods/", StringComparison.OrdinalIgnoreCase);
                    if (!isCoreLoader && !preserveWhenPresent)
                    {
                        // Documentation and other non-runtime root files are not
                        // deployed into the game directory by this product.
                        continue;
                    }

                    var relative = archivePath.Replace('/', Path.DirectorySeparatorChar);
                    var target = Path.GetFullPath(Path.Combine(context.Win64Directory, relative));
                    if (!target.StartsWith(rootPrefix, StringComparison.OrdinalIgnoreCase))
                    {
                        throw new InvalidDataException("The embedded UE4SS archive contains an unsafe path.");
                    }

                    // Preserve settings, the Mod list, and every existing built-in
                    // Mod file. Bootstrap only fills missing supporting files.
                    if (File.Exists(target) && preserveWhenPresent)
                    {
                        continue;
                    }

                    if (File.Exists(target) && string.Equals(HashFile(target), HashZipEntry(entry), StringComparison.Ordinal))
                    {
                        continue;
                    }

                    using (var stream = entry.Open())
                    using (var output = new MemoryStream())
                    {
                        stream.CopyTo(output);
                        transaction.WriteBytes(target, output.ToArray(), "Install official UE4SS v3.0.1 file");
                    }
                }
            }

            var installedDll = Path.Combine(context.Win64Directory, "UE4SS.dll");
            var installedProxy = Path.Combine(context.Win64Directory, "dwmapi.dll");
            if (!File.Exists(installedDll) || !File.Exists(installedProxy) ||
                !string.Equals(HashFile(installedDll), StableUE4SSHash, StringComparison.OrdinalIgnoreCase) ||
                !string.Equals(HashFile(installedProxy), OfficialDwmapiHash, StringComparison.OrdinalIgnoreCase))
            {
                throw new InvalidOperationException("Official UE4SS installation verification failed.");
            }
        }

        private static void ResolveConfiguredModPaths(InstallContext context)
        {
            var dsDirectory = new DirectoryInfo(context.GameRootDirectory);
            var gameInstallName = dsDirectory.Parent == null ? string.Empty : dsDirectory.Parent.Name;
            if (gameInstallName.Length != 0 &&
                Directory.Exists(Path.Combine(context.UE4SSDirectory, gameInstallName)))
            {
                throw new InvalidOperationException(
                    "This UE4SS installation uses a game-specific working directory. " +
                    "Auto Pickup 1.0.1 Setup does not guess that advanced path layout; install manually instead.");
            }

            var settingsPath = Path.Combine(context.UE4SSDirectory, "UE4SS-settings.ini");
            if (!File.Exists(settingsPath) && context.Layout == UE4SSLayout.ExperimentalNested)
            {
                var legacySettings = Path.Combine(context.Win64Directory, "UE4SS-settings.ini");
                if (File.Exists(legacySettings))
                {
                    settingsPath = legacySettings;
                }
            }
            if (!File.Exists(settingsPath))
            {
                throw new InvalidOperationException("UE4SS-settings.ini is missing from the selected UE4SS layout.");
            }
            InstallerPathSafety.Validate(
                context.GameRootDirectory,
                settingsPath,
                "active UE4SS settings file");

            var overrides = ReadOverrides(settingsPath);
            string controllingOverride;
            overrides.TryGetValue("ControllingModsTxt", out controllingOverride);
            var hasControllingOverride = !string.IsNullOrWhiteSpace(controllingOverride);
            var hasAdditionalDirectives = HasAdditionalModsPathDirectives(settingsPath);
            if (context.Layout == UE4SSLayout.StableRoot &&
                (hasControllingOverride || hasAdditionalDirectives))
            {
                throw new InvalidOperationException(
                    "The pinned stable-root UE4SS v3.0.1 build does not support ControllingModsTxt or " +
                    "+/-ModsFolderPaths. Remove those unsupported overrides or use manual installation.");
            }

            context.AdditionalModsDirectories = context.Layout == UE4SSLayout.ExperimentalNested
                ? ReadAdditionalModsDirectories(settingsPath, context.UE4SSDirectory)
                : new List<string>();
            string modsOverride;
            if (overrides.TryGetValue("ModsFolderPath", out modsOverride) && !string.IsNullOrWhiteSpace(modsOverride))
            {
                context.ModsDirectory = ResolveUE4SSPath(modsOverride, context.Win64Directory);
            }
            else
            {
                context.ModsDirectory = Path.Combine(context.UE4SSDirectory, "Mods");
            }

            if (context.Layout == UE4SSLayout.ExperimentalNested &&
                !Directory.Exists(context.ModsDirectory) &&
                !File.Exists(context.ModsDirectory))
            {
                var legacyModsDirectory = Path.Combine(context.Win64Directory, "Mods");
                if (Directory.Exists(legacyModsDirectory))
                {
                    context.ModsDirectory = legacyModsDirectory;
                }
                else if (File.Exists(legacyModsDirectory))
                {
                    throw new InvalidOperationException(
                        "Experimental UE4SS would select the legacy Win64\\Mods path, but that path is a file: " +
                        legacyModsDirectory);
                }
            }

            if (File.Exists(context.ModsDirectory))
            {
                throw new InvalidOperationException("The configured UE4SS Mods path is a file, not a directory: " + context.ModsDirectory);
            }
            EnsureInsideGameTree(context, context.ModsDirectory, "configured UE4SS Mods path");

            if (context.Layout == UE4SSLayout.ExperimentalNested && hasControllingOverride)
            {
                context.ModsTxtPath = ResolveUE4SSPath(controllingOverride, context.UE4SSDirectory);
            }
            else
            {
                context.ModsTxtPath = Path.Combine(context.ModsDirectory, "mods.txt");
            }

            if (Directory.Exists(context.ModsTxtPath))
            {
                throw new InvalidOperationException("The configured controlling mods.txt path is a directory: " + context.ModsTxtPath);
            }
            EnsureInsideGameTree(context, context.ModsTxtPath, "configured controlling mods.txt path");

            foreach (var additionalDirectory in context.AdditionalModsDirectories)
            {
                EnsureInsideGameTree(context, additionalDirectory, "additional UE4SS Mods path");
                if (string.Equals(
                    Path.GetFullPath(additionalDirectory),
                    Path.GetFullPath(context.ModsDirectory),
                    StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                if (Directory.Exists(Path.Combine(additionalDirectory, ModName)))
                {
                    throw new InvalidOperationException(
                        "An additional UE4SS Mods directory already contains " + ModName + ": " +
                        additionalDirectory + ". Remove the duplicate Mod directory before installation.");
                }

                if (!hasControllingOverride)
                {
                    var additionalModsTxt = Path.Combine(additionalDirectory, "mods.txt");
                    EnsureInsideGameTree(context, additionalModsTxt, "additional UE4SS mods.txt path");
                    if (File.Exists(additionalModsTxt) && ContainsAutoPickupEntry(additionalModsTxt))
                    {
                        throw new InvalidOperationException(
                            "An additional UE4SS mods.txt already controls " + ModName + ": " +
                            additionalModsTxt + ". Resolve the duplicate entry or set ControllingModsTxt before installation.");
                    }
                }
            }
        }

        private static void EnsureInsideGameTree(InstallContext context, string path, string description)
        {
            InstallerPathSafety.Validate(context.GameRootDirectory, path, description);
        }

        private static bool HasAdditionalModsPathDirectives(string settingsPath)
        {
            var section = string.Empty;
            foreach (var original in File.ReadAllLines(settingsPath, new UTF8Encoding(false, true)))
            {
                var line = original.Trim();
                if (line.Length == 0 || line.StartsWith(";", StringComparison.Ordinal) ||
                    line.StartsWith("#", StringComparison.Ordinal))
                {
                    continue;
                }
                if (line.StartsWith("[", StringComparison.Ordinal) && line.EndsWith("]", StringComparison.Ordinal))
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
                if (separator > 1 && string.Equals(
                        line.Substring(1, separator - 1).Trim(),
                        "ModsFolderPaths",
                        StringComparison.OrdinalIgnoreCase) &&
                    !string.IsNullOrWhiteSpace(line.Substring(separator + 1)))
                {
                    return true;
                }
            }
            return false;
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
                if (line.StartsWith("[", StringComparison.Ordinal) && line.EndsWith("]", StringComparison.Ordinal))
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
                    throw new InvalidDataException("UE4SS-settings.ini contains a duplicate " + key + " override.");
                }
                result.Add(key, line.Substring(separator + 1).Trim());
            }
            return result;
        }

        private static List<string> ReadAdditionalModsDirectories(string settingsPath, string workingDirectory)
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
                if (line.StartsWith("[", StringComparison.Ordinal) && line.EndsWith("]", StringComparison.Ordinal))
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
                if (separator <= 1 || !string.Equals(
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
                    added.Remove(resolved);
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

        private static bool ContainsAutoPickupEntry(string modsTxtPath)
        {
            var document = ReadUtf8Document(modsTxtPath);
            var pattern = "(?m)^[\\t ]*" + Regex.Escape(ModName) +
                "[\\t ]*:[^\\r\\n]*(?=\\r?$)";
            return Regex.IsMatch(document.Text, pattern, RegexOptions.CultureInvariant);
        }

        private static string ResolveUE4SSPath(string configured, string workingDirectory)
        {
            var value = configured.Trim().Trim('"');
            var combined = Path.IsPathRooted(value)
                ? value
                : Path.Combine(workingDirectory, value);
            return Path.GetFullPath(combined);
        }

        private static byte[] BuildPublicConfig(
            byte[] defaultConfig,
            string hotkey,
            string interactionKeyFallback)
        {
            var encoding = new UTF8Encoding(false, true);
            var text = encoding.GetString(defaultConfig);
            text = ReplaceUniqueSetting(text, "enabled_on_launch", "false");
            text = ReplaceUniqueSetting(text, "automatic_pickup", "true");
            text = ReplaceUniqueSetting(text, "toggle_hotkey", hotkey);
            text = ReplaceUniqueSetting(text, "interaction_key", "AUTO");
            text = ReplaceUniqueSetting(text, "interaction_key_fallback", interactionKeyFallback);
            text = ReplaceUniqueSetting(text, "debug_logging", "false");
            return new UTF8Encoding(false).GetBytes(text);
        }

        private static string ReplaceUniqueSetting(string text, string key, string value)
        {
            var pattern = "(?m)^[\\t ]*" + Regex.Escape(key) + "[\\t ]*=[^\\r\\n]*(?=\\r?$)";
            var matches = Regex.Matches(text, pattern, RegexOptions.CultureInvariant);
            if (matches.Count != 1)
            {
                throw new InvalidDataException("The embedded public config must contain exactly one " + key + " setting.");
            }
            return Regex.Replace(text, pattern, key + "=" + value, RegexOptions.CultureInvariant);
        }

        private static byte[] BuildControlledModsTxt(string path)
        {
            var document = ReadUtf8Document(path);
            var anyEntryPattern = "(?m)^[\\t ]*" + Regex.Escape(ModName) +
                "[\\t ]*:[^\\r\\n]*(?=\\r?$)";
            var validEntryPattern = "(?m)^[\\t ]*" + Regex.Escape(ModName) +
                "[\\t ]*:[\\t ]*[01][\\t ]*(?=\\r?$)";
            var anyEntries = Regex.Matches(document.Text, anyEntryPattern, RegexOptions.CultureInvariant);
            foreach (Match existingEntry in anyEntries)
            {
                if (!Regex.IsMatch(existingEntry.Value, "^[\\t ]*" + Regex.Escape(ModName) +
                        "[\\t ]*:[\\t ]*[01][\\t ]*$", RegexOptions.CultureInvariant))
                {
                    throw new InvalidDataException(
                        "The controlling mods.txt contains a malformed " + ModName + " entry. Resolve it manually.");
                }
            }

            const string entry = ModName + " : 1";
            string updated;
            if (anyEntries.Count >= 1)
            {
                updated = document.Text;
                for (var index = anyEntries.Count - 1; index >= 1; --index)
                {
                    updated = updated.Remove(anyEntries[index].Index, anyEntries[index].Length);
                }
                var first = Regex.Match(updated, validEntryPattern, RegexOptions.CultureInvariant);
                if (!first.Success)
                {
                    throw new InvalidDataException("The first controlling mods.txt entry could not be normalized.");
                }
                updated = updated.Remove(first.Index, first.Length).Insert(first.Index, entry);
            }
            else
            {
                var separator = document.Text.Length == 0 || document.Text.EndsWith("\n", StringComparison.Ordinal)
                    ? string.Empty
                    : document.NewLine;
                updated = document.Text + separator + entry + document.NewLine;
            }

            var verify = Regex.Matches(updated, validEntryPattern, RegexOptions.CultureInvariant);
            var verifyAny = Regex.Matches(updated, anyEntryPattern, RegexOptions.CultureInvariant);
            if (verify.Count != 1 || verifyAny.Count != 1 ||
                !verify[0].Value.Trim().EndsWith(": 1", StringComparison.Ordinal))
            {
                throw new InvalidDataException("The controlling mods.txt update did not verify.");
            }
            return EncodeUtf8(updated, document.HasBom);
        }

        private static void RemoveLegacyEnablementFiles(InstallContext context, InstallTransaction transaction)
        {
            var candidates = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                Path.Combine(context.ModsDirectory, ModName, "enabled.txt"),
                Path.Combine(context.StableUE4SSDirectory, "Mods", ModName, "enabled.txt"),
                Path.Combine(context.NestedUE4SSDirectory, "Mods", ModName, "enabled.txt")
            };
            foreach (var candidate in candidates)
            {
                if (File.Exists(candidate))
                {
                    transaction.DeleteFile(candidate, "Remove legacy enabled.txt that bypasses mods.txt");
                }
            }

            foreach (var additionalDirectory in context.AdditionalModsDirectories)
            {
                var candidate = Path.Combine(additionalDirectory, ModName, "enabled.txt");
                if (File.Exists(candidate))
                {
                    transaction.DeleteFile(candidate, "Remove legacy enabled.txt from additional UE4SS Mods path");
                }
            }
        }

        private static string BuildInstallRecord(
            InstallContext context,
            string layout,
            string hotkey,
            string interactionKeyFallback,
            RangeSelection rangeSelection,
            string gameHash,
            byte[] plugin,
            string backupRoot)
        {
            var builder = new StringBuilder();
            builder.AppendLine("DragonSword Native Auto Pickup install record");
            builder.AppendLine("Version: " + ProductVersion);
            builder.AppendLine("Installed UTC: " + DateTime.UtcNow.ToString("O", CultureInfo.InvariantCulture));
            builder.AppendLine("Game SHA-256: " + gameHash);
            builder.AppendLine("UE4SS layout: " + layout);
            builder.AppendLine("UE4SS bootstrapped: " + context.BootstrappedUE4SS);
            builder.AppendLine("Plugin SHA-256: " + HashBytes(plugin));
            builder.AppendLine("Toggle hotkey: " + hotkey);
            builder.AppendLine("Interaction key mode: AUTO");
            builder.AppendLine("Interaction key fallback: " + interactionKeyFallback);
            builder.AppendLine("Starts enabled: false");
            builder.AppendLine("Public debug logging: false");
            builder.AppendLine("Optional range selection: " +
                (rangeSelection == RangeSelection.None ? "None" : (int)rangeSelection + "x"));
            builder.AppendLine("Controlling mods.txt: " + context.ModsTxtPath);
            builder.AppendLine("Backup directory: " + backupRoot);
            builder.AppendLine();
            builder.AppendLine("To prevent the native plugin from loading, close the game, change the exact");
            builder.AppendLine("DragonSwordNativeAutoPickup entry in the controlling mods.txt to ': 0',");
            builder.AppendLine("and keep enabled.txt absent.");
            return builder.ToString();
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
                return new Utf8Document { Text = string.Empty, HasBom = false, NewLine = "\r\n" };
            }

            var bytes = File.ReadAllBytes(path);
            var hasBom = bytes.Length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF;
            var offset = hasBom ? 3 : 0;
            var text = new UTF8Encoding(false, true).GetString(bytes, offset, bytes.Length - offset);
            return new Utf8Document
            {
                Text = text,
                HasBom = hasBom,
                NewLine = text.Contains("\r\n") ? "\r\n" : "\n"
            };
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

        private static string ToHex(byte[] value)
        {
            var builder = new StringBuilder(value.Length * 2);
            foreach (var item in value)
            {
                builder.Append(item.ToString("X2", CultureInfo.InvariantCulture));
            }
            return builder.ToString();
        }

        private static string AppendDirectorySeparator(string value)
        {
            return value.EndsWith(Path.DirectorySeparatorChar.ToString(), StringComparison.Ordinal)
                ? value
                : value + Path.DirectorySeparatorChar;
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
                    "The " + description + " resolves outside the selected DS game directory. " +
                    "Setup will not perform elevated writes there: " + candidate);
            }

            RejectReparsePoint(root, description);
            if (!string.Equals(candidate, root, StringComparison.OrdinalIgnoreCase))
            {
                var relative = candidate.Substring(prefix.Length);
                var current = root;
                foreach (var segment in relative.Split(new[]
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

        private static void RejectReparsePoint(string path, string description)
        {
            if (!File.Exists(path) && !Directory.Exists(path))
            {
                return;
            }
            var attributes = File.GetAttributes(path);
            if ((attributes & FileAttributes.ReparsePoint) != 0)
            {
                throw new InvalidOperationException(
                    "The " + description + " crosses a symbolic link, junction, or other reparse point. " +
                    "Setup refuses elevated writes through reparse paths: " + path);
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
        private readonly Dictionary<string, OriginalFile> _originals =
            new Dictionary<string, OriginalFile>(StringComparer.OrdinalIgnoreCase);
        private readonly List<string> _order = new List<string>();
        private readonly List<string> _actions = new List<string>();
        private bool _committed;
        private bool _rolledBack;

        internal InstallTransaction(string scopeRoot, string gameRoot, string backupRoot)
        {
            _scopeRoot = AppendSeparator(Path.GetFullPath(scopeRoot));
            _gameRoot = Path.GetFullPath(gameRoot);
            _backupRoot = InstallerPathSafety.Validate(
                _gameRoot,
                backupRoot,
                "Auto Pickup backup directory");
            Directory.CreateDirectory(_backupRoot);
            InstallerPathSafety.Validate(
                _gameRoot,
                _backupRoot,
                "Auto Pickup backup directory");
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
            Directory.CreateDirectory(parent);
            InstallerPathSafety.Validate(_gameRoot, target, "installer write target");
            var temporary = Path.Combine(parent, ".dsnap-install-" + Guid.NewGuid().ToString("N") + ".tmp");
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

        internal void Commit(IEnumerable<string> summary)
        {
            var lines = new List<string>(summary);
            lines.Add(string.Empty);
            lines.Add("Recorded file operations:");
            lines.AddRange(_actions.Select(value => "- " + value));
            lines.Add(string.Empty);
            lines.Add("Original files are stored below this directory using their original relative paths.");
            var installLog = Path.Combine(_backupRoot, "INSTALL-LOG.txt");
            InstallerPathSafety.Validate(_gameRoot, installLog, "installer log file");
            File.WriteAllLines(
                installLog,
                lines,
                new UTF8Encoding(false));
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
                        InstallerPathSafety.Validate(_gameRoot, original.Target, "rollback delete target");
                        File.Delete(original.Target);
                    }
                }
                catch (Exception exception)
                {
                    failures.Add(original.Target + ": " + exception.Message);
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
                var relative = target.StartsWith(_scopeRoot, StringComparison.OrdinalIgnoreCase)
                    ? target.Substring(_scopeRoot.Length)
                    : Path.Combine(
                        "external",
                        ShortPathIdentity(target) + "-" + Path.GetFileName(target));
                backup = Path.Combine(_backupRoot, "original", relative);
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

        private void RestoreOriginal(OriginalFile original)
        {
            var parent = Path.GetDirectoryName(original.Target);
            Directory.CreateDirectory(parent);
            InstallerPathSafety.Validate(_gameRoot, original.Target, "rollback restore target");
            var temporary = Path.Combine(parent, ".dsnap-rollback-" + Guid.NewGuid().ToString("N") + ".tmp");
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
