<#
.SYNOPSIS
Build and stage the managed server. Use -Clean to remove its outputs.
.DESCRIPTION
Supports -Configuration Release|Debug, -Clean, -DotNet PATH.
Use ../Build-Tenhou.ps1 for the solution's local tool settings.
#>
[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [switch]$Clean,
    [string]$DotNet = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '../scripts/Build.Common.ps1')
if ($Clean) { Remove-TenhouBuildOutput $PSScriptRoot; return }
$DotNet = Resolve-TenhouExecutable $DotNet 'dotnet'
& $DotNet build (Join-Path $PSScriptRoot 'Tenhou.Server.csproj') --configuration $Configuration --nologo
if ($LASTEXITCODE) { throw "Tenhou.Server build failed: $LASTEXITCODE" }
Copy-TenhouBuildOutput $PSScriptRoot (Join-Path $PSScriptRoot "bin/$Configuration/net11.0")
