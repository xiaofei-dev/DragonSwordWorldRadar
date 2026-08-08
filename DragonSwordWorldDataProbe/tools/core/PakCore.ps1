# Shared adapter around the proven PakReaderCore.exe + ooz.exe extraction path.
# This file contains no game-memory access and performs read-only PAK operations.
$flagsStatic = [Reflection.BindingFlags]::Static -bor [Reflection.BindingFlags]::Public -bor [Reflection.BindingFlags]::NonPublic
$flagsInstance = [Reflection.BindingFlags]::Instance -bor [Reflection.BindingFlags]::Public -bor [Reflection.BindingFlags]::NonPublic
if ($null -eq $utf8) { $utf8 = New-Object System.Text.UTF8Encoding -ArgumentList $false }
if ($null -eq $logPath) { $logPath = Join-Path $env:TEMP 'DSWDP-PakCore.log' }
if ($null -eq $stagePath) { $stagePath = Join-Path $env:TEMP 'DSWDP-PakCore-stage.txt' }

# Keep all actual MethodInfo/ConstructorInfo invocation inside C#. Windows
# PowerShell can report a value as System.String while still passing an
# internal PSObject wrapper to MethodInfo.Invoke. The bridge accepts object
# parameters, recursively removes those wrappers, converts against the exact
# reflected parameter type, and then invokes the method from managed code.
$reflectionBridgeSource = @"
using System;
using System.Globalization;
using System.Reflection;

public static class DSModularReflectionBridge
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
if (-not ("DSModularReflectionBridge" -as [type])) { Add-Type -TypeDefinition $reflectionBridgeSource -Language CSharp }

function Log([string]$Message) {
    try {
        [IO.File]::AppendAllText($logPath, "[" + [DateTime]::Now.ToString("yyyy-MM-dd HH:mm:ss.fff") + "] " + $Message + [Environment]::NewLine, $utf8)
    } catch {}
}
function Stage([string]$Name, [string]$Details) {
    if ([string]::IsNullOrWhiteSpace($stagePath)) { return }
    $text = @(
        "DragonSword modular PAK static module",
        "Version: 1.0.0",
        ("Updated: " + [DateTime]::Now.ToString("yyyy-MM-dd HH:mm:ss")),
        ("Stage: " + $Name),
        ("Details: " + $Details)
    ) -join [Environment]::NewLine
    [IO.File]::WriteAllText($stagePath, $text, $utf8)
    Log ($Name + " | " + $Details)
}
function New-Args([int]$Count) {
    [object[]]$result = [System.Array]::CreateInstance([object], $Count)
    return ,$result
}
function Get-MethodExact($Type, [string]$Name, [int]$Count, [bool]$Static) {
    return [DSModularReflectionBridge]::FindMethod($Type, $Name, $Count, $Static)
}
function Invoke-MethodExact($Method, $Target, [object[]]$Arguments) {
    $parameters = @($Method.GetParameters())
    if ($parameters.Count -ne $Arguments.Count) {
        throw "Reflection argument count mismatch for $($Method.Name): expected $($parameters.Count), got $($Arguments.Count)."
    }
    $actualTypes = New-Object Collections.Generic.List[string]
    for ($i = 0; $i -lt $Arguments.Count; $i++) {
        $actualTypes.Add([DSModularReflectionBridge]::TypeName($Arguments[$i]))
    }
    try {
        $result = [DSModularReflectionBridge]::Invoke($Method, $Target, $Arguments)
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
    $result = [DSModularReflectionBridge]::GetMemberValue($Object, $Name)
    Write-Output -NoEnumerate $result
}
function New-CustomReader($Type, [byte[]]$Data, [byte]$StringMask, [byte]$NumberMask) {
    $ctor = [DSModularReflectionBridge]::FindConstructor($Type, 3)
    $a = New-Args 3
    $a[0] = $Data
    $a[1] = $StringMask
    $a[2] = $NumberMask
    $result = [DSModularReflectionBridge]::Construct($ctor, $a)
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

function Test-DSXmlIdentity {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [string]$ExpectedRoot = ''
    )
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [pscustomobject]@{ Valid=$false; ActualRoot=''; Error='output_missing' }
    }
    try {
        [xml]$document = [IO.File]::ReadAllText($Path)
        $actual = if ($null -ne $document.DocumentElement) { [string]$document.DocumentElement.LocalName } else { '' }
        $valid = -not [string]::IsNullOrWhiteSpace($actual)
        if ($valid -and -not [string]::IsNullOrWhiteSpace($ExpectedRoot)) {
            $valid = [string]::Equals($actual,$ExpectedRoot,[StringComparison]::Ordinal)
        }
        return [pscustomobject]@{ Valid=$valid; ActualRoot=$actual; Error=if ($valid) { '' } else { 'xml_root_mismatch' } }
    }
    catch {
        return [pscustomobject]@{ Valid=$false; ActualRoot=''; Error=(Get-ExceptionSummary -ErrorObject $_ -MaximumLength 500) }
    }
}

function Read-DSExactCompactEntry {
    param(
        [Parameter(Mandatory=$true)][object]$Context,
        [Parameter(Mandatory=$true)][object]$Entry
    )
    if ([int]$Entry.EncodedOffset -lt 0) {
        throw 'The directory entry is not stored in the compact encoded-entry buffer.'
    }
    [int]$offset = [int]$Entry.NormalizedEncodedOffset
    [int]$recordLength = [int]$Entry.EncodedRecordLength
    if ($offset -lt 0 -or $recordLength -le 0 -or $offset + $recordLength -gt $Context.EncodedEntries.Length) {
        throw ('Invalid compact entry range: offset={0}; length={1}; buffer={2}' -f $offset,$recordLength,$Context.EncodedEntries.Length)
    }

    [byte[]]$record = New-Object byte[] $recordLength
    [Array]::Copy($Context.EncodedEntries,$offset,$record,0,$recordLength)
    $reader = New-CustomReader $Context.ReaderType $record $Context.StringMask $Context.NumberMask
    [uint32]$bits = Reader-Call $reader 'ReadUInt32'
    [int]$consumed = 4

    [uint32]$compressionCode = ($bits -shr 23) -band 0x3f
    [int]$compressionSlot = if ($compressionCode -eq 0) { -1 } else { [int]$compressionCode - 1 }
    [bool]$encrypted = ($bits -band [uint32]4194304) -ne 0
    [uint32]$blockCount = ($bits -shr 6) -band 0xffff
    [uint32]$blockSizeCode = $bits -band 0x3f
    [uint32]$compressionBlockSize = 0
    if ($blockSizeCode -eq 0x3f) {
        [uint32]$compressionBlockSize = Reader-Call $reader 'ReadUInt32'
        $consumed += 4
    } else {
        [uint32]$compressionBlockSize = $blockSizeCode -shl 11
    }

    [bool]$offset32 = ($bits -band [uint32]2147483648) -ne 0
    [bool]$uncompressed32 = ($bits -band [uint32]1073741824) -ne 0
    [bool]$compressed32 = ($bits -band [uint32]536870912) -ne 0

    [uint64]$pakOffset = if ($offset32) { $consumed += 4; [uint64](Reader-Call $reader 'ReadUInt32') } else { $consumed += 8; [uint64](Reader-Call $reader 'ReadUInt64') }
    [uint64]$uncompressedSize = if ($uncompressed32) { $consumed += 4; [uint64](Reader-Call $reader 'ReadUInt32') } else { $consumed += 8; [uint64](Reader-Call $reader 'ReadUInt64') }
    [uint64]$compressedSize = $uncompressedSize
    if ($compressionCode -ne 0) {
        $compressedSize = if ($compressed32) { $consumed += 4; [uint64](Reader-Call $reader 'ReadUInt32') } else { $consumed += 8; [uint64](Reader-Call $reader 'ReadUInt64') }
    }

    $blockSizes = New-Object System.Collections.Generic.List[uint32]
    if ($compressionCode -ne 0 -and ($blockCount -gt 1 -or $encrypted)) {
        for ($i=0; $i -lt $blockCount; $i++) {
            [uint32]$blockSize = Reader-Call $reader 'ReadUInt32'
            $blockSizes.Add($blockSize)
            $consumed += 4
        }
    }

    if ($consumed -ne $recordLength) {
        throw ('Compact entry length mismatch: directory_length={0}; parsed_length={1}; bits=0x{2:X8}' -f $recordLength,$consumed,$bits)
    }
    if ($pakOffset -ge [uint64]$Context.PakLength -or $uncompressedSize -eq 0 -or $compressedSize -eq 0) {
        throw ('Compact entry values are outside the PAK: offset={0}; compressed={1}; uncompressed={2}; pak={3}' -f $pakOffset,$compressedSize,$uncompressedSize,$Context.PakLength)
    }

    # Construct the helper's typed entry at the exact same offset and with the single
    # global number mask inferred from the PAK index. This is not a mask search.
    $a = New-Args 3
    $a[0] = $Context.EncodedEntries
    $a[1] = $offset
    $a[2] = [byte]$Context.NumberMask
    $typed = Invoke-Static $Context.ProgramType 'ReadEncodedEntry' $a
    [int]$typedSlot = Get-Value $typed 'CompressionSlot'
    [bool]$typedEncrypted = Get-Value $typed 'Encrypted'
    [uint32]$typedBlocks = Get-Value $typed 'CompressionBlockCount'
    [uint64]$typedOffset = Get-Value $typed 'Offset'
    [uint64]$typedUncompressed = Get-Value $typed 'UncompressedSize'
    [uint64]$typedCompressed = Get-Value $typed 'CompressedSize'
    if ($typedSlot -ne $compressionSlot -or $typedEncrypted -ne $encrypted -or
        $typedBlocks -ne $blockCount -or $typedOffset -ne $pakOffset -or
        $typedUncompressed -ne $uncompressedSize -or $typedCompressed -ne $compressedSize) {
        throw 'The independent compact parser and PakReaderCore.ReadEncodedEntry disagree.'
    }

    return [pscustomobject]@{
        TypedEntry = $typed
        Bits = $bits
        CompressionCode = $compressionCode
        CompressionSlot = $compressionSlot
        Encrypted = $encrypted
        CompressionBlockCount = $blockCount
        CompressionBlockSize = $compressionBlockSize
        CompressionBlockSizes = @($blockSizes)
        Offset32 = $offset32
        Uncompressed32 = $uncompressed32
        Compressed32 = $compressed32
        PakOffset = $pakOffset
        CompressedSize = $compressedSize
        UncompressedSize = $uncompressedSize
        EncodedOffset = $offset
        NextEncodedOffset = [int]$Entry.NextEncodedOffset
        RecordLength = $recordLength
        ConsumedLength = $consumed
        NumberMask = [int]$Context.NumberMask
        RawRecord = $record
    }
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



function Open-DSPakContext {
    param(
        [Parameter(Mandatory=$true)][string]$Root,
        [Parameter(Mandatory=$true)][object]$Suite
    )
    $layout = Resolve-GameLayout -ProbeRoot $Root -Suite $Suite
    $corePath = Join-Path $Root 'tools\vendor\PakReaderCore.exe'
    $oozPath = Join-Path $Root 'tools\vendor\ooz.exe'
    if (-not (Test-Path -LiteralPath $corePath -PathType Leaf)) { throw "PakReaderCore.exe is missing: $corePath" }
    if (-not (Test-Path -LiteralPath $oozPath -PathType Leaf)) { throw "ooz.exe is missing: $oozPath" }
    if (-not (Test-Path -LiteralPath $layout.pak_path -PathType Leaf)) { throw "Game PAK is missing: $($layout.pak_path)" }

    $assembly = [Reflection.Assembly]::LoadFrom($corePath)
    $programType = $assembly.GetType('Program',$true)
    $readerType = $assembly.GetType('CustomReader',$true)
    $a = New-Args 2; $a[0] = $layout.exe_path; $a[1] = $layout.pak_path
    [byte[]]$aesKey = Invoke-Static $programType 'FindWorkingAesKey' $a

    $stream = [IO.File]::OpenRead($layout.pak_path)
    $binary = New-Object IO.BinaryReader $stream
    try {
        $a = New-Args 1; $a[0] = $binary
        $footer = Invoke-Static $programType 'ReadFooter' $a
        [bool]$encrypted = Get-Value $footer 'Encrypted'
        [uint64]$indexOffset = Get-Value $footer 'IndexOffset'
        [uint64]$indexSize = Get-Value $footer 'IndexSize'
        $a = New-Args 3; $a[0] = $binary; $a[1] = $indexOffset; $a[2] = $indexSize
        [byte[]]$index = Invoke-Static $programType 'ReadAt' $a
        if ($encrypted) { $a = New-Args 2; $a[0] = $index; $a[1] = $aesKey; [byte[]]$index = Invoke-Static $programType 'DecryptAes' $a }
        $a = New-Args 1; $a[0] = $index
        [byte]$numberMask = Invoke-Static $programType 'InferNumberMask' $a
        [byte]$stringMask = [byte]($index[4] -bxor [byte][char]'.')
        $main = New-CustomReader $readerType $index $stringMask $numberMask
        $mountPoint = [string](Reader-Call $main 'ReadString')
        [uint32]$entryCount = Reader-Call $main 'ReadUInt32'
        [uint64]$pathHashSeed = Reader-Call $main 'ReadUInt64'
        [uint32]$hasPathHash = Reader-Call $main 'ReadUInt32'
        if ($hasPathHash -ne 0) { [void](Reader-Call $main 'ReadUInt64'); [void](Reader-Call $main 'ReadUInt64'); [void](Reader-Call1 $main 'ReadRaw' 20) }
        [uint32]$hasDirectory = Reader-Call $main 'ReadUInt32'
        if ($hasDirectory -eq 0) { throw 'PAK full directory index is missing.' }
        [uint64]$directoryOffset = Reader-Call $main 'ReadUInt64'
        [uint64]$directorySize = Reader-Call $main 'ReadUInt64'
        [void](Reader-Call1 $main 'ReadRaw' 20)
        [uint32]$encodedSize = Reader-Call $main 'ReadUInt32'
        [byte[]]$encodedEntries = Reader-Call1 $main 'ReadRaw' ([int]$encodedSize)

        $a = New-Args 3; $a[0] = $binary; $a[1] = $directoryOffset; $a[2] = $directorySize
        [byte[]]$directoryIndex = Invoke-Static $programType 'ReadAt' $a
        if ($encrypted) { $a = New-Args 2; $a[0] = $directoryIndex; $a[1] = $aesKey; [byte[]]$directoryIndex = Invoke-Static $programType 'DecryptAes' $a }
        [byte]$directoryStringMask = if ($directoryIndex.Length -gt 8) { [byte]($directoryIndex[8] -bxor [byte][char]'/') } else { $stringMask }
        $directory = New-CustomReader $readerType $directoryIndex $directoryStringMask $numberMask
        [uint32]$directoryCount = Reader-Call $directory 'ReadUInt32'
        $entries = New-Object System.Collections.Generic.List[object]
        for ($di = 0; $di -lt $directoryCount; $di++) {
            $dirName = [string](Reader-Call $directory 'ReadString')
            [uint32]$fileCount = Reader-Call $directory 'ReadUInt32'
            for ($fi = 0; $fi -lt $fileCount; $fi++) {
                $fileName = [string](Reader-Call1 $directory 'ReadStringEndingWith' '.xml')
                [int]$encodedOffset = Reader-Call $directory 'ReadInt32'
                $full = if ([string]::IsNullOrEmpty($dirName)) { $fileName } else { $dirName.TrimEnd('/','\') + '/' + $fileName }
                $entries.Add([pscustomobject]@{
                    Directory=$dirName; File=$fileName; FullName=$full; EncodedOffset=$encodedOffset;
                    NormalizedEncodedOffset=if ($encodedOffset -ge 0) { $encodedOffset } else { $null };
                    NextEncodedOffset=$null; EncodedRecordLength=$null; StorageKind=if ($encodedOffset -ge 0) { 'compact' } elseif ($encodedOffset -eq [int]::MinValue) { 'deleted' } else { 'regular_index' }
                })
            }
        }

        # Directory entries store offsets into one shared compact-entry buffer. The next
        # greater compact offset gives the exact record boundary. Kind=15620, Place=15636,
        # World=15652 therefore form 16,16,12-byte records rather than guessed headers.
        $positiveOffsets = @($entries | Where-Object { [int]$_.EncodedOffset -ge 0 } | ForEach-Object { [int]$_.EncodedOffset } | Sort-Object -Unique)
        $nextByOffset = @{}
        for ($i=0; $i -lt $positiveOffsets.Count; $i++) {
            [int]$current = $positiveOffsets[$i]
            [int]$next = if ($i + 1 -lt $positiveOffsets.Count) { [int]$positiveOffsets[$i+1] } else { [int]$encodedEntries.Length }
            $nextByOffset[$current] = $next
        }
        foreach ($entry in $entries) {
            if ([int]$entry.EncodedOffset -ge 0) {
                [int]$current = [int]$entry.EncodedOffset
                [int]$next = [int]$nextByOffset[$current]
                $entry.NextEncodedOffset = $next
                $entry.EncodedRecordLength = $next - $current
            }
        }

        $context = [pscustomobject]@{
            Layout=$layout; CorePath=$corePath; OozPath=$oozPath; Assembly=$assembly;
            ProgramType=$programType; ReaderType=$readerType; AesKey=$aesKey;
            Stream=$stream; Binary=$binary; Footer=$footer; EncryptedIndex=$encrypted;
            IndexOffset=$indexOffset; IndexSize=$indexSize; NumberMask=$numberMask;
            StringMask=$stringMask; DirectoryStringMask=$directoryStringMask;
            PathHashSeed=$pathHashSeed; DeclaredEntryCount=$entryCount; MountPoint=$mountPoint;
            EncodedEntries=$encodedEntries; Entries=$entries; PakLength=$stream.Length
        }
        Write-Output -NoEnumerate $context
    }
    catch {
        try { $binary.Dispose() } catch {}
        try { $stream.Dispose() } catch {}
        throw
    }
}

function Close-DSPakContext {
    param([object]$Context)
    if ($null -eq $Context) { return }
    try { if ($null -ne $Context.Binary) { $Context.Binary.Dispose() } } catch {}
    try { if ($null -ne $Context.Stream) { $Context.Stream.Dispose() } } catch {}
}

function Select-DSPakEntries {
    param(
        [Parameter(Mandatory=$true)][object]$Context,
        [Parameter(Mandatory=$true)][object]$TargetProfile
    )
    $typedTargets = @{}
    foreach ($definition in @($TargetProfile.targets)) {
        if ($null -ne $definition -and -not [string]::IsNullOrWhiteSpace([string]$definition.file)) {
            $typedTargets[[string]$definition.file] = $definition
        }
    }
    $critical = @{}; foreach ($name in @($TargetProfile.critical_files)) { $critical[[string]$name] = 0 }
    $support = @{}; foreach ($name in @($TargetProfile.support_files)) { $support[[string]$name] = 1 }
    foreach ($name in $typedTargets.Keys) {
        if ($typedTargets[$name].required -eq $true) { $critical[$name] = 0 } else { $support[$name] = 1 }
    }

    $selected = New-Object System.Collections.Generic.List[object]
    foreach ($entry in @($Context.Entries)) {
        $rank = $null; $definition = $null
        if ($typedTargets.ContainsKey([string]$entry.File)) { $definition = $typedTargets[[string]$entry.File] }
        if ($critical.ContainsKey([string]$entry.File)) { $rank = 0 }
        elseif ($support.ContainsKey([string]$entry.File)) { $rank = 1 }
        else {
            foreach ($pattern in @($TargetProfile.patterns)) {
                if ([string]$entry.FullName -match [string]$pattern) { $rank = 2; break }
            }
        }
        if ($null -ne $rank) {
            $selected.Add([pscustomobject]@{
                Directory=$entry.Directory; File=$entry.File; FullName=$entry.FullName;
                EncodedOffset=$entry.EncodedOffset; NormalizedEncodedOffset=$entry.NormalizedEncodedOffset;
                NextEncodedOffset=$entry.NextEncodedOffset; EncodedRecordLength=$entry.EncodedRecordLength;
                StorageKind=$entry.StorageKind; Rank=$rank;
                Required=if ($null -ne $definition) { $definition.required -eq $true } else { $critical.ContainsKey([string]$entry.File) };
                ExpectedRoot=if ($null -ne $definition) { [string]$definition.expected_root } else { '' };
                Role=if ($null -ne $definition) { [string]$definition.role } else { 'related' }
            })
        }
    }
    $maximum = if ($null -ne $TargetProfile.maximum_candidates) { [Math]::Max(1,[int]$TargetProfile.maximum_candidates) } else { 128 }
    return @($selected | Sort-Object Rank,FullName | Select-Object -First $maximum)
}

function Extract-DSPakXmlEntry {
    param(
        [Parameter(Mandatory=$true)][object]$Context,
        [Parameter(Mandatory=$true)][object]$Entry,
        [Parameter(Mandatory=$true)][string]$OutputPath,
        [Parameter(Mandatory=$true)][string]$TempDir,
        [string]$ExpectedRoot = ''
    )
    if ([int]$Entry.EncodedOffset -eq [int]::MinValue) {
        return [pscustomobject]@{ Status='blocked'; Mode='DELETED_SENTINEL'; Error='i32_min_deleted_or_pruned_entry' }
    }
    if ([int]$Entry.EncodedOffset -lt 0) {
        return [pscustomobject]@{ Status='blocked'; Mode='NON_ENCODED_INDEX'; Error='negative_offset_requires_regular_entry_table'; NonEncodedIndex=(-[int]$Entry.EncodedOffset - 1) }
    }
    if ([string]::IsNullOrWhiteSpace($ExpectedRoot) -and $null -ne $Entry.ExpectedRoot) { $ExpectedRoot = [string]$Entry.ExpectedRoot }
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $OutputPath),$TempDir | Out-Null
    try {
        $decoded = Read-DSExactCompactEntry -Context $Context -Entry $Entry
        $encoded = $decoded.TypedEntry
        $a = New-Args 2; $a[0] = $Context.Binary; $a[1] = [uint64]$decoded.PakOffset
        $data = Invoke-Static $Context.ProgramType 'ReadDataEntry' $a
        $a = New-Args 3; $a[0] = $encoded; $a[1] = $data; $a[2] = [int64]$Context.PakLength
        [void](Invoke-Static $Context.ProgramType 'ValidateEntry' $a)
        $a = New-Args 6; $a[0] = $Context.Binary; $a[1] = $data; $a[2] = $Context.AesKey; $a[3] = $Context.OozPath; $a[4] = $TempDir; $a[5] = $OutputPath
        [void](Invoke-Static $Context.ProgramType 'WriteDecompressedEntry' $a)
        $identity = Test-DSXmlIdentity -Path $OutputPath -ExpectedRoot $ExpectedRoot
        if (-not $identity.Valid) {
            throw ('Extracted XML identity failed: expected={0}; actual={1}; error={2}' -f $ExpectedRoot,$identity.ActualRoot,$identity.Error)
        }
        return [pscustomobject]@{
            Status='success'; Mode='COMPACT_EXACT'; Error=''; ActualRoot=$identity.ActualRoot;
            PakOffset=$decoded.PakOffset; NumberMask=$decoded.NumberMask;
            Size=$decoded.UncompressedSize; CompressedSize=$decoded.CompressedSize;
            Bits=('0x{0:X8}' -f $decoded.Bits); CompressionSlot=$decoded.CompressionSlot;
            Encrypted=$decoded.Encrypted; BlockCount=$decoded.CompressionBlockCount;
            BlockSize=$decoded.CompressionBlockSize; RecordLength=$decoded.RecordLength;
            ConsumedLength=$decoded.ConsumedLength; NextEncodedOffset=$decoded.NextEncodedOffset
        }
    }
    catch {
        $message = Get-ExceptionSummary -ErrorObject $_
        Remove-Item -LiteralPath $OutputPath -Force -ErrorAction SilentlyContinue
        return [pscustomobject]@{
            Status='partial'; Mode='UNRESOLVED_COMPACT_ENTRY'; Error=$message;
            RecordLength=$Entry.EncodedRecordLength; NextEncodedOffset=$Entry.NextEncodedOffset;
            NumberMask=[int]$Context.NumberMask
        }
    }
}

function Get-DSRawEncodedWindow {
    param([byte[]]$EncodedEntries,[int]$Offset,[int]$Before=32,[int]$After=96)
    if ($Offset -lt 0) { return [byte[]]@() }
    $start = [Math]::Max(0,$Offset-$Before)
    $length = [Math]::Min($EncodedEntries.Length-$start,$Before+$After)
    if ($length -le 0) { return [byte[]]@() }
    [byte[]]$result = New-Object byte[] $length
    [Array]::Copy($EncodedEntries,$start,$result,0,$length)
    return $result
}

function Get-DSDecodeCandidates {
    param([Parameter(Mandatory=$true)][object]$Context,[Parameter(Mandatory=$true)][object]$Entry)
    try {
        $decoded = Read-DSExactCompactEntry -Context $Context -Entry $Entry
        $headerMatch = $false; $validated = $false; $dataError = ''
        try {
            $a = New-Args 2; $a[0] = $Context.Binary; $a[1] = [uint64]$decoded.PakOffset
            $data = Invoke-Static $Context.ProgramType 'ReadDataEntry' $a
            [uint64]$dataCompressed = Get-Value $data 'CompressedSize'
            [uint64]$dataUncompressed = Get-Value $data 'UncompressedSize'
            $headerMatch = $dataCompressed -eq $decoded.CompressedSize -and $dataUncompressed -eq $decoded.UncompressedSize
            $a = New-Args 3; $a[0] = $decoded.TypedEntry; $a[1] = $data; $a[2] = [int64]$Context.PakLength
            [void](Invoke-Static $Context.ProgramType 'ValidateEntry' $a); $validated = $true
        } catch { $dataError = Get-ExceptionSummary -ErrorObject $_ -MaximumLength 500 }
        return @([pscustomobject]@{
            file=$Entry.File; status='exact_parsed'; encoded_offset=$decoded.EncodedOffset;
            next_encoded_offset=$decoded.NextEncodedOffset; record_length=$decoded.RecordLength;
            parsed_length=$decoded.ConsumedLength; number_mask=$decoded.NumberMask;
            bits=('0x{0:X8}' -f $decoded.Bits); compression_slot=$decoded.CompressionSlot;
            encrypted=$decoded.Encrypted; block_count=$decoded.CompressionBlockCount;
            block_size=$decoded.CompressionBlockSize; pak_offset=$decoded.PakOffset;
            compressed_size=$decoded.CompressedSize; uncompressed_size=$decoded.UncompressedSize;
            header_match=$headerMatch; validate_entry=$validated; data_error=$dataError
        })
    }
    catch {
        return @([pscustomobject]@{
            file=$Entry.File; status='exact_parse_failed'; encoded_offset=$Entry.EncodedOffset;
            next_encoded_offset=$Entry.NextEncodedOffset; record_length=$Entry.EncodedRecordLength;
            parsed_length=''; number_mask=[int]$Context.NumberMask; bits=''; compression_slot='';
            encrypted=''; block_count=''; block_size=''; pak_offset=''; compressed_size='';
            uncompressed_size=''; header_match=$false; validate_entry=$false;
            data_error=(Get-ExceptionSummary -ErrorObject $_ -MaximumLength 700)
        })
    }
}
