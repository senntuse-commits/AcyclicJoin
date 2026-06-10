#include "../include/ORcompact.h"
#include "../include/PrimitiveProfile.h"
#include <omp.h>

static vector<int> prefixSumRangePar(const vector<int> &M, int left, int right, int threads)
{
    int n = right - left;
    vector<int> S(M.size() + 1, 0);
    if (n <= 0)
        return S;

    int blocks = std::max(1, std::min(threads, n));
    int chunk = (n + blocks - 1) / blocks;
    vector<int> blockSum(blocks, 0);

#pragma omp parallel for schedule(static) num_threads(threads)
    for (int b = 0; b < blocks; ++b)
    {
        int begin = left + b * chunk;
        int end = std::min(right, begin + chunk);
        int sum = 0;
        for (int i = begin; i < end; ++i)
            sum += M[i];
        blockSum[b] = sum;
    }

    int offset = 0;
    for (int b = 0; b < blocks; ++b)
    {
        int sum = blockSum[b];
        blockSum[b] = offset;
        offset += sum;
    }
    S[left] = 0;

#pragma omp parallel for schedule(static) num_threads(threads)
    for (int b = 0; b < blocks; ++b)
    {
        int begin = left + b * chunk;
        int end = std::min(right, begin + chunk);
        int running = blockSum[b];
        for (int i = begin; i < end; ++i)
        {
            running += M[i];
            S[i + 1] = running;
        }
    }
    return S;
}

static void serialOffCompactRows(vector<vector<int>> &D, const vector<int> &M, int left, int right, int z)
{
    int n = right - left;
    if (n == 2)
    {
        if (((1 - M[left]) * M[left + 1]) ^ z)
            swap(D[left], D[left + 1]);
    }
    else if (n > 2)
    {
        int mid = left + n / 2;
        int m = 0;
        for (int i = left; i < mid; ++i)
            m += M[i];

        serialOffCompactRows(D, M, left, mid, z % (n / 2));
        serialOffCompactRows(D, M, mid, right, (z + m) % (n / 2));

        bool s = ((z % (n / 2) + m) >= (n / 2)) ^ (z >= (n / 2));
        for (int i = 0; i < n / 2; ++i)
        {
            bool b = s ^ (i >= ((z + m) % (n / 2)));
            if (b)
                swap(D[left + i], D[left + i + n / 2]);
        }
    }
}

static void serialORCompactRows(vector<vector<int>> &D, const vector<int> &M, int left, int right)
{
    int n = right - left;
    if (n == 0)
        return;

    int n1 = 1;
    while ((n1 << 1) <= n)
        n1 <<= 1;
    int n2 = n - n1;
    int m = 0;
    for (int i = left; i < left + n2; ++i)
        m += M[i];

    serialORCompactRows(D, M, left, left + n2);
    serialOffCompactRows(D, M, left + n2, right, (n1 - n2 + m) % n1);

    for (int i = 0; i < n2; ++i)
        if (i >= m)
            swap(D[left + i], D[left + n1 + i]);
}

static void orocParRows(vector<vector<int>> &D, const vector<int> &S, int left, int right, int z)
{
    int n = right - left;
    if (n == 2)
    {
        int m0 = S[left + 1] - S[left];
        int m1 = S[left + 2] - S[left + 1];
        if (((1 - m0) * m1) ^ z)
            swap(D[left], D[left + 1]);
    }
    else if (n > 2)
    {
        int half = n / 2;
        int mid = left + half;
        int m = S[mid] - S[left];

#pragma omp task shared(D, S) firstprivate(left, mid, z, half) if (n >= 8192)
        orocParRows(D, S, left, mid, z % half);
#pragma omp task shared(D, S) firstprivate(mid, right, z, m, half) if (n >= 8192)
        orocParRows(D, S, mid, right, (z + m) % half);
#pragma omp taskwait

        bool s = ((z % half + m) >= half) ^ (z >= half);
        int threshold = (z + m) % half;
        if (n >= 8192)
        {
#pragma omp taskloop grainsize(1024) shared(D) firstprivate(left, half, s, threshold)
            for (int i = 0; i < half; ++i)
            {
                bool b = s ^ (i >= threshold);
                if (b)
                    swap(D[left + i], D[left + i + half]);
            }
        }
        else
        {
            for (int i = 0; i < half; ++i)
            {
                bool b = s ^ (i >= threshold);
                if (b)
                    swap(D[left + i], D[left + i + half]);
            }
        }
    }
}

static void orcParRows(vector<vector<int>> &D, const vector<int> &S, int left, int right)
{
    int n = right - left;
    if (n == 0)
        return;

    int n1 = 1;
    while ((n1 << 1) <= n)
        n1 <<= 1;
    int n2 = n - n1;
    int m = S[left + n2] - S[left];

#pragma omp task shared(D, S) firstprivate(left, n2) if (n >= 8192 && n2 > 0)
    orcParRows(D, S, left, left + n2);
#pragma omp task shared(D, S) firstprivate(left, right, n1, n2, m) if (n >= 8192)
    orocParRows(D, S, left + n2, right, (n1 - n2 + m) % n1);
#pragma omp taskwait

    if (n2 >= 8192)
    {
#pragma omp taskloop grainsize(1024) shared(D) firstprivate(left, n1, n2, m)
        for (int i = 0; i < n2; ++i)
        {
            if (i >= m)
                swap(D[left + i], D[left + n1 + i]);
        }
    }
    else
    {
        for (int i = 0; i < n2; ++i)
            if (i >= m)
                swap(D[left + i], D[left + n1 + i]);
    }
}

void ObliCompact::Swap(vector<vector<int>> &D, int i, int j, bool b)
{
    if (b)
    {
        swap(D[i], D[j]);
    }
    else
    {
        D[i] = D[i];
        D[j] = D[j];
    }
}

void ObliCompact::offcompact(vector<vector<int>> &D, vector<int> &M, int left, int right, int z)
{
    int n = right - left;
    if (n == 2)
    {
        Swap(D, left, left + 1, ((1 - M[left]) * M[left + 1]) ^ z);
    }
    else if (n > 2)
    {
        int mid = left + n / 2;
        int m = 0;
        for (int i = left; i < mid; ++i)
            m += M[i];

        offcompact(D, M, left, mid, z % (n / 2));
        offcompact(D, M, mid, right, (z + m) % (n / 2));

        bool s = ((z % (n / 2) + m) >= (n / 2)) ^ (z >= (n / 2));
        for (int i = 0; i < n / 2; ++i)
        {
            bool b = s ^ (i >= ((z + m) % (n / 2)));
            Swap(D, left + i, left + i + n / 2, b);
        }
    }
}

void ObliCompact::ORCompact(vector<vector<int>> &D, vector<int> &M, int left, int right)
{
    PrimitiveProfileScope primitiveScope(PrimitiveCompact);
    int n = right - left;
    if (n == 0)
        return;
    int threads = omp_get_max_threads();
    if (threads > 1 && n >= 8192)
    {
        vector<int> S = prefixSumRangePar(M, left, right, threads);
#pragma omp parallel num_threads(threads)
        {
#pragma omp single
            orcParRows(D, S, left, right);
        }
        return;
    }

    serialORCompactRows(D, M, left, right);
}

void ObliCompact::ORCompactSerial(vector<vector<int>> &D, vector<int> &M, int left, int right)
{
    PrimitiveProfileScope primitiveScope(PrimitiveCompact);
    serialORCompactRows(D, M, left, right);
}
