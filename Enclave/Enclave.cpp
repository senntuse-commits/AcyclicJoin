#include "Enclave_t.h"
#include "user_types.h"

#include <omp.h>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <utility>
#include <vector>

#include "BottomupSemiJoin.h"
#include "PrimitiveProfile.h"
#include "ObliYan.h"
#include "ParYan.h"
#include "JFYanDown.h"
#include "sgx_profile.h"

using namespace std;

bool g_sgxProfileEnabled = false;
bool g_sgxProfileTimingEnabled = false;

namespace
{
enum JoinMode
{
    ModeOurs = 0,
    ModeParYan = 1,
    ModeObliYan = 2
};

enum ProfileStage
{
    StageRestoreTables = 0,
    StageRestoreMeta = 1,
    StageSetup = 2,
    StageCore1 = 3,
    StageCore2 = 4,
    StageSummarize = 5,
    StageInsideTotal = 6,
    StageDoExactRows = 7,
    StageDoSensitivity = 8,
    StageDoProtectedRows = 9,
    StageOursUpFilter = 10,
    StageOursRootExpand = 11,
    StageJFYanDown = 12,
    StageParYanUpFilter = 13,
    StageParYanDownFilter = 14,
    StageParYanJoin = 15,
    StagePrimitiveSort = 16,
    StagePrimitiveExpand = 17,
    StagePrimitiveCompact = 18,
    StagePrimitiveAggtree = 19,
    StageObliYanUpFilter = 20,
    StageObliYanDownFilter = 21,
    StageObliYanJoin = 22,
    StagePrimitivePhaseBase = 23,
    PrimitiveKindCount = 4,
    PrimitivePhaseCount = 9,
    StageCount = StagePrimitivePhaseBase + PrimitiveKindCount * PrimitivePhaseCount
};

void logLine(const char *s)
{
    ocall_print(s);
}

double nowMs()
{
    double ms = 0.0;
    ocall_now_ms(&ms);
    return ms;
}

void setStage(double *stageMs, int stageCount, int idx, double value)
{
    if (stageMs && idx >= 0 && idx < stageCount)
        stageMs[idx] = value;
}

void setPrimitiveStages(double *stageMs, int stageCount)
{
    setStage(stageMs, stageCount, StagePrimitiveSort, getPrimitiveProfileMs(PrimitiveSort));
    setStage(stageMs, stageCount, StagePrimitiveExpand, getPrimitiveProfileMs(PrimitiveExpand));
    setStage(stageMs, stageCount, StagePrimitiveCompact, getPrimitiveProfileMs(PrimitiveCompact));
    setStage(stageMs, stageCount, StagePrimitiveAggtree, getPrimitiveProfileMs(PrimitiveAggtree));
    for (int phase = 0; phase < PrimitivePhaseCount; ++phase)
    {
        int profilePhase = PrimitivePhaseOursUpFilter + phase;
        for (int kind = 0; kind < PrimitiveKindCount; ++kind)
        {
            int stageIdx = StagePrimitivePhaseBase + phase * PrimitiveKindCount + kind;
            setStage(stageMs, stageCount, stageIdx,
                     getPrimitiveProfileMsForPhase(profilePhase, kind));
        }
    }
}

void configureThreads(int threads)
{
    omp_set_dynamic(0);
    if (threads > 0)
        omp_set_num_threads(threads);
    // JFYanDown uses bounded outer task parallelism plus inner parallel sorts.
    // Enable nested teams explicitly; otherwise SGX OpenMP can serialize the
    // inner sort regions, making 32/64 threads look like one thread.
    omp_set_max_active_levels(2);
    omp_set_nested(1);
}

bool protectedOutputTooLarge(const DOPaddingStats &stats)
{
    constexpr int MaxMaterializedProtectedRows = 50000000;
    return stats.protectedRowsExact > MaxMaterializedProtectedRows;
}

int exactOutputCap(const DOPaddingStats &stats)
{
    if (stats.exactRows <= 0)
        return 1;
    if (stats.exactRows > INT_MAX)
        return INT_MAX;
    return (int)stats.exactRows;
}

void logPaddingMaterializationChoice(const DOPaddingStats &stats, bool materializePadding, int materializedRows)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[DO padding materialization] %s materializedRows=%d protectedRows=%lld exactRows=%lld",
             materializePadding ? "enabled" : "disabled",
             materializedRows, stats.protectedRowsExact, stats.exactRows);
    logLine(buf);
}

sgx_status_t rejectUnavailableRRS(const DOPaddingStats &stats)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "DO RRS sensitivity unavailable for this query shape: exactRows=%lld genericSensitivity=%lld",
             stats.exactRows, stats.sensitivity);
    logLine(buf);
    return SGX_ERROR_UNEXPECTED;
}

sgx_status_t rejectOversizedProtectedOutput(const DOPaddingStats &stats)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "DO protected output too large for enclave materialization: exactRows=%lld sensitivity=%lld protectedRows=%lld paddingRows=%lld",
             stats.exactRows, stats.sensitivity, stats.protectedRowsExact, stats.paddingRowsExact);
    logLine(buf);
    return SGX_ERROR_OUT_OF_MEMORY;
}

sgx_status_t rejectRRSTooExpensive(const DOPaddingStats &stats)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "DO RRS intractable at this (epsilon,delta): the residual enumeration is too large for this many relations. "
             "Raise --do-epsilon (paper uses 4; try 8/16) or pass an explicit -tau. exactRows=%lld",
             stats.exactRows);
    logLine(buf);
    return SGX_ERROR_UNEXPECTED;
}

// Always-on summary of the DO padding decision, printed before the join runs so
// the real (unpadded) join size and the protected (padded) size are visible
// regardless of profiling mode.
void logDOPadding(const DOPaddingStats &stats)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "[DO padding] real=%lld protected=%lld padding=%lld materializedCap=%d (sensitivity=%lld source=%s)",
             stats.exactRows, stats.protectedRowsExact, stats.paddingRowsExact, stats.protectedRows, stats.sensitivity,
             stats.rrsAvailable ? "RRS" : "explicit/none");
    logLine(buf);
}

void logNoPaddingSizing(long long exactRows, int materializedRows)
{
    char buf[192];
    snprintf(buf, sizeof(buf),
             "[DO padding] disabled; exactRows=%lld materializedRows=%d (sensitivity/protectedRows not computed)",
             exactRows, materializedRows);
    logLine(buf);
}

bool restoreTables(const int *tablesFlat,
                   int flatLen,
                   const int *tableOffsets,
                   const int *tableRows,
                   const int *tableCols,
                   int tableCount,
                   vector<Table> &tables)
{
    if (!tablesFlat || !tableOffsets || !tableRows || !tableCols || tableCount <= 0 || flatLen < 0)
        return false;

    tables.clear();
    tables.resize(tableCount);

    for (int t = 0; t < tableCount; ++t)
    {
        int rows = tableRows[t];
        int cols = tableCols[t];
        int offset = tableOffsets[t];
        if (rows < 0 || cols < 0 || offset < 0)
            return false;

        long long cellCount = (long long)rows * (long long)cols;
        if (cellCount < 0 || offset + cellCount > flatLen)
            return false;

        Table table(rows, vector<int>(cols));
        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                table[i][j] = tablesFlat[offset + i * cols + j];
            }
        }
        tables[t] = std::move(table);
    }
    return true;
}

bool restoreVector(const int *src, int count, vector<int> &dst)
{
    if (!src || count <= 0)
        return false;
    dst.assign(src, src + count);
    return true;
}

sgx_status_t copyResult(const Table &res, int *result, int maxSize, int *retRows, int *retCols)
{
    if (!result || !retRows || !retCols || maxSize < 0)
        return SGX_ERROR_INVALID_PARAMETER;

    *retRows = (int)res.size();
    *retCols = res.empty() ? 0 : (int)res[0].size();

    long long required = (long long)(*retRows) * (long long)(*retCols);
    if (required > maxSize)
    {
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "Enclave result buffer too small. required=%lld, max=%d",
                 required, maxSize);
        logLine(buf);
        return SGX_ERROR_OUT_OF_MEMORY;
    }

    for (int i = 0; i < *retRows; ++i)
    {
        if ((int)res[i].size() != *retCols)
            return SGX_ERROR_UNEXPECTED;
        for (int j = 0; j < *retCols; ++j)
            result[i * (*retCols) + j] = res[i][j];
    }

    return SGX_SUCCESS;
}

sgx_status_t summarizeResult(const Table &res, int *retRows, int *retCols)
{
    if (!retRows || !retCols)
        return SGX_ERROR_INVALID_PARAMETER;

    *retRows = (int)res.size();
    *retCols = res.empty() ? 0 : (int)res[0].size();
    return SGX_SUCCESS;
}

sgx_status_t runJoin(int threads,
                     int mode,
                     int *tables_flat,
                     int flat_len,
                     int *table_offsets,
                     int *table_rows,
                     int *table_cols,
                     int table_count,
                     int *parent,
                     int root,
                     int *join_col_parent,
                     int *join_col_child,
                     int *table_keys,
                     int tau_or_out_size,
                     int materialize_padding,
                     double do_epsilon,
                     double do_delta,
                     Table &joinResult)
{
    configureThreads(threads);

    if (table_count <= 0 || root < 0 || root >= table_count)
        return SGX_ERROR_INVALID_PARAMETER;

    vector<Table> tables;
    vector<int> parentVec;
    vector<int> joinParentVec;
    vector<int> joinChildVec;
    vector<int> tableKeysVec;

    if (!restoreTables(tables_flat, flat_len, table_offsets, table_rows, table_cols, table_count, tables) ||
        !restoreVector(parent, table_count, parentVec) ||
        !restoreVector(join_col_parent, table_count, joinParentVec) ||
        !restoreVector(join_col_child, table_count, joinChildVec) ||
        !restoreVector(table_keys, table_count, tableKeysVec))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    joinResult.clear();
    int materializedOutSize = 1;
    if (materialize_padding)
    {
        DOPaddingStats doStats = computeDOPaddingStats(tables, parentVec, root, joinParentVec, joinChildVec,
                                                       tau_or_out_size, do_epsilon, do_delta);
        logDOPadding(doStats);
        if (tau_or_out_size <= 0 && doStats.rrsTooExpensive)
            return rejectRRSTooExpensive(doStats);
        if (tau_or_out_size <= 0 && !doStats.rrsAvailable)
            return rejectUnavailableRRS(doStats);
        materializedOutSize = doStats.protectedRows;
        logPaddingMaterializationChoice(doStats, true, materializedOutSize);
        if (protectedOutputTooLarge(doStats))
            return rejectOversizedProtectedOutput(doStats);
    }
    else
    {
        long long exactRows = computeAcyclicJoinOutputSize(tables, parentVec, root, joinParentVec, joinChildVec);
        if (tau_or_out_size > 0)
            materializedOutSize = tau_or_out_size;
        else if (exactRows > INT_MAX)
            materializedOutSize = INT_MAX;
        else
            materializedOutSize = std::max(1, (int)exactRows);
        logNoPaddingSizing(exactRows, materializedOutSize);
    }
    if (mode == ModeOurs)
    {
        logLine("Starting acyclic join: bottom-up + top-down");
        vector<Table> working = std::move(tables);
        setPrimitiveProfilePhase(PrimitivePhaseOursUpFilter);
        working[root] = bottomUpSemiJoin(working, parentVec, root, joinParentVec, joinChildVec, materializedOutSize);
        setPrimitiveProfilePhase(PrimitivePhaseUnscoped);
        setPrimitiveProfilePhase(PrimitivePhaseJFYanDown);
        if (!working[root].empty())
            joinResult = JFYanDown(working, parentVec, root, joinParentVec, joinChildVec, tableKeysVec);
        setPrimitiveProfilePhase(PrimitivePhaseUnscoped);
        logLine("Acyclic join completed.");
    }
    else if (mode == ModeParYan)
    {
        logLine("Starting acyclic join: ParYanFilter + ParYanJoin");
        vector<Table> working = std::move(tables);
        auto filtered = ParYanFilter(working, parentVec, root, joinParentVec, joinChildVec, tableKeysVec);
        int outSize = std::max(materializedOutSize, (int)std::max<long long>(filtered.second, 1));
        setPrimitiveProfilePhase(PrimitivePhaseParYanJoin);
        joinResult = ParYanJoin(filtered.first, parentVec, root, joinParentVec, joinChildVec, tableKeysVec, outSize);
        setPrimitiveProfilePhase(PrimitivePhaseUnscoped);
        logLine("ParYan acyclic join completed.");
    }
    else if (mode == ModeObliYan)
    {
        logLine("Starting acyclic join: ObliYan");
        joinResult = ObliYan(tables, parentVec, root, joinParentVec, joinChildVec, materializedOutSize);
        logLine("ObliYan completed.");
    }
    else
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    return SGX_SUCCESS;
}

sgx_status_t runJoinProfile(int threads,
                            int mode,
                            int *tables_flat,
                            int flat_len,
                            int *table_offsets,
                            int *table_rows,
                            int *table_cols,
                            int table_count,
                            int *parent,
                            int root,
                            int *join_col_parent,
                            int *join_col_child,
                            int *table_keys,
                            int tau_or_out_size,
                            int materialize_padding,
                            double do_epsilon,
                            double do_delta,
                            Table &joinResult,
                            double *stageMs,
                            int stageCount)
{
    configureThreads(threads);

    if (stageMs)
        for (int i = 0; i < stageCount; ++i)
            stageMs[i] = 0.0;

    double totalStart = nowMs();

    if (table_count <= 0 || root < 0 || root >= table_count)
        return SGX_ERROR_INVALID_PARAMETER;

    vector<Table> tables;
    vector<int> parentVec;
    vector<int> joinParentVec;
    vector<int> joinChildVec;
    vector<int> tableKeysVec;

    double t0 = nowMs();
    if (!restoreTables(tables_flat, flat_len, table_offsets, table_rows, table_cols, table_count, tables))
        return SGX_ERROR_INVALID_PARAMETER;
    double t1 = nowMs();
    setStage(stageMs, stageCount, StageRestoreTables, t1 - t0);

    if (!restoreVector(parent, table_count, parentVec) ||
        !restoreVector(join_col_parent, table_count, joinParentVec) ||
        !restoreVector(join_col_child, table_count, joinChildVec) ||
        !restoreVector(table_keys, table_count, tableKeysVec))
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    double t2 = nowMs();
    setStage(stageMs, stageCount, StageRestoreMeta, t2 - t1);

    joinResult.clear();
    int materializedOutSize = 1;
    if (materialize_padding)
    {
        DOPaddingStats doStats = computeDOPaddingStats(tables, parentVec, root, joinParentVec, joinChildVec,
                                                       tau_or_out_size, do_epsilon, do_delta);
        setStage(stageMs, stageCount, StageDoExactRows, (double)doStats.exactRows);
        setStage(stageMs, stageCount, StageDoSensitivity, (double)doStats.sensitivity);
        setStage(stageMs, stageCount, StageDoProtectedRows, (double)doStats.protectedRowsExact);
        logDOPadding(doStats);
        if (tau_or_out_size <= 0 && doStats.rrsTooExpensive)
            return rejectRRSTooExpensive(doStats);
        if (tau_or_out_size <= 0 && !doStats.rrsAvailable)
            return rejectUnavailableRRS(doStats);
        materializedOutSize = doStats.protectedRows;
        bool profileWithoutPadding = protectedOutputTooLarge(doStats);
        if (profileWithoutPadding)
        {
            char buf[256];
            constexpr int MaxMaterializedJoinOnlyRows = 50000000;
            if (doStats.exactRows > MaxMaterializedJoinOnlyRows)
            {
                snprintf(buf, sizeof(buf),
                         "DO protected output and exact output are too large to materialize; profile returns padding stats only: exactRows=%lld sensitivity=%lld protectedRows=%lld paddingRows=%lld",
                         doStats.exactRows, doStats.sensitivity, doStats.protectedRowsExact, doStats.paddingRowsExact);
                logLine(buf);
                double totalEnd = nowMs();
                setStage(stageMs, stageCount, StageInsideTotal, totalEnd - totalStart);
                return SGX_SUCCESS;
            }
            snprintf(buf, sizeof(buf),
                     "DO protected output too large to materialize; profile runs join-only with exactRows as the materialization cap: exactRows=%lld sensitivity=%lld protectedRows=%lld paddingRows=%lld",
                     doStats.exactRows, doStats.sensitivity, doStats.protectedRowsExact, doStats.paddingRowsExact);
            logLine(buf);
            materializedOutSize = exactOutputCap(doStats);
        }
        logPaddingMaterializationChoice(doStats, !profileWithoutPadding, materializedOutSize);
    }
    else
    {
        long long exactRows = computeAcyclicJoinOutputSize(tables, parentVec, root, joinParentVec, joinChildVec);
        if (tau_or_out_size > 0)
            materializedOutSize = tau_or_out_size;
        else if (exactRows > INT_MAX)
            materializedOutSize = INT_MAX;
        else
            materializedOutSize = std::max(1, (int)exactRows);
        setStage(stageMs, stageCount, StageDoExactRows, (double)exactRows);
        setStage(stageMs, stageCount, StageDoSensitivity, 0.0);
        setStage(stageMs, stageCount, StageDoProtectedRows, (double)materializedOutSize);
        logNoPaddingSizing(exactRows, materializedOutSize);
    }
    double afterPaddingStats = nowMs();
    resetPrimitiveProfile();
    if (mode == ModeOurs)
    {
        logLine("Starting acyclic join: bottom-up + top-down");
        vector<Table> working = std::move(tables);
        double t3 = afterPaddingStats;
        setStage(stageMs, stageCount, StageSetup, 0.0);

        setPrimitiveProfilePhase(PrimitivePhaseOursUpFilter);
        working[root] = bottomUpSemiJoin(working, parentVec, root, joinParentVec, joinChildVec, materializedOutSize);
        setPrimitiveProfilePhase(PrimitivePhaseUnscoped);
        double t4 = nowMs();
        double bottomUpTotal = t4 - t3;
        double rootExpand = getLastBottomUpRootExpandMs();
        if (rootExpand < 0.0 || rootExpand > bottomUpTotal)
            rootExpand = 0.0;
        setStage(stageMs, stageCount, StageCore1, bottomUpTotal);
        setStage(stageMs, stageCount, StageOursUpFilter, bottomUpTotal - rootExpand);
        setStage(stageMs, stageCount, StageOursRootExpand, rootExpand);

        setPrimitiveProfilePhase(PrimitivePhaseJFYanDown);
        if (!working[root].empty())
            joinResult = JFYanDown(working, parentVec, root, joinParentVec, joinChildVec, tableKeysVec);
        setPrimitiveProfilePhase(PrimitivePhaseUnscoped);
        double t5 = nowMs();
        setStage(stageMs, stageCount, StageCore2, t5 - t4);
        setStage(stageMs, stageCount, StageJFYanDown, t5 - t4);
        logLine("Acyclic join completed.");
    }
    else if (mode == ModeParYan)
    {
        logLine("Starting acyclic join: ParYanFilter + ParYanJoin");
        vector<Table> working = std::move(tables);
        double t3 = afterPaddingStats;
        setStage(stageMs, stageCount, StageSetup, 0.0);

        auto filtered = ParYanFilter(working, parentVec, root, joinParentVec, joinChildVec, tableKeysVec);
        double t4 = nowMs();
        setStage(stageMs, stageCount, StageCore1, t4 - t3);
        setStage(stageMs, stageCount, StageParYanUpFilter, getLastParYanUpFilterMs());
        setStage(stageMs, stageCount, StageParYanDownFilter, getLastParYanDownFilterMs());

        int outSize = std::max(materializedOutSize, (int)std::max<long long>(filtered.second, 1));
        setPrimitiveProfilePhase(PrimitivePhaseParYanJoin);
        joinResult = ParYanJoin(filtered.first, parentVec, root, joinParentVec, joinChildVec, tableKeysVec, outSize);
        setPrimitiveProfilePhase(PrimitivePhaseUnscoped);
        double t5 = nowMs();
        setStage(stageMs, stageCount, StageCore2, t5 - t4);
        setStage(stageMs, stageCount, StageParYanJoin, t5 - t4);
        logLine("ParYan acyclic join completed.");
    }
    else if (mode == ModeObliYan)
    {
        logLine("Starting acyclic join: ObliYan");
        double t3 = afterPaddingStats;
        setStage(stageMs, stageCount, StageSetup, 0.0);

        joinResult = ObliYan(tables, parentVec, root, joinParentVec, joinChildVec, materializedOutSize);
        double t4 = nowMs();
        setStage(stageMs, stageCount, StageCore1, t4 - t3);
        setStage(stageMs, stageCount, StageObliYanUpFilter, getLastObliYanUpFilterMs());
        setStage(stageMs, stageCount, StageObliYanDownFilter, getLastObliYanDownFilterMs());
        setStage(stageMs, stageCount, StageObliYanJoin, getLastObliYanJoinMs());
        logLine("ObliYan completed.");
    }
    else
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    double beforeSummary = nowMs();
    (void)beforeSummary;
    double totalEnd = nowMs();
    setPrimitiveStages(stageMs, stageCount);
    setStage(stageMs, stageCount, StageSummarize, totalEnd - beforeSummary);
    setStage(stageMs, stageCount, StageInsideTotal, totalEnd - totalStart);
    return SGX_SUCCESS;
}
}

sgx_status_t empty_ecall(int threads)
{
    configureThreads(threads);
    return SGX_SUCCESS;
}

sgx_status_t AcyclicJoinRun(int threads,
                            int mode,
                            int *tables_flat,
                            int flat_len,
                            int *table_offsets,
                            int *table_rows,
                            int *table_cols,
                            int table_count,
                            int *parent,
                            int root,
                            int *join_col_parent,
                            int *join_col_child,
                            int *table_keys,
                            int tau_or_out_size,
                            int materialize_padding,
                            double do_epsilon,
                            double do_delta,
                            int *result,
                            int max_size,
                            int *ret_rows,
                            int *ret_cols)
{
    setSgxProfileEnabled(false);
    setSgxProfileTimingEnabled(false);
    if (ret_rows)
        *ret_rows = 0;
    if (ret_cols)
        *ret_cols = 0;

    Table joinResult;
    sgx_status_t status = runJoin(threads, mode, tables_flat, flat_len, table_offsets,
                                  table_rows, table_cols, table_count, parent, root,
                                  join_col_parent, join_col_child, table_keys,
                                  tau_or_out_size, materialize_padding, do_epsilon, do_delta, joinResult);
    if (status != SGX_SUCCESS)
        return status;

    return copyResult(joinResult, result, max_size, ret_rows, ret_cols);
}

sgx_status_t AcyclicJoinRunSummary(int threads,
                                   int mode,
                                   int *tables_flat,
                                   int flat_len,
                                   int *table_offsets,
                                   int *table_rows,
                                   int *table_cols,
                                   int table_count,
                                   int *parent,
                                   int root,
                                   int *join_col_parent,
                                   int *join_col_child,
                                   int *table_keys,
                                   int tau_or_out_size,
                                   int materialize_padding,
                                   double do_epsilon,
                                   double do_delta,
                                   int *ret_rows,
                                   int *ret_cols)
{
    setSgxProfileEnabled(false);
    setSgxProfileTimingEnabled(false);
    if (ret_rows)
        *ret_rows = 0;
    if (ret_cols)
        *ret_cols = 0;

    Table joinResult;
    sgx_status_t status = runJoin(threads, mode, tables_flat, flat_len, table_offsets,
                                  table_rows, table_cols, table_count, parent, root,
                                  join_col_parent, join_col_child, table_keys,
                                  tau_or_out_size, materialize_padding, do_epsilon, do_delta, joinResult);
    if (status != SGX_SUCCESS)
        return status;

    return summarizeResult(joinResult, ret_rows, ret_cols);
}

sgx_status_t AcyclicJoinRunProfile(int threads,
                                   int mode,
                                   int *tables_flat,
                                   int flat_len,
                                   int *table_offsets,
                                   int *table_rows,
                                   int *table_cols,
                                   int table_count,
                                   int *parent,
                                   int root,
                                   int *join_col_parent,
                                   int *join_col_child,
                                   int *table_keys,
                                   int tau_or_out_size,
                                   int materialize_padding,
                                   double do_epsilon,
                                   double do_delta,
                                   int *ret_rows,
                                   int *ret_cols,
                                   double *stage_ms,
                                   int stage_count)
{
    setSgxProfileEnabled(true);
    if (ret_rows)
        *ret_rows = 0;
    if (ret_cols)
        *ret_cols = 0;

    Table joinResult;
    sgx_status_t status = runJoinProfile(threads, mode, tables_flat, flat_len, table_offsets,
                                         table_rows, table_cols, table_count, parent, root,
                                         join_col_parent, join_col_child, table_keys,
                                         tau_or_out_size, materialize_padding, do_epsilon, do_delta, joinResult, stage_ms, stage_count);
    if (status != SGX_SUCCESS)
    {
        setSgxProfileEnabled(false);
        setSgxProfileTimingEnabled(false);
        return status;
    }

    double t0 = nowMs();
    status = summarizeResult(joinResult, ret_rows, ret_cols);
    double t1 = nowMs();
    setStage(stage_ms, stage_count, StageSummarize, t1 - t0);
    if (stage_ms && StageInsideTotal < stage_count)
        stage_ms[StageInsideTotal] += (t1 - t0);
    setSgxProfileEnabled(false);
    setSgxProfileTimingEnabled(false);
    return status;
}

sgx_status_t AcyclicJoinRunStageSummary(int threads,
                                        int mode,
                                        int *tables_flat,
                                        int flat_len,
                                        int *table_offsets,
                                        int *table_rows,
                                        int *table_cols,
                                        int table_count,
                                        int *parent,
                                        int root,
                                        int *join_col_parent,
                                        int *join_col_child,
                                        int *table_keys,
                                        int tau_or_out_size,
                                        int materialize_padding,
                                        double do_epsilon,
                                        double do_delta,
                                        int *ret_rows,
                                        int *ret_cols,
                                        double *stage_ms,
                                        int stage_count)
{
    setSgxProfileEnabled(false);
    setSgxProfileTimingEnabled(true);
    if (ret_rows)
        *ret_rows = 0;
    if (ret_cols)
        *ret_cols = 0;

    Table joinResult;
    sgx_status_t status = runJoinProfile(threads, mode, tables_flat, flat_len, table_offsets,
                                         table_rows, table_cols, table_count, parent, root,
                                         join_col_parent, join_col_child, table_keys,
                                         tau_or_out_size, materialize_padding, do_epsilon, do_delta, joinResult, stage_ms, stage_count);
    if (status != SGX_SUCCESS)
    {
        setSgxProfileTimingEnabled(false);
        return status;
    }

    double t0 = nowMs();
    status = summarizeResult(joinResult, ret_rows, ret_cols);
    double t1 = nowMs();
    setStage(stage_ms, stage_count, StageSummarize, t1 - t0);
    if (stage_ms && StageInsideTotal < stage_count)
        stage_ms[StageInsideTotal] += (t1 - t0);
    setSgxProfileTimingEnabled(false);
    return status;
}
