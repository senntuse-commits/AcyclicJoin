#include "../include/ParallelOExpand.h"
#include "../include/Aggtree.h"
#include "../include/DupAggtree.h"
#include <utility>

#ifdef SGX_ENCLAVE_BUILD
#include "sgx_log.h"
#define cout sgx_cout
#define cerr sgx_cerr
#define endl sgx_endl
#endif

SOExpand::SOExpand(vector<vector<int>> T, vector<int> M, bool isTruncate)
{
    this->T = std::move(T);
    this->M = std::move(M);
    this->isTruncate = isTruncate;
}

void SOExpand::Expand()
{
    int D3;
    int n = T.size();
    if (n == 0)
    {
        return;
    }

    vector<int> B(n, 1);
    B[0] = 0;

    // count the position of each element in the table
    Q.resize(n);
    Aggtree PrefixTree(M, B, 1);
    Q = PrefixTree.PrefixTreeRun();

    D1 = M[n - 1] + Q[n - 1];
    D2 = pow(2, ceil(log2(D1)));
    D3 = D2 / 2;

    // padding
    T.resize(D2, vector<int>(T[0].size(), INT_MAX));
    Q.resize(D2, INT_MIN);

    // put the elements into the right position
#pragma omp parallel
    {
#pragma omp single
        SubSOExpand(T, Q, 0, D3, D3);
    }

    /* fill in missing entries */
    // if isTruncate is true, we will truncate the result
    if (isTruncate)
    {
        vector<int> B1(D1, 0);
#pragma omp parallel for
        for (int i = 0; i < D1; i++)
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
        T.resize(D1);
        DupAggtree DupAggtree(T, B1);
        result = DupAggtree.DupAggtreeRun();
    }
    else
    {
        vector<int> B1(D2, 0);
#pragma omp parallel for
        for (int i = 0; i < D2; i++)
        {
            if (T[i][0] != INT_MAX || i >= D1)
            {
                B1[i] = 0;
            }
            else
            {
                B1[i] = 1;
            }
        }
        DupAggtree DupAggtree(T, B1);
        result = DupAggtree.DupAggtreeRun();
    }
}

bool SOExpand::SubSOExpand(vector<vector<int>> &T, vector<int> &Q, int start, int end, int sep)
{
    bool flag = (sep >= 1);

    if (sep >= 1024)
    {
#pragma omp parallel for
        for (int i = start; i < end; i++)
        {
            int b0 = (Q[i] != INT_MIN) && (Q[i] >= end);
            int b1 = (Q[i + sep] != INT_MIN) && (Q[i + sep] < end);
            Swap(T, i, i + sep, b0 || b1);
            Swap(Q, i, i + sep, b0 || b1);
        }
    }
    else
    {
        for (int i = start; i < end; i++)
        {
            int b0 = (Q[i] != INT_MIN) && (Q[i] >= end);
            int b1 = (Q[i + sep] != INT_MIN) && (Q[i + sep] < end);
            Swap(T, i, i + sep, b0 || b1);
            Swap(Q, i, i + sep, b0 || b1);
        }
    }

    bool r1 = true, r2 = true;

#pragma omp task shared(T, Q, r1) if (sep >= 1024)
    {
        r1 = flag && SubSOExpand(T, Q, start, start + sep / 2, sep / 2) + !flag && 1;
    }

#pragma omp task shared(T, Q, r2) if (sep >= 1024)
    {
        r2 = flag && SubSOExpand(T, Q, end, end + sep / 2, sep / 2) + !flag && 1;
    }

#pragma omp taskwait
    return r1 && r2;
}

vector<vector<int>> SOExpand::getResult()
{
    return std::move(result);
}

void SOExpand::printVector(const vector<vector<int>> &vec, const vector<string> &headers)
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
