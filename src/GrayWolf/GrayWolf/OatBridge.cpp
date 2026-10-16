#include "stdafx.h"

#include "OatBridge.h"
#include "Strings.h"

#include <filesystem>
#include <fstream>
#include <vector>
#include <cctype>

namespace fs = std::filesystem;

namespace
{
    // Quotes an argument for a Windows command line (wraps in quotes, escapes embedded quotes).
    std::wstring Quote(const std::wstring& Arg)
    {
        std::wstring Out = L"\"";
        for (wchar_t C : Arg)
        {
            if (C == L'"') Out += L"\\\"";
            else Out += C;
        }
        Out += L"\"";
        return Out;
    }

    // Runs a child process in WorkDir with stdout/stderr captured to LogPath, mirrored into Log.
    // Its exit code; a process still running after TimeoutMs is killed (RunTimedOut).
    constexpr DWORD RunFailed = 0xFFFFFFF0, RunTimedOut = 0xFFFFFFF1;
    DWORD RunProcess(const std::wstring& Cmd, const std::string& WorkDir, const std::string& LogPath, std::string& Log,
                     DWORD TimeoutMs = INFINITE)
    {
        SECURITY_ATTRIBUTES Sa{};
        Sa.nLength = sizeof(Sa);
        Sa.bInheritHandle = TRUE;
        HANDLE LogHandle = CreateFileW(Strings::ToUnicodeString(LogPath).c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                       &Sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (LogHandle == INVALID_HANDLE_VALUE) { Log = "cannot open log file"; return RunFailed; }

        STARTUPINFOW Si{};
        Si.cb = sizeof(Si);
        Si.dwFlags = STARTF_USESTDHANDLES;
        Si.hStdOutput = LogHandle;
        Si.hStdError = LogHandle;
        Si.hStdInput = nullptr;

        PROCESS_INFORMATION Pi{};
        std::vector<wchar_t> Mutable(Cmd.begin(), Cmd.end());
        Mutable.push_back(0);
        const std::wstring Dir = Strings::ToUnicodeString(WorkDir);

        const BOOL Ok = CreateProcessW(nullptr, Mutable.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                       nullptr, Dir.c_str(), &Si, &Pi);
        CloseHandle(LogHandle);
        if (!Ok) { Log = "CreateProcess failed"; return RunFailed; }

        DWORD ExitCode = 1;
        if (WaitForSingleObject(Pi.hProcess, TimeoutMs) == WAIT_TIMEOUT)
        {
            TerminateProcess(Pi.hProcess, RunTimedOut);
            WaitForSingleObject(Pi.hProcess, INFINITE);
            ExitCode = RunTimedOut;
        }
        else
            GetExitCodeProcess(Pi.hProcess, &ExitCode);
        CloseHandle(Pi.hProcess);
        CloseHandle(Pi.hThread);

        std::ifstream In(LogPath, std::ios::binary);
        if (In)
            Log.assign((std::istreambuf_iterator<char>(In)), std::istreambuf_iterator<char>());
        return ExitCode;
    }

    // RunProcess in the exe's folder (so a tool resolves its own relative resources): true on exit 0.
    bool RunCaptured(const std::string& Exe, const std::wstring& Cmd, const std::string& LogPath, std::string& Log)
    {
        return RunProcess(Cmd, fs::path(Exe).parent_path().string(), LogPath, Log) == 0;
    }
}

namespace
{
    // Hint when that file exists, else the first <Name> on PATH; empty if neither.
    std::string FindTool(const std::string& Hint, const wchar_t* Name)
    {
        std::error_code Ec;
        if (!Hint.empty() && fs::exists(Hint, Ec))
            return Hint;
        wchar_t Found[MAX_PATH]{};
        const DWORD N = SearchPathW(nullptr, Name, nullptr, MAX_PATH, Found, nullptr);
        return N > 0 && N < MAX_PATH ? fs::path(Found).string() : std::string();
    }
}

std::string ActsFindExe(const std::string& Hint)
{
    return FindTool(Hint, L"acts.exe");
}

bool ActsExtractFastFile(const std::string& ActsExe, const std::string& FastFilePath,
                         const std::string& OutputDir, const ActsExtractOptions& Opts, std::string& Log)
{
    std::error_code Ec;
    const std::string Exe = ActsFindExe(ActsExe);
    if (Exe.empty()) { Log = "acts not found"; return false; }

    fs::create_directories(OutputDir, Ec);
    const std::string LogPath = (fs::path(OutputDir) / "acts_fastfile.log").string();

    // acts [-t] fastfile -o <out> [options] <ff>
    std::wstring Cmd = Quote(Strings::ToUnicodeString(Exe));
    auto Add = [&](const std::wstring& S) { Cmd += L" " + S; };
    auto AddQ = [&](const std::string& S) { Cmd += L" " + Quote(Strings::ToUnicodeString(S)); };

    Add(L"-t");                 // hide the title banner
    Add(L"fastfile");
    Add(L"-d");                 // dump decompressed - without this acts writes nothing at all
    Add(L"-o"); AddQ(OutputDir);
    if (!Opts.Handler.empty())  { Add(L"-r"); Add(Strings::ToUnicodeString(Opts.Handler)); }
    if (!Opts.Assets.empty())   { Add(L"-a"); Add(Strings::ToUnicodeString(Opts.Assets)); }
    if (!Opts.GamePath.empty()) { Add(L"-G"); AddQ(Opts.GamePath); }
    if (Opts.DisableScriptDecomp) Add(L"--disableScriptsDecomp");
    if (Opts.Graphic)             Add(L"--graphic");

    // A "*_patch.ff" (BO3's own naming for a zone's patch fastfile) has no matching ".fd" in a
    // normal install - acts needs -p (use patch files) plus -i (ignore missing fd) to read one.
    const std::string StemLower = [&]
    {
        std::string S = fs::path(FastFilePath).stem().string();
        for (char& C : S) C = (char)tolower((unsigned char)C);
        return S;
    }();
    if (StemLower.size() >= 6 && StemLower.compare(StemLower.size() - 6, 6, "_patch") == 0)
    {
        Add(L"-p");
        Add(L"-i");
    }

    AddQ(FastFilePath);

    return RunCaptured(Exe, Cmd, LogPath, Log);
}

bool ActsDecompileGsc(const std::string& ActsExe, const std::string& ScriptPath,
                      const std::string& OutDir, std::string& OutSource, std::string& Log)
{
    std::error_code Ec;
    const std::string Exe = ActsFindExe(ActsExe);
    if (Exe.empty()) { Log = "acts not found"; return false; }

    fs::remove_all(OutDir, Ec);  // a stale file from a PREVIOUS decompile must not look like this one's
    fs::create_directories(OutDir, Ec);
    const std::string LogPath = (fs::path(OutDir) / "acts_gscd.log").string();

    // acts gscd -g -t pc -o <out> <script> - acts names its own output after the script's
    // internal compiled name (e.g. "scripts/cp/_accolades.gsc"), not ScriptPath's own filename,
    // so the result is found by scanning OutDir afterwards rather than predicted.
    std::wstring Cmd = Quote(Strings::ToUnicodeString(Exe));
    auto Add = [&](const std::wstring& S) { Cmd += L" " + S; };
    auto AddQ = [&](const std::string& S) { Cmd += L" " + Quote(Strings::ToUnicodeString(S)); };

    Add(L"-t");           // hide the title banner
    Add(L"gscd");
    Add(L"-g");            // produce GSC (not just ASM)
    Add(L"-t"); Add(L"pc");
    Add(L"-o"); AddQ(OutDir);
    AddQ(ScriptPath);

    if (!RunCaptured(Exe, Cmd, LogPath, Log))
        return false;

    // Whichever file acts wrote, wherever under OutDir it landed - if more than one somehow
    // exists (should not, given OutDir was just cleared), the most recently written wins.
    std::string BestPath;
    fs::file_time_type BestTime{};
    bool Any = false;
    for (const auto& Entry : fs::recursive_directory_iterator(OutDir, fs::directory_options::skip_permission_denied, Ec))
    {
        if (Ec) break;
        if (!Entry.is_regular_file(Ec)) continue;
        if (Entry.path().filename() == "acts_gscd.log") continue;

        const auto T = Entry.last_write_time(Ec);
        if (!Any || T > BestTime) { BestPath = Entry.path().string(); BestTime = T; Any = true; }
    }

    if (!Any)
    {
        if (Log.empty()) Log = "acts exited cleanly but wrote no output file";
        return false;
    }

    std::ifstream In(BestPath, std::ios::binary);
    if (!In) { Log = "could not read " + BestPath; return false; }
    OutSource.assign((std::istreambuf_iterator<char>(In)), std::istreambuf_iterator<char>());
    return true;
}

std::string GscToolFindExe(const std::string& Hint)
{
    return FindTool(Hint, L"gsc-tool.exe");
}

namespace
{
    // An exit code of a run that went through its inputs (a script failed: gsc-tool 1, acts -1),
    // not one that crashed, timed out or did not start.
    bool RanThrough(DWORD ExitCode) { return ExitCode == 0 || ExitCode == 1 || ExitCode == 0xFFFFFFFF; }
    constexpr DWORD DecompileTimeoutMs = 30 * 60 * 1000;
}

bool ActsDecompileDir(const std::string& ActsExe, const std::string& InDir, const std::string& OutDir, std::string& Log,
                      const std::string& DecryptExe)
{
    std::error_code Ec;
    const std::string Exe = ActsFindExe(ActsExe);
    if (Exe.empty()) { Log = "acts not found"; return false; }
    fs::create_directories(OutDir, Ec);
    const std::string LogPath = (fs::path(OutDir).parent_path() / "acts_gscd.log").string();

    // acts -t [--decrypt-mod <exe>] gscd -g -t pc --path-output -o <out> <in>: <in>/<rel>.gscc
    // decompiled to <out>/<rel>.gsc
    std::wstring Cmd = Quote(Strings::ToUnicodeString(Exe));
    auto Add = [&](const std::wstring& S) { Cmd += L" " + S; };
    auto AddQ = [&](const std::string& S) { Cmd += L" " + Quote(Strings::ToUnicodeString(S)); };
    Add(L"-t");
    if (!DecryptExe.empty()) { Add(L"--decrypt-mod"); AddQ(DecryptExe); }
    Add(L"gscd");
    Add(L"-g");
    Add(L"-t"); Add(L"pc");
    Add(L"--path-output");
    Add(L"-o"); AddQ(OutDir);
    AddQ(InDir);
    return RanThrough(RunProcess(Cmd, fs::path(Exe).parent_path().string(), LogPath, Log, DecompileTimeoutMs));
}

bool ActsNoDecryption(const std::string& Log)
{
    // acts' decryptutils::LoadDecrypt: the dump does not load, or no decrypt.json signature matches once
    return Log.find("Can't load decryption") != std::string::npos;
}

bool GscToolDecompileDir(const std::string& GscToolExe, const std::string& Game, const std::string& System,
                         const std::string& InDir, const std::string& WorkDir, std::string& Log)
{
    const std::string Exe = GscToolFindExe(GscToolExe);
    if (Exe.empty()) { Log = "gsc-tool not found"; return false; }
    const std::string LogPath = (fs::path(WorkDir) / "gsc-tool.log").string();

    // gsc-tool -m decomp -g <game> -s <system> <in>: <in>/<rel>.gscbin decompiled to
    // <cwd>/decompiled/<game>/<rel>.gsc
    std::wstring Cmd = Quote(Strings::ToUnicodeString(Exe));
    auto Add = [&](const std::wstring& S) { Cmd += L" " + S; };
    auto AddQ = [&](const std::string& S) { Cmd += L" " + Quote(Strings::ToUnicodeString(S)); };
    Add(L"-m"); Add(L"decomp");
    Add(L"-g"); Add(Strings::ToUnicodeString(Game));
    Add(L"-s"); Add(Strings::ToUnicodeString(System));
    AddQ(InDir);
    return RanThrough(RunProcess(Cmd, WorkDir, LogPath, Log, DecompileTimeoutMs));
}
