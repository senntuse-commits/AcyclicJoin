#include "../include/ParallelSemiJoin.h"
#include "../include/ParallelBitonicSort.h"
#include "../include/Aggtree.h"
#include "../include/ORcompact.h"
#include "../include/PrimitiveProfile.h"
#include "../include/sgx_profile.h"
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>

namespace
{
struct FlatSemiTable
{
    int rows = 0;
    int cols = 0;
    vector<int> data;

    FlatSemiTable() = default;
    FlatSemiTable(int r, int c) : rows(r), cols(c), data((size_t)r * c, 0) {}

    inline int &at(int r, int c)
    {
        return data[(size_t)r * cols + c];
    }

    inline const int &at(int r, int c) const
    {
        return data[(size_t)r * cols + c];
    }
};

FlatSemiTable sortFlatSemiTableIdx(FlatSemiTable table, const vector<int> &keyColumns, bool direction)
{
    PrimitiveProfileScope primitiveScope(PrimitiveSort);
    int n = table.rows;
    if (n == 0)
        return table;

    int maxVal = 0;
#pragma omp parallel for reduction(max : maxVal) schedule(static)
    for (int i = 0; i < n; ++i)
    {
        for (int col : keyColumns)
        {
            int v = std::abs(table.at(i, col));
            if (v > maxVal)
                maxVal = v;
        }
    }

    __int128_t keyBase = 1;
    while (keyBase <= maxVal)
        keyBase *= 10;
    if (keyBase < 10)
        keyBase = 10;
    __int128_t encBase = 2 * keyBase;

    int len = 1;
    while (len < n)
        len <<= 1;

    vector<__int128_t> keys(len, 0);
    vector<int> rowIdx(len, -1);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i)
    {
        __int128_t key = 0;
        for (int col : keyColumns)
            key = key * encBase + ((__int128_t)table.at(i, col) + keyBase);
        keys[i] = key;
        rowIdx[i] = i;
    }

    int padValue = direction ? INT_MAX : INT_MIN;
    __int128_t padKey = 0;
    for (int k = 0; k < (int)keyColumns.size(); ++k)
    {
        int val = (k == 0) ? padValue : 0;
        padKey = padKey * encBase + ((__int128_t)val + keyBase);
    }
    for (int i = n; i < len; ++i)
        keys[i] = padKey;

#pragma omp parallel
    {
        for (int k = 2; k <= len; k <<= 1)
        {
            for (int j = k >> 1; j > 0; j >>= 1)
            {
                int pairCount = len >> 1;
                int blockSize = j << 1;
#pragma omp for schedule(static)
                for (int pair = 0; pair < pairCount; ++pair)
                {
                    int block = (pair / j) * blockSize;
                    int i = block + (pair % j);
                    int l = i + j;
                    bool asc = ((i & k) == 0) == direction;
                    if ((keys[i] > keys[l]) == asc)
                    {
                        std::swap(keys[i], keys[l]);
                        std::swap(rowIdx[i], rowIdx[l]);
                    }
                }
            }
        }
    }

    FlatSemiTable sorted(n, table.cols);
#pragma omp parallel for schedule(static)
    for (int out = 0; out < n; ++out)
    {
        int src = rowIdx[out];
        if (src >= 0)
        {
            const int *srcRow = table.data.data() + (size_t)src * table.cols;
            int *dstRow = sorted.data.data() + (size_t)out * table.cols;
            for (int k = 0; k < table.cols; ++k)
                dstRow[k] = srcRow[k];
        }
    }
    return sorted;
}
}

// valueCol = -1 -> compute degree, else aggregate value
// R: parent node S: child node
// Semi-join: R ⋉ S = {r ∈ R | ∃s ∈ S, r.key = s.key}
// joinCol_1: join column index in R
// joinCol_2: join column index in S
ParallelSemiJoin::ParallelSemiJoin(const Table &R, int joinCol_1, const Table &S, int joinCol_2, int valueCol)
    : R(R), S(S), joinCol_1(joinCol_1), joinCol_2(joinCol_2), valueCol(valueCol) {}

void ParallelSemiJoin::execute()
{
    double profStart = sgxProfileNowMs();
    // cout << value.empty() << endl;
    int n = (int)R.size() + (int)S.size();
    const int rCols = R.empty() ? 0 : (int)R[0].size();
    const int sCols = S.empty() ? 0 : (int)S[0].size();
    const bool hasValue = (valueCol != -1);

    // T(key, tid, [val], payload..., orig_idx), tid = 1 => R, tid = 0 => S.
    const int T_KEY = 0;
    const int T_TID = 1;
    const int T_VAL = 2;
    const int payloadBase = hasValue ? 3 : 2;
    const int rLogicalCols = payloadBase + (rCols - 1) + 1;
    const int sPayloadCols = sCols - 1 - (hasValue ? 1 : 0);
    const int sLogicalCols = payloadBase + sPayloadCols + 1;
    const int tCols = std::max(rLogicalCols, sLogicalCols);
    const int T_ORIG = tCols - 1;

    FlatSemiTable T(n, tCols);
    vector<int> B(n, 0), Sc(n, 0), value(n, 0);
    // T(key,tid,val,...)
    // ======== process R ========
#pragma omp parallel for
    for (int i = 0; i < (int)R.size(); i++)
    {
        T.at(i, T_KEY) = R[i][joinCol_1];
        T.at(i, T_TID) = 1;
        if (hasValue)
            T.at(i, T_VAL) = 0;

        int out = payloadBase;
        for (int j = 0; j < (int)R[i].size(); j++)
        {
            if (j == joinCol_1)
                continue;
            T.at(i, out++) = R[i][j];
        }
        T.at(i, T_ORIG) = i;
    }

#pragma omp parallel for
    for (int i = 0; i < (int)S.size(); i++)
    {
        int row = (int)R.size() + i;
        T.at(row, T_KEY) = S[i][joinCol_2];
        T.at(row, T_TID) = 0;
        if (hasValue)
            T.at(row, T_VAL) = S[i][valueCol];

        int out = payloadBase;
        for (int j = 0; j < (int)S[i].size(); j++)
        {
            if (j == joinCol_2)
                continue;
            if (hasValue && j == valueCol)
                continue;
            T.at(row, out++) = S[i][j];
        }
        T.at(row, T_ORIG) = INT_MAX;
    }
    double profBuildT = sgxProfileNowMs();

    // T(key,tid,val,...,idx)
    T = sortFlatSemiTableIdx(std::move(T), {T_KEY, T_TID}, true);
    double profSortT = sgxProfileNowMs();

#pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        // T.tid == S
        // Cs[i] = (T[i][1] == 0);
        value[i] = !hasValue ? (T.at(i, T_TID) == 0) : T.at(i, T_VAL);
        if (i > 0)
        {
            B[i] = (T.at(i, T_KEY) == T.at(i - 1, T_KEY));
        }
    }
    double profMark = sgxProfileNowMs();

    Sc = Aggtree(value, B, 0).PrefixTreeRun();
    double profAgg = sgxProfileNowMs();

    // collect results: all R rows kept, dummy rows for S (idx=INT_MAX)
    const int col_size_r = rCols;
    const int dummy_size = col_size_r + 2;
    FlatSemiTable flatResult(n, dummy_size);
#pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        bool isR = (T.at(i, T_TID) == 1);
        bool t = (Sc[i] > 0 && isR); // R row with at least one match

        if (t)
        {
            int idx = payloadBase;
            for (int j = 0; j < col_size_r; j++)
            {
                if (j == joinCol_1)
                    flatResult.at(i, j) = T.at(i, T_KEY); // key
                else
                    flatResult.at(i, j) = T.at(i, idx++);
            }
            flatResult.at(i, col_size_r) = Sc[i];
            flatResult.at(i, col_size_r + 1) = T.at(i, T_ORIG);
        }
        else if (isR)
        {
            // R row with no match: zero all data/degree, but keep real orig_idx
            // so Sorter() does NOT strip this row (it only strips INT_MAX rows)
            flatResult.at(i, col_size_r + 1) = T.at(i, T_ORIG);
        }
        else
        {
            // S row: INT_MAX sentinel so Sorter() strips it after sorting
            flatResult.at(i, col_size_r + 1) = INT_MAX;
        }
    }
    double profBuildResult = sgxProfileNowMs();
    // Sort by original R index (last column) ascending to restore R row order
    flatResult = sortFlatSemiTableIdx(std::move(flatResult), {col_size_r + 1}, true);
    double profSortResult = sgxProfileNowMs();

    result.assign(R.size(), vector<int>(col_size_r + 1));
#pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)R.size(); ++i)
    {
        for (int j = 0; j < col_size_r + 1; ++j)
            result[i][j] = flatResult.at(i, j);
    }
    double profConvert = sgxProfileNowMs();

    if (sgxProfileEnabled())
    {
        char buf[512];
        snprintf(buf, sizeof(buf),
                 "    [ParallelSemiJoin R=%d S=%d valueCol=%d] buildT=%.2f ms sortT=%.2f ms mark=%.2f ms agg=%.2f ms buildResult=%.2f ms sortResult=%.2f ms convert=%.2f ms total=%.2f ms",
                 (int)R.size(), (int)S.size(), valueCol,
                 profBuildT - profStart,
                 profSortT - profBuildT,
                 profMark - profSortT,
                 profAgg - profMark,
                 profBuildResult - profAgg,
                 profSortResult - profBuildResult,
                 profConvert - profSortResult,
                 profConvert - profStart);
        sgxProfilePrint(buf);
    }
}

void ParallelSemiJoin::executeValueOnly()
{
    double profStart = sgxProfileNowMs();
    int n = (int)R.size() + (int)S.size();
    const bool hasValue = (valueCol != -1);

    // T(key, tid, val, orig_idx), tid = 1 => R, tid = 0 => S.
    const int T_KEY = 0;
    const int T_TID = 1;
    const int T_VAL = 2;
    const int T_ORIG = 3;
    FlatSemiTable T(n, 4);
    vector<int> B(n, 0), Sc(n, 0), value(n, 0);

#pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)R.size(); ++i)
    {
        T.at(i, T_KEY) = R[i][joinCol_1];
        T.at(i, T_TID) = 1;
        T.at(i, T_VAL) = 0;
        T.at(i, T_ORIG) = i;
    }

#pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)S.size(); ++i)
    {
        int row = (int)R.size() + i;
        T.at(row, T_KEY) = S[i][joinCol_2];
        T.at(row, T_TID) = 0;
        T.at(row, T_VAL) = hasValue ? S[i][valueCol] : 1;
        T.at(row, T_ORIG) = INT_MAX;
    }
    double profBuildT = sgxProfileNowMs();

    T = sortFlatSemiTableIdx(std::move(T), {T_KEY, T_TID}, true);
    double profSortT = sgxProfileNowMs();

#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i)
    {
        value[i] = (T.at(i, T_TID) == 0) ? T.at(i, T_VAL) : 0;
        if (i > 0)
            B[i] = (T.at(i, T_KEY) == T.at(i - 1, T_KEY));
    }
    double profMark = sgxProfileNowMs();

    Sc = Aggtree(value, B, 0).PrefixTreeRun();
    double profAgg = sgxProfileNowMs();

    resultValues.assign(R.size(), 0);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i)
    {
        if (T.at(i, T_TID) == 1)
            resultValues[T.at(i, T_ORIG)] = Sc[i] > 0 ? Sc[i] : 0;
    }
    double profBuildResult = sgxProfileNowMs();

    if (sgxProfileEnabled())
    {
        char buf[512];
        snprintf(buf, sizeof(buf),
                 "    [ParallelSemiJoinValue R=%d S=%d valueCol=%d] buildT=%.2f ms sortT=%.2f ms mark=%.2f ms agg=%.2f ms buildResult=%.2f ms sortResult=%.2f ms convert=%.2f ms total=%.2f ms",
                 (int)R.size(), (int)S.size(), valueCol,
                 profBuildT - profStart,
                 profSortT - profBuildT,
                 profMark - profSortT,
                 profAgg - profMark,
                 profBuildResult - profAgg,
                 0.0,
                 0.0,
                 profBuildResult - profStart);
        sgxProfilePrint(buf);
    }
}

/* Sort the table */
Table ParallelSemiJoin::sortTable(Table table, vector<int> keyColumns, bool direction)
{
    ParallelBitonicSorter sorter(std::move(table), std::move(keyColumns), direction);
    sorter.Sorter();
    return sorter.getSortedData();
}

vector<vector<int>> ParallelSemiJoin::getResult()
{
    return std::move(result);
}

vector<int> ParallelSemiJoin::getResultValues()
{
    return std::move(resultValues);
}
