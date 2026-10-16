#pragma once

#include "ModelPreviewGPU.h"   // PreviewGpuMesh

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <string>
#include <thread>

// Builds model previews (read, parse, textures, GPU resources through the free-threaded device) on
// one worker thread so a selection never waits on it. The latest request wins: an older one still
// queued is dropped, one running is told to stop. Recent results are kept, keyed, for going back.
class PreviewLoader
{
public:
    // Job builds the mesh; Cancelled turns true once a newer request supersedes it.
    using Job = std::function<PreviewGpuMesh(const std::function<bool()>& Cancelled)>;

    explicit PreviewLoader(size_t CacheSize = 6);
    ~PreviewLoader();
    PreviewLoader(const PreviewLoader&) = delete;
    PreviewLoader& operator=(const PreviewLoader&) = delete;

    // Makes Key the wanted preview. True when it was cached: Out holds it and nothing runs.
    bool Request(const std::string& Key, Job Build, PreviewGpuMesh& Out);
    // Nothing is wanted any more (another kind of asset was selected).
    void Forget();
    // The wanted preview once it is built (true once), and how long it took.
    bool Poll(PreviewGpuMesh& Out, double& Ms);
    // A wanted preview is still being built.
    bool Busy() const;

private:
    struct Entry { std::string Key; PreviewGpuMesh Mesh; };
    void Run();
    void Remember(const std::string& Key, const PreviewGpuMesh& Mesh);   // under Lock

    mutable std::mutex Lock;
    std::condition_variable Wake;
    std::thread Worker;
    bool Stop = false;

    std::atomic<uint64_t> Wanted{ 0 };   // generation of the latest request
    uint64_t Delivered = 0;       // the generation the caller has
    std::string PendingKey;       // queued, not started (empty: none)
    Job Pending;
    uint64_t PendingGen = 0;

    bool HasDone = false;         // Done holds the result of generation DoneGen
    uint64_t DoneGen = 0;
    PreviewGpuMesh Done;
    double DoneMs = 0;

    std::list<Entry> Cache;       // most recent first
    size_t CacheSize;
};
