#include "../include/ParallelBitonicSort.h"
#include "../include/PrimitiveProfile.h"
#include <omp.h>
#include <climits>
#include <iostream>
#include <algorithm>

#ifdef SGX_ENCLAVE_BUILD
#include "sgx_log.h"
#define cout sgx_cout
#define cerr sgx_cerr
#define endl sgx_endl
#endif

namespace
{
static inline void conditionalSwapKey(__int128_t &a, __int128_t &b, bool cond)
{
    unsigned __int128 mask = (unsigned __int128)0 - (unsigned __int128)cond;
    unsigned __int128 ua = (unsigned __int128)a;
    unsigned __int128 ub = (unsigned __int128)b;
    unsigned __int128 diff = (ua ^ ub) & mask;
    a = (__int128_t)(ua ^ diff);
    b = (__int128_t)(ub ^ diff);
}

static inline void conditionalSwapInt(int &a, int &b, bool cond)
{
    unsigned int mask = 0u - (unsigned int)cond;
    unsigned int ua = (unsigned int)a;
    unsigned int ub = (unsigned int)b;
    unsigned int diff = (ua ^ ub) & mask;
    a = (int)(ua ^ diff);
    b = (int)(ub ^ diff);
}

static void conditionalSwapFlatRow(std::vector<int> &rows, int aBase, int bBase, int cols, bool cond)
{
    for (int i = 0; i < cols; ++i)
        conditionalSwapInt(rows[aBase + i], rows[bBase + i], cond);
}

}

ParallelBitonicSorter::ParallelBitonicSorter(std::vector<std::vector<int>> arr, std::vector<int> keyColumns, bool direction)
    : length((int)arr.size()),
      original_length((int)arr.size()),
      array(std::move(arr)),
      rowWidth(array.empty() ? 0 : (int)array[0].size()),
      keyCols(std::move(keyColumns)),
      dire(direction)
{
    computeLexKeys();
}

void ParallelBitonicSorter::computeLexKeys()
{
    flatRows.resize((size_t)length * rowWidth);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < length; ++i)
    {
        int base = i * rowWidth;
        for (int j = 0; j < rowWidth; ++j)
            flatRows[base + j] = array[i][j];
    }
    array.clear();
    array.shrink_to_fit();

    int maxVal = 0;
#pragma omp parallel for reduction(max : maxVal) schedule(static)
    for (int i = 0; i < length; ++i)
        for (int col : keyCols)
        {
            int v = std::abs(flatRows[i * rowWidth + col]);
            if (v > maxVal) maxVal = v;
        }

    key_base = 1;
    while (key_base <= maxVal) key_base *= 10;
    if (key_base < 10) key_base = 10;
    __int128_t enc_base = 2 * key_base;

    lex_keys.resize(length);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < length; ++i)
    {
        __int128_t key = 0;
        for (int col : keyCols)
            key = key * enc_base + ((__int128_t)flatRows[i * rowWidth + col] + key_base);
        lex_keys[i] = key;
    }
}

void ParallelBitonicSorter::pad_array()
{
    int pow2 = 1;
    while (pow2 < length)
        pow2 <<= 1;

    if (pow2 > length)
    {
        __int128_t enc_base = 2 * key_base;
        __int128_t pad_key = 0;
        for (int k = 0; k < (int)keyCols.size(); ++k)
            pad_key = pad_key * enc_base + (dire ? (enc_base - 1) : 0);

        lex_keys.resize(pow2, pad_key);
        flatRows.resize((size_t)pow2 * rowWidth, 0);
        length = pow2;
    }
}

void ParallelBitonicSorter::Sorter()
{
    PrimitiveProfileScope primitiveScope(PrimitiveSort);
    if (length <= 0 || rowWidth <= 0)
        return;

    pad_array();

    int n = length;
    int teamThreads = 1;

    // Iterative bitonic sort with a single persistent parallel region.
    // Threads are created once; #pragma omp for inside reuses them across
    // all O(log²n) passes, eliminating repeated thread-pool creation overhead.
#pragma omp parallel
    {
#pragma omp single
        {
            teamThreads = omp_get_num_threads();
        }
        for (int k = 2; k <= n; k <<= 1)
        {
            for (int j = k >> 1; j > 0; j >>= 1)
            {
                int pairCount = n >> 1;
                int blockSize = j << 1;
#pragma omp for schedule(static)
                for (int pair = 0; pair < pairCount; pair++)
                {
                    int block = (pair / j) * blockSize;
                    int i = block + (pair % j);
                    int l = i + j;
                    bool asc = ((i & k) == 0) == dire;
                    bool doSwap = (lex_keys[i] > lex_keys[l]) == asc;
                    conditionalSwapKey(lex_keys[i], lex_keys[l], doSwap);
                    conditionalSwapFlatRow(flatRows, i * rowWidth, l * rowWidth, rowWidth, doSwap);
                }
            }
        }
    }

#if defined(BITONIC_SORT_DEBUG) && !defined(SGX_ENCLAVE_BUILD)
    if (n >= 100000)
    {
#pragma omp critical(BitonicDebugPrint)
        {
            printf("    [BitonicSort debug] rows=%d cols=%d keys=%d direction=%s teamThreads=%d maxThreads=%d activeLevel=%d\n",
                   n,
                   rowWidth,
                   (int)keyCols.size(),
                   dire ? "asc" : "desc",
                   teamThreads,
                   omp_get_max_threads(),
                   omp_get_active_level());
        }
    }
#endif

    lex_keys.resize(original_length);
    flatRows.resize((size_t)original_length * rowWidth);
    length = original_length;
}

void ParallelBitonicSorter::printData()
{
#ifdef SGX_ENCLAVE_BUILD
    return;
#else
    for (int i = 0; i < length; ++i)
    {
        for (int j = 0; j < rowWidth; ++j)
        {
            cout << flatRows[i * rowWidth + j] << " ";
        }
        cout << "\n";
    }
#endif
}

std::vector<std::vector<int>> ParallelBitonicSorter::getSortedData()
{
    std::vector<std::vector<int>> sorted;
    sorted.resize(original_length, std::vector<int>(rowWidth));

#pragma omp parallel for schedule(static)
    for (int i = 0; i < original_length; ++i)
    {
        int base = i * rowWidth;
        for (int j = 0; j < rowWidth; ++j)
            sorted[i][j] = flatRows[base + j];
    }
    return sorted;
}
