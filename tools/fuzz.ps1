# tools/fuzz.ps1 - build and run GrayWolf's libFuzzer harnesses (src/Fuzz).
#
# Usage: pwsh tools/fuzz.ps1 [-Build] [-Target <name>[,<name>...]] [-Seconds 600] [-Corpus <dir>]
#                            [-Out <dir>] [-Flags '<libFuzzer flags>']
#
# -Corpus and -Out default to graywolf-fuzz\corpus and graywolf-fuzz\out under %TEMP%.
# -Flags '-fork=1 -ignore_crashes=1 -ignore_ooms=1 -ignore_timeouts=1' keeps fuzzing past a crash.
# Targets: fastfile ifs sab xpak ipak iwd iwi seanim semodel sidecar (all of them by default, in
# parallel, one process each). Each runs -Seconds (libFuzzer -max_total_time) over <Corpus>\<target>
# (new inputs are added there; seeded from src/Fuzz/seeds/make_seeds.py's synthetic files when
# empty) plus <Corpus>\<target>-real when it exists (real files, kept outside the repo). Crashes
# land in <Out>\<target>\crash-<sha1>; replay one with:
#   $env:GW_FUZZ_TARGET='<target>'; src\Fuzz\x64\DevFuzz\GrayWolfFuzz.exe <crash file>
param(
    [switch]$Build,
    [string[]]$Target = @('fastfile', 'ifs', 'sab', 'xpak', 'ipak', 'iwd', 'iwi', 'seanim', 'semodel', 'sidecar'),
    [int]$Seconds = 600,
    [string]$Corpus = (Join-Path $env:TEMP 'graywolf-fuzz\corpus'),
    [string]$Out = (Join-Path $env:TEMP 'graywolf-fuzz\out'),
    [string]$Flags = ''
)
$ErrorActionPreference = 'Stop'
$repo = Resolve-Path (Join-Path $PSScriptRoot '..')
$exe = Join-Path $repo 'src\Fuzz\x64\DevFuzz\GrayWolfFuzz.exe'

if ($Build -or -not (Test-Path $exe)) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    & $msbuild (Join-Path $repo 'src\Fuzz\Fuzz.sln') -p:Configuration=Dev -p:Platform=x64 -m -v:m -nologo
    if ($LASTEXITCODE -ne 0) { throw "fuzz build failed ($LASTEXITCODE)" }
}

$Target = $Target | ForEach-Object { $_ -split ',' } | Where-Object { $_ }
$procs = @()
foreach ($t in $Target) {
    $dir = Join-Path $Corpus $t
    $art = Join-Path $Out $t
    New-Item -ItemType Directory -Force $dir, $art | Out-Null
    if (-not (Get-ChildItem $dir -File | Select-Object -First 1)) {
        & py -3 (Join-Path $repo 'src\Fuzz\seeds\make_seeds.py') $t $dir
    }
    $dirs = @($dir)
    if (Test-Path "$dir-real") { $dirs += "$dir-real" }
    $log = Join-Path $art 'fuzz.log'
    $fargs = @("-target=$t", "-max_total_time=$Seconds", '-timeout=20', '-rss_limit_mb=2048', '-malloc_limit_mb=1024',
              '-print_final_stats=1', "-artifact_prefix=$art\") + ($Flags -split '\s+' | Where-Object { $_ }) + $dirs
    $procs += [pscustomobject]@{
        Target = $t; Log = $log
        Process = Start-Process -FilePath $exe -ArgumentList $fargs -RedirectStandardError $log -RedirectStandardOutput "$log.out" -PassThru -NoNewWindow
    }
}
$procs | ForEach-Object { $_.Process.WaitForExit() }

# MSVC's instrumentation has no PC table, so libFuzzer's "cov" stays 0: "ft" (features) is the coverage
$summary = foreach ($p in $procs) {
    $last = Get-Content $p.Log | Select-String '^#(\d+)\b.*\bft: (\d+) corp: (\d+)' | Select-Object -Last 1
    $crashes = @(Get-ChildItem (Split-Path $p.Log) -File | Where-Object { $_.Name -match '^(crash|leak|timeout|oom|slow-unit)-' })
    [pscustomobject]@{
        Target = $p.Target; Exit = $p.Process.ExitCode
        Execs = if ($last) { $last.Matches[0].Groups[1].Value } else { '' }
        Features = if ($last) { $last.Matches[0].Groups[2].Value } else { '' }
        Corpus = if ($last) { $last.Matches[0].Groups[3].Value } else { '' }
        Artifacts = $crashes.Count
    }
}
$summary | Format-Table -AutoSize | Out-String -Width 200
if ($summary | Where-Object { $_.Artifacts -gt 0 }) { exit 1 }
