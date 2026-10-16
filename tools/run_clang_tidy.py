#!/usr/bin/env python3
"""tools/run_clang_tidy.py - opt-in clang-tidy static analysis over GrayWolf source files.

Not part of the normal build. Drives clang-tidy.exe directly against a compile_commands.json it
writes itself (the flags VS's native clang-tidy integration uses), never through MSBuild: a
RunCodeAnalysisOnce + SelectedFiles build recompiles whatever else MSBuild thinks is stale and
aborts on any broken file before the selected file is analysed, and it shares x64/Dev with
concurrent `tools/build.sh` runs.

Known limitation: a translation unit that includes resource.h (UTF-16 LE with BOM, e.g. through
MainWindow.h or the *Settings.h headers) fails to parse under clang; such files are reported as
clang-tidy reports them, not skipped by guess.

Checks/config come from .clang-tidy at the repo root.

Usage:
    python tools/run_clang_tidy.py                  # representative sample (default file list)
    python tools/run_clang_tidy.py file1.cpp ...     # specific GrayWolf/GrayWolf-relative files
    python tools/run_clang_tidy.py --all             # every *.cpp in GrayWolf/GrayWolf (slow)
"""

import json
import os
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PROJ_DIR = REPO_ROOT / "src" / "GrayWolf" / "GrayWolf"
OUT_FILE = REPO_ROOT / "tools" / "clang-tidy-report.log"

DEFAULT_FILES = [
    "CoDXModelTranslator.cpp",
    "CoDXAnimTranslator.cpp",
    "AssetRegistry.cpp",
]

CLANG_TIDY_CANDIDATES = [
    r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
    r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin\clang-tidy.exe",
    r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Tools\Llvm\x64\bin\clang-tidy.exe",
]


def find_clang_tidy() -> str:
    for c in CLANG_TIDY_CANDIDATES:
        if os.path.isfile(c):
            return c
    # Fall back to PATH (e.g. `pip install clang-tidy`).
    return "clang-tidy"


def build_command(vcpkg_root: str, target_file: str) -> str:
    # The command line VS's EnableClangTidyCodeAnalysis integration writes for every
    # GrayWolf/GrayWolf .cpp; only the source file changes between entries.
    return (
        '"C:\\WINDOWS\\system32\\CL.exe" /c '
        '/I..\\OatInterop /I..\\..\\Core\\Core /I..\\..\\External\\DirectXShaderCompiler\\include '
        f'/I"{vcpkg_root}\\installed\\x64-windows-static-md\\include" '
        f'/I"{vcpkg_root}\\installed\\x64-windows\\include" '
        "/nologo /W3 /WX- /diagnostics:column /Od "
        "/D CODE_ANALYSIS /D _UNICODE /D UNICODE /D _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS "
        "/D CASCLIB_NO_AUTO_LINK_LIBRARY /D CASCLIB_UNICODE /D CASCLIB_DETECT_UNICODE_MISMATCHES "
        "/D FLAC__NO_DLL /EHa /MD /GS /fp:precise /std:c++17 "
        '/Fo"x64\\Dev\\\\" /Gd /TP --target=amd64-pc-windows-msvc /utf-8 '
        f'"{target_file}"'
    )


def main() -> int:
    args = sys.argv[1:]
    if args == ["--all"]:
        files = sorted(p.name for p in PROJ_DIR.glob("*.cpp"))
    elif args:
        files = args
    else:
        files = DEFAULT_FILES

    vcpkg_root = os.environ.get("VCPKG_ROOT", "")
    if not vcpkg_root:
        print("run_clang_tidy: VCPKG_ROOT is not set (your vcpkg folder; BUILDING.md)", file=sys.stderr)
        return 2

    db_dir = REPO_ROOT / "tools" / "_clang_tidy_db"
    db_dir.mkdir(exist_ok=True)
    entries = []
    target_paths = []
    for f in files:
        abs_path = str(PROJ_DIR / f)
        target_paths.append(abs_path)
        entries.append(
            {
                "directory": str(PROJ_DIR),
                "command": build_command(vcpkg_root, abs_path),
                "file": abs_path,
            }
        )
    (db_dir / "compile_commands.json").write_text(json.dumps(entries, indent=2), encoding="utf-8")

    clang_tidy = find_clang_tidy()
    cmd = [clang_tidy, f"-p={db_dir}", "--config-file", str(REPO_ROOT / ".clang-tidy")] + target_paths

    print(f"Running: {clang_tidy} against {len(target_paths)} file(s)...", file=sys.stderr)
    result = subprocess.run(cmd, capture_output=True, text=True)
    OUT_FILE.write_text(result.stdout + result.stderr, encoding="utf-8")

    print(f"Report written to tools/{OUT_FILE.name}", file=sys.stderr)
    print("", file=sys.stderr)
    print("Findings by check:", file=sys.stderr)
    import re
    from collections import Counter

    counts = Counter()
    for line in (result.stdout + result.stderr).splitlines():
        m = re.search(r"\[([a-zA-Z0-9._,-]+)\]\s*$", line)
        if m:
            for check in m.group(1).split(","):
                counts[check] += 1
    for check, n in counts.most_common(30):
        print(f"  {n:5d} {check}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
