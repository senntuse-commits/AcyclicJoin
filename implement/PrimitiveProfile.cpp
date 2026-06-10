#include "../include/PrimitiveProfile.h"
#include <omp.h>

namespace
{
double g_primitiveProfileMs[PrimitiveProfilePhaseCount][PrimitiveProfileCount] = {};
int g_primitiveProfilePhase = PrimitivePhaseUnscoped;
}

void resetPrimitiveProfile()
{
    for (int p = 0; p < PrimitiveProfilePhaseCount; ++p)
        for (int i = 0; i < PrimitiveProfileCount; ++i)
            g_primitiveProfileMs[p][i] = 0.0;
    g_primitiveProfilePhase = PrimitivePhaseUnscoped;
}

void setPrimitiveProfilePhase(int phase)
{
    if (phase < 0 || phase >= PrimitiveProfilePhaseCount)
        phase = PrimitivePhaseUnscoped;
#pragma omp atomic write
    g_primitiveProfilePhase = phase;
}

int getPrimitiveProfilePhase()
{
    int phase = PrimitivePhaseUnscoped;
#pragma omp atomic read
    phase = g_primitiveProfilePhase;
    return phase;
}

void addPrimitiveProfileMs(int kind, double ms)
{
    addPrimitiveProfileMsForPhase(getPrimitiveProfilePhase(), kind, ms);
}

void addPrimitiveProfileMsForPhase(int phase, int kind, double ms)
{
    if (kind < 0 || kind >= PrimitiveProfileCount || ms <= 0.0)
        return;
    if (phase < 0 || phase >= PrimitiveProfilePhaseCount)
        phase = PrimitivePhaseUnscoped;
#pragma omp atomic
    g_primitiveProfileMs[phase][kind] += ms;
}

double getPrimitiveProfileMs(int kind)
{
    if (kind < 0 || kind >= PrimitiveProfileCount)
        return 0.0;
    double total = 0.0;
    for (int phase = 0; phase < PrimitiveProfilePhaseCount; ++phase)
        total += g_primitiveProfileMs[phase][kind];
    return total;
}

double getPrimitiveProfileMsForPhase(int phase, int kind)
{
    if (phase < 0 || phase >= PrimitiveProfilePhaseCount ||
        kind < 0 || kind >= PrimitiveProfileCount)
        return 0.0;
    return g_primitiveProfileMs[phase][kind];
}
