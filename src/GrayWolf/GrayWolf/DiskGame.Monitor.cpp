#include "stdafx.h"

#include "DiskGame.Monitor.h"

#include "AppLog.h"
#include "ExportWriters.h"

#include <algorithm>
#include <cstdio>

namespace DiskGame
{
namespace
{
    constexpr size_t KeptSlowest = 20;
    constexpr double SlowAssetMs = 2000;

    std::string Fixed(double V, int Digits = 1)
    {
        char B[32];
        snprintf(B, sizeof(B), "%.*f", Digits, V);
        return B;
    }

    std::string Bytes(uint64_t B)
    {
        if (B >= (1ull << 30)) return Fixed((double)B / (1ull << 30), 2) + " GB";
        return Fixed((double)B / (1ull << 20)) + " MB";
    }

    std::string Key(std::string S)
    {
        std::replace(S.begin(), S.end(), ' ', '_');
        return S;
    }

    void Io(uint64_t& Wrote, uint64_t& Read)
    {
        IO_COUNTERS C{};
        if (!GetProcessIoCounters(GetCurrentProcess(), &C)) return;
        Wrote = C.WriteTransferCount;
        Read = C.ReadTransferCount;
    }

    KindSample Snapshot(const Totals& T, Kind K)
    {
        KindSample S;
        if (const auto It = T.Kinds.find(K); It != T.Kinds.end())
        {
            S.Seen = It->second.Seen;
            S.Written = It->second.Written;
            S.Known = It->second.Known;
            S.Linked = It->second.Linked;
            S.References = It->second.References;
        }
        S.Failed = T.Failures.size();
        Io(S.Wrote, S.Read);
        const auto& W = ExportWriters::Times();
        S.ConvertUs = W.ConvertUs;
        S.Converted = W.Converted;
        S.Unchanged = W.Unchanged;
        S.QueueWaitUs = W.QueueWaitUs;
        S.DrainUs = W.DrainUs;
        S.LinkUs = W.LinkUs;
        return S;
    }
}

void KindSample::Add(const KindSample& O)
{
    Seconds += O.Seconds;
    Seen += O.Seen; Written += O.Written; Known += O.Known; Linked += O.Linked;
    References += O.References; Failed += O.Failed;
    Wrote += O.Wrote; Read += O.Read;
    ConvertUs += O.ConvertUs; Converted += O.Converted; Unchanged += O.Unchanged; QueueWaitUs += O.QueueWaitUs;
    DrainUs += O.DrainUs; LinkUs += O.LinkUs;
}

std::string KindSample::Describe() const
{
    std::string S = std::to_string(Seen) + " seen, " + std::to_string(Written) + " written";
    if (Linked) S += " (" + std::to_string(Linked) + " linked)";
    if (Known) S += ", " + std::to_string(Known) + " known";
    if (References) S += ", " + std::to_string(References) + " references";
    if (Failed) S += ", " + std::to_string(Failed) + " failures";
    S += " in " + Fixed(Seconds) + " s";
    if (Seconds > 0 && Written) S += " (" + Fixed(Written / Seconds) + "/s)";
    S += "; wrote " + Bytes(Wrote);
    if (Seconds > 0) S += " (" + Fixed(Wrote / 1048576.0 / Seconds) + " MB/s)";
    S += ", read " + Bytes(Read);
    if (Converted)
        S += "; image writers: " + std::to_string(Converted) + " files" +
             (Unchanged ? " (" + std::to_string(Unchanged) + " unchanged, not rewritten)" : std::string()) + " in " +
             Fixed(ConvertUs / 1e6) + " s of pool time (" +
             std::to_string(ExportWriters::Times().Workers.load()) + " threads), reader waited " + Fixed(QueueWaitUs / 1e6) +
             " s for the pool, drain " + Fixed(DrainUs / 1e6) + " s, links " + Fixed(LinkUs / 1e6) + " s";
    return S;
}

KindMeter::KindMeter(const Totals& T, Kind Which) : K(Which), Start(std::chrono::steady_clock::now()), Before(Snapshot(T, Which)) {}

KindSample KindMeter::Stop(const Totals& T) const
{
    KindSample A = Snapshot(T, K);
    A.Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
    A.Seen -= Before.Seen; A.Written -= Before.Written; A.Known -= Before.Known; A.Linked -= Before.Linked;
    A.References -= Before.References; A.Failed -= Before.Failed;
    A.Wrote -= Before.Wrote; A.Read -= Before.Read;
    A.ConvertUs -= Before.ConvertUs; A.Converted -= Before.Converted; A.Unchanged -= Before.Unchanged;
    A.QueueWaitUs -= Before.QueueWaitUs;
    A.DrainUs -= Before.DrainUs; A.LinkUs -= Before.LinkUs;
    return A;
}

RunMonitor::RunMonitor(const std::string& LogPath, std::function<void(const std::string&)> Stall)
    : OnStall(std::move(Stall)), Out(AppLog::Tee(LogPath))
{
    char Env[32] = {};
    if (GetEnvironmentVariableA("GH_EXPORT_STALL_SECONDS", Env, sizeof(Env))) StallAfter = atof(Env);
    NextReport = StallAfter;
    if (StallAfter > 0) Watchdog = std::thread([this] { Watch(); });
}

RunMonitor::~RunMonitor()
{
    {
        std::lock_guard<std::mutex> L(M);
        Stop = true;
    }
    Wake.notify_all();
    if (Watchdog.joinable()) Watchdog.join();
}

double RunMonitor::Elapsed() const
{
    return std::chrono::duration<double>(Clock::now() - Started).count();
}

void RunMonitor::Log(const std::string& Text)
{
    Out->info("+{:.1f} s  {}", Elapsed(), Text);
}

void RunMonitor::EndAssetLocked(Clock::time_point Now, std::vector<std::string>& Lines)
{
    if (!Running) return;
    Running = false;
    const double Ms = std::chrono::duration<double, std::milli>(Now - AssetStart).count();
    if (Slowest.size() >= KeptSlowest && Ms <= Slowest.back().Ms && Ms < SlowAssetMs) return;
    const std::string Name = CurrentNamed ? Current : "#" + std::to_string(CurrentIndex);
    if (Ms >= SlowAssetMs) Lines.push_back("slow asset: " + Doing + " " + Name + " in " + Zone + ": " + Fixed(Ms / 1000, 2) + " s");
    if (Slowest.size() >= KeptSlowest && Ms <= Slowest.back().Ms) return;
    const auto At = std::find_if(Slowest.begin(), Slowest.end(), [Ms](const Slow& S) { return S.Ms < Ms; });
    Slowest.insert(At, { Ms, Name, Doing, Zone });
    if (Slowest.size() > KeptSlowest) Slowest.pop_back();
}

void RunMonitor::MovedLocked(Clock::time_point Now, std::vector<std::string>& Lines)
{
    LastMove = Now;
    if (!Stalled) return;
    Stalled = false;
    NextReport = StallAfter;
    Lines.push_back("resumed after " + Fixed(std::chrono::duration<double>(Now - QuietSince).count()) + " s (" + Zone + ", " + Doing + ")");
}

void RunMonitor::Flush(const std::vector<std::string>& Lines)
{
    for (const auto& Line : Lines)
    {
        Log(Line);
        if (OnStall && Line.rfind("resumed", 0) == 0) OnStall(std::string());
    }
}

void RunMonitor::Step(size_t Done, size_t Count, const std::string* Asset)
{
    const auto Now = Clock::now();
    std::vector<std::string> Lines;
    {
        std::lock_guard<std::mutex> L(M);
        EndAssetLocked(Now, Lines);
        if (Done < Count)
        {
            Running = true;
            AssetStart = Now;
            CurrentNamed = Asset != nullptr;
            if (Asset) Current.assign(*Asset);
            else CurrentIndex = Done;
        }
        MovedLocked(Now, Lines);
    }
    if (!Lines.empty()) Flush(Lines);
}

void RunMonitor::EndAsset()
{
    std::vector<std::string> Lines;
    {
        std::lock_guard<std::mutex> L(M);
        EndAssetLocked(Clock::now(), Lines);
    }
    Flush(Lines);
}

void RunMonitor::Where(const std::string& ZoneName, const std::string& What)
{
    std::vector<std::string> Lines;
    {
        std::lock_guard<std::mutex> L(M);
        const auto Now = Clock::now();
        EndAssetLocked(Now, Lines);
        MovedLocked(Now, Lines);
        Zone = ZoneName;
        Doing = What;
        DoingSince = Now;
    }
    Flush(Lines);
}

void RunMonitor::Touch()
{
    std::vector<std::string> Lines;
    {
        std::lock_guard<std::mutex> L(M);
        MovedLocked(Clock::now(), Lines);
    }
    Flush(Lines);
}

void RunMonitor::Prior(const std::string& Phase, double Seconds)
{
    AddTime(Phase, Seconds);
    std::lock_guard<std::mutex> L(M);
    PriorSeconds += Seconds;
}

std::string RunMonitor::Asset() const
{
    std::lock_guard<std::mutex> L(M);
    if (!Running) return std::string();
    return CurrentNamed ? Current : "#" + std::to_string(CurrentIndex);
}

void RunMonitor::AddTime(const std::string& Phase, double Seconds)
{
    std::lock_guard<std::mutex> L(M);
    for (auto& [N, S] : Phases)
        if (N == Phase) { S += Seconds; return; }
    Phases.emplace_back(Phase, Seconds);
}

void RunMonitor::AddKind(const std::string& Label, const KindSample& Sample)
{
    std::lock_guard<std::mutex> L(M);
    for (auto& [N, S] : KindTimes)
        if (N == Label) { S.Add(Sample); return; }
    KindTimes.emplace_back(Label, Sample);
}

void RunMonitor::Summarize(Totals& T)
{
    const double Total = Elapsed() + PriorSeconds;
    std::vector<std::string> Lines{ "summary: " + Fixed(Total) + " s in all" };
    auto& Tm = T.Timing;
    Tm.emplace_back("time_total_ms", std::to_string((long long)(Total * 1000)));
    {
        std::lock_guard<std::mutex> L(M);
        for (const auto& [N, S] : Phases)
        {
            Lines.push_back("  phase " + N + ": " + Fixed(S) + " s (" + Fixed(Total > 0 ? 100 * S / Total : 0, 0) + "%)");
            Tm.emplace_back("time_" + Key(N) + "_ms", std::to_string((long long)(S * 1000)));
        }
        for (const auto& [N, S] : KindTimes)
        {
            Lines.push_back("  kind " + N + ": " + S.Describe());
            const std::string K = Key(N);
            Tm.emplace_back("time_" + K + "_ms", std::to_string((long long)(S.Seconds * 1000)));
            if (S.Seconds <= 0) continue;
            Tm.emplace_back("rate_" + K + "_per_s", Fixed(S.Written / S.Seconds));
            Tm.emplace_back("written_mb_" + K, Fixed(S.Wrote / 1048576.0));
            Tm.emplace_back("rate_" + K + "_mb_per_s", Fixed(S.Wrote / 1048576.0 / S.Seconds));
            if (S.Converted)
            {
                Tm.emplace_back("time_" + K + "_convert_ms", std::to_string(S.ConvertUs / 1000));
                if (S.Unchanged) Tm.emplace_back("unchanged_" + K, std::to_string(S.Unchanged));
                Tm.emplace_back("time_" + K + "_pool_wait_ms", std::to_string(S.QueueWaitUs / 1000));
                Tm.emplace_back("time_" + K + "_drain_ms", std::to_string(S.DrainUs / 1000));
            }
        }
        if (!Slowest.empty()) Lines.push_back("  slowest assets:");
        for (size_t I = 0; I < Slowest.size(); I++)
        {
            const Slow& S = Slowest[I];
            const std::string Text = Fixed(S.Ms, 0) + " ms " + S.Doing + " " + S.Zone + " " + S.Name;
            Lines.push_back("    " + std::to_string(I + 1) + ". " + Text);
            char K[16];
            snprintf(K, sizeof(K), "slowest_%02zu", I + 1);
            Tm.emplace_back(K, Text);
        }
    }
    for (const auto& Line : Lines) Log(Line);
}

void RunMonitor::Watch()
{
    std::unique_lock<std::mutex> L(M);
    while (!Stop)
    {
        Wake.wait_for(L, std::chrono::seconds(1));
        if (Stop) break;
        const auto Now = Clock::now();
        const double Quiet = std::chrono::duration<double>(Now - LastMove).count();
        if (Quiet < NextReport) continue;
        if (!Stalled) QuietSince = LastMove;
        Stalled = true;
        NextReport *= 2;
        std::string Line = "no progress for " + Fixed(Quiet, 0) + " s: zone " + Zone + ", " + Doing;
        if (Running)
            Line += ", asset " + (CurrentNamed ? Current : "#" + std::to_string(CurrentIndex)) + " (started " +
                    Fixed(std::chrono::duration<double>(Now - AssetStart).count(), 0) + " s ago)";
        else
            Line += " (for " + Fixed(std::chrono::duration<double>(Now - DoingSince).count(), 0) + " s)";
        const bool Dump = !Dumped && GetEnvironmentVariableA("GH_EXPORT_STALLDUMP", nullptr, 0) != 0;
        Dumped = Dumped || Dump;
        L.unlock();
        Out->warn("+{:.1f} s  {}", Elapsed(), Line);
        if (OnStall) OnStall(Line);
        if (Dump)
        {
            char Path[MAX_PATH] = {};
            GetEnvironmentVariableA("GH_EXPORT_STALLDUMP", Path, MAX_PATH);
            Log(std::string(ExportWriters::WriteMinidump(Path) ? "minidump written to " : "minidump failed: ") + Path);
        }
        L.lock();
    }
}
}
