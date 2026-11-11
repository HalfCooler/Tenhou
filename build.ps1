param(
    [ValidateSet('Release','Debug')][string]$Configuration = 'Release',
    [string]$BuildDirectory = '',
    [ValidatePattern('^[A-Za-z0-9_.+-]*$')][string]$Target = ''
)
$ErrorActionPreference = 'Stop'
$sourceDirectory = $PSScriptRoot
$configPath = Join-Path $sourceDirectory 'toolchain.local.json'
if (!(Test-Path -LiteralPath $configPath)) { throw "Missing toolchain configuration: $configPath" }
$toolchain = Get-Content -LiteralPath $configPath -Raw | ConvertFrom-Json
foreach ($key in @('vsdevcmd','cmake')) {
    if (!(Test-Path -LiteralPath $toolchain.$key -PathType Leaf)) { throw "Invalid tool path: $key" }
}
if (!$BuildDirectory) { $BuildDirectory = Join-Path $sourceDirectory "build-$Configuration" }
if (![IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory = Join-Path $sourceDirectory $BuildDirectory }
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
if ($sourceDirectory -match '[^\x00-\x7F]' -or $BuildDirectory -match '[^\x00-\x7F]') { throw 'The NMake toolchain requires ASCII source and build paths.' }
$configure = '"{0}" -S "{1}" -B "{2}" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE={3}' -f $toolchain.cmake,$sourceDirectory,$BuildDirectory,$Configuration
$compile = '"{0}" --build "{1}"' -f $toolchain.cmake,$BuildDirectory
if ($Target) { $compile += ' --target ' + $Target }
$batchPath = Join-Path $env:TEMP ('tenhou-build-' + [guid]::NewGuid().ToString('N') + '.cmd')
try {
    $batchLines = @('@echo off','chcp 65001 >nul',('call "{0}" -no_logo -arch=x86 -host_arch=x64' -f $toolchain.vsdevcmd),'if errorlevel 1 exit /b 1','set VSLANG=1033',$configure,'if errorlevel 1 exit /b 1',$compile)
    [IO.File]::WriteAllLines($batchPath,$batchLines,[Text.UTF8Encoding]::new($false))
    & $env:ComSpec /d /c $batchPath
    if ($LASTEXITCODE) { throw "Build failed: $LASTEXITCODE" }
} finally { Remove-Item -LiteralPath $batchPath -ErrorAction SilentlyContinue }
