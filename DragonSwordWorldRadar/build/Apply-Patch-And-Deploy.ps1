#requires -Version 5.1

$ErrorActionPreference = 'Stop'

Set-StrictMode -Version 2.0



$RepositoryRoot = Split-Path -Parent $PSScriptRoot

$RuntimeRoot = Join-Path $RepositoryRoot 'runtime\patch-deploy'

New-Item -ItemType Directory -Force -Path $RuntimeRoot | Out-Null

$SessionStamp = Get-Date -Format 'yyyyMMdd-HHmmss'

$LogPath = Join-Path $RuntimeRoot ("PatchDeploy-{0}.log" -f $SessionStamp)

$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)



function Write-Log([string]$Message) {

    $line = "[{0}] {1}" -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss.fff'), $Message

    Write-Host $line

    [IO.File]::AppendAllText($LogPath, $line + [Environment]::NewLine, $Utf8NoBom)

}



function Invoke-External([string]$FilePath, [string[]]$Arguments) {

    Write-Log ("RUN {0} {1}" -f $FilePath, ($Arguments -join ' '))

    & $FilePath @Arguments

    $exitCode = $LASTEXITCODE

    if ($exitCode -ne 0) {

        throw ("Command failed with exit code {0}: {1}" -f $exitCode, $FilePath)

    }

}





function Remove-StalePatchArtifacts {

    $sourceRoot = Join-Path $RepositoryRoot 'src'

    if (-not (Test-Path -LiteralPath $sourceRoot)) {

        return

    }



    $artifacts = @(

        Get-ChildItem -LiteralPath $sourceRoot -Recurse -Force -File -ErrorAction SilentlyContinue |

            Where-Object {

                $_.Name.EndsWith('.orig', [StringComparison]::OrdinalIgnoreCase) -or

                $_.Name.EndsWith('.rej', [StringComparison]::OrdinalIgnoreCase)

            }

    )



    foreach ($artifact in $artifacts) {

        Remove-Item -LiteralPath $artifact.FullName -Force

        Write-Log ("STALE_PATCH_ARTIFACT_REMOVED path={0}" -f $artifact.FullName)

    }

}





function Get-CurrentVersion {

    $releasePath = Join-Path $RepositoryRoot 'metadata\release.json'

    if (-not (Test-Path -LiteralPath $releasePath -PathType Leaf)) { return '(unknown)' }

    try {

        $release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json

        $value = [string]$release.version

        if ([string]::IsNullOrWhiteSpace($value)) { return '(unknown)' }

        return $value

    }

    catch { return '(unreadable)' }

}



function Get-PatchMetadata([string]$PatchPath) {

    $version = '(not declared)'

    $description = '(not declared)'

    $files = New-Object System.Collections.Generic.List[string]

    foreach ($line in @(Get-Content -LiteralPath $PatchPath -TotalCount 200)) {

        if ($line -match '^\s*#\s*PATCH_VERSION\s*:\s*(.+?)\s*$') {

            $version = $Matches[1].Trim()

        }

        elseif ($line -match '^\s*#\s*PATCH_DESCRIPTION\s*:\s*(.+?)\s*$') {

            $description = $Matches[1].Trim()

        }

        elseif ($line -match '^\s*#\s*PATCH_FILES\s*:\s*(.+?)\s*$') {

            foreach ($item in @($Matches[1] -split ';')) {

                $trimmed = $item.Trim().TrimStart('\','/')

                if (-not [string]::IsNullOrWhiteSpace($trimmed)) { $files.Add($trimmed) }

            }

        }

    }

    return [pscustomobject]@{ Version=$version; Description=$description; Files=$files.ToArray() }

}



function Select-PatchFile {

    $patches = @(Get-ChildItem -LiteralPath $RepositoryRoot -File -Filter '*.patch' | Sort-Object Name)

    if ($patches.Count -eq 0) { throw 'No .patch file was found in the repository root.' }

    if ($patches.Count -eq 1) { return $patches[0] }



    Write-Host ''

    Write-Host 'PATCH FILES'

    for ($i = 0; $i -lt $patches.Count; $i++) {

        Write-Host ("  [{0}] {1}" -f ($i + 1), $patches[$i].Name)

    }

    while ($true) {

        $answer = Read-Host 'Select patch number'

        $number = 0

        if ([int]::TryParse($answer, [ref]$number) -and $number -ge 1 -and $number -le $patches.Count) {

            return $patches[$number - 1]

        }

        Write-Host 'Invalid selection.'

    }

}



function Backup-DeclaredFiles([string[]]$RelativePaths) {

    $backupRoot = Join-Path $RuntimeRoot ("source-backup-{0}" -f (Get-Date -Format 'yyyyMMdd-HHmmssfff'))

    New-Item -ItemType Directory -Force -Path $backupRoot | Out-Null

    $manifest = New-Object System.Collections.Generic.List[object]



    foreach ($relative in @($RelativePaths)) {

        if ([string]::IsNullOrWhiteSpace($relative)) { continue }

        $normalized = $relative.Replace('/','\').TrimStart('\')

        if ($normalized -match '(^|\\)\.\.($|\\)') { throw ("Unsafe PATCH_FILES path: {0}" -f $relative) }

        $source = Join-Path $RepositoryRoot $normalized

        $backup = Join-Path $backupRoot $normalized

        $existed = Test-Path -LiteralPath $source

        if ($existed) {

            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $backup) | Out-Null

            if (Test-Path -LiteralPath $source -PathType Container) {

                Copy-Item -LiteralPath $source -Destination $backup -Recurse -Force

            }

            else {

                Copy-Item -LiteralPath $source -Destination $backup -Force

            }

        }

        $manifest.Add([pscustomobject]@{ Relative=$normalized; Existed=$existed })

    }

    return [pscustomobject]@{ Root=$backupRoot; Items=$manifest.ToArray() }

}



function Restore-DeclaredFiles($Backup) {

    if ($null -eq $Backup) { return }

    foreach ($item in @($Backup.Items)) {

        $target = Join-Path $RepositoryRoot $item.Relative

        $stored = Join-Path $Backup.Root $item.Relative

        if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }

        if ($item.Existed -and (Test-Path -LiteralPath $stored)) {

            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null

            Copy-Item -LiteralPath $stored -Destination $target -Recurse -Force

        }

    }

}



function Invoke-RepositoryScript([string]$RelativePath) {

    $scriptPath = Join-Path $RepositoryRoot $RelativePath

    if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {

        throw ("Required script is missing: {0}" -f $RelativePath)

    }

    $windowsPowerShell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'

    Invoke-External $windowsPowerShell @('-NoLogo','-NoProfile','-NonInteractive','-ExecutionPolicy','Bypass','-File',$scriptPath)

}



function Resolve-GameLayout([string]$SelectedPath) {

    $current = [IO.Path]::GetFullPath($SelectedPath)

    for ($i = 0; $i -lt 6; $i++) {

        $nestedDs = Join-Path $current 'DS'

        if ((Test-Path -LiteralPath (Join-Path $nestedDs 'Binaries\Win64\DSClient-Win64-Shipping.exe') -PathType Leaf) -and

            (Test-Path -LiteralPath (Join-Path $nestedDs 'Content\Paks\pakchunk109-WindowsClient.pak') -PathType Leaf)) {

            return [pscustomobject]@{ GameRoot=$current; DsRoot=$nestedDs }

        }

        if ((Test-Path -LiteralPath (Join-Path $current 'Binaries\Win64\DSClient-Win64-Shipping.exe') -PathType Leaf) -and

            (Test-Path -LiteralPath (Join-Path $current 'Content\Paks\pakchunk109-WindowsClient.pak') -PathType Leaf)) {

            return [pscustomobject]@{ GameRoot=(Split-Path -Parent $current); DsRoot=$current }

        }

        $parent = Split-Path -Parent $current

        if ([string]::IsNullOrWhiteSpace($parent) -or $parent -eq $current) { break }

        $current = $parent

    }

    return $null

}





function Get-LocalDeployModsPath {

    $configDirectory = Join-Path $RepositoryRoot 'runtime\local-config'

    $configPath = Join-Path $configDirectory 'deploy-mods-path.txt'

    $defaultPath = 'G:\SteamLibrary\steamapps\common\DragonSword  Awakening\DS\Binaries\Win64\Mods'



    if (-not (Test-Path -LiteralPath $configDirectory)) {

        New-Item -ItemType Directory -Path $configDirectory -Force | Out-Null

    }




    $gitInfoExclude = Join-Path $RepositoryRoot '.git\info\exclude'
    $gitInfoDirectory = Split-Path -Parent $gitInfoExclude
    if (Test-Path -LiteralPath $gitInfoDirectory -PathType Container) {
        $excludeEntry = 'runtime/local-config/'
        $existingExclude = ''
        if (Test-Path -LiteralPath $gitInfoExclude -PathType Leaf) {
            $existingExclude = [IO.File]::ReadAllText($gitInfoExclude)
        }
        if ($existingExclude -notmatch '(?m)^runtime/local-config/\s*$') {
            [IO.File]::AppendAllText(
                $gitInfoExclude,
                [Environment]::NewLine + $excludeEntry + [Environment]::NewLine,
                (New-Object Text.UTF8Encoding($false))
            )
            Write-Log ("LOCAL_GIT_EXCLUDE_ADDED path={0}" -f $gitInfoExclude)
        }
    }

    if (-not (Test-Path -LiteralPath $configPath)) {

        [IO.File]::WriteAllText(

            $configPath,

            $defaultPath + [Environment]::NewLine,

            (New-Object Text.UTF8Encoding($false))

        )

        Write-Log ("LOCAL_DEPLOY_CONFIG_CREATED path={0}" -f $configPath)

    }



    $modsPath = ([IO.File]::ReadAllText($configPath)).Trim()

    if ([string]::IsNullOrWhiteSpace($modsPath)) {

        throw ("Local deploy path is empty: {0}" -f $configPath)

    }



    $modsPath = [IO.Path]::GetFullPath($modsPath)

    $expectedSuffix = '\DS\Binaries\Win64\Mods'

    if (-not $modsPath.EndsWith($expectedSuffix, [StringComparison]::OrdinalIgnoreCase)) {

        throw ("Configured path must end with {0}: {1}" -f $expectedSuffix, $modsPath)

    }



    $win64Path = Split-Path -Parent $modsPath

    $dsPath = [IO.Path]::GetFullPath((Join-Path $win64Path '..\..'))

    $exePath = Join-Path $win64Path 'DSClient-Win64-Shipping.exe'

    $pakPath = Join-Path $dsPath 'Content\Paks\pakchunk109-WindowsClient.pak'



    if (-not (Test-Path -LiteralPath $modsPath -PathType Container)) {

        throw ("Configured Mods directory does not exist: {0}" -f $modsPath)

    }

    if (-not (Test-Path -LiteralPath $exePath -PathType Leaf)) {

        throw ("Game executable was not found beside the configured Mods directory: {0}" -f $exePath)

    }

    if (-not (Test-Path -LiteralPath $pakPath -PathType Leaf)) {

        throw ("Game PAK was not found for the configured Mods directory: {0}" -f $pakPath)

    }



    $gameRoot = Split-Path -Parent $dsPath

    Write-Log ("LOCAL_DEPLOY_CONFIG_OK path={0}" -f $configPath)
    Write-Log ("DEPLOY_MODS_PATH path={0}" -f $modsPath)

    return [pscustomobject]@{
        GameRoot = $gameRoot
        DsRoot = $dsPath
        ModsRoot = $modsPath
    }

}



function Select-GameLayout {

    Add-Type -AssemblyName System.Windows.Forms

    $dialog = New-Object System.Windows.Forms.FolderBrowserDialog

    $dialog.Description = 'Select Dragon Sword Awakening, DS, or DS\Binaries\Win64 folder'

    $dialog.ShowNewFolderButton = $false

    try {

        if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {

            throw 'Game folder selection was cancelled.'

        }

        $layout = Resolve-GameLayout $dialog.SelectedPath

        if ($null -eq $layout) {

            throw 'The selected folder does not contain the expected Dragon Sword Awakening DS layout.'

        }

        return $layout

    }

    finally { $dialog.Dispose() }

}



function Copy-DirectoryContents([string]$Source, [string]$Destination) {

    New-Item -ItemType Directory -Force -Path $Destination | Out-Null

    $items = @(Get-ChildItem -LiteralPath $Source -Force)

    foreach ($item in $items) {

        Copy-Item -LiteralPath $item.FullName -Destination $Destination -Recurse -Force

    }

}



function Deploy-Release([string]$DsRoot) {

    $version = Get-CurrentVersion

    if ($version -eq '(unknown)' -or $version -eq '(unreadable)') { throw 'Cannot resolve the built release version.' }

    $stageMod = Join-Path $RepositoryRoot ("dist\DragonSwordWorldRadar-{0}\DragonSwordWorldRadar" -f $version)

    if (-not (Test-Path -LiteralPath $stageMod -PathType Container)) {

        throw ("Built release staging directory is missing: {0}" -f $stageMod)

    }



    $gameProcesses = @(Get-Process -Name 'DSClient-Win64-Shipping' -ErrorAction SilentlyContinue)

    if ($gameProcesses.Count -gt 0) { throw 'Close Dragon Sword before deployment.' }



    $modsRoot = Join-Path $DsRoot 'Binaries\Win64\Mods'

    $target = Join-Path $modsRoot 'DragonSwordWorldRadar'

    New-Item -ItemType Directory -Force -Path $modsRoot | Out-Null



    $deployBackup = Join-Path $RuntimeRoot ("game-mod-backup-{0}" -f (Get-Date -Format 'yyyyMMdd-HHmmssfff'))

    $hadTarget = Test-Path -LiteralPath $target -PathType Container

    if ($hadTarget) { Copy-Item -LiteralPath $target -Destination $deployBackup -Recurse -Force }



    try {

        $preserveRoot = Join-Path $RuntimeRoot ("preserve-{0}" -f (Get-Date -Format 'yyyyMMdd-HHmmssfff'))

        New-Item -ItemType Directory -Force -Path $preserveRoot | Out-Null

        $preserve = @('scripts\config.lua','data\treasure_overrides.txt','runtime')

        foreach ($relative in $preserve) {

            $existing = Join-Path $target $relative

            if (Test-Path -LiteralPath $existing) {

                $saved = Join-Path $preserveRoot $relative

                New-Item -ItemType Directory -Force -Path (Split-Path -Parent $saved) | Out-Null

                Copy-Item -LiteralPath $existing -Destination $saved -Recurse -Force

            }

        }



        if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }

        New-Item -ItemType Directory -Force -Path $target | Out-Null

        Copy-DirectoryContents $stageMod $target



        foreach ($relative in $preserve) {

            $saved = Join-Path $preserveRoot $relative

            if (Test-Path -LiteralPath $saved) {

                $destination = Join-Path $target $relative

                if (Test-Path -LiteralPath $destination) { Remove-Item -LiteralPath $destination -Recurse -Force }

                New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null

                Copy-Item -LiteralPath $saved -Destination $destination -Recurse -Force

            }

        }



        $installCmd = Join-Path $target 'Install.cmd'

        if (-not (Test-Path -LiteralPath $installCmd -PathType Leaf)) { throw 'Deployed Install.cmd is missing.' }

        Invoke-External $env:ComSpec @('/d','/c','call',('"{0}"' -f $installCmd))

        Write-Log ("DEPLOY_OK target={0}" -f $target)

    }

    catch {

        Write-Log ("DEPLOY_ROLLBACK reason={0}" -f $_.Exception.Message)

        if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force -ErrorAction SilentlyContinue }

        if ($hadTarget -and (Test-Path -LiteralPath $deployBackup)) {

            Copy-Item -LiteralPath $deployBackup -Destination $target -Recurse -Force

        }

        throw

    }

}



$patchFile = $null

$sourceBackup = $null

try {

    Write-Log ("START root={0}" -f $RepositoryRoot)

    $patchFile = Select-PatchFile

    $metadata = Get-PatchMetadata $patchFile.FullName

    $currentVersion = Get-CurrentVersion

    $hash = (Get-FileHash -LiteralPath $patchFile.FullName -Algorithm SHA256).Hash.ToLowerInvariant()



    Write-Host ''

    Write-Host 'PATCH FOUND'

    Write-Host ("  Current version : {0}" -f $currentVersion)

    Write-Host ("  Patch version   : {0}" -f $metadata.Version)

    Write-Host ("  Description     : {0}" -f $metadata.Description)

    Write-Host ("  Patch file      : {0}" -f $patchFile.Name)

    Write-Host ("  Patch SHA-256   : {0}" -f $hash)

    Write-Host ''

    $confirmation = Read-Host 'Install this patch and deploy? [Y/N]'

    if ($confirmation -notmatch '^(?i)y(?:es)?$') {

        Write-Log 'CANCELLED user declined patch installation'

        exit 0

    }



    if ($metadata.Files.Count -gt 0) {

        $sourceBackup = Backup-DeclaredFiles $metadata.Files

        Write-Log ("PATCH_BACKUP_OK files={0}; path={1}" -f $metadata.Files.Count, $sourceBackup.Root)

    }

    else {

        Write-Log 'PATCH_BACKUP_SKIPPED reason=PATCH_FILES metadata not declared'

    }



    $patchText = Get-Content -LiteralPath $patchFile.FullName -Raw

    $scriptBlock = [scriptblock]::Create($patchText)

    $env:DSWR_REPOSITORY_ROOT = $RepositoryRoot

    Write-Log ("PATCH_EXECUTE file={0}" -f $patchFile.Name)

    & $scriptBlock

    Write-Log 'PATCH_EXECUTE_OK'



    Remove-StalePatchArtifacts

    Invoke-RepositoryScript 'build\Verify-Source.ps1'

    Invoke-RepositoryScript 'build\Compile-Source.ps1'

    Invoke-RepositoryScript 'build\Test-Refactor.ps1'

    Invoke-RepositoryScript 'build\Build-Release.ps1'

    Write-Log 'BUILD_OK'



$layout = Get-LocalDeployModsPath

    Write-Log ("GAME_FOLDER_SELECTED game_root={0}; ds_root={1}" -f $layout.GameRoot, $layout.DsRoot)

    Deploy-Release $layout.DsRoot



    Remove-Item -LiteralPath $patchFile.FullName -Force

    Write-Log ("PATCH_DELETED file={0}" -f $patchFile.Name)

    Write-Host ''

    Write-Host 'Patch applied, validated, built, and deployed successfully.'

    Write-Host ("Log: {0}" -f $LogPath)

    exit 0

}

catch {

    $message = $_.Exception.Message

    Write-Log ("FAILED {0}" -f $message)

    if ($null -ne $sourceBackup) {

        try {

            Restore-DeclaredFiles $sourceBackup

            Write-Log 'SOURCE_ROLLBACK_OK'

        }

        catch { Write-Log ("SOURCE_ROLLBACK_FAILED {0}" -f $_.Exception.Message) }

    }

    Write-Host ''

    Write-Host 'Patch/deployment failed.' -ForegroundColor Red

    Write-Host $message -ForegroundColor Red

    Write-Host ("Log: {0}" -f $LogPath)

    exit 1

}

finally {

    Remove-Item Env:DSWR_REPOSITORY_ROOT -ErrorAction SilentlyContinue

}
