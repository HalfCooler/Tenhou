<#
.SYNOPSIS
Build and stage the four Tenhou projects, or clean their outputs.
.DESCRIPTION
Projects live at the solution root; each project's sources live in its own src/.
Each project's Build-<project>.ps1 owns its build
and deployment to build/<project>/. Edit $LocalEnvironment below for this machine;
empty values use PATH (CMake/.NET) and vswhere (Visual Studio) discovery.
.PARAMETER Target
All (default), Tenhou.Client, Tenhou.Client.Tests, Tenhou.Server, or Tenhou.Server.Tests.
.PARAMETER Configuration
Release (default) or Debug. Staging always uses build/<project>/.
.PARAMETER Clean
Only remove the selected projects' bin/, obj/, and build/<project>/ directories.
.PARAMETER Fresh
Reconfigure native CMake caches with --fresh before building.
.PARAMETER NativeReference
Also build the frozen native rule oracle. Requires Target All or Tenhou.Server.Tests.
.PARAMETER CMake
Override the local CMake executable path.
.PARAMETER VsDevCmd
Override the local Visual Studio Common7/Tools/VsDevCmd.bat path.
.PARAMETER DotNet
Override the local dotnet executable path.
.PARAMETER Help
Show full help without building. Get-Help ./Build-Tenhou.ps1 -Full also works.
.EXAMPLE
.\Build-Tenhou.ps1 -Target Tenhou.Server -Configuration Debug
.EXAMPLE
.\Build-Tenhou.ps1 -NativeReference -Fresh
.EXAMPLE
.\Build-Tenhou.ps1 -Target Tenhou.Client.Tests -Clean
#>
[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet('All', 'Tenhou.Client', 'Tenhou.Client.Tests', 'Tenhou.Server', 'Tenhou.Server.Tests')]
    [string]$Target = 'All',
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [switch]$Clean,
    [switch]$Fresh,
    [switch]$NativeReference,
    [string]$CMake = '',
    [string]$VsDevCmd = '',
    [string]$DotNet = '',
    [switch]$Help
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# ---------- If auto discover fails, fill in absolute paths below. ----------

# Local environment: fill in absolute paths as needed; '' enables auto discover.
$LocalEnvironment = @{
    CMake = ''       # e.g. C:/Program Files/CMake/bin/cmake.exe
    VsDevCmd = ''    # e.g. C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/VsDevCmd.bat
    DotNet = ''      # e.g. C:/Program Files/dotnet/dotnet.exe
}

# ---------- End of local environment ---------------------------------------

if ($Help) { Get-Help $PSCommandPath -Full; return }
if ($NativeReference -and $Target -notin @('All', 'Tenhou.Server.Tests')) {
    throw '-NativeReference requires -Target Tenhou.Server.Tests or All.'
}
$projects = @('Tenhou.Client', 'Tenhou.Client.Tests', 'Tenhou.Server', 'Tenhou.Server.Tests')
$selected = if ($Target -eq 'All') { $projects } else { @($Target) }
$nativeTools = @{
    CMake = $(if ($CMake) { $CMake } else { $LocalEnvironment.CMake })
    VsDevCmd = $(if ($VsDevCmd) { $VsDevCmd } else { $LocalEnvironment.VsDevCmd })
}
$dotnetTool = if ($DotNet) { $DotNet } else { $LocalEnvironment.DotNet }
foreach ($project in $selected) {
    $settings = @{ Configuration = $Configuration; Clean = $Clean }
    switch ($project) {
        { $_ -in 'Tenhou.Client', 'Tenhou.Client.Tests' } {
            $settings += $nativeTools
            $settings.Fresh = $Fresh
        }
        'Tenhou.Server' { $settings.DotNet = $dotnetTool }
        'Tenhou.Server.Tests' {
            $settings += $nativeTools
            $settings.DotNet = $dotnetTool
            $settings.Fresh = $Fresh
            $settings.NativeReference = $NativeReference
        }
    }
    & (Join-Path $PSScriptRoot "$project/Build-$project.ps1") @settings
}
