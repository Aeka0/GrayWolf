#pragma once

// DiskGame::Export's run log (<out>/<game>/export.log, also in the session log, AppLog.h) and its
// watchdog: phases, kinds and assets timed, the slowest kept, a stall logged. DiskGame.cpp's own.

#include "DiskGame.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace spdlog { class logger; }

namespace DiskGame
{
    // What a kind did over a stretch of the run: its counts, time, the process's I/O bytes and the
    // image writers' time (ExportWriters::Times, read before and after).
    struct KindSample
    {
        double Seconds = 0;
        size_t Seen = 0, Written = 0, Known = 0, Linked = 0, References = 0, Failed = 0;
        uint64_t Wrote = 0, Read = 0;
        uint64_t ConvertUs = 0, Converted = 0, Unchanged = 0, QueueWaitUs = 0, DrainUs = 0, LinkUs = 0;
        void Add(const KindSample& O);
        std::string Describe() const;
    };

    // Taken before a kind's export, read after it.
    class KindMeter
    {
    public:
        KindMeter(const Totals& T, Kind K);
        KindSample Stop(const Totals& T) const;

    private:
        Kind K;
        std::chrono::steady_clock::time_point Start;
        KindSample Before;
    };

    class RunMonitor
    {
    public:
        using Clock = std::chrono::steady_clock;
        // OnStall gets the watchdog's warning, then "" once the run moves again (its thread).
        RunMonitor(const std::string& LogPath, std::function<void(const std::string&)> OnStall);
        ~RunMonitor();
        RunMonitor(const RunMonitor&) = delete;
        RunMonitor& operator=(const RunMonitor&) = delete;

        void Log(const std::string& Text);
        // What the run is on ("zm_red", "open"): progress, and what a stall names.
        void Where(const std::string& Zone, const std::string& Doing);
        // Context::Step: the asset running ends; Asset (or #Done) starts unless Done == Count.
        void Step(size_t Done, size_t Count, const std::string* Asset);
        void EndAsset();
        void Touch();   // progress with no asset (an index zone)
        // a phase before the run started (the adapter's zone resolving), counted in its total
        void Prior(const std::string& Phase, double Seconds);
        std::string Asset() const;
        double Elapsed() const;

        // for the summary, in the order first met
        void AddTime(const std::string& Phase, double Seconds);
        void AddKind(const std::string& Label, const KindSample& S);
        // the summary, logged and as the report's time_*, rate_* and slowest_* lines
        void Summarize(Totals& T);

    private:
        struct Slow
        {
            double Ms;
            std::string Name, Doing, Zone;
        };
        void EndAssetLocked(Clock::time_point Now, std::vector<std::string>& Lines);
        // the run moved: a stall reported ends here
        void MovedLocked(Clock::time_point Now, std::vector<std::string>& Lines);
        void Flush(const std::vector<std::string>& Lines);
        void Watch();

        const Clock::time_point Started = Clock::now();
        std::function<void(const std::string&)> OnStall;
        std::shared_ptr<spdlog::logger> Out;

        mutable std::mutex M;
        std::string Zone, Doing, Current;
        size_t CurrentIndex = 0;
        bool Running = false, CurrentNamed = false;
        Clock::time_point AssetStart, DoingSince = Clock::now(), LastMove = Clock::now();
        std::vector<Slow> Slowest;   // longest first
        std::vector<std::pair<std::string, double>> Phases;
        std::vector<std::pair<std::string, KindSample>> KindTimes;
        double PriorSeconds = 0;

        // the watchdog: a warning after StallAfter quiet seconds, again at twice the wait
        double StallAfter = 30;
        bool Stop = false, Stalled = false, Dumped = false;
        double NextReport = 0;
        Clock::time_point QuietSince;
        std::condition_variable Wake;
        std::thread Watchdog;
    };
}
