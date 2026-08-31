$ErrorActionPreference = 'Stop'

function Get-DsnwrRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Path
    )
    $rootFull = [System.IO.Path]::GetFullPath($Root).TrimEnd('\')
    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $prefix = $rootFull + '\'
    if (-not $pathFull.StartsWith(
            $prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Payload path escaped its root: $pathFull"
    }
    return $pathFull.Substring($prefix.Length).Replace('\', '/')
}

function Assert-DsnwrNoReparseTree {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$Description
    )

    $rootFull = [System.IO.Path]::GetFullPath($Root)
    if (-not (Test-Path -LiteralPath $rootFull)) {
        throw "$Description is missing: $rootFull"
    }
    $pending = [System.Collections.Generic.Stack[string]]::new()
    $pending.Push($rootFull)
    while ($pending.Count -ne 0) {
        $current = $pending.Pop()
        $item = Get-Item -LiteralPath $current -Force
        if (($item.Attributes -band `
                [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "$Description contains a reparse point: $current"
        }
        if (-not $item.PSIsContainer) {
            continue
        }
        foreach ($child in @(Get-ChildItem -LiteralPath $current -Force)) {
            if (($child.Attributes -band `
                    [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "$Description contains a reparse point: $($child.FullName)"
            }
            if ($child.PSIsContainer) {
                $pending.Push($child.FullName)
            }
        }
    }
}

function Get-DsnwrValidatedSourceFile {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$Path
    )

    $rootFull = [System.IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\')
    $pathFull = [System.IO.Path]::GetFullPath($Path)
    $prefix = $rootFull + '\'
    if (-not $pathFull.StartsWith(
            $prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Runtime payload source escaped the project root: $pathFull"
    }
    if (-not (Test-Path -LiteralPath $pathFull -PathType Leaf)) {
        throw "Required payload source is missing: $pathFull"
    }

    $rootItem = Get-Item -LiteralPath $rootFull -Force
    if (($rootItem.Attributes -band `
            [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Runtime payload source root is a reparse point: $rootFull"
    }
    $current = $rootFull
    $relative = $pathFull.Substring($prefix.Length)
    foreach ($segment in @($relative -split '\\')) {
        $current = Join-Path $current $segment
        $item = Get-Item -LiteralPath $current -Force
        if (($item.Attributes -band `
                [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Runtime payload source contains a reparse point: $current"
        }
    }
    return (Get-Item -LiteralPath $pathFull -Force)
}

function Test-DsnwrInstallerOnlyDefaultPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)
    $normalized = $RelativePath.Replace('\', '/').ToLowerInvariant()
    return $normalized -eq 'config/visibility.example.ini' `
        -or $normalized -eq 'config/diagnostics.example.ini'
}

function Get-DsnwrRuntimePayloadSpecification {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$DllPath,
        [string]$BuildReceiptPath,
        [string]$BuildLockPath
    )
    if ([string]::IsNullOrWhiteSpace($BuildReceiptPath)) {
        $BuildReceiptPath = Join-Path $ProjectRoot `
            'dist\work\build\native\native-build-receipt.json'
    }
    if ([string]::IsNullOrWhiteSpace($BuildLockPath)) {
        $BuildLockPath = Join-Path $ProjectRoot 'metadata\native-build-lock.json'
    }
    $items = [System.Collections.Generic.List[object]]::new()
    $relativePaths = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    $projectFull = [System.IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\')
    $projectPrefix = $projectFull + '\'
    function Add-PayloadFile([string]$Source, [string]$RelativePath) {
        $sourceFull = [System.IO.Path]::GetFullPath($Source)
        if (-not $sourceFull.StartsWith(
                $projectPrefix,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Runtime payload source escaped the project root: $sourceFull"
        }
        $normalized = $RelativePath.Replace('\', '/')
        $segments = @($normalized -split '/')
        if ([string]::IsNullOrWhiteSpace($normalized) `
            -or [System.IO.Path]::IsPathRooted($RelativePath) `
            -or @($segments | Where-Object {
                    [string]::IsNullOrWhiteSpace($_) `
                        -or $_ -eq '.' -or $_ -eq '..'
                }).Count -ne 0) {
            throw "Runtime payload destination is not a safe relative path: $RelativePath"
        }
        $destinationProbe = [System.IO.Path]::GetFullPath((Join-Path `
                    $projectFull ($normalized.Replace('/', '\'))))
        if (-not $destinationProbe.StartsWith(
                $projectPrefix,
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Runtime payload destination escaped its root: $RelativePath"
        }
        if (-not $relativePaths.Add($normalized)) {
            throw "Runtime payload specification repeats a destination: $normalized"
        }
        $items.Add([pscustomobject]@{
                Source = $sourceFull
                RelativePath = $normalized
            })
    }

    Add-PayloadFile $DllPath 'dlls/main.dll'
    Add-PayloadFile `
        $BuildReceiptPath `
        'metadata/native-build-receipt.json'
    Add-PayloadFile `
        $BuildLockPath `
        'metadata/native-build-lock.json'
    Add-PayloadFile (Join-Path $ProjectRoot 'config\visibility.ini') `
        'config/visibility.example.ini'
    Add-PayloadFile (Join-Path $ProjectRoot 'config\diagnostics.ini') `
        'config/diagnostics.example.ini'

    foreach ($name in @(
            'area-quests.tsv', 'assault-actors.tsv', 'assaults.lua',
            'boss-actors.tsv', 'bosses.lua', 'mole-anchor-candidates.tsv',
            'moles.lua', 'save_owner_pointer.cfg', 'treasure-actors.tsv',
            'treasures.lua')) {
        Add-PayloadFile `
            (Join-Path $ProjectRoot "src\data\generated\$name") `
            "data/generated/$name"
    }
    Add-PayloadFile `
        (Join-Path $ProjectRoot 'src\data\defaults\treasure_overrides.txt') `
        'data/defaults/treasure_overrides.txt'
    Add-PayloadFile `
        (Join-Path $ProjectRoot 'assets\vendor\sqlcipher\e_sqlcipher.dll') `
        'vendor/sqlcipher/e_sqlcipher.dll'
    Add-PayloadFile (Join-Path $ProjectRoot 'metadata\release.json') `
        'metadata/release.json'
    Add-PayloadFile (Join-Path $ProjectRoot 'metadata\data-providers.json') `
        'metadata/data-providers.json'
    Add-PayloadFile `
        (Join-Path $ProjectRoot 'metadata\assault-inference-policy.xml') `
        'metadata/assault-inference-policy.xml'
    Add-PayloadFile (Join-Path $ProjectRoot 'licenses\APACHE-2.0.txt') `
        'licenses/APACHE-2.0.txt'
    Add-PayloadFile (Join-Path $ProjectRoot 'licenses\SQLCIPHER.txt') `
        'licenses/SQLCIPHER.txt'
    Add-PayloadFile (Join-Path $ProjectRoot 'licenses\FMT.txt') `
        'licenses/FMT.txt'
    Add-PayloadFile (Join-Path $ProjectRoot 'licenses\UE4SS.txt') `
        'licenses/UE4SS.txt'
    Add-PayloadFile (Join-Path $ProjectRoot 'licenses\LIBTOMCRYPT.txt') `
        'licenses/LIBTOMCRYPT.txt'
    Add-PayloadFile (Join-Path $ProjectRoot 'LICENSE') 'LICENSE'
    Add-PayloadFile (Join-Path $ProjectRoot 'THIRD_PARTY_NOTICES.txt') `
        'THIRD_PARTY_NOTICES.txt'
    Add-PayloadFile (Join-Path $ProjectRoot 'README.md') 'README.txt'
    return @($items)
}

function New-DsnwrRuntimePayload {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$DllPath,
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [string]$BuildReceiptPath,
        [string]$BuildLockPath
    )
    $destinationFull = [System.IO.Path]::GetFullPath($DestinationRoot)
    if (Test-Path -LiteralPath $destinationFull) {
        $destinationItem = Get-Item -LiteralPath $destinationFull -Force
        if (($destinationItem.Attributes -band `
                [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw "Runtime payload destination is a reparse point: $destinationFull"
        }
        $existing = @(Get-ChildItem -LiteralPath $destinationFull -Force)
        if ($existing.Count -ne 0) {
            throw "Runtime payload destination is not empty: $destinationFull"
        }
    } else {
        New-Item -ItemType Directory -Path $destinationFull -Force | Out-Null
    }

    $specification = @(Get-DsnwrRuntimePayloadSpecification `
            -ProjectRoot $ProjectRoot -DllPath $DllPath `
            -BuildReceiptPath $BuildReceiptPath -BuildLockPath $BuildLockPath)
    foreach ($item in $specification) {
        $sourceItem = Get-DsnwrValidatedSourceFile `
            -ProjectRoot $ProjectRoot -Path $item.Source
        $destination = Join-Path `
            $destinationFull ($item.RelativePath.Replace('/', '\'))
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) `
            -Force | Out-Null
        Copy-Item -LiteralPath $item.Source -Destination $destination -Force
    }
    Assert-DsnwrNoReparseTree -Root $destinationFull `
        -Description 'Runtime payload destination'

    $release = Get-Content -LiteralPath `
        (Join-Path $ProjectRoot 'metadata\release.json') -Raw |
        ConvertFrom-Json
    $manifestFiles = @(Get-ChildItem -LiteralPath $destinationFull -Recurse `
            -Force -File | Where-Object {
            $relative = Get-DsnwrRelativePath `
                -Root $destinationFull -Path $_.FullName
            -not (Test-DsnwrInstallerOnlyDefaultPath -RelativePath $relative)
        } | Sort-Object FullName | ForEach-Object {
            [pscustomobject]@{
                path = Get-DsnwrRelativePath `
                    -Root $destinationFull -Path $_.FullName
                size = [int64]$_.Length
                sha256 = (Get-FileHash -LiteralPath $_.FullName `
                        -Algorithm SHA256).Hash
            }
        })
    $manifest = [ordered]@{
        schema_version = 1
        name = 'DragonSwordNativeWorldRadarPostRender'
        version = [string]$release.version
        file_count = $manifestFiles.Count
        files = $manifestFiles
    }
    $manifestPath = Join-Path `
        $destinationFull 'metadata\package-manifest.json'
    $manifest | ConvertTo-Json -Depth 6 |
        Set-Content -LiteralPath $manifestPath -Encoding UTF8
    return $manifestPath
}

function Test-DsnwrRuntimePayload {
    param(
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$DllPath,
        [Parameter(Mandatory = $true)][string]$PayloadRoot,
        [switch]$AllowUserVisibility,
        [switch]$AllowUserDiagnostics,
        [switch]$InstalledConfiguration,
        [string]$BuildReceiptPath,
        [string]$BuildLockPath
    )
    $payloadFull = [System.IO.Path]::GetFullPath($PayloadRoot)
    $payloadRootItem = Get-Item -LiteralPath $payloadFull -Force
    if (($payloadRootItem.Attributes -band `
            [System.IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw "Runtime payload root is a reparse point: $payloadFull"
    }
    Assert-DsnwrNoReparseTree -Root $payloadFull `
        -Description 'Runtime payload'
    $expected = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    $specification = @(Get-DsnwrRuntimePayloadSpecification `
            -ProjectRoot $ProjectRoot -DllPath $DllPath `
            -BuildReceiptPath $BuildReceiptPath -BuildLockPath $BuildLockPath)
    foreach ($item in $specification) {
        if ($InstalledConfiguration -and
            (Test-DsnwrInstallerOnlyDefaultPath -RelativePath $item.RelativePath)) {
            continue
        }
        [void]$expected.Add($item.RelativePath.Replace('\', '/'))
    }
    [void]$expected.Add('metadata/package-manifest.json')
    if ($AllowUserVisibility) {
        [void]$expected.Add('config/visibility.ini')
    }
    if ($AllowUserDiagnostics) {
        [void]$expected.Add('config/diagnostics.ini')
    }
    if ($InstalledConfiguration -and
        (-not $AllowUserVisibility -or -not $AllowUserDiagnostics)) {
        throw 'InstalledConfiguration requires both live user configuration files.'
    }

    $expectedDirectories = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($relative in $expected) {
        $parent = [System.IO.Path]::GetDirectoryName(
            $relative.Replace('/', '\'))
        while (-not [string]::IsNullOrWhiteSpace($parent)) {
            [void]$expectedDirectories.Add($parent.Replace('\', '/'))
            $parent = [System.IO.Path]::GetDirectoryName($parent)
        }
    }

    $actual = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($file in @(Get-ChildItem -LiteralPath $payloadFull -Recurse `
            -Force -File)) {
        [void]$actual.Add((Get-DsnwrRelativePath `
                    -Root $payloadFull -Path $file.FullName))
    }
    $missing = @($expected | Where-Object { -not $actual.Contains($_) })
    $unexpected = @($actual | Where-Object { -not $expected.Contains($_) })
    if ($missing.Count -ne 0 -or $unexpected.Count -ne 0) {
        throw "Runtime payload file set mismatch. Missing=$($missing -join ',') Unexpected=$($unexpected -join ',')"
    }
    $actualDirectories = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($directory in @(Get-ChildItem -LiteralPath $payloadFull `
            -Recurse -Force -Directory)) {
        [void]$actualDirectories.Add((Get-DsnwrRelativePath `
                    -Root $payloadFull -Path $directory.FullName))
    }
    $missingDirectories = @($expectedDirectories | Where-Object {
            -not $actualDirectories.Contains($_)
        })
    $unexpectedDirectories = @($actualDirectories | Where-Object {
            -not $expectedDirectories.Contains($_)
        })
    if ($missingDirectories.Count -ne 0 `
        -or $unexpectedDirectories.Count -ne 0) {
        throw "Runtime payload directory set mismatch. Missing=$($missingDirectories -join ',') Unexpected=$($unexpectedDirectories -join ',')"
    }

    # The package manifest is integrity metadata, not an independent trust
    # root. Bind every allowlisted payload file directly to its release-source
    # input so changing both a payload and its manifest cannot pass review.
    foreach ($item in $specification) {
        if ($InstalledConfiguration -and
            (Test-DsnwrInstallerOnlyDefaultPath -RelativePath $item.RelativePath)) {
            continue
        }
        $payloadPath = Join-Path $payloadFull `
            ($item.RelativePath.Replace('/', '\'))
        $sourceItem = Get-DsnwrValidatedSourceFile `
            -ProjectRoot $ProjectRoot -Path $item.Source
        $payloadItem = Get-Item -LiteralPath $payloadPath -Force
        $sourceHash = (Get-FileHash -LiteralPath $sourceItem.FullName `
                -Algorithm SHA256).Hash
        $payloadHash = (Get-FileHash -LiteralPath $payloadItem.FullName `
                -Algorithm SHA256).Hash
        if ([int64]$sourceItem.Length -ne [int64]$payloadItem.Length `
            -or $sourceHash -ne $payloadHash) {
            throw "Runtime payload differs from its release source: $($item.RelativePath)"
        }
    }

    $manifestPath = Join-Path $payloadFull 'metadata\package-manifest.json'
    $manifest = Get-Content -LiteralPath $manifestPath -Raw |
        ConvertFrom-Json
    $release = Get-Content -LiteralPath `
        (Join-Path $ProjectRoot 'metadata\release.json') -Raw |
        ConvertFrom-Json
    if ([int]$manifest.schema_version -ne 1 `
        -or [string]$manifest.version -ne [string]$release.version `
        -or [string]$manifest.name -ne `
            'DragonSwordNativeWorldRadarPostRender') {
        throw 'Runtime payload manifest identity is stale.'
    }
    $manifestEntries = @($manifest.files)
    if ([int]$manifest.file_count -ne $manifestEntries.Count) {
        throw 'Runtime payload manifest count is invalid.'
    }
    $manifestExpected = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($relative in $expected) {
        if ($relative -ne 'metadata/package-manifest.json' `
            -and $relative -ne 'config/visibility.ini' `
            -and $relative -ne 'config/diagnostics.ini' `
            -and -not (Test-DsnwrInstallerOnlyDefaultPath `
                -RelativePath $relative)) {
            [void]$manifestExpected.Add($relative)
        }
    }
    $manifestActual = [System.Collections.Generic.HashSet[string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $manifestEntries) {
        $manifestRelative = ([string]$entry.path).Replace('\', '/')
        if (-not $manifestActual.Add($manifestRelative)) {
            throw "Runtime payload manifest contains a duplicate path: $manifestRelative"
        }
        $relative = $manifestRelative.Replace('/', '\')
        $path = [System.IO.Path]::GetFullPath((Join-Path $payloadFull $relative))
        if (-not $path.StartsWith(
                $payloadFull.TrimEnd('\') + '\',
                [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Runtime payload manifest path escaped its root: $manifestRelative"
        }
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Manifest file is missing: $($entry.path)"
        }
        $item = Get-Item -LiteralPath $path
        $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        if ([int64]$entry.size -ne [int64]$item.Length `
            -or [string]$entry.sha256 -ne $hash) {
            throw "Manifest hash or size mismatch: $($entry.path)"
        }
    }
    $manifestMissing = @($manifestExpected | Where-Object {
            -not $manifestActual.Contains($_)
        })
    $manifestUnexpected = @($manifestActual | Where-Object {
            -not $manifestExpected.Contains($_)
        })
    if ($manifestMissing.Count -ne 0 -or $manifestUnexpected.Count -ne 0) {
        throw "Runtime payload manifest file set mismatch. Missing=$($manifestMissing -join ',') Unexpected=$($manifestUnexpected -join ',')"
    }
    return $true
}
