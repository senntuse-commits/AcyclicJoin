#pragma once

#ifdef SGX_ENCLAVE_BUILD

namespace std
{
namespace chrono
{
struct milli
{
};

struct SgxNoClock
{
    struct duration
    {
        using rep = long long;
        using period = milli;

        static duration zero()
        {
            return duration();
        }
    };

    using rep = duration::rep;
    using period = duration::period;

    struct time_point
    {
        explicit time_point(duration = duration())
        {
        }
    };

    static constexpr bool is_steady = false;

    static time_point now()
    {
        return time_point();
    }
};
}
}

using SgxNoClock = std::chrono::SgxNoClock;
#define high_resolution_clock SgxNoClock

#endif
