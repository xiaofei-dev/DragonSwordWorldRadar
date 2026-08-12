using System.Text.RegularExpressions;
using CUE4Parse.Encryption.Aes;
using CUE4Parse.FileProvider;
using CUE4Parse.UE4.Objects.Core.Misc;
using CUE4Parse.UE4.Versions;
using Newtonsoft.Json.Linq;
using Newtonsoft.Json;

if (args.Length != 4)
{
    Console.Error.WriteLine("Usage: NativeAssetExtractor <game-root> <crypto-json> <asset-name> <output-root>");
    return 2;
}

var gameRoot = Path.GetFullPath(args[0]);
var cryptoPath = Path.GetFullPath(args[1]);
var assetName = args[2];
var outputRoot = Path.GetFullPath(args[3]);

var keySource = File.ReadAllText(cryptoPath);
var keyMatch = Regex.Match(keySource, "(?<![0-9a-fA-F])(?:0x)?[0-9a-fA-F]{64}(?![0-9a-fA-F])");
string? configuredKey = null;
int? configuredVersion = null;
try
{
    var configuration = JToken.Parse(keySource);
    configuredKey = configuration.SelectToken("EncryptionKey.Key")?.Value<string>();
    if (configuration["PerDirectory"] is JObject directories &&
        directories[gameRoot] is JToken gameProfile)
    {
        configuredKey = gameProfile["AesKeys"]?["mainKey"]?.Value<string>() ?? configuredKey;
        configuredVersion = gameProfile["UeVersion"]?.Value<int>();
    }
}
catch
{
    // The source may be a local program or configuration in another format.
}
var keyText = !string.IsNullOrWhiteSpace(configuredKey)
    ? configuredKey
    : keyMatch.Success ? keyMatch.Value : null;
if (keyText is null)
{
    throw new InvalidDataException("No 256-bit AES key was found in the supplied crypto configuration.");
}

using var provider = new DefaultFileProvider(
    gameRoot,
    SearchOption.AllDirectories,
    new VersionContainer(configuredVersion.HasValue ? (EGame)configuredVersion.Value : EGame.GAME_UE5_4));
provider.Initialize();
var keyBytes = Regex.IsMatch(keyText, "^(?:0x)?[0-9a-fA-F]{64}$")
    ? Convert.FromHexString(keyText.StartsWith("0x", StringComparison.OrdinalIgnoreCase) ? keyText[2..] : keyText)
    : Convert.FromBase64String(keyText);
provider.SubmitKey(new FGuid(), new FAesKey(keyBytes));

var matches = provider.Files.Keys
    .Where(path => path.Contains(assetName, StringComparison.OrdinalIgnoreCase))
    .OrderBy(path => path)
    .ToArray();
if (matches.Length == 0)
{
    var vehicleCandidates = provider.Files.Keys
        .Where(path => path.Contains("Vehicle", StringComparison.OrdinalIgnoreCase))
        .Take(20)
        .ToArray();
    var packageCandidates = provider.Files.Keys
        .Where(path => path.EndsWith(".uasset", StringComparison.OrdinalIgnoreCase))
        .Take(20)
        .ToArray();
    Console.Error.WriteLine($"INDEXED_FILES {provider.Files.Count}");
    foreach (var candidate in vehicleCandidates) Console.Error.WriteLine($"CANDIDATE {candidate}");
    foreach (var candidate in packageCandidates) Console.Error.WriteLine($"PACKAGE {candidate}");
    throw new FileNotFoundException($"Asset was not found: {assetName}");
}

foreach (var assetPath in matches)
{
    if (!provider.TrySavePackage(assetPath, out var packageFiles))
    {
        throw new InvalidDataException($"Could not extract package: {assetPath}");
    }

    foreach (var (relativePath, data) in packageFiles)
    {
        var destination = Path.GetFullPath(Path.Combine(outputRoot, relativePath.Replace('/', Path.DirectorySeparatorChar)));
        if (!destination.StartsWith(outputRoot + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidDataException($"Unsafe package path: {relativePath}");
        }
        Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
        File.WriteAllBytes(destination, data);
        Console.WriteLine($"EXTRACTED {relativePath} {data.Length}");
    }

    try
    {
        var exports = provider.LoadPackage(assetPath).GetExports();
        var propertiesPath = Path.Combine(outputRoot, assetName + ".properties.json");
        File.WriteAllText(propertiesPath, JsonConvert.SerializeObject(exports, Formatting.Indented));
        Console.WriteLine($"PROPERTIES {propertiesPath}");
    }
    catch (Exception error)
    {
        Console.Error.WriteLine($"PROPERTIES_FAILED {error.GetType().Name}: {error.Message}");
    }
}

return 0;
