#include "../include/TRA.h"
#include "../include/ParallelSemiJoin.h"
#include "../include/ObliViatorJoin.h"
#include "../include/PrimitiveProfile.h"
#include "../include/sgx_profile.h"
#ifndef SGX_ENCLAVE_BUILD
#include <chrono>
#endif

#ifdef SGX_ENCLAVE_BUILD
#include "sgx_log.h"
#define cout sgx_cout
#define cerr sgx_cerr
#define endl sgx_endl
#endif

#ifndef SGX_ENCLAVE_BUILD
using namespace std::chrono;
#endif

static double g_lastObliViatorJoinParallelMs = 0.0;
static double g_lastTwoPhaseFilterParallelMs = 0.0;
static double g_lastTwoPhaseFilterUpMs = 0.0;
static double g_lastTwoPhaseFilterMaterializeMs = 0.0;
static double g_lastTwoPhaseFilterDownMs = 0.0;

double getLastObliViatorJoinParallelMs()
{
    return g_lastObliViatorJoinParallelMs;
}

double getLastTwoPhaseFilterParallelMs()
{
    return g_lastTwoPhaseFilterParallelMs;
}

double getLastTwoPhaseFilterUpMs()
{
    return g_lastTwoPhaseFilterUpMs;
}

double getLastTwoPhaseFilterMaterializeMs()
{
    return g_lastTwoPhaseFilterMaterializeMs;
}

double getLastTwoPhaseFilterDownMs()
{
    return g_lastTwoPhaseFilterDownMs;
}

pair<vector<Table>, long long> TwoPhaseFilter(vector<Table> &tables, const vector<int> &parent, int root,
                                              const vector<int> &joinColInParent, const vector<int> &joinColInChild, const vector<int> &tableKeys)
{
    g_lastTwoPhaseFilterParallelMs = 0.0;
    g_lastTwoPhaseFilterUpMs = 0.0;
    g_lastTwoPhaseFilterMaterializeMs = 0.0;
    g_lastTwoPhaseFilterDownMs = 0.0;
    int n = tables.size();

    // Build children list from parent array
    vector<vector<int>> children(n);
    for (int i = 0; i < n; i++)
        if (parent[i] != -1)
            children[parent[i]].push_back(i);

    // BFS from root, grouped by depth. This supports subtree-level parallelism:
    // bottom-up runs deeper levels first, top-down runs shallower levels first.
    vector<int> order;
    vector<vector<int>> levels;
    vector<int> depth(n, -1);
    queue<int> q;
    q.push(root);
    depth[root] = 0;
    while (!q.empty())
    {
        int cur = q.front();
        q.pop();
        if ((int)levels.size() <= depth[cur])
            levels.resize(depth[cur] + 1);
        levels[depth[cur]].push_back(cur);
        order.push_back(cur);
        for (int c : children[cur])
        {
            depth[c] = depth[cur] + 1;
            q.push(c);
        }
    }

    long long total_delta = 0;
#ifndef SGX_ENCLAVE_BUILD
    vector<double> bottomUpNodeMs(n, 0.0), topDownNodeMs(n, 0.0);
#endif

    auto bottomUpNode = [&](int p, bool parallelChildren)
    {
        if (children[p].empty())
            return;

#ifndef SGX_ENCLAVE_BUILD
        auto nodeStart = high_resolution_clock::now();
#endif
        int k = (int)children[p].size();
        vector<vector<int>> semiJoinValues(k);

        if (parallelChildren)
        {
#pragma omp parallel for schedule(static)
            for (int i = 0; i < k; i++)
            {
                int c = children[p][i];
                int valueCol = children[c].empty() ? -1 : (int)tables[c][0].size() - 1;
                ParallelSemiJoin semiJoin(tables[p], joinColInParent[c], tables[c], joinColInChild[c], valueCol);
                semiJoin.executeValueOnly();
                semiJoinValues[i] = semiJoin.getResultValues();
            }
        }
        else
        {
            for (int i = 0; i < k; i++)
            {
                int c = children[p][i];
                int valueCol = children[c].empty() ? -1 : (int)tables[c][0].size() - 1;
                ParallelSemiJoin semiJoin(tables[p], joinColInParent[c], tables[c], joinColInChild[c], valueCol);
                semiJoin.executeValueOnly();
                semiJoinValues[i] = semiJoin.getResultValues();
            }
        }

#pragma omp parallel for schedule(static)
        for (int i = 0; i < (int)tables[p].size(); i++)
        {
            int oldSize = tables[p][i].size();
            tables[p][i].resize(oldSize + k + 1);

            int product = 1;

            for (int j = 0; j < k; j++)
            {
                int val = semiJoinValues[j][i];
                product *= val;
                tables[p][i][oldSize + j] = val;
            }

            tables[p][i][oldSize + k] = product;
        }
#ifndef SGX_ENCLAVE_BUILD
        bottomUpNodeMs[p] = duration<double, milli>(high_resolution_clock::now() - nodeStart).count();
#endif
    };

    setPrimitiveProfilePhase(PrimitivePhaseOblivUpFilter);
    double sgxBottomUpStart = sgxProfileNowMs();
    // 1、Bottom-up filter: independent subtrees at the same depth run in parallel.
    for (int d = (int)levels.size() - 1; d >= 0; --d)
    {
        vector<int> internalNodes;
        internalNodes.reserve(levels[d].size());
        for (int p : levels[d])
            if (!children[p].empty())
                internalNodes.push_back(p);
        if (internalNodes.empty())
            continue;

        if ((int)internalNodes.size() == 1)
        {
            bottomUpNode(internalNodes[0], false);
        }
        else
        {
#pragma omp parallel for schedule(static)
            for (int i = 0; i < (int)internalNodes.size(); ++i)
                bottomUpNode(internalNodes[i], false);
        }
    }
    double sgxBottomUpEnd = sgxProfileNowMs();
    setPrimitiveProfilePhase(PrimitivePhaseUnscoped);

#ifndef SGX_ENCLAVE_BUILD
    vector<double> bottomUpCritical(n, 0.0);
    for (int d = (int)levels.size() - 1; d >= 0; --d)
    {
        for (int u : levels[d])
        {
            double slowestChild = 0.0;
            for (int c : children[u])
                slowestChild = max(slowestChild, bottomUpCritical[c]);
            bottomUpCritical[u] = slowestChild + bottomUpNodeMs[u];
        }
    }

    auto materializeStart = high_resolution_clock::now();
#endif
    double sgxMaterializeStart = sgxProfileNowMs();
    vector<Table> semiJoinTables(n);
    for (int oi = (int)order.size() - 1; oi >= 0; --oi)
    {
        int p = order[oi];
        if (children[p].empty())
        {
            semiJoinTables[p] = tables[p];
        }
        else
        {
            int keyCount = tableKeys[p];
            int sz = (int)tables[p].size();
            semiJoinTables[p].resize(sz);

            if (p == root)
            {
#pragma omp parallel for reduction(+ : total_delta)
                for (int i = 0; i < sz; i++)
                {
                    long long product = tables[p][i].back();
                    if (product == 0)
                        semiJoinTables[p][i].assign(keyCount, 0);
                    else
                    {
                        total_delta += product;
                        semiJoinTables[p][i].assign(tables[p][i].begin(),
                                                    tables[p][i].begin() + keyCount);
                    }
                }
            }
            else
            {
#pragma omp parallel for
                for (int i = 0; i < sz; i++)
                {
                    if (tables[p][i].back() == 0)
                        semiJoinTables[p][i].assign(keyCount, 0);
                    else
                        semiJoinTables[p][i].assign(tables[p][i].begin(),
                                                    tables[p][i].begin() + keyCount);
                }
            }
        }
    }
#ifndef SGX_ENCLAVE_BUILD
    double materializeMs = duration<double, milli>(high_resolution_clock::now() - materializeStart).count();
#endif
    double sgxMaterializeEnd = sgxProfileNowMs();

    // 2. Top-down filter: use parent to filter child tuples
    setPrimitiveProfilePhase(PrimitivePhaseOblivDownFilter);
    double sgxTopDownStart = sgxProfileNowMs();
    vector<Table> topDownTables(n);
    topDownTables[root] = std::move(semiJoinTables[root]);

    auto topDownNode = [&](int p, bool parallelChildren)
    {
        if (children[p].empty())
            return;

#ifndef SGX_ENCLAVE_BUILD
        auto nodeStart = high_resolution_clock::now();
#endif
        int k = (int)children[p].size();
        vector<vector<int>> tdValues(k);

        if (parallelChildren)
        {
            // Semi-join each child against current parent: child ⋉ parent
#pragma omp parallel for schedule(static)
            for (int i = 0; i < k; i++)
            {
                int c = children[p][i];
                ParallelSemiJoin semiJoin(semiJoinTables[c], joinColInChild[c],
                                          topDownTables[p], joinColInParent[c], -1);
                semiJoin.executeValueOnly();
                tdValues[i] = semiJoin.getResultValues();
            }
        }
        else
        {
            for (int i = 0; i < k; i++)
            {
                int c = children[p][i];
                ParallelSemiJoin semiJoin(semiJoinTables[c], joinColInChild[c],
                                          topDownTables[p], joinColInParent[c], -1);
                semiJoin.executeValueOnly();
                tdValues[i] = semiJoin.getResultValues();
            }
        }

        // Build topDownTables[c]: keep row if Sc > 0, else zero out
        for (int i = 0; i < k; i++)
        {
            int c = children[p][i];
            int sz = (int)semiJoinTables[c].size();
            int keyCount = tableKeys[c];
            topDownTables[c].resize(sz);

#pragma omp parallel for schedule(static)
            for (int j = 0; j < sz; j++)
            {
                if (tdValues[i][j] == 0)
                    topDownTables[c][j].assign(keyCount, 0);
                else
                    topDownTables[c][j] = semiJoinTables[c][j];
            }
            Table().swap(semiJoinTables[c]);
        }
#ifndef SGX_ENCLAVE_BUILD
        topDownNodeMs[p] = duration<double, milli>(high_resolution_clock::now() - nodeStart).count();
#endif
    };

    // Process top-down. Nodes at the same depth have already received their
    // parent-filtered tables, and they write to disjoint child subtrees.
    for (int d = 0; d < (int)levels.size(); ++d)
    {
        vector<int> internalNodes;
        internalNodes.reserve(levels[d].size());
        for (int p : levels[d])
            if (!children[p].empty())
                internalNodes.push_back(p);
        if (internalNodes.empty())
            continue;

        if ((int)internalNodes.size() == 1)
        {
            topDownNode(internalNodes[0], false);
        }
        else
        {
#pragma omp parallel for schedule(static)
            for (int i = 0; i < (int)internalNodes.size(); ++i)
                topDownNode(internalNodes[i], false);
        }
    }
    double sgxTopDownEnd = sgxProfileNowMs();
    setPrimitiveProfilePhase(PrimitivePhaseUnscoped);

    g_lastTwoPhaseFilterUpMs = sgxBottomUpEnd - sgxBottomUpStart;
    g_lastTwoPhaseFilterMaterializeMs = sgxMaterializeEnd - sgxMaterializeStart;
    g_lastTwoPhaseFilterDownMs = sgxTopDownEnd - sgxTopDownStart;
    g_lastTwoPhaseFilterParallelMs = g_lastTwoPhaseFilterUpMs +
                                     g_lastTwoPhaseFilterMaterializeMs +
                                     g_lastTwoPhaseFilterDownMs;

#ifndef SGX_ENCLAVE_BUILD
    vector<double> topDownCritical(n, 0.0);
    topDownCritical[root] = topDownNodeMs[root];
    for (int u : order)
    {
        for (int c : children[u])
            topDownCritical[c] = topDownCritical[u] + topDownNodeMs[c];
    }
    double topDownPathMs = 0.0;
    for (double v : topDownCritical)
        topDownPathMs = max(topDownPathMs, v);

    g_lastTwoPhaseFilterUpMs = bottomUpCritical[root];
    g_lastTwoPhaseFilterMaterializeMs = materializeMs;
    g_lastTwoPhaseFilterDownMs = topDownPathMs;
    g_lastTwoPhaseFilterParallelMs = g_lastTwoPhaseFilterUpMs +
                                     g_lastTwoPhaseFilterMaterializeMs +
                                     g_lastTwoPhaseFilterDownMs;
    printf("  [TwoPhaseFilter parallel-estimate] bottomUpCritical=%.2f ms  materialize=%.2f ms  topDownCritical=%.2f ms  total=%.2f ms\n",
           g_lastTwoPhaseFilterUpMs, g_lastTwoPhaseFilterMaterializeMs,
           g_lastTwoPhaseFilterDownMs, g_lastTwoPhaseFilterParallelMs);

    cout << "TRA: sum of delta at root = " << total_delta << endl;
#endif
    return {topDownTables, total_delta};
}

Table JoinByObliViator(vector<Table> &tables, const vector<int> &parent, int root,
                       const vector<int> &joinColInParent, const vector<int> &joinColInChild, const vector<int> &tableKeys, const int out_size)
{
    g_lastObliViatorJoinParallelMs = 0.0;
    double sgxJoinStart = sgxProfileNowMs();
    int n = tables.size();

    // Build children list
    vector<vector<int>> children(n);
    for (int i = 0; i < n; i++)
        if (parent[i] != -1)
            children[parent[i]].push_back(i);

    // BFS order: root first, then children level by level. The level grouping
    // lets independent subtrees join in parallel during the bottom-up pass.
    vector<int> order;
    vector<vector<int>> levels;
    vector<int> depth(n, -1);
    queue<int> q;
    q.push(root);
    depth[root] = 0;
    while (!q.empty())
    {
        int cur = q.front(); q.pop();
        if ((int)levels.size() <= depth[cur])
            levels.resize(depth[cur] + 1);
        levels[depth[cur]].push_back(cur);
        order.push_back(cur);
        for (int c : children[cur])
        {
            depth[c] = depth[cur] + 1;
            q.push(c);
        }
    }

    // Keep the old output column order: root-first BFS order.
    // The joins below are bottom-up, so subtree columns may be appended in a
    // different order; offsets let us restore the original layout at the end.
    vector<int> outputOrder = order;
    int outputCols = 0;
    for (int u : outputOrder)
        outputCols += tableKeys[u];

    vector<Table> partial(n);
    vector<int> partialCols(n, 0);
    vector<vector<int>> colOffset(n, vector<int>(n, -1));
    for (int u = 0; u < n; ++u)
    {
        partial[u] = tables[u];
        partialCols[u] = tableKeys[u];
        colOffset[u][u] = 0;
    }

#ifndef SGX_ENCLAVE_BUILD
    vector<double> augByNode(n, 0.0), fillByNode(n, 0.0), expByNode(n, 0.0), alnByNode(n, 0.0), resByNode(n, 0.0);
    vector<double> joinNodeMs(n, 0.0);
#endif

    auto joinNode = [&](int p)
    {
#ifndef SGX_ENCLAVE_BUILD
        auto nodeStart = high_resolution_clock::now();
        double localAug = 0.0, localFill = 0.0, localExp = 0.0, localAln = 0.0, localRes = 0.0;
#endif

        for (int c : children[p])
        {
            // Map both join columns to their positions in the two accumulated
            // subtree results.
            int joinCol1 = colOffset[p][p] + joinColInParent[c];
            int joinCol2 = colOffset[c][c] + joinColInChild[c];
            int keys2 = partialCols[c];
            int leftInputRows = (int)partial[p].size();
            int rightInputRows = (int)partial[c].size();

            ObliViatorJoin joiner(std::move(partial[p]), joinCol1, partialCols[p],
                                  std::move(partial[c]), joinCol2, keys2, out_size);
            joiner.join();
            partial[p] = joiner.getResult();
            Table().swap(partial[c]);
            if (sgxProfileEnabled())
            {
                char buf[320];
                snprintf(buf, sizeof(buf),
                         "  [OblivJoin edge p=%d c=%d] inputRows=%d,%d outSize=%d resultRows=%d cols=%d augment=%.2f fillDim=%.2f expand=%.2f align=%.2f result=%.2f",
                         p, c, leftInputRows, rightInputRows, out_size,
                         (int)partial[p].size(), partial[p].empty() ? 0 : (int)partial[p][0].size(),
                         joiner.ms_augment, joiner.ms_fillDim, joiner.ms_expand, joiner.ms_align, joiner.ms_result);
                sgxProfilePrint(buf);
            }

#ifndef SGX_ENCLAVE_BUILD
#pragma omp critical(OblivJoinPrint)
            {
            printf("  [OblivJoin p=%d->c=%d] augment=%.1f ms  fillDim=%.1f ms  expand=%.1f ms  align=%.1f ms  result=%.1f ms\n",
                   p, c, joiner.ms_augment, joiner.ms_fillDim, joiner.ms_expand, joiner.ms_align, joiner.ms_result);
            }
            localAug += joiner.ms_augment;
            localFill += joiner.ms_fillDim;
            localExp += joiner.ms_expand;
            localAln += joiner.ms_align;
            localRes += joiner.ms_result;
#endif

            int childBase = partialCols[p];
            for (int v = 0; v < n; ++v)
            {
                if (colOffset[c][v] >= 0)
                    colOffset[p][v] = childBase + colOffset[c][v];
            }
            partialCols[p] += keys2;
            partialCols[c] = 0;
        }
#ifndef SGX_ENCLAVE_BUILD
        augByNode[p] = localAug;
        fillByNode[p] = localFill;
        expByNode[p] = localExp;
        alnByNode[p] = localAln;
        resByNode[p] = localRes;
        joinNodeMs[p] = duration<double, milli>(high_resolution_clock::now() - nodeStart).count();
#endif
    };

    // Join children into their parent in bottom-up order. Nodes on the same
    // depth own disjoint subtrees, so they can run concurrently. A single
    // parent still joins its children sequentially because each join updates
    // partial[p] and the next child depends on that accumulated result.
    for (int d = (int)levels.size() - 1; d >= 0; --d)
    {
        vector<int> internalNodes;
        internalNodes.reserve(levels[d].size());
        for (int p : levels[d])
            if (!children[p].empty())   
                internalNodes.push_back(p);
        if (internalNodes.empty())
            continue;

        // If there is only one node at this depth, run it outside an outer
        // OpenMP region so the parallel sort/expand inside ObliViatorJoin can
        // use all threads. With nested OpenMP disabled, wrapping one node in a
        // parallel-for serializes most inner parallel work.
        if ((int)internalNodes.size() == 1)
        {
            joinNode(internalNodes[0]);
            continue;
        }

        // Independent subtree roots at this level can really run in parallel.
#pragma omp parallel for
        for (int i = 0; i < (int)internalNodes.size(); ++i)
            joinNode(internalNodes[i]);
    }

#ifndef SGX_ENCLAVE_BUILD
    double _tot_aug = 0, _tot_fill = 0, _tot_exp = 0, _tot_aln = 0, _tot_res = 0;
    for (int u = 0; u < n; ++u)
    {
        _tot_aug += augByNode[u];
        _tot_fill += fillByNode[u];
        _tot_exp += expByNode[u];
        _tot_aln += alnByNode[u];
        _tot_res += resByNode[u];
    }

    printf("  [OblivJoin TOTAL] augment=%.1f ms  fillDim=%.1f ms  expand=%.1f ms  align=%.1f ms  result=%.1f ms\n",
           _tot_aug, _tot_fill, _tot_exp, _tot_aln, _tot_res);

    vector<double> criticalMs(n, 0.0);
    for (int d = (int)levels.size() - 1; d >= 0; --d)
    {
        for (int u : levels[d])
        {
            double slowestChild = 0.0;
            for (int c : children[u])
                slowestChild = max(slowestChild, criticalMs[c]);
            criticalMs[u] = slowestChild + joinNodeMs[u];
        }
    }

    auto finalStart = high_resolution_clock::now();
#endif
    Table result(partial[root].size(), vector<int>(outputCols, 0));
#pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)partial[root].size(); ++i)
    {
        int dst = 0;
        for (int u : outputOrder)
        {
            int src = colOffset[root][u];
            for (int k = 0; k < tableKeys[u]; ++k)
                result[i][dst + k] = partial[root][i][src + k];
            dst += tableKeys[u];
        }
    }
#ifndef SGX_ENCLAVE_BUILD
    double finalMaterializeMs = duration<double, milli>(high_resolution_clock::now() - finalStart).count();
    g_lastObliViatorJoinParallelMs = criticalMs[root] + finalMaterializeMs;
    printf("  [OblivJoin parallel] criticalJoin=%.1f ms  finalMaterialize=%.1f ms  total=%.1f ms\n",
           criticalMs[root], finalMaterializeMs, g_lastObliViatorJoinParallelMs);
#else
    g_lastObliViatorJoinParallelMs = sgxProfileNowMs() - sgxJoinStart;
#endif

    return result;
}
