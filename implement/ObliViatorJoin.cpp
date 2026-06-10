#include "../include/ObliViatorJoin.h"
#include "../include/ParallelBitonicSort.h"
#include "../include/Aggtree.h"
#include "../include/DupAggtree.h"
#include "../include/ObliViatorExpand.h"
#include "../include/ORcompact.h"
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

/* This file implements the ObliViatorJoin(USNEIX 2025) methods
 * Table schemas:
 *   - T1: {k1,k2,k3,k4,...}
 *   - T2: {k1,k2,k3,k4,...}
 *   - join condition: T1.joinCol_1 = T2.joinCol_2
 *   - result: T1.{k1,k2,...} + T2.{k1,k2,...}
 */

void ObliViatorJoin::join()
{
#ifndef SGX_ENCLAVE_BUILD
    using namespace std::chrono;
#endif
    result.clear();
    // 1、 Augment tables
    Table mergedTable;
#ifndef SGX_ENCLAVE_BUILD
    auto _j0 = high_resolution_clock::now();
#else
    double _j0 = sgxProfileNowMs();
#endif
    augmentTables(mergedTable);
    Table().swap(T1);
    Table().swap(T2);
#ifndef SGX_ENCLAVE_BUILD
    auto _j1 = high_resolution_clock::now();
#else
    double _j1 = sgxProfileNowMs();
#endif
    // cout << "debug 1" << endl;
    // printTable(mergedTable, {"h", "tid", "other keys..."});

    // 2、 Fill dimension
    fillDimension(mergedTable);
#ifndef SGX_ENCLAVE_BUILD
    auto _j2 = high_resolution_clock::now();
#else
    double _j2 = sgxProfileNowMs();
#endif
    // cout << "debug 2" << endl;
    // printTable(mergedTable, {"h", "d", "tid", "other keys...", "a1", "a2"});

    // 3、 Expand table
    Table ExpandT1, ExpandT2;
    fetchExpand(mergedTable, ExpandT1, ExpandT2);
    Table().swap(mergedTable);
#ifndef SGX_ENCLAVE_BUILD
    auto _j3 = high_resolution_clock::now();
#else
    double _j3 = sgxProfileNowMs();
#endif
    if(ExpandT1.empty() || ExpandT2.empty()){
        return;
    }
    // // [DEBUG] Check sizes after expansion
    // cerr << "[DBG3] ExpandT1.size=" << ExpandT1.size()
    //      << " ExpandT2.size=" << ExpandT2.size()
    //      << " out_size=" << out_size << "\n";
    // if (ExpandT1.size() != ExpandT2.size())
    //     cerr << "[ERROR] ExpandT1/T2 size mismatch after expand!\n";

    // 4、Align table
    ExpandT1 = sortTable(std::move(ExpandT1), {0, static_cast<int>(ExpandT1[0].size()) - 1}, true); // Sort by h, idx
    alignTable(ExpandT2);
#ifndef SGX_ENCLAVE_BUILD
    auto _j4 = high_resolution_clock::now();
#else
    double _j4 = sgxProfileNowMs();
#endif
    // // [DEBUG] Check key pairing after sort/align
    // {
    //     int key_mismatch = 0, nonzero_pairs = 0;
    //     int typeA = 0, typeB = 0, typeC = 0;
    //     int n4 = (int)ExpandT1.size();
    //     int first_mm = -1;
    //     for (int i = 0; i < n4; i++)
    //     {
    //         if (ExpandT1[i][0] != 0) nonzero_pairs++;
    //         if (ExpandT1[i][0] != ExpandT2[i][0])
    //         {
    //             if (first_mm < 0) first_mm = i;
    //             key_mismatch++;
    //             if      (ExpandT1[i][0] == 0) typeA++;  // dummy T1, real T2
    //             else if (ExpandT2[i][0] == 0) typeB++;  // real T1, dummy T2
    //             else                           typeC++;  // real-real but different key
    //         }
    //     }
    //     cerr << "[DBG4] After sort/align: key_mismatch=" << key_mismatch
    //          << " (A:T1=0,T2!=0=" << typeA
    //          << " B:T1!=0,T2=0=" << typeB
    //          << " C:both!=0,diff=" << typeC << ")"
    //          << " nonzero_T1_rows=" << nonzero_pairs << "/" << n4 << "\n";
    //     if (first_mm >= 0)
    //     {
    //         cerr << "[DBG4] First mismatch idx=" << first_mm
    //              << " T1[0]=" << ExpandT1[first_mm][0]
    //              << " T2[0]=" << ExpandT2[first_mm][0] << "\n";
    //     }
    //     // Print first 15 h-values of each table to see ordering
    //     cerr << "[DBG4] T1 h[0..14]: ";
    //     for (int i = 0; i < min(15, n4); i++) cerr << ExpandT1[i][0] << " ";
    //     cerr << "\n[DBG4] T2 h[0..14]: ";
    //     for (int i = 0; i < min(15, n4); i++) cerr << ExpandT2[i][0] << " ";
    //     cerr << "\n[DBG4] T1 h[last 10]: ";
    //     for (int i = max(0, n4-10); i < n4; i++) cerr << ExpandT1[i][0] << " ";
    //     cerr << "\n[DBG4] T2 h[last 10]: ";
    //     for (int i = max(0, n4-10); i < n4; i++) cerr << ExpandT2[i][0] << " ";
    //     cerr << "\n";
    // }

    // 5、 Get result
    int n = ExpandT1.size();
    result.resize(n, vector<int>(keys_1 + keys_2));
#pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        // Reconstruct T1 original row: put joinkey back to joinCol_1, fill others in order
        int k = 2;
        result[i][joinCol_1] = ExpandT1[i][0];
        for (int j = 0; j < keys_1; j++)
        {
            if (j != joinCol_1)
                result[i][j] = ExpandT1[i][k++];
        }

        // Reconstruct T2 original row: put joinkey back to joinCol_2, fill others in order
        k = 2;
        result[i][keys_1 + joinCol_2] = ExpandT2[i][0];
        for (int j = 0; j < keys_2; j++)
        {
            if (j != joinCol_2)
                result[i][keys_1 + j] = ExpandT2[i][k++];
        }
    }
    Table().swap(ExpandT1);
    Table().swap(ExpandT2);
    // // [DEBUG] Count non-zero result rows
    // {
    //     int nonzero = 0;
    //     for (auto& r : result)
    //         for (int v : r) { if (v != 0) { nonzero++; break; } }
    //     cerr << "[DBG5] Result: total=" << result.size()
    //          << " nonzero=" << nonzero << " out_size=" << out_size << "\n";
    // }
    // // Pad with zero rows if result size < out_size
    // if ((int)result.size() < out_size)
    //     result.resize(out_size, vector<int>(keys_1 + keys_2, 0));
#ifndef SGX_ENCLAVE_BUILD
    auto _j5 = high_resolution_clock::now();
    ms_augment  = duration<double, milli>(_j1 - _j0).count();
    ms_fillDim  = duration<double, milli>(_j2 - _j1).count();
    ms_expand   = duration<double, milli>(_j3 - _j2).count();
    ms_align    = duration<double, milli>(_j4 - _j3).count();
    ms_result   = duration<double, milli>(_j5 - _j4).count();
#else
    double _j5 = sgxProfileNowMs();
    ms_augment = _j1 - _j0;
    ms_fillDim = _j2 - _j1;
    ms_expand = _j3 - _j2;
    ms_align = _j4 - _j3;
    ms_result = _j5 - _j4;
#endif
}

void ObliViatorJoin::augmentTables(Table &T)
{
    int n1 = T1.size();
    int n2 = T2.size();
    int augWidth = std::max(keys_1, keys_2) + 1; // joinkey + tid + max(other keys)
    T.resize(n1 + n2);
    // T(joinkey, tid, otherkeys...)
#pragma omp parallel for
    for (int i = 0; i < n1; i++)
    {
        // T1: [joinCol_1, tid=1, other keys of T1 in order]
        T[i].assign(augWidth, 0);
        T[i][0] = T1[i][joinCol_1];
        T[i][1] = 1;
        int out = 2;
        for (int j = 0; j < keys_1; j++)
        {
            if (j != joinCol_1)
                T[i][out++] = T1[i][j];
        }
    }
#pragma omp parallel for
    for (int i = 0; i < n2; i++)
    {
        // T2: [joinCol_2, tid=2, other keys of T2 in order]
        T[n1 + i].assign(augWidth, 0);
        T[n1 + i][0] = T2[i][joinCol_2];
        T[n1 + i][1] = 2;
        int out = 2;
        for (int j = 0; j < keys_2; j++)
        {
            if (j != joinCol_2)
                T[n1 + i][out++] = T2[i][j];
        }
    }
    T = sortTable(std::move(T), {0, 1, 2}, true); // Sort by joinkey, tid, first_other_key
}

void ObliViatorJoin::fillDimension(Table &T)
{
    // T(h, d, tid)
    int n = T.size();
    vector<int> B(n, 0);
    vector<int> M1(n, 0);
    vector<int> M2(n, 0);

#pragma omp parallel for
    for (int i = 1; i < n; i++)
    {
        B[i] = (T[i][0] == T[i - 1][0]);
    }

#pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        M1[i] = (T[i][1] == 1);
        M2[i] = (T[i][1] == 2);
    }

    M1 = Aggtree(M1, B, 1).FullRun();
    M2 = Aggtree(M2, B, 1).FullRun();

#pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        T[i].reserve(T[i].size() + 2);
        T[i].push_back(M1[i]); // a1
        T[i].push_back(M2[i]); // a2
    }
}

void ObliViatorJoin::fetchExpand(const Table &T, Table &ExpandT1, Table &ExpandT2)
{
    vector<int> g1;
    vector<int> g2;
    int cnt = 0;
    int idx = 1;
    int n = T.size();
    ExpandT1.clear();
    ExpandT2.clear();

    // T(h, tid, otherkeys, a1, a2)
    // joinkey=0 ====> dummy tuple, no need to expand
    const int t1Width = keys_1 + 4; // T1 adds a unique idx after a1/a2.
    const int t2Width = keys_2 + 3;
    Table T1Candidates(n, vector<int>(t1Width, 0));
    Table T2Candidates(n, vector<int>(t2Width, 0));
    vector<int> M1(n, 0), M2(n, 0);

#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        if (T[i][0] != 0 && T[i][1] == 1 && T[i].back() != 0) // T1
        {
            for (int k = 0; k < keys_1 + 1; ++k)
                T1Candidates[i][k] = T[i][k];
            T1Candidates[i][keys_1 + 1] = T[i][(int)T[i].size() - 2];
            T1Candidates[i][keys_1 + 2] = T[i][(int)T[i].size() - 1];
            M1[i] = 1;
        }
        if (T[i][0] != 0 && T[i][1] == 2 && T[i][T[i].size() - 2] != 0) // T2
        {
            for (int k = 0; k < keys_2 + 1; ++k)
                T2Candidates[i][k] = T[i][k];
            T2Candidates[i][keys_2 + 1] = T[i][(int)T[i].size() - 2];
            T2Candidates[i][keys_2 + 2] = T[i][(int)T[i].size() - 1];
            M2[i] = 1;
        }
    }

    // Paper lines 14-15: compact the two candidate streams by table id.
    ObliCompact().ORCompact(T1Candidates, M1, 0, (int)T1Candidates.size());
    ObliCompact().ORCompact(T2Candidates, M2, 0, (int)T2Candidates.size());

    int t1Count = 0;
    int t2Count = 0;
    for (int flag : M1)
        t1Count += flag;
    for (int flag : M2)
        t2Count += flag;

    ExpandT1.reserve(t1Count + 1);
    ExpandT2.reserve(t2Count + 1);
    g1.reserve(t1Count + 1);
    g2.reserve(t2Count + 1);

    for (int i = 0; i < t1Count; ++i)
    {
        T1Candidates[i].back() = idx++; // unique idx for T1 sorting.
        int weight = T1Candidates[i][t1Width - 2];
        ExpandT1.push_back(std::move(T1Candidates[i]));
        g1.push_back(weight);
        cnt += weight;
    }
    for (int i = 0; i < t2Count; ++i)
    {
        int weight = T2Candidates[i][t2Width - 2];
        ExpandT2.push_back(std::move(T2Candidates[i]));
        g2.push_back(weight);
    }

    if (cnt < out_size)
    {
        int to_add = out_size - cnt;
        // T1 dummy: keys_1 zeros, tid=1, a1=to_add, a2=to_add
        vector<int> dummy1(keys_1 + 4, 0);
        dummy1[1] = 1;
        dummy1[keys_1 + 1] = 1;
        dummy1[keys_1 + 2] = to_add;
        dummy1[keys_1 + 3] = idx++; // unique idx for T1
        ExpandT1.push_back(dummy1);
        g1.push_back(to_add);
        // T2 dummy: keys_2 zeros, tid=2, a1=1, a2=to_add
        // a1=1 keeps oid = C[i] (0..to_add-1), preventing oid*base from
        // overflowing the lex key and mis-sorting dummy rows after real rows.
        vector<int> dummy2(keys_2 + 3, 0);
        dummy2[1] = 2;
        dummy2[keys_2 + 1] = 1;       // a1=1 (not to_add)
        dummy2[keys_2 + 2] = to_add;
        ExpandT2.push_back(dummy2);
        g2.push_back(to_add);
    }

    if (ExpandT1.empty() || ExpandT2.empty())
    {
#ifndef SGX_ENCLAVE_BUILD
        cerr << "No valid tuples to expand." << endl;
#endif
        return;
    }

    // print ExpandT1 and ExpandT2 before expansion
    // cout << "=== Before Expansion ===\n";
    // printTable(ExpandT1, {"h", "tid", "other keys...", "a1", "a2"});
    // printTable(ExpandT2, {"h", "tid", "other keys...", "a1", "a2"});

    ObliViatorExpand expand(std::move(ExpandT1), std::move(g1));
    ExpandT1 = expand.getResult();
    expand = ObliViatorExpand(std::move(ExpandT2), std::move(g2));
    ExpandT2 = expand.getResult();

    // print ExpandT1 and ExpandT2 after expansion
    // cout << "=== After Expansion ===\n";
    // printTable(ExpandT1, {"h", "tid", "other keys...", "a1", "a2"});
    // printTable(ExpandT2, {"h", "tid", "other keys...", "a1", "a2"});
}

void ObliViatorJoin::alignTable(Table &T)
{
    // T(h, tid, k,..., a1, a2)
    int n = T.size();
    if (n == 0)
        return;
    vector<int> C(n, 0);
    vector<int> flag(n, 1);
    flag[0] = 0;

#pragma omp parallel for
    for (int i = 1; i < n; i++)
    {
        C[i] = T[i][0] == T[i - 1][0];
    }
    C = Aggtree(C, C, 0).PrefixTreeRun();

#pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        int a1 = T[i][T[i].size() - 2];
        int a2 = T[i][T[i].size() - 1];
        int q = (C[i] / a1) + (C[i] % a1) * a2;
        T[i].push_back(q); // oid
    }

    // T(h, tid, k,..., a1, a2, q)
    T = sortTable(std::move(T), {0, static_cast<int>(T[0].size()) - 1, 1}, true); // Sort by h, oid, d
}

Table ObliViatorJoin::sortTable(Table table, vector<int> keyColumns, bool direction)
{
    ParallelBitonicSorter sorter(std::move(table), std::move(keyColumns), direction);
    sorter.Sorter();
    return sorter.getSortedData();
}

void ObliViatorJoin::printTable(const Table &vec, const vector<string> &headers)
{
#ifdef SGX_ENCLAVE_BUILD
    (void)vec;
    (void)headers;
    return;
#else
    int width = 6;
    for (const auto &h : headers)
        cout << left << setw(width) << h;
    cout << '\n';

    for (const auto &row : vec)
    {
        for (int num : row)
            cout << left << setw(width) << num;
        cout << '\n';
    }
#endif
}
