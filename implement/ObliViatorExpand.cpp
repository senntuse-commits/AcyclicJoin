#include "../include/ObliViatorExpand.h"
#include "../include/Aggtree.h"
#include "../include/DupAggtree.h"
#include "../include/PrimitiveProfile.h"
#include <climits>

#ifdef SGX_ENCLAVE_BUILD
#include "sgx_log.h"
#define cout sgx_cout
#define cerr sgx_cerr
#define endl sgx_endl
#endif

namespace
{
constexpr int kTaskLoopThreshold = 4096;
}

ObliViatorExpand::ObliViatorExpand(vector<vector<int>> T_input, vector<int> M_input)
{
    PrimitiveProfileScope primitiveScope(PrimitiveExpand);
    if (T_input.empty() || M_input.empty())
    {
#ifdef SGX_ENCLAVE_BUILD
        return;
#else
        throw invalid_argument("T_input or M_input is empty.");
#endif
    }
    const size_t n = T_input.size();

    T = std::move(T_input);
    M = std::move(M_input);

    vector<int> flag(M.size(), 1);
    flag[0] = 0;

    Q = Aggtree(M, flag, 1).PrefixTreeRun();

    int outSize = M[n - 1] + Q[n - 1];
    size_t rowSize = T[0].size();

    T.resize(outSize, vector<int>(rowSize, INT_MAX));
    Q.resize(outSize, INT_MAX);

#pragma omp parallel
    {
#pragma omp single
        OExpand(0, outSize);
    }

    // fill in missing entries
    vector<int> B1(outSize, 0);
#pragma omp parallel for
    for (int i = 0; i < outSize; i++)
    {
        if (T[i][0] != INT_MAX)
        {
            B1[i] = 0;
        }
        else
        {
            B1[i] = 1;
        }
    }
    DupAggtree DupAggtree(std::move(T), std::move(B1));
    T = DupAggtree.DupAggtreeRun();
}

// s: start index, e: end index, z: offset
// This function performs the distributes on the range [s, e].
void ObliViatorExpand::OExpandSub(int s, int e, int z)
{
    int n = e - s + 1;
    if (n <= 1)
        return;

    int temp2 = n / 2;
    int mid = s + temp2;

    int m = 0;
    for (int i = 0; i < n; i++)
    {
        m += (Q[s + i] < mid);
    }

    if (n == 2)
    {
        bool cond = !(m ^ z);
        Swap(T, s, e, cond);
        Swap(Q, s, e, cond);
        return;
    }

    bool condition_0 = (z < temp2);
    bool condition_1 = (((z % temp2) + m) < temp2);
    bool condition_3 = condition_1 ^ condition_0;
    int ii = (z + m) % temp2;

    if (temp2 >= kTaskLoopThreshold)
    {
#pragma omp taskloop
        for (int i = 0; i < temp2; i++)
        {
            bool condition_2 = condition_3 ^ (ii <= i);
            Swap(T, s + i, s + i + n / 2, condition_2);
            Swap(Q, s + i, s + i + n / 2, condition_2);
        }
    }
    else
    {
        for (int i = 0; i < temp2; i++)
        {
            bool condition_2 = condition_3 ^ (ii <= i);
            Swap(T, s + i, s + i + n / 2, condition_2);
            Swap(Q, s + i, s + i + n / 2, condition_2);
        }
    }

#pragma omp task shared(T, Q) if (temp2 >= 1024)
    {
        OExpandSub(s, mid - 1, z % temp2);
    }

#pragma omp task shared(T, Q) if (temp2 >= 1024)
    {
        OExpandSub(mid, e, (z + m) % temp2);
    }

#pragma omp taskwait
}

// s: start index, e: end index
// This function performs the distribute on the range [s, e).
void ObliViatorExpand::OExpand(int s, int e)
{
    int n = e - s;
    if (n <= 1)
        return;

    int n1 = 1 << int(log2(n));
    int n2 = n - n1;
    int temp = s + n2;

    int m = 0;
    for (int i = 0; i < n2; i++)
    {
        m += (Q[s + i] < temp);
    }

    if (n2 >= kTaskLoopThreshold)
    {
#pragma omp taskloop
        for (int i = 0; i < n2; i++)
        {
            bool b = (m <= i);
            Swap(T, s + i, s + i + n1, b);
            Swap(Q, s + i, s + i + n1, b);
        }
    }
    else
    {
        for (int i = 0; i < n2; i++)
        {
            bool b = (m <= i);
            Swap(T, s + i, s + i + n1, b);
            Swap(Q, s + i, s + i + n1, b);
        }
    }

#pragma omp task shared(T, Q) if (n >= 1024)
    {
        OExpand(s, s + n2);
    }

#pragma omp task shared(T, Q) if (n >= 1024)
    {
        OExpandSub(s + n2, e - 1, (n1 - n2 + m) % n1);
    }

#pragma omp taskwait
}

void ObliViatorExpand::printVector(const vector<vector<int>> &vec, const vector<string> &headers)
{
#ifdef SGX_ENCLAVE_BUILD
    (void)vec;
    (void)headers;
    return;
#else
    int width = 15;
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
