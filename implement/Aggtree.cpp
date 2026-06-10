#include "../include/Aggtree.h"
#include "../include/PrimitiveProfile.h"

// Aggtree.cpp implements prefix-style aggregation trees used by oblivious
// expansion and compaction primitives. These routines are performance-critical,
// so they are wrapped with primitive profiling scopes for benchmark breakdowns.

Aggtree::Aggtree(const vector<int> &D, const vector<int> &B, int exclusive)
{
    this->D = D;
    this->B = B;
    this->exclusive = exclusive;
}

vector<int> Aggtree::PrefixTreeRun()
{
    PrimitiveProfileScope primitiveScope(PrimitiveAggtree);
    int n = D.size();
    if (n == 1)
    {
        result.push_back(exclusive == 1 ? 0 : D[0]);
        return result;
    }
    int level = int(ceil(log2(n))) + 1;
    // level-wise tree: h: [0..level-1], each A[h] has ceil(n / 2^h) entries
    vector<vector<AggTreeNode>> A(level);

    // Initialize the tree nodes
    A[0].resize(n);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        A[0][i] = {0, D[i], 0, 0, B[i], B[i]};
    }

    // Upstream
    for (int h = 1; h < level; h++)
    {
        int size = ceil(n / pow(2, h));
        A[h].resize(size);

        for (int i = 0; i < size; i++)
        {
            A[h][i].px = 0;
            A[h][i].lpx = 0;
        }

#pragma omp parallel for schedule(static, 64)
        for (int i = 0; i < size; i++)
        {
            AggTreeNode &left = A[h - 1][2 * i];
            AggTreeNode &node = A[h][i];
            AggTreeNode rightTmp(0, 0, 0, 0, 0, 0);
            AggTreeNode &right = (2 * i + 1 < A[h - 1].size()) ? A[h - 1][2 * i + 1] : rightTmp;

            node.lpx = left.px;
            node.l = left.l;
            node.c = left.c & right.c;
            node.px = (!right.c) * right.px + right.c * (left.px + right.px);
        }
    }

    // Downstream
    for (int h = level - 1; h > 0; h--)
    {
        int size = A[h].size();

#pragma omp parallel for schedule(static, 64)
        for (int i = 0; i < size; i++)
        {
            const AggTreeNode &parent = A[h][i];
            AggTreeNode &left = A[h - 1][2 * i];

            if (2 * i + 1 < (int)A[h - 1].size())
            {
                AggTreeNode &right = A[h - 1][2 * i + 1];
                left.px = parent.px * left.l;
                right.px = right.l * (!left.c) * parent.lpx +
                           right.l * left.c * (parent.lpx + parent.px);
            }
            else
            {
                left.px = parent.px * left.l;
            }
        }
    }

    // Collect the result
    result.resize(n);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        if (exclusive == 1)
        {
            result[i] = A[0][i].px;
        }
        else
        {
            result[i] = D[i] + A[0][i].px;
        }
    }

    return result;

    //     // Upstream
    //     for (int h = 1; h < level; h++)
    //     {
    //         int size = ceil(n / pow(2, h));
    //         A[h].resize(size);
    // #pragma omp parallel for schedule(static, 64)
    //         for (int i = 0; i < size; i++)
    //         {
    //             AggTreeNode &left = A[h - 1][2 * i];
    //             AggTreeNode temp = {0, 0, 0, 0, 0, 0};
    //             AggTreeNode &right = temp;
    //             if (2 * i + 1 < A[h - 1].size())
    //             {
    //                 right = A[h - 1][2 * i + 1];
    //             }
    //             AggTreeNode t(0, 0, 0, 0, 0, 0);
    //             t.lpx = left.px;
    //             t.l = left.l;
    //             t.c = left.c & right.c;
    //             t.px = (!right.c) * right.px + right.c * (left.px + right.px);
    //             A[h][i] = t;
    //         }
    //     }

    //     // Downstream
    //     for (int h = level - 1; h > 0; h--)
    //     {
    //         int size = A[h].size();
    // #pragma omp parallel for schedule(static, 64)
    //         for (int i = 0; i < size; i++)
    //         {
    //             AggTreeNode parent = A[h][i];
    //             AggTreeNode &left = A[h - 1][2 * i];
    //             if (2 * i + 1 < A[h - 1].size())
    //             {
    //                 AggTreeNode &right = A[h - 1][2 * i + 1];
    //                 left.px = parent.px * left.l;
    //                 right.px = right.l * (!left.c) * parent.lpx + right.l * left.c * (parent.lpx + parent.px);
    //             }
    //             else
    //             {
    //                 left.px = parent.px * left.l;
    //             }
    //         }
    //     }

    //     // Collect the result
    //     result.resize(n);
    //     if (exclusive == 1)
    //     {
    // #pragma omp parallel for schedule(static)
    //         for (int i = 0; i < n; i++)
    //         {
    //             result[i] = A[0][i].px;
    //         }
    //     }
    //     else
    //     {
    // #pragma omp parallel for schedule(static)
    //         for (int i = 0; i < n; i++)
    //         {
    //             result[i] = D[i] + A[0][i].px;
    //         }
    //     }

    //     return result;
}

vector<int> Aggtree::SuffixTreeRun()
{
    PrimitiveProfileScope primitiveScope(PrimitiveAggtree);
    int n = D.size();
    int level = int(ceil(log2(n))) + 1;
    // level-wise tree: h: [0..level-1], each A[h] has ceil(n / 2^h) entries
    vector<vector<AggTreeNode>> A(level);

    // Initialize the tree nodes
    A[0].resize(n);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        int sx = D[i] * B[i];
        A[0][i] = {0, D[i], 0, sx, B[i], B[i]};
    }

    // Upstream
    for (int h = 1; h < level; h++)
    {
        int size = ceil(n / pow(2, h));
        A[h].resize(size);

#pragma omp parallel for schedule(static, 64)
        for (int i = 0; i < size; i++)
        {
            const AggTreeNode &left = A[h - 1][2 * i];
            const AggTreeNode *right = (2 * i + 1 < (int)A[h - 1].size())
                                           ? &A[h - 1][2 * i + 1]
                                           : nullptr;

            AggTreeNode &node = A[h][i];

            node.rsx = right ? right->sx : 0;
            node.l = left.l;
            node.c = right ? (left.c & right->c) : left.c;
            node.sx = (!left.c) * left.sx + left.c * (left.sx + (right ? right->sx : 0));
        }
    }

    // Downstream
    for (int h = level - 1; h > 0; h--)
    {
        int size = A[h].size();

#pragma omp parallel for schedule(static, 64)
        for (int i = 0; i < size; i++)
        {
            const AggTreeNode &parent = A[h][i];
            AggTreeNode &left = A[h - 1][2 * i];

            if (2 * i + 1 < (int)A[h - 1].size())
            {
                AggTreeNode &right = A[h - 1][2 * i + 1];
                left.sx = right.c * (parent.rsx + parent.sx) + (!right.c) * parent.rsx;
                right.sx = parent.sx;
            }
            else
            {
                left.sx = 0;
            }
        }
    }

    // Collect the result
    result.resize(n);
    if (exclusive == 1)
    {
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++)
        {
            result[i] = D[i] + A[0][i].sx;
        }
    }
    else
    {
#pragma omp parallel for schedule(static)
        for (int i = 0; i < n; i++)
        {
            result[i] = A[0][i].sx;
        }
    }

    return result;

    //     // Upstream
    //     for (int h = 1; h < level; h++)
    //     {
    //         int size = ceil(n / pow(2, h));
    //         A[h].resize(size);
    // #pragma omp parallel for schedule(static, 64)
    //         for (int i = 0; i < size; i++)
    //         {
    //             AggTreeNode &left = A[h - 1][2 * i];
    //             AggTreeNode temp = {0, 0, 0, 0, 0, 0};
    //             AggTreeNode &right = temp;
    //             if (2 * i + 1 < A[h - 1].size())
    //             {
    //                 right = A[h - 1][2 * i + 1];
    //             }
    //             AggTreeNode t(0, 0, 0, 0, 0, 0);
    //             t.rsx = right.sx;
    //             t.l = left.l;
    //             t.c = left.c & right.c;
    //             t.sx = (!left.c) * left.sx + left.c * (left.sx + right.sx);
    //             A[h][i] = t;
    //         }
    //     }

    //     // Downstream
    //     for (int h = level - 1; h > 0; h--)
    //     {
    //         int size = A[h].size();
    // #pragma omp parallel for schedule(static, 64)
    //         for (int i = 0; i < size; i++)
    //         {
    //             AggTreeNode parent = A[h][i];
    //             AggTreeNode &left = A[h - 1][2 * i];
    //             if (2 * i + 1 < A[h - 1].size())
    //             {
    //                 AggTreeNode &right = A[h - 1][2 * i + 1];
    //                 left.sx = right.c * (parent.rsx + parent.sx) + (!right.c) * parent.rsx;
    //                 right.sx = parent.sx;
    //             }
    //             else
    //             {
    //                 left.sx = 0;
    //             }
    //         }
    //     }

    //     // Collect the result
    //     result.resize(n);
    //     if (exclusive == 1)
    //     {
    // #pragma omp parallel for schedule(static)
    //         for (int i = 0; i < n; i++)
    //         {
    //             result[i] = D[i] + A[0][i].sx;
    //         }
    //     }
    //     else
    //     {
    // #pragma omp parallel for schedule(static)
    //         for (int i = 0; i < n; i++)
    //         {
    //             result[i] = A[0][i].sx;
    //         }
    //     }
    //     return result;
}

vector<int> Aggtree::SuffixProductRun()
{
    PrimitiveProfileScope primitiveScope(PrimitiveAggtree);
    int n = D.size();
    if (n == 0)
    {
        result.clear();
        return result;
    }
    if (n == 1)
    {
        result.assign(1, exclusive == 1 ? 1 : D[0]);
        return result;
    }

    int level = int(ceil(log2(n))) + 1;
    vector<vector<long long>> totalProd(level);
    vector<vector<long long>> prefixProd(level);
    vector<vector<int>> allConnected(level);
    vector<vector<int>> boundary(level);
    vector<vector<long long>> suffix(level);

    totalProd[0].resize(n);
    prefixProd[0].resize(n);
    allConnected[0].resize(n);
    boundary[0].resize(n);
    suffix[0].assign(n, 1);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i)
    {
        totalProd[0][i] = D[i];
        prefixProd[0][i] = D[i];
        allConnected[0][i] = 1;
        // B[i] follows the segmented AggTree convention: it is 1 when item i
        // belongs to the same segment as item i - 1. For suffix propagation we
        // need the boundary from i to i + 1, which is therefore B[i + 1].
        boundary[0][i] = (i + 1 < n) ? B[i + 1] : 0;
    }

    // Upstream: for each segment keep the whole product, the product of the
    // connected prefix, whether the whole segment is connected, and whether
    // the segment can connect to the next segment on its right.
    for (int h = 1; h < level; ++h)
    {
        int size = (int)ceil(n / pow(2, h));
        totalProd[h].resize(size);
        prefixProd[h].resize(size);
        allConnected[h].resize(size);
        boundary[h].resize(size);
        suffix[h].assign(size, 1);

#pragma omp parallel for schedule(static, 64)
        for (int i = 0; i < size; ++i)
        {
            int leftIdx = 2 * i;
            int rightIdx = leftIdx + 1;

            if (rightIdx < (int)totalProd[h - 1].size())
            {
                totalProd[h][i] = totalProd[h - 1][leftIdx] * totalProd[h - 1][rightIdx];

                if (allConnected[h - 1][leftIdx] && boundary[h - 1][leftIdx])
                    prefixProd[h][i] = totalProd[h - 1][leftIdx] * prefixProd[h - 1][rightIdx];
                else
                    prefixProd[h][i] = prefixProd[h - 1][leftIdx];

                allConnected[h][i] = allConnected[h - 1][leftIdx] &&
                                     boundary[h - 1][leftIdx] &&
                                     allConnected[h - 1][rightIdx];
                boundary[h][i] = boundary[h - 1][rightIdx];
            }
            else
            {
                totalProd[h][i] = totalProd[h - 1][leftIdx];
                prefixProd[h][i] = prefixProd[h - 1][leftIdx];
                allConnected[h][i] = allConnected[h - 1][leftIdx];
                boundary[h][i] = boundary[h - 1][leftIdx];
            }
        }
    }

    // Downstream: push the product of all valid elements to the right of each
    // segment to its children.
    for (int h = level - 1; h > 0; --h)
    {
        int size = (int)totalProd[h].size();
#pragma omp parallel for schedule(static, 64)
        for (int i = 0; i < size; ++i)
        {
            int leftIdx = 2 * i;
            int rightIdx = leftIdx + 1;
            long long parentSuffix = suffix[h][i];

            if (rightIdx < (int)totalProd[h - 1].size())
            {
                suffix[h - 1][rightIdx] = parentSuffix;
                if (boundary[h - 1][leftIdx])
                {
                    suffix[h - 1][leftIdx] = prefixProd[h - 1][rightIdx];
                    if (allConnected[h - 1][rightIdx])
                        suffix[h - 1][leftIdx] *= parentSuffix;
                }
                else
                {
                    suffix[h - 1][leftIdx] = 1;
                }
            }
            else
            {
                suffix[h - 1][leftIdx] = parentSuffix;
            }
        }
    }

    result.resize(n);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i)
    {
        long long value = suffix[0][i];
        if (exclusive == 0)
            value *= D[i];
        result[i] = (int)value;
    }
    return result;
}

vector<int> Aggtree::FullRun()
{
    vector<int> prefixResult = PrefixTreeRun();
    vector<int> suffixResult = SuffixTreeRun();
    int n = D.size();
    result.resize(n);
#pragma omp parallel for schedule(static)
    for (int i = 0; i < n; i++)
    {
        if (exclusive == 1)
        {
            result[i] = prefixResult[i] + suffixResult[i];
        }
        else
        {
            result[i] = D[i] + prefixResult[i] + suffixResult[i];
        }
    }
    return result;
}
