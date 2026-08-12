[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot 'Verify-Overlay.ps1')

$sequence = 7
$body = '5|1|2|1234.500|1|radar|1|0|1|1|1|17179869183|0|0|0|0|0|0|1.000|100.000000|200.000000|300.000000|1|22500.000000|0|0|0|0|0|0|0|0|0|0|0'
$payload = "$sequence|$body|$sequence`n"
$assembly = [DragonSwordWorldRadar.Program].Assembly
$parser = $assembly.GetType('DragonSwordWorldRadar.MotionRecordParser', $true)
$frameType = $assembly.GetType('DragonSwordWorldRadar.MotionFrame', $true)
$mapType = $assembly.GetType('DragonSwordWorldRadar.WorldMapState', $true)
$frame = [Activator]::CreateInstance($frameType, $true)
$map = [Activator]::CreateInstance($mapType, $true)
$method = $parser.GetMethod('TryParse', [Reflection.BindingFlags]'Static,NonPublic,Public')
$bytes = [Text.Encoding]::ASCII.GetBytes($payload)
$args = @($bytes, $bytes.Length, $frame, $map)
$accepted = $method.Invoke($null, $args)
if (-not $accepted) { throw 'Protocol v5 compact record was rejected by the packaged Overlay parser' }
$enabled = $frameType.GetField('Enabled').GetValue($frame)
$radius = $frameType.GetField('Radius').GetValue($frame)
if (-not $enabled -or [Math]::Abs([double]$radius - 22500.0) -gt 0.001) { throw 'Protocol publication values changed during parsing' }
Write-Host "PROTOCOL_V5_OK fields=$($payload.Trim().Split('|').Count) radius=$radius"
