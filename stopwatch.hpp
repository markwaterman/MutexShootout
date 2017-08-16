// Written by Mark Waterman, and placed in the public domain.
// The author hereby disclaims copyright to this source code.

#pragma once
#include <chrono>

namespace mw {

// Stopwatch utility class, start with static StopWatch::start_new().
class StopWatch
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using time_point = std::chrono::time_point<Clock>;
    using duration = std::chrono::duration<double>;

    // Returns a new, started stopwatch.
    inline static StopWatch start_new()
    {
        return StopWatch{Clock::now()};
    }

    inline duration elapsed() const
    {
        return (Clock::now() - start_time_);
    }

    inline double elapsed_seconds() const 
    {
        return elapsed().count();
    }

private:
    time_point start_time_;

    StopWatch(time_point start_time) : start_time_(start_time) {}
};

}