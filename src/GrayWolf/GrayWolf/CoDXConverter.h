#pragma once

#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

// A class that handles parallel operations
class CoDXConverter
{
public:

    // Spawns a new converter, then waits for it to finish execution before continuing
    CoDXConverter(const std::function<void(void)> Job, uint32_t DegreeOfParallelism)
    {
        std::vector<std::thread> Workers;
        
        for (uint32_t i = 0; i < DegreeOfParallelism; i++)
        {
            Workers.emplace_back(Job);
        }

        for (auto& Worker : Workers)
        {
            try
            {
                Worker.join();
            }
            catch (...)
            {
                // Nothing, thread already exited
            }
        }
    }
};