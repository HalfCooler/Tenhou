<#
.SYNOPSIS
Build and stage the MSVC x64 client. Use -Clean to remove its outputs.
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
$build = Join-Path $PSScriptRoot "obj/native/$Configuration"
Invoke-TenhouNativeBuild -SourceDirectory $PSScriptRoot -BuildDirectory $build `
    -OutputDirectory $output -Configuration $Configuration -Target tenhou_rebuild -CMake $CMake -VsDevCmd $VsDevCmd -Fresh:$Fresh
Copy-TenhouBuildOutput $PSScriptRoot $output `
    -ResourceCleanupScript (Join-Path $build 'generated/remove_retired_sprites.cmake') -CMake $CMake
