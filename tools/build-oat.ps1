<#
.SYNOPSIS
  Builds OpenAssetTools' static libs from this repo's vendored submodule so GrayWolf
  links them with NO prebuilt binaries committed to git.

.DESCRIPTION
  OAT is a premake5 project with a code-generation pre-build step; we cannot
  reference its (git-ignored, regenerated) .vcxproj files directly, so instead we
  build OAT via its OWN generated solution and consume the resulting static libs.

  Idempotent: does nothing if the libs already exist. Pass -Force to rebuild.
  GrayWolf.vcxproj runs it before the first build; it can also be run by hand.

  Requires: git submodules initialized, internet on first run (premake download),
  and MSBuild (VS 2022 v143 toolset or newer).
#>
param([switch]$Force)

$ErrorActionPreference = 'Stop'

$oat = Resolve-Path (Join-Path $PSScriptRoot '..\src\External\OpenAssetTools')
$libDir   = Join-Path $oat 'build\lib\Release_x64'
$sentinel = Join-Path $libDir 'ObjWriting.lib'

if ((Test-Path $sentinel) -and -not $Force) {
    Write-Host "[build-oat] OAT libs already present; skipping. (use -Force to rebuild)"
    exit 0
}

if (-not (Test-Path (Join-Path $oat 'generate.bat'))) {
    throw "[build-oat] OAT submodule not initialized. Run: git submodule update --init --recursive"
}

function Find-MSBuild {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $p = & $vswhere -latest -prerelease -products * -requires Microsoft.Component.MSBuild `
                        -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
        if ($p) { return $p }
    }
    $cands = Get-ChildItem 'C:\Program Files*\Microsoft Visual Studio\*\*\MSBuild\Current\Bin\MSBuild.exe' -ErrorAction SilentlyContinue
    if ($cands) { return $cands[0].FullName }
    throw "[build-oat] MSBuild.exe not found; install the VS 2022 C++ build tools."
}

Push-Location $oat
try {
    # 1. Ensure premake5 (generate.bat downloads+verifies it and inits OAT's own submodules).
    $premake = Join-Path $oat 'build\premake5.exe'
    if (-not (Test-Path $premake)) {
        Write-Host "[build-oat] Fetching premake + initializing OAT submodules..."
        $env:PREMAKE_NO_PROMPT = '1'
        & cmd /c '.\generate.bat' | Out-Null
        if ($LASTEXITCODE -ne 0) { throw "[build-oat] generate.bat failed ($LASTEXITCODE)" }
    }
    # generate.bat uses a premake5 already on PATH and downloads none then (PREMAKE_NO_GLOBAL off)
    if (-not (Test-Path $premake)) {
        $global = Get-Command premake5 -ErrorAction SilentlyContinue
        if (-not $global) { throw "[build-oat] no premake5: neither $premake nor one on PATH" }
        $premake = $global.Source
    }

    # 2. Generate a classic .sln (vs2022). generate.bat targets vs2026 (a .slnx the
    #    stable MSBuild can't build); vs2022 emits an OpenAssetTools.sln we can build.
    Write-Host "[build-oat] Generating OpenAssetTools.sln (premake vs2022)..."
    & $premake vs2022 | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "[build-oat] premake vs2022 failed ($LASTEXITCODE)" }

    # 3. Build the solution (Release|x64). We build the whole .sln rather than
    #    naming lib targets: in a solution build the projects live under solution
    #    folders (src/, thirdparty/), so `-t:ObjWriting` is read as a target name,
    #    not a project, and fails. Any test/GUI project that fails is irrelevant -
    #    we only consume the static libs, so success is judged by the sentinel lib
    #    below, not by MSBuild's exit code.
    $msbuild = Find-MSBuild
    # The submodule tracks the unmerged MW3 x64 PR (Laupetin/OpenAssetTools#1007) plus our MW2 port of it, whose generated
    # loaders pass a size_t to an unsigned parameter. OAT builds with warnings as errors, so VS 2022
    # stops on C4267 there. Harmless for zone sizes; drop this once the PR fixes it.
    $env:CL = ("$env:CL /wd4267").Trim()
    Write-Host "[build-oat] Building OpenAssetTools.sln (Release|x64)..."
    & $msbuild 'build\OpenAssetTools.sln' -p:Configuration=Release -p:Platform=x64 -m -v:minimal -nologo
}
finally { Pop-Location }

if (-not (Test-Path $sentinel)) {
    throw "[build-oat] build finished but the libs are missing ($sentinel). See MSBuild output above."
}
Write-Host "[build-oat] OK - OAT libs built at $libDir"
