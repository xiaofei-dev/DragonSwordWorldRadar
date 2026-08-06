#requires -Version 5.1
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = [IO.Path]::GetFullPath((Split-Path -Parent $PSScriptRoot))
$logDirectory = Join-Path $root 'runtime\patch-deploy'
New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null
$logPath = Join-Path $logDirectory ('PatchDeploy-' + [DateTime]::Now.ToString('yyyyMMdd-HHmmss') + '.log')
$utf8 = New-Object Text.UTF8Encoding($false)

function Write-Log([string]$Message) {
    $line = '[{0:yyyy-MM-dd HH:mm:ss.fff}] {1}' -f [DateTime]::Now,$Message
    Write-Host $line
    [IO.File]::AppendAllText($logPath,$line + [Environment]::NewLine,$utf8)
}

function Read-ReleaseVersion {
    $releasePath = Join-Path $root 'metadata\release.json'
    if (-not (Test-Path -LiteralPath $releasePath -PathType Leaf)) {
        throw 'metadata\release.json is missing.'
    }
    $release = Get-Content -LiteralPath $releasePath -Raw | ConvertFrom-Json
    $version = [string]$release.version
    if ([string]::IsNullOrWhiteSpace($version)) {
        throw 'metadata\release.json does not define a version.'
    }
    return $version.Trim()
}

function Invoke-Native {
    param(
        [Parameter(Mandatory=$true)][string]$FilePath,
        [Parameter(Mandatory=$true)][string[]]$Arguments,
        [string]$WorkingDirectory = $root
    )
    Write-Log ('RUN ' + $FilePath + ' ' + ($Arguments -join ' '))
    Push-Location -LiteralPath $WorkingDirectory
    try {
        $output = @(& $FilePath @Arguments 2>&1)
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location
    }
    foreach ($line in $output) {
        $text = [string]$line
        Write-Host $text
        [IO.File]::AppendAllText($logPath,$text + [Environment]::NewLine,$utf8)
    }
    if ($exitCode -ne 0) {
        throw ("Command failed with exit code {0}: {1}" -f $exitCode, $FilePath)
    }
}

function Invoke-WindowsPowerShellScript([string]$RelativePath) {
    $windowsPowerShell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    if (-not (Test-Path -LiteralPath $windowsPowerShell -PathType Leaf)) {
        throw 'Windows PowerShell 5.1 is required.'
    }
    $scriptPath = Join-Path $root $RelativePath
    if (-not (Test-Path -LiteralPath $scriptPath -PathType Leaf)) {
        throw "Required script is missing: $RelativePath"
    }
    & $windowsPowerShell -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $scriptPath
    if ($LASTEXITCODE -ne 0) {
        throw "Validation/build script failed: $RelativePath (exit=$LASTEXITCODE)"
    }
}

function Select-PatchFile {
    $patches = @(
        Get-ChildItem -LiteralPath $root -File |
            Where-Object { $_.Extension -in @('.patch','.diff') } |
            Sort-Object Name
    )
    if ($patches.Count -eq 0) {
        throw 'No .patch or .diff file was found in the repository root.'
    }
    if ($patches.Count -eq 1) { return $patches[0] }

    Write-Host ''
    Write-Host 'Available patches:'
    for ($index = 0; $index -lt $patches.Count; $index++) {
        Write-Host ('  [{0}] {1}' -f ($index + 1),$patches[$index].Name)
    }
    while ($true) {
        $raw = Read-Host 'Select patch number'
        $selection = 0
        if ([Int32]::TryParse($raw,[ref]$selection) -and
            $selection -ge 1 -and $selection -le $patches.Count) {
            return $patches[$selection - 1]
        }
        Write-Host 'Invalid selection.' -ForegroundColor Yellow
    }
}

function Select-GameExecutable {
    Add-Type -AssemblyName System.Windows.Forms
    $dialog = New-Object Windows.Forms.OpenFileDialog
    $dialog.Title = 'Select DSClient-Win64-Shipping.exe'
    $dialog.Filter = 'Dragon Sword executable (DSClient-Win64-Shipping.exe)|DSClient-Win64-Shipping.exe|Executable files (*.exe)|*.exe'
    $dialog.CheckFileExists = $true
    $dialog.Multiselect = $false
    if ($dialog.ShowDialog() -ne [Windows.Forms.DialogResult]::OK) {
        throw 'Game executable selection was cancelled.'
    }
    $selected = [IO.Path]::GetFullPath($dialog.FileName)
    if ([IO.Path]::GetFileName($selected) -ine 'DSClient-Win64-Shipping.exe') {
        throw 'The selected file is not DSClient-Win64-Shipping.exe.'
    }
    $win64 = Split-Path -Parent $selected
    $ds = Split-Path -Parent (Split-Path -Parent $win64)
    if ([IO.Path]::GetFileName($ds) -ine 'DS') {
        throw 'The selected executable is not in the expected DS\Binaries\Win64 layout.'
    }
    $gameRoot = Split-Path -Parent $ds
    $pakPath = Join-Path $gameRoot 'DS\Content\Paks\pakchunk109-WindowsClient.pak'
    if (-not (Test-Path -LiteralPath $pakPath -PathType Leaf)) {
        throw 'The selected game installation does not contain pakchunk109-WindowsClient.pak.'
    }
    return [pscustomobject]@{
        Executable = $selected
        Win64 = $win64
        GameRoot = $gameRoot
        ModsRoot = Join-Path $win64 'Mods'
        ModRoot = Join-Path (Join-Path $win64 'Mods') 'DragonSwordWorldRadar'
    }
}

function Copy-ReleaseToGame {
    param(
        [Parameter(Mandatory=$true)][string]$Source,
        [Parameter(Mandatory=$true)][string]$Destination
    )
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    $robocopy = Join-Path $env:SystemRoot 'System32\robocopy.exe'
    & $robocopy $Source $Destination /MIR /COPY:DAT /DCOPY:DAT /R:2 /W:1 /XF 'config.lua' 'treasure_overrides.txt' /XD 'runtime' /NFL /NDL /NJH /NJS /NP
    $code = $LASTEXITCODE
    if ($code -ge 8) {
        throw "Deployment copy failed. robocopy exit code=$code"
    }
}

try {
    Set-Location -LiteralPath $root
    Write-Log ('START root=' + $root)

    $git = (Get-Command git.exe -ErrorAction Stop).Source
    $inside = (& $git -C $root rev-parse --is-inside-work-tree 2>$null).Trim()
    if ($inside -ne 'true') { throw 'Repository root is not a Git work tree.' }
    $patch = Select-PatchFile
    $allowedPatchNames = @(
        Get-ChildItem -LiteralPath $root -File |
            Where-Object { $_.Extension -in @('.patch','.diff') } |
            ForEach-Object { $_.Name }
    )
    $status = @(& $git -C $root status --porcelain)
    $blockingStatus = @($status | Where-Object {
        $line = [string]$_
        if (-not $line.StartsWith('?? ')) { return $true }
        $relative = $line.Substring(3).Trim('"')
        return -not ($allowedPatchNames -contains $relative)
    })
    if ($blockingStatus.Count -ne 0) {
        throw "The Git working tree is not clean. Commit, stash, or remove current changes before applying a patch.`n$($blockingStatus -join [Environment]::NewLine)"
    }

    $baseVersion = Read-ReleaseVersion
    $baseCommit = (& $git -C $root rev-parse --short HEAD).Trim()
    $patchHash = (Get-FileHash -LiteralPath $patch.FullName -Algorithm SHA256).Hash.ToLowerInvariant()

    Write-Host ''
    Write-Host 'PATCH CONFIRMATION' -ForegroundColor Cyan
    Write-Host ('  Current version : {0}' -f $baseVersion)
    Write-Host ('  Current commit  : {0}' -f $baseCommit)
    Write-Host ('  Patch file      : {0}' -f $patch.Name)
    Write-Host ('  Patch SHA-256   : {0}' -f $patchHash)
    Write-Host ''
    $baseConfirmation = Read-Host "Type the current version exactly to apply this patch"
    if ($baseConfirmation -cne $baseVersion) {
        throw 'Base version confirmation did not match. No patch was applied.'
    }

    Invoke-Native -FilePath $git -Arguments @('-C',$root,'apply','--check','--whitespace=error-all','--',$patch.FullName)
    Invoke-Native -FilePath $git -Arguments @('-C',$root,'apply','--index','--whitespace=fix','--',$patch.FullName)

    $targetVersion = Read-ReleaseVersion
    $changed = @(& $git -C $root diff --cached --name-status)
    Write-Host ''
    Write-Host 'PATCH APPLIED TO INDEX' -ForegroundColor Green
    Write-Host ('  Base version   : {0}' -f $baseVersion)
    Write-Host ('  Target version : {0}' -f $targetVersion)
    Write-Host '  Changed files:'
    foreach ($line in $changed) { Write-Host ('    ' + $line) }
    Write-Host ''
    $targetConfirmation = Read-Host "Type the target version exactly to validate, build, and deploy"
    if ($targetConfirmation -cne $targetVersion) {
        throw 'Target version confirmation did not match. Patch remains staged, but nothing was built or deployed.'
    }

    Invoke-WindowsPowerShellScript 'build\Verify-Source.ps1'
    Invoke-WindowsPowerShellScript 'build\Compile-Source.ps1'
    Invoke-WindowsPowerShellScript 'build\Test-Refactor.ps1'
    Invoke-WindowsPowerShellScript 'build\Build-Release.ps1'

    $stageMod = Join-Path $root ('dist\DragonSwordWorldRadar-' + $targetVersion + '\DragonSwordWorldRadar')
    $archive = Join-Path $root ('dist\DragonSwordWorldRadar-v' + $targetVersion + '.zip')
    if (-not (Test-Path -LiteralPath $stageMod -PathType Container)) {
        throw "Built release staging directory is missing: $stageMod"
    }
    if (-not (Test-Path -LiteralPath $archive -PathType Leaf)) {
        throw "Built release archive is missing: $archive"
    }
    $archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    Write-Log ('BUILD_OK archive=' + $archive + '; sha256=' + $archiveHash)

    if (Get-Process -Name 'DSClient-Win64-Shipping' -ErrorAction SilentlyContinue) {
        throw 'Close Dragon Sword Awakening before deployment.'
    }
    $game = Select-GameExecutable
    Write-Host ''
    Write-Host 'DEPLOYMENT CONFIRMATION' -ForegroundColor Cyan
    Write-Host ('  Version       : {0}' -f $targetVersion)
    Write-Host ('  Game EXE      : {0}' -f $game.Executable)
    Write-Host ('  Mod directory : {0}' -f $game.ModRoot)
    $deployConfirmation = Read-Host 'Type DEPLOY to copy and run the installer'
    if ($deployConfirmation -cne 'DEPLOY') {
        throw 'Deployment was cancelled. Patch remains staged and the release remains built.'
    }

    Copy-ReleaseToGame -Source $stageMod -Destination $game.ModRoot
    $install = Join-Path $game.ModRoot 'Install.cmd'
    if (-not (Test-Path -LiteralPath $install -PathType Leaf)) {
        throw 'Deployment completed, but Install.cmd is missing from the target Mod directory.'
    }
    Write-Log ('DEPLOY_COPY_OK target=' + $game.ModRoot)
    & $install
    if ($LASTEXITCODE -ne 0) {
        throw "The deployed installer failed with exit code $LASTEXITCODE."
    }

    Write-Host ''
    Write-Host 'PATCH_BUILD_DEPLOY_COMPLETE' -ForegroundColor Green
    Write-Host ('Version:       {0}' -f $targetVersion)
    Write-Host ('Release ZIP:   {0}' -f $archive)
    Write-Host ('ZIP SHA-256:   {0}' -f $archiveHash)
    Write-Host ('Game Mod path: {0}' -f $game.ModRoot)
    Write-Host ('Log:           {0}' -f $logPath)
    Write-Host ''
    Write-Host 'The patch changes are staged in Git. Review and commit them when ready.'
    exit 0
} catch {
    Write-Log ('FAILED ' + $_.Exception.Message)
    Write-Host ''
    Write-Host 'Patch/deployment failed.' -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host ('Log: ' + $logPath)
    exit 1
}
