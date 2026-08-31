using System.Globalization;
using System.Security.Cryptography;
using System.Text.Json;
using UAssetAPI;
using UAssetAPI.ExportTypes;
using UAssetAPI.PropertyTypes.Objects;
using UAssetAPI.PropertyTypes.Structs;
using UAssetAPI.UnrealTypes;
using UAssetAPI.Unversioned;

static Dictionary<string, string> ParseArguments(string[] arguments)
{
    if (arguments.Length % 2 != 0)
    {
        throw new ArgumentException("Arguments must be supplied as --name value pairs.");
    }
    var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    for (var index = 0; index < arguments.Length; index += 2)
    {
        if (!arguments[index].StartsWith("--", StringComparison.Ordinal) ||
            !result.TryAdd(arguments[index][2..], arguments[index + 1]))
        {
            throw new ArgumentException("Invalid or duplicate argument: " + arguments[index]);
        }
    }
    return result;
}

static string Require(Dictionary<string, string> arguments, string name)
{
    if (!arguments.TryGetValue(name, out var value) || string.IsNullOrWhiteSpace(value))
    {
        throw new ArgumentException("Missing required argument --" + name + ".");
    }
    return value;
}

static string HashFile(string path)
{
    using var stream = File.OpenRead(path);
    return Convert.ToHexString(SHA256.HashData(stream));
}

static string PartnerPath(string uassetPath) => Path.ChangeExtension(uassetPath, ".uexp");

static string PropertySignature(PropertyData property)
{
    if (property is StructPropertyData structure)
    {
        return $"{property.Name}|{property.GetType().Name}|[{string.Join(";", structure.Value.Select(PropertySignature))}]";
    }
    if (property is ArrayPropertyData array)
    {
        return $"{property.Name}|{property.GetType().Name}|[{string.Join(";", array.Value.Select(PropertySignature))}]";
    }
    return $"{property.Name}|{property.GetType().Name}|{property.RawValue}";
}

static string ExportSignature(NormalExport export) =>
    string.Join("\n", export.Data.Select(PropertySignature));

var parsed = ParseArguments(args);
var input = Path.GetFullPath(Require(parsed, "input"));
var output = Path.GetFullPath(Require(parsed, "output"));
var mappingsPath = Path.GetFullPath(Require(parsed, "mappings"));
var expectedUassetHash = Require(parsed, "expected-uasset-sha256").ToUpperInvariant();
var expectedUexpHash = Require(parsed, "expected-uexp-sha256").ToUpperInvariant();
var multiplier = double.Parse(Require(parsed, "multiplier"), CultureInfo.InvariantCulture);

if (string.Equals(input, output, StringComparison.OrdinalIgnoreCase))
{
    throw new InvalidOperationException("Input and output asset paths must differ.");
}
if (multiplier is < 1.0 or > 20.0 || !double.IsFinite(multiplier))
{
    throw new ArgumentOutOfRangeException(nameof(multiplier), "Multiplier must be finite and in [1, 20].");
}
var inputUexp = PartnerPath(input);
if (!File.Exists(input) || !File.Exists(inputUexp) || !File.Exists(mappingsPath))
{
    throw new FileNotFoundException("Input uasset, paired uexp, and mappings file are all required.");
}
if (!string.Equals(HashFile(input), expectedUassetHash, StringComparison.Ordinal) ||
    !string.Equals(HashFile(inputUexp), expectedUexpHash, StringComparison.Ordinal))
{
    throw new InvalidDataException("Source asset hash does not match the reviewed manifest.");
}

var mappings = new Usmap(mappingsPath);
var asset = new UAsset(input, EngineVersion.VER_UE5_3, mappings);
if (!asset.VerifyBinaryEquality())
{
    throw new InvalidDataException("Source asset cannot be serialized with binary equality.");
}

var normalExports = asset.Exports.OfType<NormalExport>().ToArray();
var overlap = normalExports.Single(export => export.ObjectName.ToString() == "SphereOverlapComp");
var originalOverlapProperties = overlap.Data.Select(property => property.Name.ToString()).ToArray();
if (originalOverlapProperties.Length != 1 || originalOverlapProperties[0] != "AttachParent")
{
    throw new InvalidDataException(
        "SphereOverlapComp must contain only the reviewed AttachParent property before patching.");
}
foreach (var protectedName in new[] { "CapsulePhysicsComp", "SphereHitComp" })
{
    if (normalExports.Count(export => export.ObjectName.ToString() == protectedName) != 1)
    {
        throw new InvalidDataException("Expected exactly one protected component export: " + protectedName);
    }
}
var beforeSignatures = normalExports.ToDictionary(
    export => export.ObjectName.ToString(),
    ExportSignature,
    StringComparer.Ordinal);

var scaleTemplates = normalExports
    .SelectMany(export => export.Data.OfType<StructPropertyData>())
    .Where(property => property.Name.ToString() == "RelativeScale3D" &&
        property.Value.OfType<VectorPropertyData>().Count() == 1)
    .ToArray();
var scale = scaleTemplates.Length == 0
    ? new StructPropertyData(new FName(asset, "RelativeScale3D"), new FName(asset, "Vector"))
    {
        Value = new List<PropertyData>
        {
            new VectorPropertyData(new FName(asset, "RelativeScale3D"))
            {
                Value = new FVector(multiplier, multiplier, multiplier)
            }
        }
    }
    : (StructPropertyData)scaleTemplates[0].Clone();
scale.Value.OfType<VectorPropertyData>().Single().Value = new FVector(multiplier, multiplier, multiplier);
overlap.Data.Add(scale);

foreach (var export in normalExports.Where(export => export != overlap))
{
    if (!string.Equals(beforeSignatures[export.ObjectName.ToString()], ExportSignature(export), StringComparison.Ordinal))
    {
        throw new InvalidDataException("Unexpected semantic mutation outside SphereOverlapComp: " + export.ObjectName);
    }
}

Directory.CreateDirectory(Path.GetDirectoryName(output)!);
asset.Write(output);
var outputUexp = PartnerPath(output);
if (!File.Exists(output) || !File.Exists(outputUexp))
{
    throw new InvalidDataException("Structured writer did not produce both uasset and uexp outputs.");
}

var reread = new UAsset(output, EngineVersion.VER_UE5_3, mappings);
if (!reread.VerifyBinaryEquality())
{
    throw new InvalidDataException("Patched asset cannot be re-serialized with binary equality.");
}
var rereadExports = reread.Exports.OfType<NormalExport>().ToArray();
var rereadOverlap = rereadExports.Single(export => export.ObjectName.ToString() == "SphereOverlapComp");
var rereadScale = rereadOverlap.Data.OfType<StructPropertyData>()
    .Single(property => property.Name.ToString() == "RelativeScale3D")
    .Value.OfType<VectorPropertyData>().Single().Value;
if (rereadScale.X != multiplier || rereadScale.Y != multiplier || rereadScale.Z != multiplier)
{
    throw new InvalidDataException("Patched SphereOverlapComp scale did not survive reload.");
}
foreach (var protectedName in new[] { "CapsulePhysicsComp", "SphereHitComp" })
{
    var rereadProtected = rereadExports.Single(export => export.ObjectName.ToString() == protectedName);
    if (!string.Equals(beforeSignatures[protectedName], ExportSignature(rereadProtected), StringComparison.Ordinal))
    {
        throw new InvalidDataException("Protected component changed after serialization: " + protectedName);
    }
}

Console.WriteLine(JsonSerializer.Serialize(new
{
    input,
    output,
    component = "SphereOverlapComp",
    property = "RelativeScale3D",
    scale = new[] { multiplier, multiplier, multiplier },
    protected_components = new[] { "CapsulePhysicsComp", "SphereHitComp" },
    output_uasset_sha256 = HashFile(output),
    output_uexp_sha256 = HashFile(outputUexp),
    binary_equality_after_reload = true
}));
