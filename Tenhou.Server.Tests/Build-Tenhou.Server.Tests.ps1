<#
.SYNOPSIS
Build and stage the managed server tests and optional native rule oracle.
.DESCRIPTION
Supports -Configuration Release|Debug, -Clean, -DotNet PATH, -NativeReference,
-Fresh, -CMake PATH, -VsDevCmd PATH. Native tools are only needed with -NativeReference.
Use ../Build-Tenhou.ps1 for the solution's local tool settings.
#>
[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [switch]$Clean,
    [switch]$NativeReference,
    [switch]$Fresh,
    [string]$CMake = '',
    [string]$VsDevCmd = '',
    [string]$DotNet = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../scripts/Build.Common.ps1')
if ($Clean) { Remove-TenhouBuildOutput $PSScriptRoot; return }
$DotNet = Resolve-TenhouExecutable $DotNet 'dotnet'
& $DotNet build (Join-Path $PSScriptRoot 'Tenhou.Server.Tests.csproj') --configuration $Configuration --nologo
if ($LASTEXITCODE) { throw "Tenhou.Server.Tests build failed: $LASTEXITCODE" }
if ($NativeReference) {
    Invoke-TenhouNativeBuild -SourceDirectory (Join-Path $PSScriptRoot '../Tenhou.Client') `
        -BuildDirectory (Join-Path $PSScriptRoot "obj/native/$Configuration") `
        -OutputDirectory (Join-Path $PSScriptRoot "bin/$Configuration/net11.0") `
        -Configuration $Configuration -Target tenhou_rule_oracle -CMake $CMake -VsDevCmd $VsDevCmd -Fresh:$Fresh
}
Copy-TenhouBuildOutput $PSScriptRoot (Join-Path $PSScriptRoot "bin/$Configuration/net11.0")
