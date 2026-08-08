$ErrorActionPreference = "Stop"
$modDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$toolsDir = Join-Path $modDir "tools"
$corePath = Join-Path $toolsDir "PakReaderCore.exe"
$oozPath = Join-Path $toolsDir "ooz.exe"
$outputDir = Join-Path $modDir "assault_static_probe"
$rawDir = Join-Path $outputDir "xml"
$tempDir = Join-Path ([IO.Path]::GetTempPath()) ("DSAssaultProbe-" + [Guid]::NewGuid().ToString("N"))
$stagePath = Join-Path $modDir "assault_static_probe_last_stage.txt"
$logPath = Join-Path $modDir "assault_static_probe.log"
$mutexName = "Local\DragonSwordAssaultStaticProbe_v01a4"
$utf8 = New-Object System.Text.UTF8Encoding -ArgumentList $false
$flagsStatic = [Reflection.BindingFlags]::Static -bor [Reflection.BindingFlags]::Public -bor [Reflection.BindingFlags]::NonPublic
$flagsInstance = [Reflection.BindingFlags]::Instance -bor [Reflection.BindingFlags]::Public -bor [Reflection.BindingFlags]::NonPublic


# Keep all actual MethodInfo/ConstructorInfo invocation inside C#. Windows
# PowerShell can report a value as System.String while still passing an
# internal PSObject wrapper to MethodInfo.Invoke. The bridge accepts object
# parameters, recursively removes those wrappers, converts against the exact
# reflected parameter type, and then invokes the method from managed code.
$reflectionBridgeSource = @"
using System;
using System.Globalization;
using System.Reflection;

public static class DSAssaultReflectionBridge
{
    public static object Unwrap(object value)
    {
        object current = value;
        for (int i = 0; i < 16 && current != null; i++)
        {
            Type type = current.GetType();
            if (!String.Equals(
                    type.FullName,
                    "System.Management.Automation.PSObject",
                    StringComparison.Ordinal))
            {
                break;
            }

            PropertyInfo baseObject = type.GetProperty(
                "BaseObject",
                BindingFlags.Instance | BindingFlags.Public);
            if (baseObject == null)
            {
                break;
            }

            object next = baseObject.GetValue(current, null);
            if (next == null || Object.ReferenceEquals(current, next))
            {
                break;
            }
            current = next;
        }
        return current;
    }

    private static object ConvertArgument(object value, Type targetType)
    {
        value = Unwrap(value);
        if (targetType.IsByRef)
        {
            targetType = targetType.GetElementType();
        }

        Type nullableType = Nullable.GetUnderlyingType(targetType);
        if (nullableType != null)
        {
            if (value == null)
            {
                return null;
            }
            targetType = nullableType;
        }

        if (value == null)
        {
            if (targetType.IsValueType)
            {
                throw new InvalidCastException(
                    "Cannot pass null to " + targetType.FullName + ".");
            }
            return null;
        }

        if (targetType.IsInstanceOfType(value))
        {
            return value;
        }

        if (targetType == typeof(string))
        {
            return Convert.ToString(value, CultureInfo.InvariantCulture);
        }

        if (targetType.IsEnum)
        {
            if (value is string)
            {
                return Enum.Parse(targetType, (string)value, true);
            }
            Type underlying = Enum.GetUnderlyingType(targetType);
            object numeric = Convert.ChangeType(
                value,
                underlying,
                CultureInfo.InvariantCulture);
            return Enum.ToObject(targetType, numeric);
        }

        if (targetType == typeof(IntPtr))
        {
            return new IntPtr(Convert.ToInt64(
                value,
                CultureInfo.InvariantCulture));
        }

        if (targetType.IsArray && value is Array)
        {
            Array source = (Array)value;
            Type elementType = targetType.GetElementType();
            Array converted = Array.CreateInstance(elementType, source.Length);
            for (int i = 0; i < source.Length; i++)
            {
                converted.SetValue(
                    ConvertArgument(source.GetValue(i), elementType),
                    i);
            }
            return converted;
        }

        return Convert.ChangeType(
            value,
            targetType,
            CultureInfo.InvariantCulture);
    }

    private static object[] NormalizeArguments(
        ParameterInfo[] parameters,
        object[] arguments)
    {
        arguments = arguments ?? new object[0];
        if (parameters.Length != arguments.Length)
        {
            throw new TargetParameterCountException(
                "Expected " + parameters.Length +
                " arguments, received " + arguments.Length + ".");
        }

        object[] normalized = new object[parameters.Length];
        for (int i = 0; i < parameters.Length; i++)
        {
            normalized[i] = ConvertArgument(
                arguments[i],
                parameters[i].ParameterType);
        }
        return normalized;
    }

    public static object Invoke(
        object methodObject,
        object target,
        object[] arguments)
    {
        MethodInfo method = Unwrap(methodObject) as MethodInfo;
        if (method == null)
        {
            throw new InvalidCastException(
                "The reflected member is not a MethodInfo.");
        }

        object[] normalized = NormalizeArguments(
            method.GetParameters(),
            arguments);
        try
        {
            return Unwrap(method.Invoke(Unwrap(target), normalized));
        }
        catch (TargetInvocationException error)
        {
            if (error.InnerException != null)
            {
                throw error.InnerException;
            }
            throw;
        }
    }

    public static object Construct(
        object constructorObject,
        object[] arguments)
    {
        ConstructorInfo constructor =
            Unwrap(constructorObject) as ConstructorInfo;
        if (constructor == null)
        {
            throw new InvalidCastException(
                "The reflected member is not a ConstructorInfo.");
        }

        object[] normalized = NormalizeArguments(
            constructor.GetParameters(),
            arguments);
        try
        {
            return Unwrap(constructor.Invoke(normalized));
        }
        catch (TargetInvocationException error)
        {
            if (error.InnerException != null)
            {
                throw error.InnerException;
            }
            throw;
        }
    }

    private static Type ResolveType(object typeOrTarget)
    {
        object unwrapped = Unwrap(typeOrTarget);
        Type directType = unwrapped as Type;
        if (directType != null)
        {
            return directType;
        }
        if (unwrapped == null)
        {
            throw new ArgumentNullException("typeOrTarget");
        }
        return unwrapped.GetType();
    }

    public static object FindMethod(
        object typeOrTarget,
        string name,
        int parameterCount,
        bool isStatic)
    {
        Type type = ResolveType(typeOrTarget);
        BindingFlags flags = isStatic
            ? BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic
            : BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic;
        foreach (MethodInfo method in type.GetMethods(flags))
        {
            if (String.Equals(method.Name, name, StringComparison.Ordinal)
                && method.GetParameters().Length == parameterCount)
            {
                return method;
            }
        }
        throw new MissingMethodException(
            type.FullName,
            name + "/" + parameterCount);
    }

    public static object FindConstructor(
        object typeOrTarget,
        int parameterCount)
    {
        Type type = ResolveType(typeOrTarget);
        BindingFlags flags = BindingFlags.Instance
            | BindingFlags.Public
            | BindingFlags.NonPublic;
        foreach (ConstructorInfo constructor in type.GetConstructors(flags))
        {
            if (constructor.GetParameters().Length == parameterCount)
            {
                return constructor;
            }
        }
        throw new MissingMethodException(
            type.FullName,
            ".ctor/" + parameterCount);
    }

    public static object GetMemberValue(object target, string name)
    {
        object unwrapped = Unwrap(target);
        if (unwrapped == null)
        {
            throw new ArgumentNullException("target");
        }
        Type type = unwrapped.GetType();
        BindingFlags flags = BindingFlags.Instance
            | BindingFlags.Public
            | BindingFlags.NonPublic;
        PropertyInfo property = type.GetProperty(name, flags);
        if (property != null)
        {
            return Unwrap(property.GetValue(unwrapped, null));
        }
        FieldInfo field = type.GetField(name, flags);
        if (field != null)
        {
            return Unwrap(field.GetValue(unwrapped));
        }
        throw new MissingMemberException(type.FullName, name);
    }

    public static string TypeName(object value)
    {
        object unwrapped = Unwrap(value);
        return unwrapped == null
            ? "<null>"
            : unwrapped.GetType().FullName;
    }
}
"@
Add-Type -TypeDefinition $reflectionBridgeSource -Language CSharp

function Log([string]$Message) {
    try {
        [IO.File]::AppendAllText($logPath, "[" + [DateTime]::Now.ToString("yyyy-MM-dd HH:mm:ss.fff") + "] " + $Message + [Environment]::NewLine, $utf8)
    } catch {}
}
function Stage([string]$Name, [string]$Details) {
    $text = @(
        "DragonSword Assault static PAK probe",
        "Version: 0.1a4-no-task-data-discovery",
        ("Updated: " + [DateTime]::Now.ToString("yyyy-MM-dd HH:mm:ss")),
        ("Stage: " + $Name),
        ("Details: " + $Details),
        "",
        "This probe discovers counts and linkage fields; it does not assume a Boss total or a task parameter."
    ) -join [Environment]::NewLine
    [IO.File]::WriteAllText($stagePath, $text, $utf8)
    Log ($Name + " | " + $Details)
}
function New-Args([int]$Count) {
    [object[]]$result = [System.Array]::CreateInstance([object], $Count)
    return ,$result
}
function Get-MethodExact($Type, [string]$Name, [int]$Count, [bool]$Static) {
    return [DSAssaultReflectionBridge]::FindMethod($Type, $Name, $Count, $Static)
}
function Invoke-MethodExact($Method, $Target, [object[]]$Arguments) {
    $parameters = @($Method.GetParameters())
    if ($parameters.Count -ne $Arguments.Count) {
        throw "Reflection argument count mismatch for $($Method.Name): expected $($parameters.Count), got $($Arguments.Count)."
    }
    $actualTypes = New-Object Collections.Generic.List[string]
    for ($i = 0; $i -lt $Arguments.Count; $i++) {
        $actualTypes.Add([DSAssaultReflectionBridge]::TypeName($Arguments[$i]))
    }
    try {
        $result = [DSAssaultReflectionBridge]::Invoke($Method, $Target, $Arguments)
        Write-Output -NoEnumerate $result
        return
    }
    catch {
        $expected = (@($parameters | ForEach-Object { $_.ParameterType.FullName }) -join ', ')
        $actual = ($actualTypes -join ', ')
        throw "Managed reflection bridge failed: $($Method.DeclaringType.FullName).$($Method.Name); expected=[$expected]; actual=[$actual]; $($_.Exception.Message)"
    }
}
function Invoke-Static($Type, [string]$Name, [object[]]$Arguments) {
    $method = Get-MethodExact $Type $Name $Arguments.Count $true
    $result = Invoke-MethodExact $method $null $Arguments
    Write-Output -NoEnumerate $result
}
function Invoke-Instance($Object, [string]$Name, [object[]]$Arguments) {
    $method = Get-MethodExact $Object $Name $Arguments.Count $false
    $result = Invoke-MethodExact $method $Object $Arguments
    Write-Output -NoEnumerate $result
}
function Get-Value($Object, [string]$Name) {
    $result = [DSAssaultReflectionBridge]::GetMemberValue($Object, $Name)
    Write-Output -NoEnumerate $result
}
function New-CustomReader($Type, [byte[]]$Data, [byte]$StringMask, [byte]$NumberMask) {
    $ctor = [DSAssaultReflectionBridge]::FindConstructor($Type, 3)
    $a = New-Args 3
    $a[0] = $Data
    $a[1] = $StringMask
    $a[2] = $NumberMask
    $result = [DSAssaultReflectionBridge]::Construct($ctor, $a)
    Write-Output -NoEnumerate $result
}
function Reader-Call($Reader, [string]$Name) {
    $a = New-Args 0
    $result = Invoke-Instance $Reader $Name $a
    Write-Output -NoEnumerate $result
}
function Reader-Call1($Reader, [string]$Name, $Value) {
    $a = New-Args 1; $a[0] = $Value
    $result = Invoke-Instance $Reader $Name $a
    Write-Output -NoEnumerate $result
}

function Align16([int64]$Value) {
    return [int64](($Value + 15) -band (-bnot 15))
}
function Test-LooksLikeXml([byte[]]$Data) {
    if ($null -eq $Data -or $Data.Length -lt 4) { return $false }
    $start = 0
    if ($Data.Length -ge 3 -and $Data[0] -eq 0xEF -and $Data[1] -eq 0xBB -and $Data[2] -eq 0xBF) { $start = 3 }
    while ($start -lt $Data.Length -and ($Data[$start] -eq 0x20 -or $Data[$start] -eq 0x09 -or $Data[$start] -eq 0x0D -or $Data[$start] -eq 0x0A)) { $start++ }
    if ($start -lt $Data.Length -and $Data[$start] -eq [byte][char]'<' ) { return $true }
    if ($Data.Length -ge 4 -and $Data[0] -eq 0xFF -and $Data[1] -eq 0xFE -and $Data[2] -eq [byte][char]'<' -and $Data[3] -eq 0) { return $true }
    if ($Data.Length -ge 4 -and $Data[0] -eq 0xFE -and $Data[1] -eq 0xFF -and $Data[2] -eq 0 -and $Data[3] -eq [byte][char]'<' ) { return $true }
    return $false
}
function Try-Extract-UncompressedEntry(
    $ProgramType,
    [byte[]]$EncodedEntries,
    [int]$EncodedOffset,
    $Binary,
    [byte[]]$AesKey,
    [string]$OutputPath)
{
    $offsetCandidates = New-Object Collections.Generic.List[int]
    [void]$offsetCandidates.Add($EncodedOffset)
    [byte[]]$offsetBytes = [BitConverter]::GetBytes([int]$EncodedOffset)
    [uint32]$unsignedOffset = [BitConverter]::ToUInt32($offsetBytes, 0)
    [int]$maskedOffset = [int]($unsignedOffset -band 0x7FFFFFFF)
    if (-not $offsetCandidates.Contains($maskedOffset)) { [void]$offsetCandidates.Add($maskedOffset) }

    foreach ($candidateOffset in $offsetCandidates) {
        if ($candidateOffset -lt 0 -or $candidateOffset -ge $EncodedEntries.Length) { continue }
        for ($maskValue = 0; $maskValue -le 255; $maskValue++) {
            try {
                $a = New-Args 3
                $a[0] = $EncodedEntries
                $a[1] = [int]$candidateOffset
                $a[2] = [byte]$maskValue
                $encoded = Invoke-Static $ProgramType 'ReadEncodedEntry' $a
                [int]$compressionSlot = Get-Value $encoded 'CompressionSlot'
                [bool]$entryEncrypted = Get-Value $encoded 'Encrypted'
                [uint32]$blockCount = Get-Value $encoded 'CompressionBlockCount'
                [uint64]$entryOffset = Get-Value $encoded 'Offset'
                [uint64]$uncompressedSize = Get-Value $encoded 'UncompressedSize'
                [uint64]$compressedSize = Get-Value $encoded 'CompressedSize'

                # The original treasure extractor intentionally rejects blockCount=0.
                # Small generated XML files use exactly that valid uncompressed layout.
                if ($compressionSlot -ne -1 -or $blockCount -ne 0) { continue }
                if ($uncompressedSize -eq 0 -or $uncompressedSize -gt 67108864) { continue }
                if ($compressedSize -ne $uncompressedSize) { continue }
                if ($entryOffset -ge [uint64]$Binary.BaseStream.Length) { continue }

                $a = New-Args 2
                $a[0] = $Binary
                $a[1] = $entryOffset
                $data = Invoke-Static $ProgramType 'ReadDataEntry' $a
                [uint64]$dataPakOffset = Get-Value $data 'PakOffset'
                [uint64]$dataRelativeOffset = Get-Value $data 'Offset'
                [uint64]$dataCompressed = Get-Value $data 'CompressedSize'
                [uint64]$dataUncompressed = Get-Value $data 'UncompressedSize'
                [uint32]$compressionIndex = Get-Value $data 'CompressionIndex'
                [byte]$flags = Get-Value $data 'Flags'
                $blocks = Get-Value $data 'Blocks'
                $dataBlockCount = if ($null -eq $blocks) { 0 } else { [int]$blocks.Count }

                if ($dataPakOffset -ne $entryOffset -or $dataRelativeOffset -ne 0) { continue }
                if ($compressionIndex -ne 0 -or $dataBlockCount -ne 0) { continue }
                if ($dataCompressed -ne $compressedSize -or $dataUncompressed -ne $uncompressedSize) { continue }

                [int64]$payloadPosition = $Binary.BaseStream.Position
                [int64]$storedLength = if (($flags -band 1) -ne 0 -or $entryEncrypted) { Align16 ([int64]$compressedSize) } else { [int64]$compressedSize }
                if ($storedLength -le 0 -or $payloadPosition + $storedLength -gt $Binary.BaseStream.Length) { continue }
                $Binary.BaseStream.Position = $payloadPosition
                [byte[]]$stored = $Binary.ReadBytes([int]$storedLength)
                if ($stored.Length -ne $storedLength) { continue }
                [byte[]]$raw = $stored
                if (($flags -band 1) -ne 0 -or $entryEncrypted) {
                    $a = New-Args 2
                    $a[0] = $stored
                    $a[1] = $AesKey
                    [byte[]]$raw = Invoke-Static $ProgramType 'DecryptAes' $a
                }
                if ($raw.Length -lt [int]$uncompressedSize) { continue }
                if ($raw.Length -ne [int]$uncompressedSize) {
                    [byte[]]$trimmed = New-Object byte[] ([int]$uncompressedSize)
                    [Array]::Copy($raw, 0, $trimmed, 0, [int]$uncompressedSize)
                    $raw = $trimmed
                }
                if (-not (Test-LooksLikeXml $raw)) { continue }
                [IO.File]::WriteAllBytes($OutputPath, $raw)
                return [pscustomobject]@{
                    Mode = 'UNCOMPRESSED'
                    NumberMask = $maskValue
                    EncodedOffset = $candidateOffset
                    PakOffset = $entryOffset
                    Size = $uncompressedSize
                    Encrypted = (($flags -band 1) -ne 0 -or $entryEncrypted)
                }
            }
            catch {
                # Wrong number masks intentionally fail; continue to the next one.
            }
        }
    }
    return $null
}

function Sanitize([string]$Name) {
    if ([string]::IsNullOrWhiteSpace($Name)) { return "unnamed.xml" }
    return [Regex]::Replace($Name, '[^A-Za-z0-9._-]+', '_')
}
function Resolve-Win64([string]$Start) {
    $candidate = New-Object IO.DirectoryInfo $Start
    for ($i = 0; $i -lt 12 -and $null -ne $candidate; $i++) {
        $exe = Join-Path $candidate.FullName "DSClient-Win64-Shipping.exe"
        if ([IO.File]::Exists($exe)) { return $candidate.FullName }
        $candidate = $candidate.Parent
    }
    throw "Could not locate DSClient-Win64-Shipping.exe above the mod directory."
}
function Get-ScalarFields([System.Xml.XmlNode]$Node) {
    $result = [ordered]@{}
    if ($null -ne $Node.Attributes) {
        foreach ($a in $Node.Attributes) { $result[$a.Name] = $a.Value }
    }
    foreach ($child in @($Node.ChildNodes | Where-Object { $_.NodeType -eq [System.Xml.XmlNodeType]::Element })) {
        $elementGrandchildren = @($child.ChildNodes | Where-Object { $_.NodeType -eq [System.Xml.XmlNodeType]::Element })
        if ($elementGrandchildren.Count -eq 0) { $result[$child.Name] = $child.InnerText.Trim() }
    }
    return $result
}
function Add-SchemaLine($Schemas, [string]$FileName, [string]$ElementName, $Fields) {
    $key = $FileName + " | " + $ElementName
    if (-not $Schemas.ContainsKey($key)) { $Schemas[$key] = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase) }
    foreach ($name in $Fields.Keys) { [void]$Schemas[$key].Add([string]$name) }
}

$created = $false
$mutex = New-Object Threading.Mutex($true, $mutexName, [ref]$created)
if (-not $created) { Log "Another static probe instance is already running."; exit 0 }

try {
    Remove-Item -LiteralPath $outputDir -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Path $rawDir -Force | Out-Null
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    Remove-Item -LiteralPath $logPath -Force -ErrorAction SilentlyContinue
    Stage "LOCATE_GAME" "resolving game and PAK paths"

    if (-not [IO.File]::Exists($corePath)) { throw "PakReaderCore.exe is missing." }
    if (-not [IO.File]::Exists($oozPath)) { throw "ooz.exe is missing." }
    $win64 = Resolve-Win64 $modDir
    $dsRoot = (New-Object IO.DirectoryInfo $win64).Parent.Parent.FullName
    $gameRoot = (New-Object IO.DirectoryInfo $dsRoot).Parent.FullName
    $exePath = Join-Path $win64 "DSClient-Win64-Shipping.exe"
    $pakPath = Join-Path $gameRoot "DS\Content\Paks\pakchunk109-WindowsClient.pak"
    if (-not [IO.File]::Exists($pakPath)) { throw "pakchunk109-WindowsClient.pak was not found: $pakPath" }

    Stage "LOAD_PAK_CORE" "loading read-only extraction core and managed reflection bridge"
    $assembly = [Reflection.Assembly]::LoadFrom($corePath)
    $programType = $assembly.GetType("Program", $true)
    $readerType = $assembly.GetType("CustomReader", $true)

    $a = New-Args 2; $a[0] = $exePath; $a[1] = $pakPath
    Stage "DETECT_AES" "detecting the current game PAK key through managed reflection bridge"
    Log "REFLECTION_BRIDGE_READY | C# invocation and recursive PSObject unwrapping enabled"
    [byte[]]$aesKey = Invoke-Static $programType "FindWorkingAesKey" $a

    $stream = [IO.File]::OpenRead($pakPath)
    $binary = New-Object IO.BinaryReader $stream
    try {
        $a = New-Args 1; $a[0] = $binary
        $footer = Invoke-Static $programType "ReadFooter" $a
        [bool]$encrypted = Get-Value $footer "Encrypted"
        [uint64]$indexOffset = Get-Value $footer "IndexOffset"
        [uint64]$indexSize = Get-Value $footer "IndexSize"
        $a = New-Args 3; $a[0] = $binary; $a[1] = $indexOffset; $a[2] = $indexSize
        [byte[]]$index = Invoke-Static $programType "ReadAt" $a
        if ($encrypted) {
            $a = New-Args 2; $a[0] = $index; $a[1] = $aesKey
            [byte[]]$index = Invoke-Static $programType "DecryptAes" $a
        }
        $a = New-Args 1; $a[0] = $index
        [byte]$numberMask = Invoke-Static $programType "InferNumberMask" $a
        [byte]$stringMask = [byte]($index[4] -bxor [byte][char]'.')
        $main = New-CustomReader $readerType $index $stringMask $numberMask
        $mountPoint = [string](Reader-Call $main "ReadString")
        [void](Reader-Call $main "ReadUInt32")
        [void](Reader-Call $main "ReadUInt64")
        [uint32]$hasPathHash = Reader-Call $main "ReadUInt32"
        if ($hasPathHash -ne 0) { [void](Reader-Call $main "ReadUInt64"); [void](Reader-Call $main "ReadUInt64"); [void](Reader-Call1 $main "ReadRaw" 20) }
        [uint32]$hasDirectory = Reader-Call $main "ReadUInt32"
        if ($hasDirectory -eq 0) { throw "PAK directory index is missing." }
        [uint64]$directoryOffset = Reader-Call $main "ReadUInt64"
        [uint64]$directorySize = Reader-Call $main "ReadUInt64"
        [void](Reader-Call1 $main "ReadRaw" 20)
        [uint32]$encodedSize = Reader-Call $main "ReadUInt32"
        [byte[]]$encodedEntries = Reader-Call1 $main "ReadRaw" ([int]$encodedSize)

        $a = New-Args 3; $a[0] = $binary; $a[1] = $directoryOffset; $a[2] = $directorySize
        [byte[]]$directoryIndex = Invoke-Static $programType "ReadAt" $a
        if ($encrypted) { $a = New-Args 2; $a[0] = $directoryIndex; $a[1] = $aesKey; [byte[]]$directoryIndex = Invoke-Static $programType "DecryptAes" $a }
        [byte]$directoryStringMask = if ($directoryIndex.Length -gt 8) { [byte]($directoryIndex[8] -bxor [byte][char]'/') } else { $stringMask }
        $directory = New-CustomReader $readerType $directoryIndex $directoryStringMask $numberMask

        Stage "LIST_XML" "reading all generated game-data XML names"
        [uint32]$directoryCount = Reader-Call $directory "ReadUInt32"
        $entries = New-Object Collections.Generic.List[object]
        for ($di = 0; $di -lt $directoryCount; $di++) {
            $dirName = [string](Reader-Call $directory "ReadString")
            [uint32]$fileCount = Reader-Call $directory "ReadUInt32"
            for ($fi = 0; $fi -lt $fileCount; $fi++) {
                $fileName = [string](Reader-Call1 $directory "ReadStringEndingWith" ".xml")
                [int]$encodedOffset = Reader-Call $directory "ReadInt32"
                $full = if ([string]::IsNullOrEmpty($dirName)) { $fileName } else { $dirName.TrimEnd('/','\') + "/" + $fileName }
                $entries.Add([pscustomobject]@{ Directory=$dirName; File=$fileName; FullName=$full; EncodedOffset=$encodedOffset })
            }
        }
        [IO.File]::WriteAllLines((Join-Path $outputDir "all_game_data_xml_files.txt"), [string[]]@($entries | Sort-Object FullName | ForEach-Object { $_.FullName }), $utf8)
        @($entries | Sort-Object FullName | Select-Object FullName,Directory,File,EncodedOffset) |
            Export-Csv -LiteralPath (Join-Path $outputDir 'all_game_data_xml_entries.csv') -NoTypeInformation -Encoding UTF8

        $patterns = @(
            'UnexpectedMission(World|Place|Kind)?Data',
            'UnexpectedMission',
            'Warrior',
            'Champion',
            'Assault',
            '(ActorPosition|ActorSpawnCondition|SectionMonster|SpawnGroup|SpawnMonsterGroup)Data',
            '(MonsterCharacter|MonsterLink|ActionMonsterLink)Data',
            '(SpawnCondition|Weather|Climate|RespawnCycle)Data',
            '(MapField|MapFieldMeta)Data',
            'String.*Unexpected',
            'String.*Mission'
        )
        function Rank-Candidate($Entry) {
            $name = $Entry.FullName
            if ($name -match 'UnexpectedMission(World|Place|Kind)?Data') { return 0 }
            if ($name -match 'UnexpectedMission') { return 1 }
            if ($name -match '(Warrior|Champion|Assault)') { return 2 }
            if ($name -match '(SectionMonster|SpawnGroup|SpawnMonsterGroup|ActorSpawnCondition)') { return 3 }
            if ($name -match '(SpawnCondition|Weather|Climate|RespawnCycle)') { return 4 }
            if ($name -match '(MonsterCharacter|MonsterLink|ActionMonsterLink)') { return 5 }
            if ($name -match '(MapField|ActorPosition)') { return 6 }
            return 10
        }
        $candidates = @($entries | Where-Object {
            $n = $_.FullName
            ($patterns | Where-Object { $n -match $_ }).Count -gt 0
        } | Sort-Object @{Expression={Rank-Candidate $_}}, FullName)
        if ($candidates.Count -gt 240) { $candidates = @($candidates | Select-Object -First 240) }

        Stage "EXTRACT_XML" ("candidates=" + $candidates.Count)
        $manifest = New-Object Collections.Generic.List[string]
        $manifest.Add("MountPoint: " + $mountPoint)
        $manifest.Add("AllXmlCount: " + $entries.Count)
        $manifest.Add("CandidateCount: " + $candidates.Count)
        $extracted = New-Object Collections.Generic.List[object]
        $counter = 0
        foreach ($entry in $candidates) {
            $counter++
            $outputName = ("{0:D3}_{1}" -f $counter, (Sanitize $entry.File))
            $outputPath = Join-Path $rawDir $outputName
            try {
                $primaryError = $null
                $mode = 'COMPRESSED'
                try {
                    $a = New-Args 3; $a[0] = $encodedEntries; $a[1] = [int]$entry.EncodedOffset; $a[2] = $binary
                    $encoded = Invoke-Static $programType 'FindEncodedEntry' $a
                    [uint64]$dataOffset = Get-Value $encoded 'Offset'
                    $a = New-Args 2; $a[0] = $binary; $a[1] = $dataOffset
                    $data = Invoke-Static $programType 'ReadDataEntry' $a
                    $a = New-Args 3; $a[0] = $encoded; $a[1] = $data; $a[2] = [int64]$stream.Length
                    [void](Invoke-Static $programType 'ValidateEntry' $a)
                    $a = New-Args 6; $a[0] = $binary; $a[1] = $data; $a[2] = $aesKey; $a[3] = $oozPath; $a[4] = $tempDir; $a[5] = $outputPath
                    [void](Invoke-Static $programType 'WriteDecompressedEntry' $a)
                }
                catch {
                    $primaryError = $_.Exception.Message
                    $fallback = Try-Extract-UncompressedEntry $programType $encodedEntries ([int]$entry.EncodedOffset) $binary $aesKey $outputPath
                    if ($null -eq $fallback) { throw $primaryError }
                    $mode = [string]$fallback.Mode
                }
                [xml]$verifyXml = [IO.File]::ReadAllText($outputPath)
                $manifest.Add('OK_' + $mode + ' | ' + $entry.FullName + ' | ' + $outputName + ' | encodedOffset=' + $entry.EncodedOffset)
                $extracted.Add([pscustomobject]@{ Source=$entry.FullName; Path=$outputPath; Output=$outputName; Mode=$mode; EncodedOffset=$entry.EncodedOffset })
            }
            catch {
                $manifest.Add('ERROR | ' + $entry.FullName + ' | encodedOffset=' + $entry.EncodedOffset + ' | ' + $_.Exception.Message)
                Remove-Item -LiteralPath $outputPath -Force -ErrorAction SilentlyContinue
            }
        }
        [IO.File]::WriteAllLines((Join-Path $outputDir "extraction_manifest.txt"), [string[]]$manifest, $utf8)
    }
    finally { if ($null -ne $binary) { $binary.Dispose() }; if ($null -ne $stream) { $stream.Dispose() } }

    Stage "ANALYZE_XML" ("extracted=" + $extracted.Count)
    $schemas = @{}
    $allRows = New-Object Collections.Generic.List[object]
    $bossRows = New-Object Collections.Generic.List[object]
    foreach ($file in $extracted) {
        try {
            [xml]$doc = [IO.File]::ReadAllText($file.Path)
            foreach ($node in @($doc.SelectNodes("//*"))) {
                $fields = Get-ScalarFields $node
                if ($fields.Count -lt 2) { continue }
                Add-SchemaLine $schemas $file.Source $node.Name $fields
                $row = [pscustomobject]@{ File=$file.Source; Element=$node.Name; Fields=$fields }
                $allRows.Add($row)
                $hasId = $fields.Contains("ID") -or $fields.Contains("Id") -or $fields.Contains("id")
                $bossSignal = $fields.Contains("WorldMapSectionID") -or $fields.Contains("MapIconName") -or $fields.Contains("SwitchID") -or $node.Name -match 'FieldBoss'
                if ($hasId -and $bossSignal -and ($file.Source -match 'FieldBoss' -or $node.Name -match 'FieldBoss')) { $bossRows.Add($row) }
            }
        } catch { Log ("XML_PARSE_ERROR " + $file.Source + " | " + $_.Exception.Message) }
    }

    $schemaLines = New-Object Collections.Generic.List[string]
    foreach ($key in @($schemas.Keys | Sort-Object)) {
        $schemaLines.Add($key)
        $schemaLines.Add("  " + ((@($schemas[$key]) | Sort-Object) -join ", "))
    }
    [IO.File]::WriteAllLines((Join-Path $outputDir "candidate_xml_schemas.txt"), [string[]]$schemaLines, $utf8)

    $dedupe = @{}
    $uniqueBoss = New-Object Collections.Generic.List[object]
    foreach ($row in $bossRows) {
        $f = $row.Fields
        $id = if ($f.Contains("ID")) { $f["ID"] } elseif ($f.Contains("Id")) { $f["Id"] } else { $f["id"] }
        $key = $row.File + "|" + $row.Element + "|" + $id + "|" + $f["MapIconName"] + "|" + $f["WorldMapSectionID"] + "|" + $f["SwitchID"]
        if (-not $dedupe.ContainsKey($key)) { $dedupe[$key] = $true; $uniqueBoss.Add($row) }
    }

    $fieldNames = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($row in $uniqueBoss) { foreach ($name in $row.Fields.Keys) { [void]$fieldNames.Add([string]$name) } }
    $orderedFields = @($fieldNames | Sort-Object)
    $flatBoss = foreach ($row in $uniqueBoss) {
        $o = [ordered]@{ SourceFile=$row.File; Element=$row.Element }
        foreach ($name in $orderedFields) { $o[$name] = if ($row.Fields.Contains($name)) { $row.Fields[$name] } else { "" } }
        [pscustomobject]$o
    }
    if (@($flatBoss).Count -gt 0) { $flatBoss | Export-Csv -LiteralPath (Join-Path $outputDir "boss_definition_candidates.csv") -NoTypeInformation -Encoding UTF8 }

    $valueIndex = @{}
    foreach ($row in $allRows) {
        if ($row.File -match 'FieldBoss') { continue }
        foreach ($pair in $row.Fields.GetEnumerator()) {
            $value = [string]$pair.Value
            if ([string]::IsNullOrWhiteSpace($value) -or $value -eq "0") { continue }
            if (-not $valueIndex.ContainsKey($value)) { $valueIndex[$value] = New-Object Collections.Generic.List[object] }
            if ($valueIndex[$value].Count -lt 80) { $valueIndex[$value].Add([pscustomobject]@{ File=$row.File; Element=$row.Element; Field=[string]$pair.Key }) }
        }
    }
    $cross = New-Object Collections.Generic.List[object]
    $lookupFields = @('ID','SwitchID','SwitchWeekID','Kind','WorldMapSectionID','UpdateTarget')
    foreach ($row in $uniqueBoss) {
        $bossId = if ($row.Fields.Contains('ID')) { [string]$row.Fields['ID'] } else { '' }
        foreach ($sourceField in $lookupFields) {
            if (-not $row.Fields.Contains($sourceField)) { continue }
            $value = [string]$row.Fields[$sourceField]
            if ([string]::IsNullOrWhiteSpace($value) -or $value -eq '0' -or -not $valueIndex.ContainsKey($value)) { continue }
            foreach ($hit in $valueIndex[$value]) {
                $cross.Add([pscustomobject]@{ BossID=$bossId; BossField=$sourceField; Value=$value; CandidateFile=$hit.File; CandidateElement=$hit.Element; CandidateField=$hit.Field; MissionLike=([bool](($hit.File + ' ' + $hit.Field + ' ' + $hit.Element) -match '(Mission|Quest|Task|Request|Weekly|Contents|Group|List|Switch)')) })
            }
        }
    }
    if ($cross.Count -gt 0) { $cross | Sort-Object BossID,BossField,CandidateFile,CandidateField | Export-Csv -LiteralPath (Join-Path $outputDir "boss_cross_reference_candidates.csv") -NoTypeInformation -Encoding UTF8 }

    $directLinkFields = @($orderedFields | Where-Object { $_ -match '(Mission|Quest|Task|Request|Weekly|Contents|Group|List|Switch)' })
    $missionCrossCount = @($cross | Where-Object { $_.MissionLike }).Count
    $sourceCounts = @($uniqueBoss | Group-Object File | Sort-Object Count -Descending | ForEach-Object { [pscustomobject]@{ File=$_.Name; Count=$_.Count } })
    $summary = [ordered]@{
        version = '0.1a4-no-task-data-discovery'
        generatedAt = [DateTime]::Now.ToString('o')
        assumptions = [ordered]@{ expectedBossCount = $null; expectedCurrentRotationCount = $null; assumedTaskLinkField = $null }
        allXmlCount = $entries.Count
        extractedCandidateCount = $extracted.Count
        extractedByMode = @($extracted | Group-Object Mode | Sort-Object Name | ForEach-Object { [pscustomobject]@{ Mode=$_.Name; Count=$_.Count } })
        extractedSourceFiles = @($extracted | Sort-Object Source | ForEach-Object { $_.Source })
        bossDefinitionCandidateCount = $uniqueBoss.Count
        bossDefinitionCountBySource = $sourceCounts
        bossDefinitionFields = $orderedFields
        directMissionLikeFieldsOnBossRecord = $directLinkFields
        crossReferenceCandidateCount = $cross.Count
        missionLikeCrossReferenceCount = $missionCrossCount
        interpretation = @(
            'bossDefinitionCandidateCount is the static definition count found in PAK XML, not automatically the current server rotation count.',
            'A mission/task mapping is only confirmed after reviewing direct fields or cross-reference candidates.',
            'The probe deliberately does not assume that the current rotation contains 9 bosses.'
        )
    }
    [IO.File]::WriteAllText((Join-Path $outputDir "world_boss_static_summary.json"), ($summary | ConvertTo-Json -Depth 8), $utf8)

    Stage 'COMPLETE' ('staticBossCandidates=' + $uniqueBoss.Count + '; missionLikeCrossRefs=' + $missionCrossCount + '; extracted=' + $extracted.Count)
}
catch {
    Log ($_ | Out-String)
    Stage "ERROR" $_.Exception.Message
    exit 1
}
finally {
    Remove-Item -LiteralPath $tempDir -Recurse -Force -ErrorAction SilentlyContinue
    if ($null -ne $mutex) { try { $mutex.ReleaseMutex() } catch {}; $mutex.Dispose() }
}
