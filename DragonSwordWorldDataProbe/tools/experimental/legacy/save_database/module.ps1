$global:DSProbeModuleRegistry['live.save_database'] = [pscustomobject]@{
    Id='live.save_database'; Phase='live'; Risk='read_process_memory_and_read_only_sqlcipher'
    Initialize={ param($Context)
        $dll = Find-SqlCipherDll $Context.Root
        if (-not $dll) {
            Write-ProbeLog $Context 'SAVE_DATABASE_DISABLED' 'reason=sqlcipher_not_found; raw-save collection remains available'
            $Context.ModuleState['live.save_database'] = @{ Enabled=$false }
            return
        }
        try {
            if (-not ('DragonSwordWorldDataProbe.WorldBossSaveMonitor' -as [type])) {
                $source = [IO.File]::ReadAllText((Join-Path $Context.Root 'tools\modules\save_database\WorldBossSaveMonitor.cs'))
                Add-Type -TypeDefinition $source -Language CSharp
            }
            [DragonSwordWorldDataProbe.WorldBossSaveMonitor]::Configure(
                $dll,
                (Join-Path $Context.Root 'metadata\world-boss-catalog.json'),
                (Join-Path $Context.Root 'runtime\logs\save-state.tsv'),
                (Join-Path $Context.Root 'runtime\logs\save-changes.log'),
                (Join-Path $Context.Root 'runtime\reports\save-status.txt'),
                $Context.Root)
            $Context.ModuleState['live.save_database'] = @{ Enabled=$true; SqlCipher=$dll; LastResult=$null; LastLog=[DateTime]::MinValue }
            Write-ProbeLog $Context 'SAVE_DATABASE_READY' ('sqlcipher="{0}"; logical_full_database_snapshots=enabled' -f $dll)
        } catch {
            $Context.ModuleState['live.save_database'] = @{ Enabled=$false; Error=$_.Exception.Message }
            Write-ProbeLog $Context 'SAVE_DATABASE_DISABLED' ('reason=compile_or_config_error; error="{0}"' -f $_.Exception.Message.Replace('"',"'"))
        }
    }
    Poll={ param($Context)
        $state = $Context.ModuleState['live.save_database']
        if (-not $state -or -not $state.Enabled -or -not $Context.Game) { return }
        try {
            $result = [DragonSwordWorldDataProbe.WorldBossSaveMonitor]::Poll(
                [int]$Context.Game.ProcessId,
                [string]$Context.Game.Executable,
                [string]$Context.Game.SaveRoot,
                [string]$Context.Game.PakRoot)
            if ($result -ne $state.LastResult -or ((Get-Date)-$state.LastLog).TotalSeconds -ge 30) {
                Write-ProbeLog $Context 'SAVE_DATABASE_POLL' $result
                $state.LastResult=$result; $state.LastLog=Get-Date
            }
        } catch { Write-ProbeLog $Context 'SAVE_DATABASE_POLL_ERROR' $_.Exception.Message }
    }
    Stop={ param($Context)
        try { if ('DragonSwordWorldDataProbe.WorldBossSaveMonitor' -as [type]) { [DragonSwordWorldDataProbe.WorldBossSaveMonitor]::Reset() } } catch {}
    }
    Collect=$null
}
