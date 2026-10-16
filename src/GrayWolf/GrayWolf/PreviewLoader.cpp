#include "stdafx.h"

#include "PreviewLoader.h"
#include "AppLog.h"

#include <objbase.h>   // CoInitializeEx: WIC decodes textures on the worker

PreviewLoader::PreviewLoader(size_t CacheSize) : CacheSize(CacheSize)
{
    Worker = std::thread([this] { Run(); });
}

PreviewLoader::~PreviewLoader()
{
    {
        std::lock_guard<std::mutex> Lk(Lock);
        Stop = true;
        Wanted++;   // a running job sees itself cancelled
    }
    Wake.notify_all();
    Worker.join();
}

bool PreviewLoader::Request(const std::string& Key, Job Build, PreviewGpuMesh& Out)
{
    std::lock_guard<std::mutex> Lk(Lock);
    const uint64_t Gen = ++Wanted;
    HasDone = false;
    for (auto It = Cache.begin(); It != Cache.end(); ++It)
    {
        if (It->Key != Key) continue;
        Cache.splice(Cache.begin(), Cache, It);
        Out = Cache.front().Mesh;
        Delivered = Gen;
        PendingKey.clear(); Pending = nullptr;
        return true;
    }
    PendingKey = Key; Pending = std::move(Build); PendingGen = Gen;
    Wake.notify_all();
    return false;
}

void PreviewLoader::Forget()
{
    std::lock_guard<std::mutex> Lk(Lock);
    Delivered = ++Wanted;
    HasDone = false;
    PendingKey.clear(); Pending = nullptr;
}

bool PreviewLoader::Poll(PreviewGpuMesh& Out, double& Ms)
{
    std::lock_guard<std::mutex> Lk(Lock);
    if (!HasDone || DoneGen != Wanted) return false;
    Out = std::move(Done);
    Done = PreviewGpuMesh{};
    Ms = DoneMs;
    HasDone = false;
    Delivered = DoneGen;
    return true;
}

bool PreviewLoader::Busy() const
{
    std::lock_guard<std::mutex> Lk(Lock);
    return Delivered != Wanted;
}

void PreviewLoader::Remember(const std::string& Key, const PreviewGpuMesh& Mesh)
{
    for (auto It = Cache.begin(); It != Cache.end(); ++It)
        if (It->Key == Key) { Cache.erase(It); break; }
    Cache.push_front({ Key, Mesh });
    while (Cache.size() > CacheSize) Cache.pop_back();
}

void PreviewLoader::Run()
{
    const HRESULT Com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    for (;;)
    {
        std::string Key; Job Build; uint64_t Gen = 0;
        {
            std::unique_lock<std::mutex> Lk(Lock);
            Wake.wait(Lk, [&] { return Stop || Pending; });
            if (Stop) break;
            Key = std::move(PendingKey); Build = std::move(Pending); Gen = PendingGen;
            PendingKey.clear(); Pending = nullptr;
        }

        AppLog::Timer T("preview load " + Key, spdlog::level::info, 2000);
        const auto Cancelled = [this, Gen] { return Wanted.load() != Gen; };
        PreviewGpuMesh Mesh = Build(Cancelled);
        const bool WasCancelled = Cancelled();
        T.Detail(WasCancelled ? "(superseded)" : Mesh.Valid ? "" : "(failed)");

        std::lock_guard<std::mutex> Lk(Lock);
        // a superseded job that still finished is kept: going back to it is then instant
        if (Mesh.Valid) Remember(Key, Mesh);
        if (Gen == Wanted.load())
        {
            Done = std::move(Mesh); DoneGen = Gen; DoneMs = T.Ms(); HasDone = true;
        }
    }
    if (SUCCEEDED(Com)) CoUninitialize();
}
