<#
.SYNOPSIS
Build and stage the native client checks. Use -Clean to remove their outputs.
.DESCRIPTION
Supports -Configuration Release|Debug, -Clean, -Fresh, -CMake PATH, -VsDevCmd PATH.
Use ../Build-Tenhou.ps1 for the solution's local tool settings.
#>
[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [switch]$Clean,
    [switch]$Fresh,
    [string]$CMake = '',
    [string]$VsDevCmd = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../scripts/Build.Common.ps1')
if ($Clean) { Remove-TenhouBuildOutput $PSScriptRoot; return }
$output = Join-Path $PSScriptRoot "bin/$Configuration"
Invoke-TenhouNativeBuild -SourceDirectory (Join-Path $PSScriptRoot '../Tenhou.Client') `
    -BuildDirectory (Join-Path $PSScriptRoot "obj/native/$Configuration") -OutputDirectory $output `
    -Configuration $Configuration -Target tenhou_client_checks -CMake $CMake -VsDevCmd $VsDevCmd -Fresh:$Fresh
Copy-TenhouBuildOutput $PSScriptRoot $output
