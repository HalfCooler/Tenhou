param([ValidateSet('start','stop')][string]$Action='start',[string]$Config)
$ErrorActionPreference='Stop'
$taskServerRoot=$PSScriptRoot
$taskCandidates=@(
    (Join-Path $taskServerRoot 'tenhou_server.exe'),
    (Join-Path $taskServerRoot '../build-Release/Tenhou.Server/tenhou_server.exe'),
    (Join-Path $taskServerRoot 'build-Release/tenhou_server.exe'),
    (Join-Path $taskServerRoot '../build-Release/native_server/tenhou_server.exe')
)
$taskExe=$taskCandidates | Where-Object {Test-Path -LiteralPath $_ -PathType Leaf} | Select-Object -First 1
if(!$taskExe) {throw '请先构建 Release 服务程序。'}
$taskExe=(Resolve-Path -LiteralPath $taskExe).Path
if(!$Config) {$Config=Join-Path $taskServerRoot 'server.ini'}
$Config=(Resolve-Path -LiteralPath $Config).Path
if($Action -eq 'stop') {& $taskExe --config $Config --stop;exit $LASTEXITCODE}
$taskOutput=Join-Path $taskServerRoot 'logs/startup'
New-Item -ItemType Directory -Force -Path $taskOutput | Out-Null
$taskProcess=Start-Process -FilePath $taskExe -ArgumentList @('--config',('"'+$Config+'"')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $taskOutput 'stdout.log') -RedirectStandardError (Join-Path $taskOutput 'stderr.log')
if($taskProcess.WaitForExit(1000)) {Get-Content -LiteralPath (Join-Path $taskOutput 'stderr.log');exit 1}
Write-Output "本机服务已启动，PID $($taskProcess.Id)。"
