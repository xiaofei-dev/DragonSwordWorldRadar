#requires -Version 5.1
<#
.SYNOPSIS
Interactive local-only GitHub publish helper for DragonSwordWorldRadar.

.DESCRIPTION
- The script excludes itself locally via .git/info/exclude, so it is not committed.
- Adds local-only ignore rules for generated/build/runtime artifacts and ooz/ozz executables.
- Optionally runs the repository release validation/build.
- Reviews and stages changes on the current branch.
- Prevents filtered files from being uploaded by skipping/unstaging them only; it never deletes or untracks them.
- Commits and pushes the current branch after confirmation.
- Finally asks whether to merge the current branch into main, pushes main, then returns
  to the original development branch.

This script intentionally does NOT modify .gitignore.
#>

[CmdletBinding()]
param()

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$env:GIT_PAGER = 'cat'
$env:PAGER = 'cat'

function Write-Section {
    param([Parameter(Mandatory = $true)][string]$Text)
    Write-Host ""
    Write-Host ("=" * 72) -ForegroundColor DarkCyan
    Write-Host $Text -ForegroundColor Cyan
    Write-Host ("=" * 72) -ForegroundColor DarkCyan
}

function Confirm-Action {
    param(
        [Parameter(Mandatory = $true)][string]$Message,
        [bool]$DefaultYes = $false
    )

    while ($true) {
        $suffix = if ($DefaultYes) { "[Y/n]" } else { "[y/N]" }
        $answer = Read-Host "$Message $suffix"

        if ([string]::IsNullOrWhiteSpace($answer)) {
            return $DefaultYes
        }

        switch ($answer.Trim().ToLowerInvariant()) {
            'y'   { return $true }
            'yes' { return $true }
            'n'   { return $false }
            'no'  { return $false }
            default {
                Write-Host "Please enter Y or N." -ForegroundColor Yellow
            }
        }
    }
}

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [string]$FailureMessage = "Git command failed."
    )

    & git @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FailureMessage`nCommand: git $($Arguments -join ' ')"
    }
}

function Get-GitOutput {
    param(
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [string]$FailureMessage = "Git command failed."
    )

    $output = @(& git @Arguments)
    if ($LASTEXITCODE -ne 0) {
        throw "$FailureMessage`nCommand: git $($Arguments -join ' ')"
    }
    return $output
}

function Test-GitSuccess {
    param([Parameter(Mandatory = $true)][string[]]$Arguments)

    & git @Arguments *> $null
    return ($LASTEXITCODE -eq 0)
}

function Normalize-GitPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    $normalized = ($Path -replace '\\', '/')
    while ($normalized.StartsWith('./', [System.StringComparison]::Ordinal)) {
        $normalized = $normalized.Substring(2)
    }
    while ($normalized.StartsWith('/', [System.StringComparison]::Ordinal)) {
        $normalized = $normalized.Substring(1)
    }
    return $normalized
}

function Test-LocalOnlyPath {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [string]$ScriptRelativePath
    )

    $p = (Normalize-GitPath $Path).ToLowerInvariant()
    $scriptPath = $null
    if (-not [string]::IsNullOrWhiteSpace($ScriptRelativePath)) {
        $scriptPath = (Normalize-GitPath $ScriptRelativePath).ToLowerInvariant()
    }

    if ($scriptPath -and $p -eq $scriptPath) { return $true }

    if ($p -match '^(dist|runtime|artifacts|out|coverage)/') { return $true }
    if ($p -match '^(\.vs|\.idea|\.vscode)/') { return $true }
    if ($p -match '(^|/)(bin|obj|__pycache__)/') { return $true }

    $name = [System.IO.Path]::GetFileName($p)
    if ($name -eq 'ooz.exe' -or $name -eq 'ozz.exe') { return $true }
    if ($name -eq '.ds_store' -or $name -eq 'thumbs.db') { return $true }

    $ext = [System.IO.Path]::GetExtension($p)
    if ($ext -in @(
        '.log', '.tmp', '.temp',
        '.zip', '.7z', '.rar',
        '.user', '.suo', '.pdb',
        '.cache', '.nupkg'
    )) {
        return $true
    }

    return $false
}

function Get-RelativeScriptPath {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [Parameter(Mandatory = $true)][string]$ScriptPath
    )

    $root = [System.IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
    $script = [System.IO.Path]::GetFullPath($ScriptPath)
    $prefix = $root + [System.IO.Path]::DirectorySeparatorChar

    if ($script.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        return ($script.Substring($prefix.Length) -replace '\\', '/')
    }

    return $null
}

function Install-LocalExcludes {
    param(
        [Parameter(Mandatory = $true)][string]$RepoRoot,
        [string]$ScriptRelativePath
    )

    $gitExcludeLines = @(Get-GitOutput @('rev-parse', '--git-path', 'info/exclude') "Could not locate Git local exclude file.")
    if (@($gitExcludeLines).Length -eq 0) {
        throw "Could not locate Git local exclude file."
    }
    $gitExcludeRaw = $gitExcludeLines[0]
    if ([System.IO.Path]::IsPathRooted($gitExcludeRaw)) {
        $excludePath = $gitExcludeRaw
    }
    else {
        $excludePath = Join-Path $RepoRoot $gitExcludeRaw
    }

    $excludeDir = Split-Path -Parent $excludePath
    if (-not (Test-Path -LiteralPath $excludeDir)) {
        New-Item -ItemType Directory -Path $excludeDir -Force | Out-Null
    }

    $beginMarker = '# BEGIN DragonSwordWorldRadar local publish excludes'
    $endMarker   = '# END DragonSwordWorldRadar local publish excludes'

    $patterns = New-Object System.Collections.Generic.List[string]

    if (-not [string]::IsNullOrWhiteSpace($ScriptRelativePath)) {
        $patterns.Add('/' + (Normalize-GitPath $ScriptRelativePath))
    }

    foreach ($pattern in @(
        '/dist/',
        '/runtime/',
        '/artifacts/',
        '/out/',
        '/coverage/',
        '/.vs/',
        '/.idea/',
        '/.vscode/',
        '**/bin/',
        '**/obj/',
        '**/__pycache__/',
        '**/ooz.exe',
        '**/ozz.exe',
        '*.log',
        '*.tmp',
        '*.temp',
        '*.zip',
        '*.7z',
        '*.rar',
        '*.user',
        '*.suo',
        '*.pdb',
        '*.cache',
        '*.nupkg',
        '.DS_Store',
        'Thumbs.db'
    )) {
        $patterns.Add($pattern)
    }

    $block = @(
        $beginMarker
        '# Local-only rules. This block is stored under .git and is never committed.'
        $patterns
        $endMarker
    ) -join [Environment]::NewLine

    $existing = ''
    if (Test-Path -LiteralPath $excludePath) {
        $existing = [System.IO.File]::ReadAllText($excludePath)
    }

    $escapedBegin = [regex]::Escape($beginMarker)
    $escapedEnd = [regex]::Escape($endMarker)
    $regex = "(?s)$escapedBegin.*?$escapedEnd"

    if ([regex]::IsMatch($existing, $regex)) {
        $updated = [regex]::Replace($existing, $regex, $block)
    }
    else {
        if ($existing.Length -gt 0 -and -not $existing.EndsWith([Environment]::NewLine)) {
            $existing += [Environment]::NewLine
        }
        $updated = $existing + $block + [Environment]::NewLine
    }

    [System.IO.File]::WriteAllText(
        $excludePath,
        $updated,
        (New-Object System.Text.UTF8Encoding($false))
    )

    Write-Host "Local exclude rules updated: $excludePath" -ForegroundColor DarkGray
}

function Get-AheadBehind {
    param(
        [Parameter(Mandatory = $true)][string]$LocalRef,
        [Parameter(Mandatory = $true)][string]$RemoteRef
    )

    $lines = @(Get-GitOutput @('rev-list', '--left-right', '--count', "$LocalRef...$RemoteRef") "Could not compare local and remote branches.")
    if (@($lines).Length -eq 0) {
        throw "Could not compare local and remote branches."
    }
    $line = $lines[0]
    $parts = @($line -split '\s+' | Where-Object { $_ -ne '' })
    if (@($parts).Length -lt 2) {
        throw "Unexpected git rev-list output: $line"
    }

    return [pscustomobject]@{
        Ahead  = [int]$parts[0]
        Behind = [int]$parts[1]
    }
}

function Return-To-Branch {
    param([Parameter(Mandatory = $true)][string]$Branch)

    Write-Host "Returning to development branch '$Branch'..." -ForegroundColor Cyan
    & git checkout $Branch
    if ($LASTEXITCODE -ne 0) {
        Write-Host "WARNING: Could not automatically return to '$Branch'." -ForegroundColor Red
    }
}

# ---------------------------------------------------------------------------
# Repository discovery
# ---------------------------------------------------------------------------

Write-Section "DragonSwordWorldRadar - Interactive GitHub Publish"

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw "git.exe was not found in PATH."
}

$repoRootLine = @(& git rev-parse --show-toplevel 2>$null)
if ($LASTEXITCODE -ne 0 -or @($repoRootLine).Length -eq 0) {
    throw "Run this script from inside the Git repository."
}

$repoRoot = $repoRootLine[0].Trim()
Set-Location -LiteralPath $repoRoot

$branchLines = @(Get-GitOutput @('branch', '--show-current') "Could not determine current branch.")
$currentBranch = if (@($branchLines).Length -gt 0) { [string]$branchLines[0] } else { '' }
$currentBranch = $currentBranch.Trim()

if ([string]::IsNullOrWhiteSpace($currentBranch)) {
    throw "Detached HEAD is not supported. Check out a development branch first."
}

$originUrlLines = @(Get-GitOutput @('remote', 'get-url', 'origin') "Remote 'origin' is not configured.")
if (@($originUrlLines).Length -eq 0) {
    throw "Remote 'origin' is not configured."
}
$originUrl = ([string]$originUrlLines[0]).Trim()

$scriptFullPath = $MyInvocation.MyCommand.Path
$scriptRelativePath = $null
if (-not [string]::IsNullOrWhiteSpace($scriptFullPath)) {
    $scriptRelativePath = Get-RelativeScriptPath -RepoRoot $repoRoot -ScriptPath $scriptFullPath
}

Write-Host "Repository : $repoRoot"
Write-Host "Branch     : $currentBranch"
Write-Host "Origin     : $originUrl"
if ($scriptRelativePath) {
    Write-Host "This script: $scriptRelativePath (local-only)" -ForegroundColor Green
}
else {
    Write-Host "This script is outside the repository and cannot be staged." -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# Local-only filtering
# ---------------------------------------------------------------------------

Write-Section "1. Install local-only filters"
Install-LocalExcludes -RepoRoot $repoRoot -ScriptRelativePath $scriptRelativePath

Write-Host "Filtered categories:" -ForegroundColor DarkGray
Write-Host "  - this publish script" -ForegroundColor DarkGray
Write-Host "  - dist/runtime/artifacts/out/coverage" -ForegroundColor DarkGray
Write-Host "  - .vs/.idea/.vscode, bin/obj/__pycache__" -ForegroundColor DarkGray
Write-Host "  - *.log/*.tmp/*.zip/*.7z/*.rar/*.pdb and similar generated files" -ForegroundColor DarkGray
Write-Host "  - ooz.exe and ozz.exe anywhere in the repository" -ForegroundColor DarkGray

# ---------------------------------------------------------------------------
# Fetch current remote state
# ---------------------------------------------------------------------------

Write-Section "2. Refresh remote state"
Write-Host "Fetching origin (does not modify your working files)..."
Invoke-Git @('fetch', '--prune', 'origin') "git fetch origin failed."

$remoteCurrent = "refs/remotes/origin/$currentBranch"
$remoteCurrentExists = Test-GitSuccess @('show-ref', '--verify', '--quiet', $remoteCurrent)

if ($remoteCurrentExists) {
    $sync = Get-AheadBehind -LocalRef 'HEAD' -RemoteRef "origin/$currentBranch"
    Write-Host "Current branch vs origin/$currentBranch : ahead=$($sync.Ahead), behind=$($sync.Behind)"
    if ($sync.Behind -gt 0) {
        Write-Host "Remote has newer commits. Local changes will be committed first; a rebase will be offered before push." -ForegroundColor Yellow
    }
}
else {
    Write-Host "origin/$currentBranch does not exist yet. First push will create it." -ForegroundColor Yellow
}

# ---------------------------------------------------------------------------
# Optional validation/build
# ---------------------------------------------------------------------------

Write-Section "3. Validate before publishing"
$buildScript = Join-Path $repoRoot 'build\Build-Release.ps1'

if (Test-Path -LiteralPath $buildScript) {
    if (Confirm-Action "Run full Build-Release validation before staging?" $true) {
        Write-Host "Running Windows PowerShell 5.1 release validation/build..." -ForegroundColor Cyan
        & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $buildScript
        if ($LASTEXITCODE -ne 0) {
            throw "Build-Release.ps1 failed. Nothing was committed or pushed."
        }
        Write-Host "Build validation passed." -ForegroundColor Green
    }
    else {
        if (-not (Confirm-Action "Skip validation and continue anyway?" $false)) {
            Write-Host "Stopped by user."
            exit 0
        }
    }
}
else {
    Write-Host "build\Build-Release.ps1 was not found." -ForegroundColor Yellow
    if (-not (Confirm-Action "Continue without repository build validation?" $false)) {
        Write-Host "Stopped by user."
        exit 0
    }
}

# ---------------------------------------------------------------------------
# Report tracked local-only files.
# IMPORTANT: never remove, untrack, or delete them here.
# ---------------------------------------------------------------------------

Write-Section "4. Check tracked local-only files"
$tracked = @(Get-GitOutput @('ls-files') "Could not enumerate tracked files.")
$trackedFiltered = @(
    $tracked | Where-Object { Test-LocalOnlyPath -Path $_ -ScriptRelativePath $scriptRelativePath }
)

if (@($trackedFiltered).Length -gt 0) {
    Write-Host "These filtered/local-only files are currently TRACKED by Git:" -ForegroundColor Yellow
    $trackedFiltered | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
    Write-Host ""
    Write-Host "They will NOT be removed, untracked, or deleted by this script." -ForegroundColor Green
    Write-Host "If git add stages a modification/deletion for one of them, this script will only unstage that path." -ForegroundColor Green
}
else {
    Write-Host "No filtered/local-only files are currently tracked." -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# Review and stage
# ---------------------------------------------------------------------------

Write-Section "5. Review working tree"
Invoke-Git @('status', '--short') "git status failed."

if (-not (Confirm-Action "Stage all non-filtered changes on '$currentBranch'?" $true)) {
    Write-Host "Stopped before staging."
    exit 0
}

Invoke-Git @('add', '-A') "git add failed."

# Safety gate: filtered additions/modifications must never be committed.
$stagedChanged = @(
    Get-GitOutput @('diff', '--cached', '--name-only') "Could not inspect staged files."
)
$unsafeStaged = @(
    $stagedChanged | Where-Object { Test-LocalOnlyPath -Path $_ -ScriptRelativePath $scriptRelativePath }
)

if (@($unsafeStaged).Length -gt 0) {
    Write-Host "Safety gate caught filtered paths in the staging area:" -ForegroundColor Yellow
    $unsafeStaged | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }

    foreach ($path in $unsafeStaged) {
        & git reset -q HEAD -- $path
        if ($LASTEXITCODE -ne 0) {
            throw "Could not unstage filtered path: $path"
        }
    }

    $recheck = @(
        Get-GitOutput @('diff', '--cached', '--name-only') "Could not recheck staged files."
    )
    $stillUnsafe = @(
        $recheck | Where-Object { Test-LocalOnlyPath -Path $_ -ScriptRelativePath $scriptRelativePath }
    )

    if (@($stillUnsafe).Length -gt 0) {
        throw "Filtered files are still staged. Aborting for safety."
    }

    Write-Host "Filtered paths were automatically unstaged only; local files and Git tracking were not changed." -ForegroundColor Green
}

& git diff --cached --quiet
$hasStagedChanges = ($LASTEXITCODE -eq 1)
if ($LASTEXITCODE -notin @(0, 1)) {
    throw "Could not determine whether staged changes exist."
}

if (-not $hasStagedChanges) {
    Write-Host "Nothing to commit after filtering." -ForegroundColor Green
    exit 0
}

Write-Host ""
Write-Host "Staged file summary:" -ForegroundColor Cyan
Invoke-Git @('--no-pager', 'diff', '--cached', '--stat') "Could not show staged diff stat."
Write-Host ""
Invoke-Git @('--no-pager', 'diff', '--cached', '--name-status') "Could not show staged file list."

if (-not (Confirm-Action "Commit exactly the staged changes shown above?" $true)) {
    Write-Host "Stopped. Changes remain staged; nothing was pushed."
    exit 0
}

$defaultMessage = "Update $currentBranch $(Get-Date -Format 'yyyy-MM-dd HH:mm')"
$commitMessage = Read-Host "Commit message (Enter = '$defaultMessage')"
if ([string]::IsNullOrWhiteSpace($commitMessage)) {
    $commitMessage = $defaultMessage
}

Invoke-Git @('commit', '-m', $commitMessage) "git commit failed."
$currentCommitLines = @(Get-GitOutput @('rev-parse', 'HEAD') "Could not read new commit SHA.")
if (@($currentCommitLines).Length -eq 0) {
    throw "Could not read new commit SHA."
}
$currentCommit = ([string]$currentCommitLines[0]).Trim()
Write-Host "Created commit: $currentCommit" -ForegroundColor Green

# ---------------------------------------------------------------------------
# Rebase if origin branch moved, then push current branch
# ---------------------------------------------------------------------------

Write-Section "6. Sync current branch '$currentBranch'"
Invoke-Git @('fetch', '--prune', 'origin') "git fetch origin failed."

$remoteCurrentExists = Test-GitSuccess @('show-ref', '--verify', '--quiet', "refs/remotes/origin/$currentBranch")
if ($remoteCurrentExists) {
    $sync = Get-AheadBehind -LocalRef 'HEAD' -RemoteRef "origin/$currentBranch"
    Write-Host "Before push: ahead=$($sync.Ahead), behind=$($sync.Behind)"

    if ($sync.Behind -gt 0) {
        if (-not (Confirm-Action "Rebase '$currentBranch' onto origin/$currentBranch before push?" $true)) {
            throw "Remote branch is ahead. Push cancelled to avoid overwriting remote history."
        }

        & git rebase "origin/$currentBranch"
        if ($LASTEXITCODE -ne 0) {
            Write-Host "Rebase failed. Aborting rebase to restore the branch." -ForegroundColor Red
            & git rebase --abort *> $null
            throw "Resolve the remote/local divergence manually, then run the script again."
        }
        Write-Host "Rebase completed." -ForegroundColor Green
    }
}

if (-not (Confirm-Action "Push '$currentBranch' to origin now?" $true)) {
    Write-Host "Commit exists locally but was not pushed."
    exit 0
}

Invoke-Git @('push', '-u', 'origin', $currentBranch) "Push of current branch failed."
Write-Host "origin/$currentBranch is synchronized." -ForegroundColor Green

# ---------------------------------------------------------------------------
# Optional current branch -> main merge
# ---------------------------------------------------------------------------

if ($currentBranch -eq 'main') {
    Write-Section "7. Main branch"
    Write-Host "You are already on main. No development-branch merge is required." -ForegroundColor Green
    exit 0
}

Write-Section "7. Optional merge into main"

if (-not (Test-GitSuccess @('show-ref', '--verify', '--quiet', 'refs/remotes/origin/main'))) {
    Write-Host "origin/main does not exist. Main merge is unavailable." -ForegroundColor Yellow
    exit 0
}

Write-Host "Commits currently in '$currentBranch' but not in origin/main:" -ForegroundColor Cyan
& git --no-pager log --oneline --decorate "origin/main..origin/$currentBranch"
if ($LASTEXITCODE -ne 0) {
    throw "Could not compare origin/main with origin/$currentBranch."
}

if (-not (Confirm-Action "Merge '$currentBranch' into main, push main, then return to '$currentBranch'?" $false)) {
    Write-Host "Done. Staying on '$currentBranch' for continued development." -ForegroundColor Green
    exit 0
}

$originalBranch = $currentBranch
$mergeStarted = $false

try {
    Write-Host "Switching to main..." -ForegroundColor Cyan

    if (Test-GitSuccess @('show-ref', '--verify', '--quiet', 'refs/heads/main')) {
        Invoke-Git @('checkout', 'main') "Could not check out local main."
    }
    else {
        Invoke-Git @('checkout', '-b', 'main', '--track', 'origin/main') "Could not create local main tracking branch."
    }

    Write-Host "Fast-forwarding local main to origin/main..." -ForegroundColor Cyan
    Invoke-Git @('pull', '--ff-only', 'origin', 'main') "Local main cannot be fast-forwarded to origin/main."

    Write-Host "Merging '$originalBranch' into main..." -ForegroundColor Cyan
    & git merge --no-ff $originalBranch -m "Merge $originalBranch into main"
    if ($LASTEXITCODE -ne 0) {
        $mergeStarted = $true
        Write-Host "Merge failed or conflicted. Attempting to abort merge." -ForegroundColor Red
        & git merge --abort *> $null
        throw "Merge into main failed. Main was not pushed."
    }

    Write-Host "Main after merge:" -ForegroundColor Cyan
    Invoke-Git @('--no-pager', 'log', '-5', '--oneline', '--decorate') "Could not show main history."

    if (-not (Confirm-Action "Push merged main to origin?" $true)) {
        throw "Main merge exists only locally; main was not pushed."
    }

    Invoke-Git @('push', 'origin', 'main') "Push of main failed."
    Write-Host "origin/main synchronized successfully." -ForegroundColor Green
}
finally {
    Return-To-Branch -Branch $originalBranch
}

Write-Section "Complete"
Write-Host "Current development branch: $originalBranch" -ForegroundColor Green
Write-Host "Current branch was pushed, main was merged/pushed, and you were returned to '$originalBranch'." -ForegroundColor Green
