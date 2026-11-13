# The solution's build policy lives here. Project scripts only execute these settings.
[CmdletBinding(PositionalBinding = $false)]
param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [ValidateSet('All', 'Tenhou.Client', 'Tenhou.Client.Tests', 'Tenhou.Server', 'Tenhou.Server.Tests')]
    [string]$Target = 'All',
    [switch]$Clean,
    [switch]$Fresh,
    [switch]$NativeReference,
    [string]$CMake = '',
    [string]$VsDevCmd = '',
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Options = @()
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

# PowerShell uses -Name; also accept the documented --Name syntax.
for ($index = 0; $index -lt $Options.Count; ++$index) {
    switch ($Options[$index]) {
        '--Clean' { $Clean = $true }
        '--Fresh' { $Fresh = $true }
        '--NativeReference' { $NativeReference = $true }
        { $_ -in '--Target', '--Configuration', '--CMake', '--VsDevCmd' } {
            $name = $_.Substring(2)
            if (++$index -ge $Options.Count) { throw "Missing value for --$name" }
            Set-Variable -Name $name -Value $Options[$index]
        }
        default { throw "Unknown build option: $_" }
    }
}
$projects = @('Tenhou.Client', 'Tenhou.Client.Tests', 'Tenhou.Server', 'Tenhou.Server.Tests')
if ($Target -notin (@('All') + $projects)) { throw "Unknown project: $Target" }
if ($Configuration -notin @('Release', 'Debug')) { throw "Unknown configuration: $Configuration" }
if ($NativeReference -and $Target -notin @('All', 'Tenhou.Server.Tests')) {
    throw '--NativeReference requires --Target Tenhou.Server.Tests or All.'
}
$root = $PSScriptRoot
$outputRoot = Join-Path $root 'build'
$selected = if ($Target -eq 'All') { $projects } else { @($Target) }

function Remove-BuildDirectory([string]$Path) {
    $resolved = [IO.Path]::GetFullPath($Path)
    if (!$resolved.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean outside the repository: $resolved"
    }
    if (Test-Path -LiteralPath $resolved) {
        $item = Get-Item -LiteralPath $resolved -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Refusing to clean linked directory: $resolved" }
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}

if ($Clean) {
    foreach ($project in $selected) {
        foreach ($directory in @('bin', 'obj')) { Remove-BuildDirectory (Join-Path $root "$project/$directory") }
        Remove-BuildDirectory (Join-Path $outputRoot $project)
    }
    if ($Target -eq 'All') {
        foreach ($directory in @('build-Release', 'build-Debug', 'Tenhou.Client/cmake-build-debug', 'Tenhou.Client/cmake-build-release')) {
            Remove-BuildDirectory (Join-Path $root $directory)
        }
        if ((Test-Path -LiteralPath $outputRoot) -and !(Get-ChildItem -LiteralPath $outputRoot -Force | Select-Object -First 1)) {
            Remove-BuildDirectory $outputRoot
        }
    }
    Write-Host "Cleaned: $($selected -join ', ')"
    return
}

function Resolve-NativeTools {
    if (!$script:CMake) { $script:CMake = (Get-Command cmake -ErrorAction Stop).Source }
    if (!$script:VsDevCmd) {
        $locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        if (Test-Path -LiteralPath $locator) {
            $installation = & $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
            if ($LASTEXITCODE) { throw 'Visual Studio discovery failed.' }
            if ($installation) { $script:VsDevCmd = Join-Path $installation 'Common7/Tools/VsDevCmd.bat' }
        }
        if (!$script:VsDevCmd -and $env:VSINSTALLDIR) { $script:VsDevCmd = Join-Path $env:VSINSTALLDIR 'Common7/Tools/VsDevCmd.bat' }
    }
    foreach ($path in @($script:CMake, $script:VsDevCmd)) {
        if (!$path -or !(Test-Path -LiteralPath $path -PathType Leaf)) {
            throw 'MSVC x86 and CMake are required. Use --VsDevCmd PATH and --CMake PATH to override discovery.'
        }
    }
}

function Build-Native([string]$Project, [string]$NativeTarget) {
    Resolve-NativeTools
    $settings = @{
        Configuration = $Configuration
        BuildDirectory = Join-Path $root "$Project/obj/native/$Configuration"
        OutputDirectory = Join-Path $root "$Project/bin/$Configuration"
        Target = $NativeTarget
        CMake = $script:CMake
        VsDevCmd = $script:VsDevCmd
        Fresh = $Fresh
    }
    & (Join-Path $root 'Tenhou.Client/build.ps1') @settings
}

foreach ($project in $selected) {
    Write-Host "Building $project ($Configuration)..."
    switch ($project) {
        'Tenhou.Client' { Build-Native $project 'tenhou_rebuild' }
        'Tenhou.Client.Tests' { Build-Native $project 'tenhou_client_checks' }
        default {
            $projectFile = Join-Path $root "$project/$project.csproj"
            & dotnet build $projectFile --configuration $Configuration --nologo
            if ($LASTEXITCODE) { throw "$project build failed: $LASTEXITCODE" }
            if ($project -eq 'Tenhou.Server.Tests' -and $NativeReference) {
                Build-Native $project 'tenhou_rule_oracle'
            }
        }
    }
    $source = if ($project -like 'Tenhou.Server*') { Join-Path $root "$project/bin/$Configuration/net11.0" } else { Join-Path $root "$project/bin/$Configuration" }
    $destination = Join-Path $outputRoot $project
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Get-ChildItem -LiteralPath $source -Force | Copy-Item -Destination $destination -Recurse -Force
    if ($project -eq 'Tenhou.Server.Tests' -and $NativeReference) {
        Copy-Item -LiteralPath (Join-Path $root "$project/bin/$Configuration/tenhou_rule_oracle.exe") -Destination $destination -Force
    }
    Write-Host "Output: $destination"
}
