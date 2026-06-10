#include "../include/DupAggtree.h"
#include "../include/PrimitiveProfile.h"
#include <cstring>

DupAggtree::DupAggtree(const vector<vector<int>> &D, const vector<int> &B)
{
    if (D.empty())
#ifdef SGX_ENCLAVE_BUILD
        return;
#else
        throw invalid_argument("Data matrix D is empty.");
#endif
    this->D = D;
    this->B = B;
}

DupAggtree::DupAggtree(const vector<int>& D_flat, int n_rows, int n_cols, const vector<int>& B_flat)
{
    this->D_flat      = &D_flat;
    this->n_rows_flat = n_rows;
    this->n_cols_flat = n_cols;
    this->B_flat      = &B_flat;
}

// Flat-array implementation: replaces the original per-node vector<int> px/lpx
// with a single pre-allocated block, eliminating O(n log n) small heap allocations.
//
// Layout: node (h, i) stores px  at A_px [(loff[h]+i)*dim .. +dim-1],
//                              lpx at A_lpx[(loff[h]+i)*dim .. +dim-1].
vector<vector<int>> DupAggtree::DupAggtreeRun()
{
    PrimitiveProfileScope primitiveScope(PrimitiveAggtree);
    int n   = D.size();
    int dim = D[0].size();
    int level = int(ceil(log2(max(n, 2)))) + 1;

    // Per-level sizes and flat offsets
    vector<int> lsz(level), loff(level);
    int total = 0;
    for (int h = 0; h < level; h++)
    {
        lsz[h]  = (n + (1 << h) - 1) >> h;
        loff[h] = total;
        total  += lsz[h];
    }

    // Single allocation for all tree-node data (no per-node vector)
    vector<int> A_px ((total * dim), 0);
    vector<int> A_lpx((total * dim), 0);
    vector<int> A_l  (total, 0);
    vector<int> A_c  (total, 0);

    // Initialize level 0
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        int base = (loff[0] + i) * dim;
        for (int j = 0; j < dim; j++)
            A_px[base + j] = D[i][j];
        A_l[loff[0] + i] = B[i];
        A_c[loff[0] + i] = B[i];
    }

    // Upstream phase
    for (int h = 1; h < level; h++)
    {
        int sz      = lsz[h];
        int sz_prev = lsz[h - 1];
#pragma omp parallel for schedule(static)
        for (int i = 0; i < sz; i++)
        {
            int t_base = (loff[h]     + i      ) * dim;
            int l_base = (loff[h - 1] + 2 * i  ) * dim;
            int l_c    =  A_c[loff[h - 1] + 2 * i];

            memcpy(&A_lpx[t_base], &A_px[l_base], dim * sizeof(int)); // t.lpx = left.px
            A_l[loff[h] + i] = A_l[loff[h - 1] + 2 * i];

            if (2 * i + 1 < sz_prev)
            {
                int r_base = (loff[h - 1] + 2 * i + 1) * dim;
                int r_c    =  A_c[loff[h - 1] + 2 * i + 1];
                A_c[loff[h] + i] = l_c & r_c;
                for (int j = 0; j < dim; j++)
                    A_px[t_base + j] = (!r_c) * A_px[r_base + j] + r_c * A_px[l_base + j];
            }
            // else: A_c stays 0, A_px stays 0 (zero-initialized) — matches original
        }
    }

    // Downstream phase
    for (int h = level - 1; h > 0; h--)
    {
        int sz      = lsz[h];
        int sz_prev = lsz[h - 1];
#pragma omp parallel for schedule(static)
        for (int i = 0; i < sz; i++)
        {
            int par_base = (loff[h]     + i    ) * dim;
            int l_base   = (loff[h - 1] + 2 * i) * dim;

            if (2 * i + 1 < sz_prev)
            {
                int r_base = (loff[h - 1] + 2 * i + 1) * dim;
                int r_l    =  A_l[loff[h - 1] + 2 * i + 1];
                int l_c    =  A_c[loff[h - 1] + 2 * i];
                for (int j = 0; j < dim; j++)
                    A_px[r_base + j] = r_l * (!l_c) * A_lpx[par_base + j]
                                     + r_l *   l_c  * A_px [par_base + j];
            }
            // left.px = parent.px (done after right so A_lpx[par_base] is still valid)
            memcpy(&A_px[l_base], &A_px[par_base], dim * sizeof(int));
        }
    }

    // Collect result
    result.resize(n);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        result[i].resize(dim);
        int base = (loff[0] + i) * dim;
        for (int j = 0; j < dim; j++)
            result[i][j] = B[i] * A_px[base + j] + (1 - B[i]) * D[i][j];
    }
    return result;
}

// Flat-array variant: D_flat is row-major n×dim, result is row-major n×dim.
// Identical tree logic but operates on flat arrays throughout — avoids the
// O(n*dim) vector<vector<int>> copy that the regular variant performs.
vector<int> DupAggtree::DupAggtreeRunFlat()
{
    PrimitiveProfileScope primitiveScope(PrimitiveAggtree);
    int n   = n_rows_flat;
    int dim = n_cols_flat;
    const int* D = D_flat->data();
    const int* B = B_flat->data();

    int level = int(ceil(log2(max(n, 2)))) + 1;

    vector<int> lsz(level), loff(level);
    int total = 0;
    for (int h = 0; h < level; h++)
    {
        lsz[h]  = (n + (1 << h) - 1) >> h;
        loff[h] = total;
        total  += lsz[h];
    }

    vector<int> A_px ((total * dim), 0);
    vector<int> A_lpx((total * dim), 0);
    vector<int> A_l  (total, 0);
    vector<int> A_c  (total, 0);

    // Initialize level 0
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        int base = (loff[0] + i) * dim;
        memcpy(&A_px[base], D + i * dim, dim * sizeof(int));
        A_l[loff[0] + i] = B[i];
        A_c[loff[0] + i] = B[i];
    }

    // Upstream phase
    for (int h = 1; h < level; h++)
    {
        int sz      = lsz[h];
        int sz_prev = lsz[h - 1];
#pragma omp parallel for schedule(static)
        for (int i = 0; i < sz; i++)
        {
            int t_base = (loff[h]     + i      ) * dim;
            int l_base = (loff[h - 1] + 2 * i  ) * dim;
            int l_c    =  A_c[loff[h - 1] + 2 * i];

            memcpy(&A_lpx[t_base], &A_px[l_base], dim * sizeof(int));
            A_l[loff[h] + i] = A_l[loff[h - 1] + 2 * i];

            if (2 * i + 1 < sz_prev)
            {
                int r_base = (loff[h - 1] + 2 * i + 1) * dim;
                int r_c    =  A_c[loff[h - 1] + 2 * i + 1];
                A_c[loff[h] + i] = l_c & r_c;
                for (int j = 0; j < dim; j++)
                    A_px[t_base + j] = (!r_c) * A_px[r_base + j] + r_c * A_px[l_base + j];
            }
        }
    }

    // Downstream phase
    for (int h = level - 1; h > 0; h--)
    {
        int sz      = lsz[h];
        int sz_prev = lsz[h - 1];
#pragma omp parallel for schedule(static)
        for (int i = 0; i < sz; i++)
        {
            int par_base = (loff[h]     + i    ) * dim;
            int l_base   = (loff[h - 1] + 2 * i) * dim;

            if (2 * i + 1 < sz_prev)
            {
                int r_base = (loff[h - 1] + 2 * i + 1) * dim;
                int r_l    =  A_l[loff[h - 1] + 2 * i + 1];
                int l_c    =  A_c[loff[h - 1] + 2 * i];
                for (int j = 0; j < dim; j++)
                    A_px[r_base + j] = r_l * (!l_c) * A_lpx[par_base + j]
                                     + r_l *   l_c  * A_px [par_base + j];
            }
            memcpy(&A_px[l_base], &A_px[par_base], dim * sizeof(int));
        }
    }

    // Collect result (flat)
    vector<int> res((size_t)n * dim);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        int base = (loff[0] + i) * dim;
        for (int j = 0; j < dim; j++)
            res[i * dim + j] = B[i] * A_px[base + j] + (1 - B[i]) * D[i * dim + j];
    }
    return res;
}
