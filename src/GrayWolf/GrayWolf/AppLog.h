#pragma once

// AppLog: the session log (spdlog "GrayWolf", async, GrayWolfLog.txt beside the exe), with
// time, level and thread per line; GH_LOG_LEVEL=debug|info|warn, info by default.

// Timer times a step; a UI frame over 200 ms logs its slowest step, one frozen for 1 s its open steps.

#include <chrono>
#include <memory>
#include <string>

#include <spdlog/spdlog.h>

namespace AppLog
{
    void Init();
    spdlog::logger& Log();
    std::string SessionPath();
    bool Verbose();
    void SetVerbose(bool On);

    // A synchronous logger writing to Path (a run's export.log) and to the session log.
    std::shared_ptr<spdlog::logger> Tee(const std::string& Path);

    // "<what>: N ms" when it ends, at Level, or as a warning past SlowMs. Two clock reads when
    // the line is not logged.
    class Timer
    {
    public:
        explicit Timer(std::string What, spdlog::level::level_enum Level = spdlog::level::debug, double SlowMs = 500);
        ~Timer();
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        double Ms() const;
        void Detail(std::string More) { Extra = std::move(More); }
        // the step waits on the user (a folder dialog): the UI frame is not stalled
        void UserWait() { WaitsOnUser = true; }
        const std::string& What() const { return Name; }
        bool WaitingOnUser() const { return WaitsOnUser; }
        std::chrono::steady_clock::time_point Started() const { return Start; }

    private:
        std::string Name, Extra;
        spdlog::level::level_enum Level;
        double SlowMs;
        bool WaitsOnUser = false, OnUi = false;
        std::chrono::steady_clock::time_point Start;
    };

    // The UI thread's frame, from after its message pump to after Present.
    void FrameBegin();
    void FrameEnd();
}
