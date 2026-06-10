#include "../include/BottomupSemiJoin.h"
#include "../include/ParallelSemiJoin.h"
#include "../include/Aggtree.h"
#include "../include/ObliViatorExpand.h"
#include "../include/PrimitiveProfile.h"
#include "../include/RRSSensitivity.h"
#include "../include/sgx_profile.h"
#include <cmath>
#include <cstdio>
#include <climits>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#ifndef SGX_ENCLAVE_BUILD
#include <chrono>

using namespace std::chrono;
#endif

static double g_lastBottomUpParallelMs = 0.0;
static double g_lastBottomUpSemiJoinMs = 0.0;
static double g_lastBottomUpRootExpandMs = 0.0;
static const int DO_DUMMY_VALUE = 100000007;

double getLastBottomUpParallelMs()
{
    return g_lastBottomUpParallelMs;
}

double getLastBottomUpSemiJoinMs()
{
    return g_lastBottomUpSemiJoinMs;
}

double getLastBottomUpRootExpandMs()
{
    return g_lastBottomUpRootExpandMs;
}

static long long satMul(long long a, long long b)
{
    if (a <= 0 || b <= 0)
        return 0;
    if (a > LLONG_MAX / b)
        return LLONG_MAX;
    return a * b;
}

static long long satAdd(long long a, long long b)
{
    if (b > 0 && a > LLONG_MAX - b)
        return LLONG_MAX;
    if (b < 0 && a < LLONG_MIN - b)
        return LLONG_MIN;
    return a + b;
}

static vector<vector<int>> buildChildrenList(const vector<int> &parent, int n)
{
    vector<vector<int>> children(n);
    for (int i = 0; i < n; i++)
        if (parent[i] != -1)
            children[parent[i]].push_back(i);
    return children;
}

static vector<vector<int>> buildLevels(const vector<vector<int>> &children, int root)
{
    vector<vector<int>> levels;
    vector<int> depth(children.size(), -1);
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
        for (int c : children[cur])
        {
            depth[c] = depth[cur] + 1;
            q.push(c);
        }
    }
    return levels;
}

static int edgeJoinColAtNode(int node, int neighbor, const vector<int> &parent,
                             const vector<int> &joinColInParent, const vector<int> &joinColInChild)
{
    if (parent[neighbor] == node)
        return joinColInParent[neighbor];
    if (parent[node] == neighbor)
        return joinColInChild[node];
    return -1;
}

static long long maxFrequencyOnColumn(const Table &table, int col)
{
    if (table.empty() || col < 0)
        return 0;

    unordered_map<int, long long> counts;
    counts.reserve(table.size() * 2 + 1);
    long long best = 0;
    for (const auto &row : table)
    {
        if (col >= (int)row.size())
            continue;
        long long v = ++counts[row[col]];
        if (v > best)
            best = v;
    }
    return best;
}

static std::string projectionKey(const vector<int> &row, const vector<int> &cols)
{
    std::string key;
    for (int col : cols)
    {
        if (col >= 0 && col < (int)row.size())
            key += std::to_string(row[col]);
        key += '#';
    }
    return key;
}

static long long maxFrequencyOnColumns(const Table &table, const vector<int> &cols)
{
    if (table.empty())
        return 0;
    if (cols.empty())
        return (long long)table.size();
    if (cols.size() == 1)
        return maxFrequencyOnColumn(table, cols[0]);

    unordered_map<std::string, long long> counts;
    counts.reserve(table.size() * 2 + 1);
    long long best = 0;
    for (const auto &row : table)
    {
        long long v = ++counts[projectionKey(row, cols)];
        if (v > best)
            best = v;
    }
    return best;
}

static int attrsToMask(const vector<int> &attrs)
{
    int mask = 0;
    for (int attr : attrs)
        if (attr >= 0 && attr < 30)
            mask |= (1 << attr);
    return mask;
}

static vector<int> maskToAttrs(int mask, int attrCount)
{
    vector<int> attrs;
    for (int a = 0; a < attrCount; ++a)
        if ((mask & (1 << a)) != 0)
            attrs.push_back(a);
    return attrs;
}

static vector<int> subsetRelations(int subsetMask, int n)
{
    vector<int> relations;
    for (int r = 0; r < n; ++r)
        if ((subsetMask & (1 << r)) != 0)
            relations.push_back(r);
    return relations;
}

struct QuerySpecificRRSConfig
{
    int relationCount = 0;
    int attrCount = 0;
    vector<int> relationAttrMask;
    vector<vector<int>> attrColumn;
    vector<vector<int>> parentMatrix;
    map<int, vector<int>> possibleRelaxations;
    map<pair<int, int>, long long> secondRelaxedTEs;
};

static vector<vector<int>> buildAllPairsNextHop(const vector<int> &parent)
{
    int n = (int)parent.size();
    vector<vector<int>> children = buildChildrenList(parent, n);
    vector<vector<int>> nextHop(n, vector<int>(n, -1));
    for (int src = 0; src < n; ++src)
    {
        queue<int> q;
        vector<int> prev(n, -1);
        q.push(src);
        prev[src] = src;
        while (!q.empty())
        {
            int cur = q.front();
            q.pop();
            for (int c : children[cur])
            {
                if (prev[c] == -1)
                {
                    prev[c] = cur;
                    q.push(c);
                }
            }
            if (parent[cur] != -1 && prev[parent[cur]] == -1)
            {
                prev[parent[cur]] = cur;
                q.push(parent[cur]);
            }
        }
        for (int dst = 0; dst < n; ++dst)
        {
            if (dst == src)
            {
                nextHop[src][dst] = -1;
                continue;
            }
            int cur = dst;
            while (prev[cur] != src && prev[cur] != cur && prev[cur] != -1)
                cur = prev[cur];
            nextHop[src][dst] = cur;
        }
    }
    return nextHop;
}

static int boundaryAttrMaskForConfig(const QuerySpecificRRSConfig &cfg, int subsetMask)
{
    int subsetAttrs = 0;
    int complementAttrs = 0;
    for (int r = 0; r < cfg.relationCount; ++r)
    {
        if ((subsetMask & (1 << r)) != 0)
            subsetAttrs |= cfg.relationAttrMask[r];
        else
            complementAttrs |= cfg.relationAttrMask[r];
    }
    return subsetAttrs & complementAttrs;
}

static vector<int> allRelaxationMasksForBoundary(int boundaryMask, int attrCount)
{
    vector<int> masks;
    vector<int> attrs = maskToAttrs(boundaryMask, attrCount);
    int total = 1 << (int)attrs.size();
    masks.reserve(total);
    for (int s = 0; s < total; ++s)
    {
        int mask = 0;
        for (int i = 0; i < (int)attrs.size(); ++i)
            if ((s & (1 << i)) != 0)
                mask |= (1 << attrs[i]);
        masks.push_back(mask);
    }
    return masks;
}

static void fillDefaultPossibleRelaxations(QuerySpecificRRSConfig &cfg)
{
    int totalMasks = 1 << cfg.relationCount;
    for (int subset = 1; subset < totalMasks - 1; ++subset)
    {
        int boundary = boundaryAttrMaskForConfig(cfg, subset);
        cfg.possibleRelaxations[subset] = allRelaxationMasksForBoundary(boundary, cfg.attrCount);
    }
}

static bool buildSQL18RRSConfig(const vector<int> &parent,
                                const vector<int> &joinColInParent,
                                const vector<int> &joinColInChild,
                                QuerySpecificRRSConfig &cfg)
{
    static const vector<int> sql18 = {-1, 0, 0, 0, 0, 4, 4};
    if (parent != sql18)
        return false;

    cfg.relationCount = 7;
    cfg.attrCount = 6;
    cfg.relationAttrMask.assign(cfg.relationCount, 0);
    cfg.attrColumn.assign(cfg.relationCount, vector<int>(cfg.attrCount, -1));

    const int edges[][3] = {
        {1, 0, 0},
        {2, 0, 1},
        {3, 0, 2},
        {4, 0, 3},
        {5, 4, 4},
        {6, 4, 5},
    };
    for (const auto &edge : edges)
    {
        int child = edge[0], p = edge[1], attr = edge[2];
        cfg.relationAttrMask[p] |= (1 << attr);
        cfg.relationAttrMask[child] |= (1 << attr);
        cfg.attrColumn[p][attr] = joinColInParent[child];
        cfg.attrColumn[child][attr] = joinColInChild[child];
    }
    cfg.parentMatrix = buildAllPairsNextHop(parent);
    fillDefaultPossibleRelaxations(cfg);
    return true;
}

static bool buildSQL85RRSConfig(const vector<int> &parent,
                                const vector<int> &joinColInParent,
                                const vector<int> &joinColInChild,
                                QuerySpecificRRSConfig &cfg)
{
    static const vector<int> sql85 = {-1, 0, 0, 0, 3, 3, 3, 3};
    if (parent != sql85)
        return false;

    cfg.relationCount = 8;
    cfg.attrCount = 7;
    cfg.relationAttrMask.assign(cfg.relationCount, 0);
    cfg.attrColumn.assign(cfg.relationCount, vector<int>(cfg.attrCount, -1));

    const int edges[][3] = {
        {1, 0, 0},
        {2, 0, 1},
        {3, 0, 2},
        {4, 3, 3},
        {5, 3, 4},
        {6, 3, 5},
        {7, 3, 6},
    };
    for (const auto &edge : edges)
    {
        int child = edge[0], p = edge[1], attr = edge[2];
        cfg.relationAttrMask[p] |= (1 << attr);
        cfg.relationAttrMask[child] |= (1 << attr);
        cfg.attrColumn[p][attr] = joinColInParent[child];
        cfg.attrColumn[child][attr] = joinColInChild[child];
    }
    cfg.parentMatrix = buildAllPairsNextHop(parent);
    fillDefaultPossibleRelaxations(cfg);
    return true;
}

static bool buildTernaryL3RRSConfig(const vector<int> &parent,
                                    const vector<int> &joinColInParent,
                                    const vector<int> &joinColInChild,
                                    QuerySpecificRRSConfig &cfg)
{
    static const vector<int> ternaryL3 = {-1, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3};
    if (parent != ternaryL3)
        return false;

    cfg.relationCount = 13;
    cfg.attrCount = 12;
    cfg.relationAttrMask.assign(cfg.relationCount, 0);
    cfg.attrColumn.assign(cfg.relationCount, vector<int>(cfg.attrCount, -1));

    for (int child = 1; child < cfg.relationCount; ++child)
    {
        int p = parent[child];
        int attr = child - 1;
        cfg.relationAttrMask[p] |= (1 << attr);
        cfg.relationAttrMask[child] |= (1 << attr);
        cfg.attrColumn[p][attr] = joinColInParent[child];
        cfg.attrColumn[child][attr] = joinColInChild[child];
    }
    cfg.parentMatrix = buildAllPairsNextHop(parent);
    fillDefaultPossibleRelaxations(cfg);
    return true;
}

static bool buildQuerySpecificRRSConfig(const vector<int> &parent,
                                        const vector<int> &joinColInParent,
                                        const vector<int> &joinColInChild,
                                        QuerySpecificRRSConfig &cfg)
{
    return buildSQL18RRSConfig(parent, joinColInParent, joinColInChild, cfg) ||
           buildSQL85RRSConfig(parent, joinColInParent, joinColInChild, cfg) ||
           buildTernaryL3RRSConfig(parent, joinColInParent, joinColInChild, cfg);
}

class DOStyleRSCalculator
{
public:
    DOStyleRSCalculator(int n, const vector<long long> &inTE, double inBeta)
        : n_(n), beta_(inBeta), te_(inTE), isPublic_(n, false), tupleDis_(n, 0)
    {
    }

    long long RunResidual()
    {
        m_ = 0;
        for (bool isPublic : isPublic_)
            if (!isPublic)
                ++m_;
        if (m_ <= 1 || beta_ <= 0.0)
            return 1;

        maxK_ = (int)(((double)m_ - 1.0) / beta_) + n_;
        maxRes_ = 0.0;
        kWithMaxRes_ = 0;

        for (int i = 0; i < n_; ++i)
        {
            if (isPublic_[i])
                continue;
            deletedId_ = i;
            CallMaxTE();
        }

        if (maxRes_ > (long double)LLONG_MAX)
            return LLONG_MAX;
        return std::max(1LL, (long long)std::ceil(maxRes_));
    }

private:
    int n_;
    double beta_;
    vector<long long> te_;
    vector<bool> isPublic_;
    vector<int> tupleDis_;
    int m_ = 0;
    int maxK_ = 0;
    int deletedId_ = 0;
    int firstId_ = 0;
    int secondId_ = 0;
    long double maxRes_ = 0.0;
    int kWithMaxRes_ = 0;

    void CallMaxTE()
    {
        std::fill(tupleDis_.begin(), tupleDis_.end(), 0);
        int leftRelationNum = m_ - 1;
        firstId_ = 0;
        secondId_ = 0;
        for (int i = 0; i < n_; ++i)
        {
            if (!isPublic_[i] && i != deletedId_)
            {
                if (leftRelationNum == 2)
                    firstId_ = i;
                if (leftRelationNum == 1)
                    secondId_ = i;
                --leftRelationNum;
            }
        }
        CallMaxTERec(0, maxK_);
    }

    void CallMaxTERec(int curId, int leftK)
    {
        if (curId == firstId_)
        {
            CallMaxTEWithTupleDis(leftK);
            return;
        }
        if (curId == deletedId_ || isPublic_[curId])
        {
            CallMaxTERec(curId + 1, leftK);
            return;
        }
        for (int i = 0; i <= leftK; ++i)
        {
            tupleDis_[curId] = i;
            CallMaxTERec(curId + 1, leftK - i);
        }
    }

    void CallMaxTEWithTupleDis(int leftK)
    {
        long double a = CalCoe(0, 0, 0, 0);
        long double b = CalCoe(0, 0, 1, 0);
        long double c = CalCoe(0, 1, 0, 0);
        long double d = CalCoe(0, 1, 1, 0);
        int assignedK = maxK_ - leftK;
        int left = 0;
        int right = leftK;

        while (true)
        {
            int step = (right - left) / 20 + 1;
            if (step <= 5)
                break;

            long double best = 0.0;
            int bestK = left;
            for (int tk = left; tk <= right; tk += step)
            {
                long double cur = MaxQuadraticValue(a, b, c, d, tk);
                long double relaxed = std::expl(-beta_ * (long double)(tk + assignedK)) * cur;
                if (relaxed > best)
                {
                    best = relaxed;
                    bestK = tk;
                }
            }
            left = std::max(left, bestK - step);
            right = std::min(right, bestK + step);
        }

        for (int tk = left; tk <= right; ++tk)
        {
            long double cur = MaxQuadraticValue(a, b, c, d, tk);
            long double relaxed = std::expl(-beta_ * (long double)(tk + assignedK)) * cur;
            if (relaxed > maxRes_)
            {
                maxRes_ = relaxed;
                kWithMaxRes_ = tk + assignedK;
            }
        }
    }

    static long double MaxQuadraticValue(long double a, long double b, long double c, long double d, int tk)
    {
        long double best = std::max(b * tk + d, c * tk + d);
        if (a != 0.0)
        {
            long double t = (a * tk + b - c) / (2.0L * a);
            int x1 = (int)t;
            int x2 = x1 + 1;
            if (0 < x1 && x1 < tk)
                best = std::max(best, a * x1 * (tk - x1) + b * x1 + c * (tk - x1) + d);
            if (0 < x2 && x2 < tk)
                best = std::max(best, a * x2 * (tk - x2) + b * x2 + c * (tk - x2) + d);
        }
        return best;
    }

    long double CalCoe(int curId, int firstValue, int secondValue, int mask)
    {
        if (curId >= n_)
            return (mask >= 0 && mask < (int)te_.size()) ? (long double)te_[mask] : 1.0L;
        if (isPublic_[curId])
            return CalCoe(curId + 1, firstValue, secondValue, mask * 2 + 1);
        if (curId == deletedId_)
            return CalCoe(curId + 1, firstValue, secondValue, mask * 2);
        if (curId == firstId_)
            return CalCoe(curId + 1, firstValue, secondValue, mask * 2 + firstValue);
        if (curId == secondId_)
            return CalCoe(curId + 1, firstValue, secondValue, mask * 2 + secondValue);
        return CalCoe(curId + 1, firstValue, secondValue, mask * 2) * tupleDis_[curId] +
               CalCoe(curId + 1, firstValue, secondValue, mask * 2 + 1);
    }
};

class DOStyleRRSCalculator
{
public:
    DOStyleRRSCalculator(const vector<Table> &inTables,
                         const vector<int> &inParent,
                         int inRoot,
                         const vector<int> &inJoinColInParent,
                         const vector<int> &inJoinColInChild,
                         double inEpsilon,
                         double inDelta)
        : tables(inTables),
          parent(inParent),
          root(inRoot),
          joinColInParent(inJoinColInParent),
          joinColInChild(inJoinColInChild),
          epsilon(inEpsilon),
          delta(inDelta),
          num_of_relations((int)inTables.size()),
          children(buildChildrenList(inParent, (int)inTables.size()))
    {
        if (epsilon <= 0.0)
            epsilon = 1.0;
        if (delta <= 0.0 || delta >= 1.0)
            delta = 1e-9;
        double one_over_delta = (2.0 * std::exp(epsilon / 2.0)) / delta;
        beta = epsilon / (2.0 * std::log(one_over_delta));
    }

    long long CollectRRS()
    {
        if (num_of_relations == 0 || root < 0 || root >= num_of_relations || beta <= 0.0)
            return 0;

        if (!BuildQuerySpecificConfig())
            return 0;
        BuildDegreeInfo();
        vector<long long> tes = ComputeThirdRelaxedTEs();

        return DOStyleRSCalculator(num_of_relations, tes, beta).RunResidual();
    }

private:
    const vector<Table> &tables;
    const vector<int> &parent;
    int root;
    const vector<int> &joinColInParent;
    const vector<int> &joinColInChild;
    double epsilon;
    double delta;
    double beta = 0.0;
    int num_of_relations;
    vector<vector<int>> children;
    vector<int> relationAttrMask;
    vector<vector<int>> attrColumn;
    vector<vector<int>> parentMatrix;
    map<int, vector<int>> possibleRelaxations;
    map<pair<int, int>, long long> secondRelaxedTEs;
    vector<vector<long long>> degreeInfo;

    bool BuildQuerySpecificConfig()
    {
        QuerySpecificRRSConfig cfg;
        if (!buildQuerySpecificRRSConfig(parent, joinColInParent, joinColInChild, cfg))
            return false;
        if (cfg.relationCount != num_of_relations)
            return false;
        relationAttrMask = std::move(cfg.relationAttrMask);
        attrColumn = std::move(cfg.attrColumn);
        parentMatrix = std::move(cfg.parentMatrix);
        possibleRelaxations = std::move(cfg.possibleRelaxations);
        secondRelaxedTEs = std::move(cfg.secondRelaxedTEs);
        return true;
    }

    void BuildDegreeInfo()
    {
        int totalMasks = 1 << num_of_relations;
        degreeInfo.assign(num_of_relations, vector<long long>(totalMasks, 1));
        for (int r = 0; r < num_of_relations; ++r)
        {
            int incident = relationAttrMask[r];
            for (int mask = 0; mask < totalMasks; ++mask)
            {
                if ((mask & ~incident) != 0)
                    continue;
                vector<int> cols;
                for (int a = 0; a < num_of_relations; ++a)
                    if ((mask & (1 << a)) != 0)
                        cols.push_back(attrColumn[r][a]);
                degreeInfo[r][mask] = std::max(1LL, maxFrequencyOnColumns(tables[r], cols));
            }
        }
    }

    int NeighborOnPath(int from, int target) const
    {
        if (from == target)
            return from;
        if (from < 0 || target < 0 || from >= (int)parentMatrix.size() || target >= (int)parentMatrix[from].size())
            return -1;
        return parentMatrix[from][target];
    }

    int BoundaryAttrMask(int subsetMask) const
    {
        int subsetAttrs = 0;
        int complementAttrs = 0;
        for (int r = 0; r < num_of_relations; ++r)
        {
            if ((subsetMask & (1 << r)) != 0)
                subsetAttrs |= relationAttrMask[r];
            else
                complementAttrs |= relationAttrMask[r];
        }
        return subsetAttrs & complementAttrs;
    }

    vector<long long> ComputeThirdRelaxedTEs()
    {
        int totalMasks = 1 << num_of_relations;
        vector<long long> tes(totalMasks, 1);
        for (int subset = 1; subset < totalMasks - 1; ++subset)
        {
            int boundary = BoundaryAttrMask(subset);
            long long chosen = LLONG_MAX;
            auto relaxIt = possibleRelaxations.find(subset);
            vector<int> relaxations = (relaxIt == possibleRelaxations.end())
                                          ? vector<int>{0}
                                          : relaxIt->second;
            for (int relaxedAttrs : relaxations)
            {
                int effectiveBoundary = boundary & ~relaxedAttrs;
                long long bestForRelaxation = 1;
                for (int i = 0; i < num_of_relations; ++i)
                {
                    if ((subset & (1 << i)) != 0)
                        continue;
                    long long cur = 1;
                    for (int j = 0; j < num_of_relations; ++j)
                    {
                        if ((subset & (1 << j)) == 0)
                            continue;
                        int neighbor = NeighborOnPath(j, i);
                        if (neighbor < 0)
                            continue;
                        int degAttrs = relationAttrMask[j] & relationAttrMask[neighbor];
                        degAttrs |= effectiveBoundary & relationAttrMask[j];
                        cur = satMul(cur, degreeInfo[j][degAttrs]);
                    }
                    bestForRelaxation = std::max(bestForRelaxation, cur);
                }
                chosen = std::min(chosen, bestForRelaxation);
            }
            if (chosen == LLONG_MAX)
                chosen = 1;
            tes[ToRSIndex(subset)] = chosen;
        }
        return tes;
    }

    int ToRSIndex(int subsetMask) const
    {
        int idx = 0;
        for (int r = 0; r < num_of_relations; ++r)
            idx = idx * 2 + (((subsetMask & (1 << r)) != 0) ? 1 : 0);
        return idx;
    }
};

static long long computeQuerySpecificRRSSensitivity(const vector<Table> &tables, const vector<int> &parent,
                                                    int root, const vector<int> &joinColInParent,
                                                    const vector<int> &joinColInChild,
                                                    double epsilon, double deltaPrivacy,
                                                    rrs::RRSEngine::Breakdown *breakdown)
{
    // RRS (relaxed-residual sensitivity) following the construction of Lemma 3.5
    // in the paper: build a smooth upper bound That_E on each maximum boundary,
    // then feed {That_E} into the residual-sensitivity calculator (Eq. (2)/(3))
    // to obtain the smooth local sensitivity S(I). OUT = |Q(I)| + f_{eps,delta}*S(I).
    //
    // RRSEngine supports any acyclic single-attribute-edge tree encoded by
    // parent[] and the per-edge join columns. Query loaders that fold composite
    // edges into one encoded column, such as TPCH Q9, can therefore use the same
    // path as SQL18/SQL85 without a separate hand-written RRS model.
    //
    // The actual RRS construction lives in include/RRSSensitivity.h (pure, host-
    // testable; validated by main/RRSSensitivityTest.cpp). It builds the real
    // maximum boundaries (RST) for free-connex subsets, falls back to the degree
    // product (EST) elsewhere via a consistency-preserving hybrid, runs the
    // residual construction, and returns min(global, S_RRS, S_EST).
    (void)root;
    rrs::RRSEngine engine(tables, parent, joinColInParent, joinColInChild, epsilon, deltaPrivacy);
    rrs::RRSEngine::Breakdown bd = engine.ComputeBreakdown();
    if (breakdown)
        *breakdown = bd;
    if (!bd.ok)
        return 0;
    if (bd.tooExpensive)
        return -1;
    return bd.finalS;
}

static DOPaddingStats computeOutputSizeStats(vector<Table> tables, const vector<int> &parent, int root,
                                             const vector<int> &joinColInParent, const vector<int> &joinColInChild)
{
    DOPaddingStats stats;
    int n = (int)tables.size();
    if (n == 0 || root < 0 || root >= n || tables[root].empty())
        return stats;

    vector<vector<int>> children = buildChildrenList(parent, n);
    vector<vector<int>> levels = buildLevels(children, root);
    vector<vector<vector<int>>> childCounts(n);
    vector<vector<long long>> delta(n);
    for (int u = 0; u < n; ++u)
        delta[u].assign(tables[u].size(), 1);

    for (int d = (int)levels.size() - 1; d >= 0; --d)
    {
        for (int p : levels[d])
        {
            int k = (int)children[p].size();
            if (k == 0)
                continue;

            childCounts[p].assign(k, vector<int>());
            for (int j = 0; j < k; ++j)
            {
                int c = children[p][j];
                int valueCol = children[c].empty() ? -1 : (int)tables[c][0].size() - 1;
                ParallelSemiJoin semiJoin(tables[p], joinColInParent[c], tables[c], joinColInChild[c], valueCol);
                semiJoin.executeValueOnly();
                childCounts[p][j] = semiJoin.getResultValues();
            }

            delta[p].assign(tables[p].size(), 0);
            for (int i = 0; i < (int)tables[p].size(); ++i)
            {
                long long product = 1;
                for (int j = 0; j < k; ++j)
                    product = satMul(product, childCounts[p][j][i]);
                delta[p][i] = product;
            }

            for (int i = 0; i < (int)tables[p].size(); ++i)
            {
                int oldSize = (int)tables[p][i].size();
                tables[p][i].resize(oldSize + k + 1);
                for (int j = 0; j < k; ++j)
                    tables[p][i][oldSize + j] = childCounts[p][j][i];
                tables[p][i][oldSize + k] = (delta[p][i] > INT_MAX) ? INT_MAX : (int)delta[p][i];
            }
        }
    }

    for (long long v : delta[root])
        stats.exactRows = satAdd(stats.exactRows, v);

    vector<vector<long long>> outside(n);
    for (int u = 0; u < n; ++u)
        outside[u].assign(tables[u].size(), 0);
    outside[root].assign(tables[root].size(), 1);

    for (int d = 0; d < (int)levels.size(); ++d)
    {
        for (int p : levels[d])
        {
            for (int i = 0; i < (int)tables[p].size(); ++i)
                stats.sensitivity = std::max(stats.sensitivity, satMul(outside[p][i], delta[p][i]));

            int k = (int)children[p].size();
            for (int childIdx = 0; childIdx < k; ++childIdx)
            {
                int c = children[p][childIdx];
                unordered_map<int, long long> byKey;
                byKey.reserve(tables[p].size() * 2 + 1);
                for (int i = 0; i < (int)tables[p].size(); ++i)
                {
                    long long factor = outside[p][i];
                    for (int s = 0; s < k; ++s)
                    {
                        if (s == childIdx)
                            continue;
                        factor = satMul(factor, childCounts[p][s][i]);
                    }
                    if (factor == 0)
                        continue;
                    int key = tables[p][i][joinColInParent[c]];
                    byKey[key] = satAdd(byKey[key], factor);
                }

                for (int r = 0; r < (int)tables[c].size(); ++r)
                {
                    int key = tables[c][r][joinColInChild[c]];
                    auto it = byKey.find(key);
                    if (it != byKey.end())
                        outside[c][r] = satAdd(outside[c][r], it->second);
                }
            }
        }
    }

    for (int u = 0; u < n; ++u)
        for (int i = 0; i < (int)tables[u].size(); ++i)
            stats.sensitivity = std::max(stats.sensitivity, satMul(outside[u][i], delta[u][i]));

    return stats;
}

long long computeAcyclicJoinOutputSize(vector<Table> tables, const vector<int> &parent, int root,
                                       const vector<int> &joinColInParent, const vector<int> &joinColInChild)
{
    return computeOutputSizeStats(std::move(tables), parent, root, joinColInParent, joinColInChild).exactRows;
}

DOPaddingStats computeDOPaddingStats(vector<Table> tables, const vector<int> &parent, int root,
                                     const vector<int> &joinColInParent, const vector<int> &joinColInChild,
                                     int explicitOutputRows,
                                     double epsilon,
                                     double deltaPrivacy)
{
    rrs::RRSEngine::Breakdown rrsBreakdown;
    long long rrsSensitivity = computeQuerySpecificRRSSensitivity(tables, parent, root, joinColInParent, joinColInChild,
                                                                  epsilon, deltaPrivacy, &rrsBreakdown);
    bool rrsTooExpensive = (rrsSensitivity < 0); // -1: supported shape, intractable at these (eps,delta)
    if (rrsTooExpensive)
        rrsSensitivity = 0;
    DOPaddingStats stats = computeOutputSizeStats(std::move(tables), parent, root, joinColInParent, joinColInChild);
    stats.rrsTooExpensive = rrsTooExpensive;
    stats.rrsSEST = rrsBreakdown.sEST;
    stats.rrsSRRS = rrsBreakdown.sRRS;
    stats.rrsSHybrid = rrsBreakdown.sHybrid;
    stats.rrsGS = rrsBreakdown.gs;
    stats.rrsMaxK = rrsBreakdown.maxK;
    stats.rrsEstLeaves = rrsBreakdown.estLeaves;
    stats.rrsMaxEstTE = rrsBreakdown.maxEstTE;
    stats.rrsMaxEstSubset = rrsBreakdown.maxEstSubset;
    stats.rrsMaxRRSTE = rrsBreakdown.maxRRSTE;
    stats.rrsMaxRRSSubset = rrsBreakdown.maxRRSSubset;
    stats.rrsMaxHybridTE = rrsBreakdown.maxHybridTE;
    stats.rrsMaxHybridSubset = rrsBreakdown.maxHybridSubset;
    if (rrsSensitivity > 0)
    {
        stats.sensitivity = rrsSensitivity;
        stats.rrsAvailable = true;
    }

    long long target = stats.exactRows;
    if (explicitOutputRows > 0)
        target = std::max(target, (long long)explicitOutputRows);
    else
    {
        if (epsilon <= 0.0)
            epsilon = 1.0;
        if (deltaPrivacy <= 0.0 || deltaPrivacy >= 1.0)
            deltaPrivacy = 1e-9;
        stats.factor = (2.0 * std::exp(2.0) / epsilon) *
                       (1.0 + std::log(1.0 + ((2.0 * std::exp(epsilon) -
                                                2.0 * std::exp(epsilon / 2.0)) /
                                               deltaPrivacy)));
        long double noise = (long double)stats.factor * (long double)std::max(1LL, stats.sensitivity);
        long long paddedNoise = LLONG_MAX;
        if (noise < (long double)LLONG_MAX)
            paddedNoise = (long long)std::ceil(noise);
        target = satAdd(target, paddedNoise);
    }
    if (target < stats.exactRows)
        target = stats.exactRows;
    if (target < 1)
        target = 1;
    stats.protectedRowsExact = target;
    stats.paddingRowsExact = target > stats.exactRows ? target - stats.exactRows : 0;
    stats.protectedRows = (target > INT_MAX) ? INT_MAX : (int)target;
    if (sgxProfileEnabled())
    {
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "  [DO output padding] exactRows=%lld sensitivity=%lld protectedRowsExact=%lld paddingRowsExact=%lld materializedCap=%d explicit=%d epsilon=%.6g delta=%.6g",
                 stats.exactRows, stats.sensitivity, stats.protectedRowsExact, stats.paddingRowsExact,
                 stats.protectedRows, explicitOutputRows, epsilon, deltaPrivacy);
        sgxProfilePrint(buf);
        if (rrsBreakdown.ok)
        {
            snprintf(buf, sizeof(buf),
                     "  [DO RRS breakdown] sEST=%lld sRRS=%lld sHybrid=%lld GS=%lld final=%lld maxK=%lld estLeaves=%.3g rstAvailable=%d rstUsed=%d tooExpensive=%d maxEST=%lld@0x%x maxRRS=%lld@0x%x maxHybrid=%lld@0x%x",
                     stats.rrsSEST, stats.rrsSRRS, stats.rrsSHybrid, stats.rrsGS, stats.sensitivity,
                     stats.rrsMaxK, stats.rrsEstLeaves, rrsBreakdown.rstAvailable,
                     rrsBreakdown.rstUsed, rrsBreakdown.tooExpensive ? 1 : 0,
                     stats.rrsMaxEstTE, stats.rrsMaxEstSubset,
                     stats.rrsMaxRRSTE, stats.rrsMaxRRSSubset,
                     stats.rrsMaxHybridTE, stats.rrsMaxHybridSubset);
            sgxProfilePrint(buf);
        }
    }
    return stats;
}

int computeDOProtectedOutputSize(vector<Table> tables, const vector<int> &parent, int root,
                                 const vector<int> &joinColInParent, const vector<int> &joinColInChild,
                                 int explicitOutputRows,
                                 double epsilon,
                                 double deltaPrivacy)
{
    return computeDOPaddingStats(std::move(tables), parent, root, joinColInParent, joinColInChild,
                                 explicitOutputRows, epsilon, deltaPrivacy)
        .protectedRows;
}

Table bottomUpSemiJoin(vector<Table> &tables, const vector<int> &parent, int root,
                       const vector<int> &joinColInParent, const vector<int> &joinColInChild,
                       int protectedOutputRows)
{
    g_lastBottomUpParallelMs = 0.0;
    g_lastBottomUpSemiJoinMs = 0.0;
    g_lastBottomUpRootExpandMs = 0.0;
    int n = tables.size();

    /*
    Step 1: Bottom-up semi-join and compute delta_c and u[c]
    */

    // Build children list from parent array
    vector<vector<int>> children = buildChildrenList(parent, n);

    // BFS from root, grouped by depth. Bottom-up processing later runs one
    // depth level at a time, so independent subtrees at the same depth can run
    // concurrently after their children have already been processed.
    vector<vector<int>> levels = buildLevels(children, root);

#ifndef SGX_ENCLAVE_BUILD
    vector<double> nodeWorkMs(n, 0.0);
#endif

    auto processNode = [&](int p, bool parallelChildren)
    {
        if (children[p].empty())
            return;

#ifndef SGX_ENCLAVE_BUILD
        auto nodeStart = high_resolution_clock::now();
#endif
        vector<vector<int>> semiJoinValues(children[p].size());
        if (parallelChildren)
        {
#pragma omp parallel for schedule(static)
            for (int i = 0; i < (int)children[p].size(); i++)
            {
                int c = children[p][i];
                int valueCol = children[c].empty() ? -1 : (int)tables[c][0].size() - 1;
                double edgeStart = sgxProfileNowMs();
                ParallelSemiJoin semiJoin(tables[p], joinColInParent[c], tables[c], joinColInChild[c], valueCol);
                semiJoin.executeValueOnly();
                semiJoinValues[i] = semiJoin.getResultValues();
                double edgeEnd = sgxProfileNowMs();
                if (sgxProfileEnabled())
                {
                    char buf[192];
                    snprintf(buf, sizeof(buf),
                             "  [BottomUp edge p=%d c=%d] semiJoinTotal=%.2f ms",
                             p, c, edgeEnd - edgeStart);
                    sgxProfilePrint(buf);
                }
            }
        }
        else
        {
            for (int i = 0; i < (int)children[p].size(); i++)
            {
                int c = children[p][i];
                int valueCol = children[c].empty() ? -1 : (int)tables[c][0].size() - 1;
                double edgeStart = sgxProfileNowMs();
                ParallelSemiJoin semiJoin(tables[p], joinColInParent[c], tables[c], joinColInChild[c], valueCol);
                semiJoin.executeValueOnly();
                semiJoinValues[i] = semiJoin.getResultValues();
                double edgeEnd = sgxProfileNowMs();
                if (sgxProfileEnabled())
                {
                    char buf[192];
                    snprintf(buf, sizeof(buf),
                             "  [BottomUp edge p=%d c=%d] semiJoinTotal=%.2f ms",
                             p, c, edgeEnd - edgeStart);
                    sgxProfilePrint(buf);
                }
            }
        }

        double updateStart = sgxProfileNowMs();
#pragma omp parallel for schedule(static)
        for (int i = 0; i < (int)tables[p].size(); i++)
        {
            int oldSize = tables[p][i].size();
            int k = (int)children[p].size();
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
        double updateEnd = sgxProfileNowMs();
        if (sgxProfileEnabled())
        {
            char buf[192];
            snprintf(buf, sizeof(buf),
                     "  [BottomUp node p=%d] updateParent=%.2f ms rows=%d children=%d",
                     p, updateEnd - updateStart, (int)tables[p].size(), (int)children[p].size());
            sgxProfilePrint(buf);
        }
#ifndef SGX_ENCLAVE_BUILD
        nodeWorkMs[p] = duration<double, milli>(high_resolution_clock::now() - nodeStart).count();
#endif
    };

    // Process from leaves' parents up to the root. Nodes at the same depth own
    // disjoint subtrees, so they can run in parallel.
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
#ifdef SGX_ENCLAVE_BUILD
            // In SGX, running several large semi-joins for the same parent at
            // once can multiply transient memory and cause EPC paging. Run the
            // edges one by one so each semi-join can use the full OpenMP team.
            processNode(internalNodes[0], false);
#else
            processNode(internalNodes[0], true);
#endif
        }
        else
        {
#pragma omp parallel for schedule(static)
            for (int i = 0; i < (int)internalNodes.size(); ++i)
                processNode(internalNodes[i], false);
        }
    }

#ifndef SGX_ENCLAVE_BUILD
    vector<double> criticalMs(n, 0.0);
    for (int d = (int)levels.size() - 1; d >= 0; --d)
    {
        for (int u : levels[d])
        {
            double slowestChild = 0.0;
            for (int c : children[u])
                slowestChild = max(slowestChild, criticalMs[c]);
            criticalMs[u] = slowestChild + nodeWorkMs[u];
        }
    }

    auto finalStart = high_resolution_clock::now();
#endif

    /*
    Step 2: Assign starting position pos to each tuple R_root
    */
    double profileFinalStart = sgxProfileNowMs();
    setPrimitiveProfilePhase(PrimitivePhaseOursRootExpand);
    vector<int> delta, flag(tables[root].size(), 1);
    delta.resize(tables[root].size());
    flag[0] = 0;
#pragma omp parallel for
    for (int i = 0; i < (int)tables[root].size(); i++)
    {
        int product = tables[root][i].back();
        delta[i] = product;
    }
    // Compute prefix sum of delta to get pos (1-based: pos[i] = 1 + sum of delta[0..i-1])
    vector<int> Pos = Aggtree(delta, flag, 1).PrefixTreeRun();
    double profileAfterPrefix = sgxProfileNowMs();

    /*
    Step 3: Expand R_root
    */
    Table R_root;
    vector<int> degree;
    R_root.reserve(tables[root].size());
    degree.reserve(tables[root].size());
    for (int i = 0; i < (int)tables[root].size(); i++)
    {
        const vector<int> &src = tables[root][i];
        int product = src.back();
        if (product == 0)
            continue;
        vector<int> row(src.size() + 3);
        for (int j = 0; j < (int)src.size(); ++j)
            row[j] = src[j];
        degree.push_back(product);
        row[src.size()] = Pos[i];     // b
        row[src.size() + 1] = 0;      // P, filled after expansion
        row[src.size() + 2] = 1;      // lambda
        R_root.push_back(std::move(row));
    }
    double profileAfterBuildRoot = sgxProfileNowMs();

    Table C;
    if (!R_root.empty())
    {
        ObliViatorExpand oExpand(std::move(R_root), std::move(degree));
        C = oExpand.getResult();
    }
    double profileAfterExpand = sgxProfileNowMs();

    int realRows = (int)C.size();
    if (C.empty() && protectedOutputRows > 0 && !tables[root].empty())
    {
        int rowWidth = (int)tables[root][0].size() + 3;
        C.assign(protectedOutputRows, vector<int>(rowWidth, 0));
    }

    const int pCol = C.empty() ? -1 : (int)C[0].size() - 2;
#pragma omp parallel for
    for (int i = 0; i < (int)C.size(); i++)
        C[i][pCol] = i + 1; // row index P

    if (protectedOutputRows > realRows && !C.empty())
    {
        int rowWidth = (int)C[0].size();
        int childCount = (int)children[root].size();
        int origCols = rowWidth - childCount - 4;
        int deltaCol = origCols + childCount;
        int bCol = deltaCol + 1;
        int lambdaCol = deltaCol + 3;
        C.resize(protectedOutputRows, vector<int>(rowWidth, 0));
#pragma omp parallel for schedule(static)
        for (int i = realRows; i < protectedOutputRows; ++i)
        {
            for (int k = 0; k < origCols; ++k)
                C[i][k] = DO_DUMMY_VALUE;
            C[i][deltaCol] = 0;
            C[i][bCol] = i + 1;
            C[i][pCol] = i + 1;
            C[i][lambdaCol] = 1;
        }
    }
    double profileAfterAppend = sgxProfileNowMs();

    if (sgxProfileEnabled())
    {
        char buf[320];
        snprintf(buf, sizeof(buf),
                 "  [BottomUp rootExpand] prefix=%.2f ms buildRoot=%.2f ms expand=%.2f ms appendState=%.2f ms outputRows=%d",
                 profileAfterPrefix - profileFinalStart,
                 profileAfterBuildRoot - profileAfterPrefix,
                 profileAfterExpand - profileAfterBuildRoot,
                 profileAfterAppend - profileAfterExpand,
                 (int)C.size());
        sgxProfilePrint(buf);
    }
    g_lastBottomUpRootExpandMs = profileAfterAppend - profileFinalStart;

#ifndef SGX_ENCLAVE_BUILD
    double finalMs = duration<double, milli>(high_resolution_clock::now() - finalStart).count();
    g_lastBottomUpSemiJoinMs = criticalMs[root];
    g_lastBottomUpRootExpandMs = finalMs;
    g_lastBottomUpParallelMs = g_lastBottomUpSemiJoinMs + g_lastBottomUpRootExpandMs;
    printf("  [BottomUp parallel-estimate] semijoinCritical=%.2f ms  rootExpand=%.2f ms  total=%.2f ms\n",
           g_lastBottomUpSemiJoinMs, g_lastBottomUpRootExpandMs, g_lastBottomUpParallelMs);
#endif

    return C;
}
