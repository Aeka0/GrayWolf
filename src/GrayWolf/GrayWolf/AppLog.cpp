#include "stdafx.h"

#include "AppLog.h"

#include "CoDAssets.h"
#include "FileSystems.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>

#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

namespace AppLog
{
namespace
{
    using Clock = std::chrono::steady_clock;

    std::shared_ptr<spdlog::sinks::basic_file_sink_mt> SessionSink;
    std::string SessionFile;
    constexpr const char* Pattern = "[%Y-%m-%d %H:%M:%S.%e] [%l] [t%t] %v";

    double MsSince(Clock::time_point T)
    {
        return std::chrono::duration<double, std::milli>(Clock::now() - T).count();
    }

    spdlog::level::level_enum Quiet()
    {
        char V[16] = {};
        if (GetEnvironmentVariableA("GH_LOG_LEVEL", V, sizeof(V)) == 0) return spdlog::level::info;
        const auto L = spdlog::level::from_str(V);
        return L == spdlog::level::off && std::string(V) != "off" ? spdlog::level::info : L;
    }

    // The UI thread's frame and the timed steps open in it. Never destroyed: the watch thread
    // may outlive static destruction at exit.
    struct Ui
    {
        std::mutex M;
        DWORD Thread = 0;
        bool InFrame = false, Frozen = false;
        Clock::time_point FrameStart;
        std::vector<const Timer*> Open;
        std::string Slowest;
        double SlowestMs = 0;
        bool UserWaited = false;
        double NextReport = 1000;
    };
    Ui& UiState()
    {
        static Ui* U = new Ui();
        return *U;
    }

    std::string OpenSteps(const Ui& U)
    {
        std::string S;
        for (const Timer* T : U.Open)
            S += (S.empty() ? "" : " > ") + T->What() + " (" + std::to_string((long long)MsSince(T->Started())) + " ms)";
        return S.empty() ? "no timed step" : S;
    }

    void WatchUi()
    {
        Ui& U = UiState();
        for (;;)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            std::string Line;
            {
                std::lock_guard<std::mutex> L(U.M);
                if (!U.InFrame) continue;
                bool Waiting = false;
                for (const Timer* T : U.Open) Waiting = Waiting || T->WaitingOnUser();
                const double Ms = MsSince(U.FrameStart);
                if (Waiting || Ms < U.NextReport) continue;
                U.Frozen = true;
                U.NextReport *= 2;
                Line = "UI not responding for " + std::to_string((long long)(Ms / 1000)) + " s, in: " + OpenSteps(U);
            }
            Log().warn(Line);
        }
    }
}

void Init()
{
    SessionFile = (std::filesystem::path(FileSystems::GetApplicationPath()) / "GrayWolfLog.txt").string();
    // kept to one earlier session's worth past 20 MB
    std::error_code Ec;
    if (std::filesystem::file_size(SessionFile, Ec) > 20ull << 20)
        std::filesystem::rename(SessionFile, SessionFile + ".old", Ec);
    try
    {
        SessionSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(SessionFile);
        spdlog::init_thread_pool(8192, 1);
        auto L = std::make_shared<spdlog::async_logger>("GrayWolf", SessionSink, spdlog::thread_pool(),
                                                         spdlog::async_overflow_policy::block);
        L->set_pattern(Pattern);
        L->set_level(Quiet());
        L->flush_on(spdlog::level::warn);
        spdlog::register_logger(L);
        CoDAssets::Log = L;
    }
    catch (const spdlog::spdlog_ex&)
    {
        CoDAssets::Log = std::make_shared<spdlog::logger>("GrayWolf", std::make_shared<spdlog::sinks::null_sink_mt>());
    }
    spdlog::flush_every(std::chrono::seconds(3));
    Log().info("session start: pid {}, {}", GetCurrentProcessId(), GetCommandLineA());
}

spdlog::logger& Log()
{
    static auto None = std::make_shared<spdlog::logger>("none", std::make_shared<spdlog::sinks::null_sink_mt>());
    return CoDAssets::Log ? *CoDAssets::Log : *None;
}

std::string SessionPath()
{
    return SessionFile;
}

bool Verbose()
{
    return Log().should_log(spdlog::level::debug);
}

void SetVerbose(bool On)
{
    Log().set_level(On ? spdlog::level::debug : spdlog::level::info);
    Log().info("log level {}", On ? "debug" : "info");
}

std::shared_ptr<spdlog::logger> Tee(const std::string& Path)
{
    std::vector<spdlog::sink_ptr> Sinks;
    if (SessionSink) Sinks.push_back(SessionSink);
    try
    {
        Sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(Path));
    }
    catch (const spdlog::spdlog_ex& E)
    {
        Log().warn("cannot write {}: {}", Path, E.what());
    }
    auto L = std::make_shared<spdlog::logger>("run", Sinks.begin(), Sinks.end());
    L->set_pattern(Pattern);
    L->set_level(spdlog::level::trace);
    L->flush_on(spdlog::level::trace);
    return L;
}

Timer::Timer(std::string What, spdlog::level::level_enum Lvl, double Slow)
    : Name(std::move(What)), Level(Lvl), SlowMs(Slow), Start(Clock::now())
{
    Ui& U = UiState();
    if (U.Thread != GetCurrentThreadId()) return;
    OnUi = true;
    std::lock_guard<std::mutex> L(U.M);
    U.Open.push_back(this);
}

Timer::~Timer()
{
    const double T = Ms();
    if (OnUi)
    {
        Ui& U = UiState();
        std::lock_guard<std::mutex> L(U.M);
        if (!U.Open.empty() && U.Open.back() == this) U.Open.pop_back();
        U.UserWaited = U.UserWaited || WaitsOnUser;
        if (!WaitsOnUser && T > U.SlowestMs)
        {
            U.SlowestMs = T;
            U.Slowest = Name;
        }
    }
    const bool Slow = !WaitsOnUser && T >= SlowMs;
    const auto Lvl = Slow ? spdlog::level::warn : Level;
    if (!Log().should_log(Lvl)) return;
    Log().log(Lvl, "{}{}{}{} {:.1f} ms{}", Name, Extra.empty() || Extra[0] == ':' ? "" : " ", Extra, Extra.empty() || Extra[0] != ':' ? ":" : ",",
              T, Slow ? " (slow)" : "");
}

double Timer::Ms() const
{
    return MsSince(Start);
}

void FrameBegin()
{
    Ui& U = UiState();
    if (U.Thread == 0)
    {
        U.Thread = GetCurrentThreadId();
        std::thread(WatchUi).detach();
    }
    std::lock_guard<std::mutex> L(U.M);
    U.InFrame = true;
    U.FrameStart = Clock::now();
    U.Slowest.clear();
    U.SlowestMs = 0;
    U.UserWaited = false;
}

void FrameEnd()
{
    Ui& U = UiState();
    std::string Line;
    bool Warn = false;
    {
        std::lock_guard<std::mutex> L(U.M);
        U.InFrame = false;
        const double Ms = MsSince(U.FrameStart);
        if (U.Frozen)
            Line = "UI responding again after " + std::to_string((long long)Ms) + " ms";
        else if (Ms > 200 && !U.UserWaited)
        {
            Line = "UI frame took " + std::to_string((long long)Ms) + " ms; slowest step: " +
                   (U.Slowest.empty() ? "none timed" : U.Slowest + " (" + std::to_string((long long)U.SlowestMs) + " ms)");
            Warn = true;
        }
        U.Frozen = false;
        U.NextReport = 1000;

        // GH_FRAME_STATS=1: a line a second, frames drawn and the slowest (a stall shows as a gap)
        static const bool Stats = GetEnvironmentVariableA("GH_FRAME_STATS", nullptr, 0) > 0;
        static Clock::time_point Since = Clock::now();
        static int Frames = 0;
        static double Slowest = 0;
        if (Stats)
        {
            Frames++;
            Slowest = std::max(Slowest, Ms);
            if (MsSince(Since) >= 1000 && Line.empty())
            {
                Line = "UI frames: " + std::to_string(Frames) + " in " + std::to_string((long long)MsSince(Since)) +
                       " ms, slowest " + std::to_string((long long)Slowest) + " ms";
                Since = Clock::now();
                Frames = 0;
                Slowest = 0;
            }
        }
    }
    if (Line.empty()) return;
    if (Warn) Log().warn(Line);
    else Log().info(Line);
}
}
