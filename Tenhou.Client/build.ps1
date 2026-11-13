# Internal native executor. Use ../build.ps1 for the public build interface.
param(
    [Parameter(Mandatory = $true)][ValidateSet('Release', 'Debug')][string]$Configuration,
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [Parameter(Mandatory = $true)][ValidateSet('tenhou_rebuild', 'tenhou_client_checks', 'tenhou_rule_oracle')][string]$Target,
    [Parameter(Mandatory = $true)][string]$CMake,
    [Parameter(Mandatory = $true)][string]$VsDevCmd,
    [switch]$Fresh
)
$ErrorActionPreference = 'Stop'
foreach ($path in @($PSScriptRoot, $BuildDirectory, $OutputDirectory, $CMake, $VsDevCmd)) {
    if ($path -match '["%\r\n!&|<>^]') { throw "Unsupported character in native build path: $path" }
    if ($path -match '[^\x00-\x7F]') { throw 'The NMake toolchain requires ASCII build paths.' }
}
$checks = if ($Target -eq 'tenhou_client_checks') { 'ON' } else { 'OFF' }
$reference = if ($Target -eq 'tenhou_rule_oracle') { 'ON' } else { 'OFF' }
$configure = '"{0}" -S "{1}" -B "{2}" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE={3} -DTENHOU_BUILD_CLIENT_TESTS={4} -DTENHOU_BUILD_RULE_ORACLE={5} -DTENHOU_OUTPUT_DIRECTORY="{6}"' -f $CMake, $PSScriptRoot, $BuildDirectory, $Configuration, $checks, $reference, $OutputDirectory.Replace('\', '/')
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
