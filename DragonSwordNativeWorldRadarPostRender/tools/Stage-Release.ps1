[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

throw @'
Stage-Release.ps1 is retired because its loose archive used a second
load-authority contract. Use tools/Build-Release.ps1, which builds, tests,
re-extracts, and audits the single four-file Setup archive.
'@
