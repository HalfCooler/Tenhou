param(
    [ValidateSet('start', 'stop')][string]$Action = 'start',
    [string]$ConfigurationFile = '',
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release'
)
$ErrorActionPreference = 'Stop'
$taskRoot = $PSScriptRoot
if (!$ConfigurationFile) { $ConfigurationFile = Join-Path $taskRoot 'server.json' }
$ConfigurationFile = [IO.Path]::GetFullPath($ConfigurationFile)
$taskCandidates = @(
    (Join-Path $taskRoot 'Tenhou.Server.dll'),
    (Join-Path $taskRoot "bin/$Configuration/net11.0/Tenhou.Server.dll"),
    (Join-Path $taskRoot '../build/Tenhou.Server/Tenhou.Server.dll')
)
$taskBinary = $taskCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
if (!$taskBinary) { throw 'Build the server first: ./build.ps1 --Target Tenhou.Server' }
if ($Action -eq 'stop') {
    & dotnet $taskBinary --config $ConfigurationFile --stop
    if ($LASTEXITCODE) { throw 'The server did not acknowledge the shutdown request.' }
} else {
    $taskProcess = Start-Process -FilePath 'dotnet' -ArgumentList @('"' + $taskBinary + '"', '--config', '"' + $ConfigurationFile + '"') -WindowStyle Hidden -PassThru
    Write-Output "Server process started: $($taskProcess.Id)."
}
