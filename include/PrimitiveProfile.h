#pragma once

#include "../include/sgx_profile.h"

#ifndef SGX_ENCLAVE_BUILD
#include <chrono>
#endif

enum PrimitiveProfileKind
{
    PrimitiveSort = 0,
    PrimitiveExpand = 1,
    PrimitiveCompact = 2,
    PrimitiveAggtree = 3,
    PrimitiveProfileCount = 4
};

enum PrimitiveProfilePhase
{
    PrimitivePhaseUnscoped = 0,
    PrimitivePhaseOursUpFilter = 1,
    PrimitivePhaseOursRootExpand = 2,
    PrimitivePhaseOursTopDown = 3,
    PrimitivePhaseOblivUpFilter = 4,
    PrimitivePhaseOblivDownFilter = 5,
    PrimitivePhaseOblivJoin = 6,
    PrimitivePhaseRelaxedUpFilter = 7,
    PrimitivePhaseRelaxedDownFilter = 8,
    PrimitivePhaseRelaxedJoin = 9,
    PrimitiveProfilePhaseCount = 10
};

void resetPrimitiveProfile();
void setPrimitiveProfilePhase(int phase);
int getPrimitiveProfilePhase();
void addPrimitiveProfileMs(int kind, double ms);
void addPrimitiveProfileMsForPhase(int phase, int kind, double ms);
double getPrimitiveProfileMs(int kind);
double getPrimitiveProfileMsForPhase(int phase, int kind);

class PrimitiveProfileScope
{
public:
    explicit PrimitiveProfileScope(int kind)
        : kind_(kind), phase_(getPrimitiveProfilePhase())
    {
#ifdef SGX_ENCLAVE_BUILD
        startMs_ = sgxProfileNowMs();
#else
        start_ = std::chrono::high_resolution_clock::now();
#endif
    }

    ~PrimitiveProfileScope()
    {
#ifdef SGX_ENCLAVE_BUILD
        double endMs = sgxProfileNowMs();
        if (endMs > 0.0 && startMs_ > 0.0)
            addPrimitiveProfileMsForPhase(phase_, kind_, endMs - startMs_);
#else
        auto end = std::chrono::high_resolution_clock::now();
        addPrimitiveProfileMsForPhase(phase_, kind_,
                                      std::chrono::duration<double, std::milli>(end - start_).count());
#endif
    }

private:
    int kind_;
    int phase_;
#ifdef SGX_ENCLAVE_BUILD
    double startMs_ = 0.0;
#else
    std::chrono::high_resolution_clock::time_point start_;
#endif
};
