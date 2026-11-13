# Shared mechanics only; project entry points select sources, targets, and outputs.
Set-StrictMode -Version Latest

function Remove-TenhouBuildOutput {
    param([string]$ProjectDirectory)
    $repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
    $project = [IO.Path]::GetFullPath($ProjectDirectory)
    if ((Split-Path $project -Parent) -ne $repository -or
        (Split-Path $project -Leaf) -notin @('Tenhou.Client', 'Tenhou.Client.Tests', 'Tenhou.Server', 'Tenhou.Server.Tests')) {
        throw "Not a Tenhou project directory: $project"
    }
    foreach ($path in @((Join-Path $project 'bin'), (Join-Path $project 'obj'),
            (Join-Path $repository ('build/' + (Split-Path $project -Leaf))))) {
        $resolved = [IO.Path]::GetFullPath($path)
        if (!$resolved.StartsWith($repository + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to clean outside the repository: $resolved"
        }
        $ancestor = $resolved
        while ($ancestor -ne $repository) {
            if ((Test-Path -LiteralPath $ancestor) -and
                ((Get-Item -LiteralPath $ancestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                throw "Refusing to clean linked path: $ancestor"
            }
            $ancestor = Split-Path $ancestor -Parent
        }
        if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
    }
    Write-Host "Cleaned: $(Split-Path $project -Leaf)"
}

function Copy-TenhouBuildOutput {
    param([string]$ProjectDirectory, [string]$SourceDirectory)
    $destination = Join-Path $PSScriptRoot ('../build/' + (Split-Path $ProjectDirectory -Leaf))
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    Get-ChildItem -LiteralPath $SourceDirectory -Force | Copy-Item -Destination $destination -Recurse -Force
    Write-Host "Output: $([IO.Path]::GetFullPath($destination))"
}

function Resolve-TenhouExecutable {
    param([string]$Path, [string]$Name)
    if (!$Path) { $Path = (Get-Command $Name -CommandType Application -ErrorAction Stop).Source }
    if (!(Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Executable not found: $Path" }
    return (Resolve-Path -LiteralPath $Path).Path
}

function Invoke-TenhouNativeBuild {
    param(
        [string]$SourceDirectory,
        [string]$BuildDirectory,
        [string]$OutputDirectory,
        [ValidateSet('Release', 'Debug')][string]$Configuration,
        [ValidateSet('tenhou_rebuild', 'tenhou_client_checks', 'tenhou_rule_oracle')][string]$Target,
        [string]$CMake,
        [string]$VsDevCmd,
        [switch]$Fresh
    )
    $CMake = Resolve-TenhouExecutable $CMake 'cmake'
    if (!$VsDevCmd) {
        $locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        if (Test-Path -LiteralPath $locator) {
            $installation = & $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
            if ($LASTEXITCODE) { throw 'Visual Studio discovery failed.' }
            if ($installation) { $VsDevCmd = Join-Path $installation 'Common7/Tools/VsDevCmd.bat' }
        }
        if (!$VsDevCmd -and $env:VSINSTALLDIR) { $VsDevCmd = Join-Path $env:VSINSTALLDIR 'Common7/Tools/VsDevCmd.bat' }
    }
    if (!$VsDevCmd -or !(Test-Path -LiteralPath $VsDevCmd -PathType Leaf)) {
        throw 'MSVC x86 is required. Set LocalEnvironment.VsDevCmd in Build-Tenhou.ps1 or pass -VsDevCmd PATH.'
    }
    $VsDevCmd = (Resolve-Path -LiteralPath $VsDevCmd).Path
    foreach ($path in @($SourceDirectory, $BuildDirectory, $OutputDirectory, $CMake, $VsDevCmd)) {
        if ($path -match '["%\r\n!&|<>^]') { throw "Unsupported character in native build path: $path" }
        if ($path -match '[^\x00-\x7F]') { throw 'The NMake toolchain requires ASCII build paths.' }
    }
    $checks = if ($Target -eq 'tenhou_client_checks') { 'ON' } else { 'OFF' }
    $reference = if ($Target -eq 'tenhou_rule_oracle') { 'ON' } else { 'OFF' }
    $configure = '"{0}" -S "{1}" -B "{2}" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE={3} -DTENHOU_BUILD_CLIENT_TESTS={4} -DTENHOU_BUILD_RULE_ORACLE={5} -DTENHOU_OUTPUT_DIRECTORY="{6}"' -f $CMake, $SourceDirectory, $BuildDirectory, $Configuration, $checks, $reference, $OutputDirectory.Replace('\', '/')
    if ($Fresh) { $configure += ' --fresh' }
    $compile = '"{0}" --build "{1}" --target {2}' -f $CMake, $BuildDirectory, $Target
    $batchPath = Join-Path $env:TEMP ('tenhou-build-' + [guid]::NewGuid().ToString('N') + '.cmd')
    try {
        $lines = @('@echo off', 'chcp 65001 >nul', ('call "{0}" -no_logo -arch=x86 -host_arch=x64' -f $VsDevCmd),
            'if errorlevel 1 exit /b 1', 'set VSLANG=1033', $configure, 'if errorlevel 1 exit /b 1', $compile, 'exit /b %errorlevel%')
        [IO.File]::WriteAllLines($batchPath, $lines, [Text.UTF8Encoding]::new($false))
        & $env:ComSpec /d /c "`"$batchPath`""
        if ($LASTEXITCODE) { throw "Native build failed: $LASTEXITCODE" }
    }
    finally { Remove-Item -LiteralPath $batchPath -ErrorAction SilentlyContinue }
}
