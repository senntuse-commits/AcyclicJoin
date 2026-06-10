#pragma once

#ifdef SGX_ENCLAVE_BUILD
#include "Enclave_t.h"

extern bool g_sgxProfileEnabled;
extern bool g_sgxProfileTimingEnabled;

inline bool sgxProfileEnabled()
{
    return g_sgxProfileEnabled;
}

inline void setSgxProfileEnabled(bool enabled)
{
    g_sgxProfileEnabled = enabled;
    g_sgxProfileTimingEnabled = enabled;
}

inline void setSgxProfileTimingEnabled(bool enabled)
{
    g_sgxProfileTimingEnabled = enabled;
}

inline double sgxProfileNowMs()
{
    if (!g_sgxProfileTimingEnabled)
        return 0.0;
    double outMs = 0.0;
    ocall_now_ms(&outMs);
    return outMs;
}

inline void sgxProfilePrint(const char *s)
{
    if (g_sgxProfileEnabled)
        ocall_print(s);
}
#else
inline bool sgxProfileEnabled() { return false; }
inline void setSgxProfileEnabled(bool) {}
inline void setSgxProfileTimingEnabled(bool) {}
inline double sgxProfileNowMs() { return 0.0; }
inline void sgxProfilePrint(const char *) {}
#endif
