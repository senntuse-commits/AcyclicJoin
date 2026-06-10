#pragma once
//
// Relaxed-Residual Sensitivity (RRS) for DO multi-way joins.
//
// Pure computation, no SGX / OpenMP dependencies, so it can be unit-tested on
// the host and reused inside the enclave. Faithful to:
//   Wu, Dong, Hu. "Differentially Oblivious Multi-way Join", SIGMOD 2026.
//   - Lemma 3.5 / Eq. (2),(3): residual construction of the smooth local
//     sensitivity S(I) from smooth upper bounds T_hat_E on maximum boundaries.
//   - Section 4.2/4.3: RST (real maximum boundary) and EST (degree product).
//   - Algorithm 2 (HybridConstruct) with the Section 5.3 heuristic "drop
//     attributes only when necessary": for each subset E we keep EST at the
//     unrelaxed boundary, and only use a relaxed exact RST candidate when it is
//     both consistency-compatible and strictly tighter than EST.
//
// Validity: every T_hat_E we build is a smooth upper bound on the maximum
// boundary (Def. 3.4), so S(I) is a smooth upper bound on the local sensitivity
// and OUT = |Q(I)| + f_{eps,delta} * S(I) is a valid (eps,delta)-DP upper bound.
// We additionally take the min over {global sensitivity, EST construction, RRS
// construction}; the pointwise min of smooth upper bounds is again a smooth
// upper bound, so the result stays valid while being tighter.
//
// Join model: an acyclic "single-attribute-edge" tree join. The join tree is
// the given parent[] array; each tree edge (child c, parent[c]) carries one
// shared attribute (id = c), present in exactly those two relations. sql18,
// sql85 and the ternary-L3 trees all have this form.

#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <cmath>
#include <climits>
#include <string>
#include <utility>

namespace rrs
{
using std::vector;

// ---------------------------------------------------------------------------
// Saturating arithmetic (avoid overflow blowing up the noise scale).
// ---------------------------------------------------------------------------
inline long long satMulLL(long long a, long long b)
{
    if (a <= 0 || b <= 0)
        return 0;
    if (a > LLONG_MAX / b)
        return LLONG_MAX;
    return a * b;
}

inline long long satAddLL(long long a, long long b)
{
    if (a > 0 && b > 0 && a > LLONG_MAX - b)
        return LLONG_MAX;
    return a + b;
}

inline int popcountMask(int x)
{
    int c = 0;
    while (x != 0)
    {
        x &= x - 1;
        ++c;
    }
    return c;
}

// ---------------------------------------------------------------------------
// Residual-sensitivity calculator (faithful port of DOJoin/rs_calculator.h).
// Given the smooth maximum-boundary bounds te_[mask] (indexed by relation
// subset, MSB = relation 0), returns S(I) = max_k e^{-beta k} LShat_k(I).
// ---------------------------------------------------------------------------
class ResidualCalculator
{
public:
    ResidualCalculator(int n, const vector<long long> &inTE, double inBeta)
        : n_(n), beta_(inBeta), te_(inTE), tupleDis_(n, 0) {}

    long long Run()
    {
        m_ = n_; // all relations private
        if (m_ <= 1 || beta_ <= 0.0)
            return 1;
        maxK_ = (int)(((double)m_ - 1.0) / beta_) + n_;
        maxRes_ = 0.0;
        for (int i = 0; i < n_; ++i)
        {
            deletedId_ = i;
            CallMaxTE();
        }
        if (maxRes_ > (long double)LLONG_MAX)
            return LLONG_MAX;
        return std::max(1LL, (long long)std::ceil((double)maxRes_));
    }

private:
    int n_;
    double beta_;
    vector<long long> te_;
    vector<int> tupleDis_;
    int m_ = 0;
    int maxK_ = 0;
    int deletedId_ = 0;
    int firstId_ = 0;
    int secondId_ = 0;
    long double maxRes_ = 0.0;

    void CallMaxTE()
    {
        std::fill(tupleDis_.begin(), tupleDis_.end(), 0);
        int left = m_ - 1;
        firstId_ = 0;
        secondId_ = 0;
        for (int i = 0; i < n_; ++i)
            if (i != deletedId_)
            {
                if (left == 2)
                    firstId_ = i;
                if (left == 1)
                    secondId_ = i;
                --left;
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
        if (curId == deletedId_)
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
        int left = 0, right = leftK;
        while (true)
        {
            int step = (right - left) / 20 + 1;
            if (step <= 5)
                break;
            long double best = 0.0;
            int bestK = left;
            for (int tk = left; tk <= right; tk += step)
            {
                long double relaxed = std::expl(-beta_ * (long double)(tk + assignedK)) *
                                      MaxQuadratic(a, b, c, d, tk);
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
            long double relaxed = std::expl(-beta_ * (long double)(tk + assignedK)) *
                                  MaxQuadratic(a, b, c, d, tk);
            if (relaxed > maxRes_)
                maxRes_ = relaxed;
        }
    }

    static long double MaxQuadratic(long double a, long double b, long double c, long double d, int tk)
    {
        long double best = std::max(b * tk + d, c * tk + d);
        if (a != 0.0)
        {
            long double t = (a * tk + b - c) / (2.0L * a);
            int x1 = (int)t, x2 = x1 + 1;
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

// ---------------------------------------------------------------------------
// RRS engine.
// ---------------------------------------------------------------------------
class RRSEngine
{
public:
    using Table = vector<vector<int>>;

    RRSEngine(const vector<Table> &tables,
              const vector<int> &parent,
              const vector<int> &joinColInParent,
              const vector<int> &joinColInChild,
              double epsilon,
              double delta)
        : tables_(tables), parent_(parent),
          joinColInParent_(joinColInParent), joinColInChild_(joinColInChild),
          m_((int)tables.size())
    {
        if (epsilon <= 0.0)
            epsilon = 1.0;
        if (delta <= 0.0 || delta >= 1.0)
            delta = 1e-9;
        beta_ = epsilon / (2.0 * std::log((2.0 * std::exp(epsilon / 2.0)) / delta));
    }

    struct Breakdown
    {
        bool ok = false;          // shape supported and model built
        bool tooExpensive = false; // residual enumeration would be intractable
        long long maxK = 0;
        double estLeaves = 0.0;
        long long sEST = 0;  // EST-only construction residual
        long long sRRS = 0;  // hybrid RST/EST construction residual
        long long sHybrid = 0; // original-style relaxed TE propagation residual
        long long gs = 0;    // global-sensitivity ceiling
        long long finalS = 0; // min of the above (>= 1)
        int rstAvailable = 0;
        int rstUsed = 0;
        long long maxEstTE = 0;
        int maxEstSubset = 0;
        long long maxRRSTE = 0;
        int maxRRSSubset = 0;
        long long maxHybridTE = 0;
        int maxHybridSubset = 0;
    };

    // Budget on the number of leaf evaluations in the residual enumeration. The
    // calculator is ~ n * 2 * C(maxK, m-3) and maxK ~ m*ln(1/delta)/epsilon, so
    // small epsilon / large m blows up. Above the budget we refuse (caller asks
    // the user to raise epsilon) instead of hanging.
    static constexpr double kResidualLeafBudget = 5e9;

    // Returns S(I) (> 0); 0 if the shape is unsupported; -1 if supported but the
    // residual computation would be intractable at these (epsilon, delta).
    long long Compute()
    {
        Breakdown b = ComputeBreakdown();
        if (!b.ok)
            return 0;
        if (b.tooExpensive)
            return -1;
        return b.finalS;
    }

    // Estimated leaf count of one residual enumeration, ~ n * 2 * maxK^(m-3)/(m-3)!.
    static double EstimateResidualLeaves(long long maxK, int m)
    {
        int free = m - 3;
        if (free <= 0 || maxK <= 1)
            return (double)m * 2.0;
        double fact = 1.0;
        for (int i = 2; i <= free; ++i)
            fact *= i;
        return (double)m * 2.0 * std::pow((double)maxK, (double)free) / fact;
    }

    Breakdown ComputeBreakdown()
    {
        Breakdown out;
        if (m_ <= 0 || beta_ <= 0.0)
            return out;
        if (!BuildModel())
            return out;

        // Cheap feasibility gate before any heavy work (degree info / residual).
        out.maxK = (long long)(((double)m_ - 1.0) / beta_) + m_;
        out.estLeaves = EstimateResidualLeaves(out.maxK, m_);
        if (out.estLeaves > kResidualLeafBudget)
        {
            out.ok = true;
            out.tooExpensive = true;
            return out;
        }

        BuildAdjacencyAndHops();
        BuildDegreeInfo();

        int total = 1 << m_;
        vector<long long> estTE(total, 1);  // best relaxed EST construction
        vector<long long> rrsTE(total, 1);  // hybrid (RST where free-connex+consistent)
        vector<long long> hybridTE(total, 1); // original-style relaxed propagation
        vector<long long> rstVal(total, -1); // best relaxed real T_E if free-connex, else -1
        vector<char> freeConnex(total, 0);
        std::map<std::pair<int, int>, long long> firstTE;
        std::map<std::pair<int, int>, long long> thirdTE;

        for (int E = 1; E < total - 1; ++E)
        {
            int boundary = BoundaryAttrMask(E);
            long long est = LLONG_MAX;
            long long rst = LLONG_MAX;
            vector<int> relaxations = RelaxationMasks(boundary);
            for (int relaxed : relaxations)
            {
                int effectiveBoundary = boundary & ~relaxed;
                std::pair<int, int> key = std::make_pair(E, relaxed);
                thirdTE[key] = ComputeEST(E, effectiveBoundary);
                est = std::min(est, thirdTE[key]);
                long long curRst = ComputeRST(E, effectiveBoundary);
                firstTE[key] = curRst;
                if (curRst >= 0)
                    rst = std::min(rst, curRst);
            }
            if (est == LLONG_MAX)
                est = 1;
            estTE[ToRSIndex(E)] = est;
            if (est > out.maxEstTE)
            {
                out.maxEstTE = est;
                out.maxEstSubset = E;
            }
            if (rst != LLONG_MAX && rst < est)
            {
                freeConnex[E] = 1;
                rstVal[E] = rst;
                ++out.rstAvailable;
            }
        }

        // Hybrid construction in decreasing subset size: use RST only if this E
        // is free-connex AND every immediate superset also uses RST (Case 2).
        vector<char> useRST(total, 0);
        vector<int> order;
        order.reserve(total);
        for (int E = 1; E < total - 1; ++E)
            order.push_back(E);
        std::sort(order.begin(), order.end(), [](int a, int b) {
            int pa = popcountMask(a), pb = popcountMask(b);
            if (pa != pb)
                return pa > pb;
            return a < b;
        });
        for (int E : order)
        {
            int idx = ToRSIndex(E);
            bool ok = freeConnex[E] && rstVal[E] < estTE[idx];
            if (ok)
            {
                for (int r = 0; r < m_ && ok; ++r)
                {
                    if ((E & (1 << r)) != 0)
                        continue;
                    int sup = E | (1 << r); // immediate superset
                    if (sup == total - 1)
                        continue; // [m] itself is out of range -> no constraint
                    if (!useRST[sup])
                        ok = false;
                }
            }
            useRST[E] = ok;
            if (ok)
                ++out.rstUsed;
            rrsTE[idx] = ok ? rstVal[E] : estTE[idx];
            if (rrsTE[idx] > out.maxRRSTE)
            {
                out.maxRRSTE = rrsTE[idx];
                out.maxRRSSubset = E;
            }
        }

        out.sEST = ResidualCalculator(m_, estTE, beta_).Run();
        out.sRRS = ResidualCalculator(m_, rrsTE, beta_).Run();
        BuildOriginalStyleHybridTEs(estTE, firstTE, thirdTE, hybridTE);
        for (int E = 1; E < total - 1; ++E)
        {
            long long v = hybridTE[ToRSIndex(E)];
            if (v > out.maxHybridTE)
            {
                out.maxHybridTE = v;
                out.maxHybridSubset = E;
            }
        }
        out.sHybrid = ResidualCalculator(m_, hybridTE, beta_).Run();
        out.gs = GlobalSensitivity();

        long long best = out.sEST;
        best = std::min(best, out.sRRS);
        best = std::min(best, out.sHybrid);
        if (out.gs > 0)
            best = std::min(best, out.gs);
        out.finalS = std::max(1LL, best);
        out.ok = true;
        return out;
    }

    // Exposed for testing: the EST-only smooth sensitivity (upper-most baseline).
    long long ComputeESTOnly()
    {
        if (m_ <= 0 || beta_ <= 0.0 || !BuildModel())
            return 0;
        BuildAdjacencyAndHops();
        BuildDegreeInfo();
        int total = 1 << m_;
        vector<long long> estTE(total, 1);
        for (int E = 1; E < total - 1; ++E)
        {
            int boundary = BoundaryAttrMask(E);
            long long best = LLONG_MAX;
            vector<int> relaxations = RelaxationMasks(boundary);
            for (int relaxed : relaxations)
                best = std::min(best, ComputeEST(E, boundary & ~relaxed));
            estTE[ToRSIndex(E)] = (best == LLONG_MAX) ? 1 : best;
        }
        return ResidualCalculator(m_, estTE, beta_).Run();
    }

    double beta() const { return beta_; }

private:
    const vector<Table> &tables_;
    const vector<int> &parent_;
    const vector<int> &joinColInParent_;
    const vector<int> &joinColInChild_;
    int m_;
    double beta_ = 0.0;

    int attrCount_ = 0;
    vector<int> relAttrMask_;            // relAttrMask_[r] = attribute bitmask of relation r
    vector<vector<int>> attrColumn_;     // attrColumn_[r][a] = column of attr a in r, or -1
    vector<vector<int>> adj_;            // tree adjacency (undirected)
    vector<vector<int>> nextHop_;        // nextHop_[src][dst]
    vector<vector<long long>> degree_;   // degree_[r][attrMask] = max frequency
    std::map<std::pair<int, int>, long long> rstComponentCache_;
    static constexpr long long kMaxExactRSTComponentRows = 1000000;

    // Build the single-attribute-edge model from parent[] + join columns.
    bool BuildModel()
    {
        if ((int)parent_.size() != m_ ||
            (int)joinColInParent_.size() != m_ ||
            (int)joinColInChild_.size() != m_)
            return false;
        attrCount_ = m_; // attribute id == child node id
        relAttrMask_.assign(m_, 0);
        attrColumn_.assign(m_, vector<int>(attrCount_, -1));
        int roots = 0;
        for (int c = 0; c < m_; ++c)
        {
            if (parent_[c] == -1)
            {
                ++roots;
                continue;
            }
            int p = parent_[c];
            if (p < 0 || p >= m_)
                return false;
            int a = c; // attribute id of edge (c, p)
            relAttrMask_[p] |= (1 << a);
            relAttrMask_[c] |= (1 << a);
            attrColumn_[p][a] = joinColInParent_[c];
            attrColumn_[c][a] = joinColInChild_[c];
        }
        return roots == 1 && attrCount_ <= 30;
    }

    void BuildAdjacencyAndHops()
    {
        adj_.assign(m_, {});
        for (int c = 0; c < m_; ++c)
            if (parent_[c] != -1)
            {
                adj_[c].push_back(parent_[c]);
                adj_[parent_[c]].push_back(c);
            }
        nextHop_.assign(m_, vector<int>(m_, -1));
        for (int s = 0; s < m_; ++s)
        {
            vector<int> prev(m_, -2);
            std::queue<int> q;
            q.push(s);
            prev[s] = s;
            while (!q.empty())
            {
                int u = q.front();
                q.pop();
                for (int w : adj_[u])
                    if (prev[w] == -2)
                    {
                        prev[w] = u;
                        q.push(w);
                    }
            }
            for (int d = 0; d < m_; ++d)
            {
                if (d == s)
                {
                    nextHop_[s][d] = s;
                    continue;
                }
                int cur = d;
                while (prev[cur] != s && prev[cur] != cur && prev[cur] != -2)
                    cur = prev[cur];
                nextHop_[s][d] = cur;
            }
        }
    }

    static long long maxFreqOnColumns(const Table &t, const vector<int> &cols)
    {
        if (t.empty())
            return 0;
        if (cols.empty())
            return (long long)t.size();
        std::unordered_map<std::string, long long> cnt;
        cnt.reserve(t.size() * 2 + 1);
        long long best = 0;
        for (const auto &row : t)
        {
            std::string key;
            for (int col : cols)
            {
                if (col >= 0 && col < (int)row.size())
                    key += std::to_string(row[col]);
                key += '#';
            }
            long long v = ++cnt[key];
            if (v > best)
                best = v;
        }
        return best;
    }

    void BuildDegreeInfo()
    {
        int total = 1 << m_;
        degree_.assign(m_, vector<long long>(total, 1));
        for (int r = 0; r < m_; ++r)
        {
            int incident = relAttrMask_[r];
            for (int mask = 0; mask < total; ++mask)
            {
                if ((mask & ~incident) != 0)
                    continue;
                vector<int> cols;
                for (int a = 0; a < attrCount_; ++a)
                    if ((mask & (1 << a)) != 0)
                        cols.push_back(attrColumn_[r][a]);
                degree_[r][mask] = std::max(1LL, maxFreqOnColumns(tables_[r], cols));
            }
        }
    }

    int BoundaryAttrMask(int E) const
    {
        int in = 0, out = 0;
        for (int r = 0; r < m_; ++r)
        {
            if ((E & (1 << r)) != 0)
                in |= relAttrMask_[r];
            else
                out |= relAttrMask_[r];
        }
        return in & out;
    }

    int ToRSIndex(int E) const
    {
        int idx = 0;
        for (int r = 0; r < m_; ++r)
            idx = idx * 2 + (((E & (1 << r)) != 0) ? 1 : 0);
        return idx;
    }

    void BuildOriginalStyleHybridTEs(
        const vector<long long> &estTE,
        const std::map<std::pair<int, int>, long long> &firstTE,
        const std::map<std::pair<int, int>, long long> &thirdTE,
        vector<long long> &hybridTE) const
    {
        int total = 1 << m_;
        vector<int> ordered;
        ordered.reserve(total);
        for (int E = 1; E < total - 1; ++E)
            ordered.push_back(E);
        std::sort(ordered.begin(), ordered.end(), [](int a, int b) {
            int pa = popcountMask(a), pb = popcountMask(b);
            if (pa != pb)
                return pa > pb;
            return a < b;
        });

        vector<int> chosenRelax(total, 0);
        vector<int> chosenType(total, -1); // 1=first/RST, 3=third/EST

        for (int E : ordered)
        {
            int boundary = BoundaryAttrMask(E);
            int requiredRelax = 0;
            bool thirdFlag = false;
            for (int r = 0; r < m_; ++r)
            {
                if ((E & (1 << r)) != 0)
                    continue;
                int sup = E | (1 << r);
                if (sup == total - 1)
                    continue;
                if (chosenType[sup] == 1 || chosenType[sup] == 3)
                    requiredRelax |= chosenRelax[sup];
                if (chosenType[sup] == 3)
                    thirdFlag = true;
            }
            requiredRelax &= boundary;

            long long best = LLONG_MAX;
            int bestRelax = requiredRelax;
            int bestType = -1;

            auto tryCandidate = [&](int relaxed, int type, long long value) {
                if (value <= 0)
                    return;
                if ((relaxed & ~boundary) != 0)
                    return;
                if ((relaxed & requiredRelax) != requiredRelax)
                    return;
                if (value < best)
                {
                    best = value;
                    bestRelax = relaxed;
                    bestType = type;
                }
            };

            if (!thirdFlag)
            {
                auto itFirst = firstTE.find(std::make_pair(E, requiredRelax));
                if (itFirst != firstTE.end())
                    tryCandidate(requiredRelax, 1, itFirst->second);
                auto itThird = thirdTE.find(std::make_pair(E, requiredRelax));
                if (itThird != thirdTE.end())
                    tryCandidate(requiredRelax, 3, itThird->second);

                vector<int> relaxations = RelaxationMasks(boundary);
                for (int relaxed : relaxations)
                {
                    auto it = firstTE.find(std::make_pair(E, relaxed));
                    if (it != firstTE.end())
                        tryCandidate(relaxed, 1, it->second);
                }
            }
            else
            {
                auto itThird = thirdTE.find(std::make_pair(E, requiredRelax));
                if (itThird != thirdTE.end())
                    tryCandidate(requiredRelax, 3, itThird->second);
            }

            if (best == LLONG_MAX)
            {
                best = estTE[ToRSIndex(E)];
                bestRelax = 0;
                bestType = 3;
            }

            chosenRelax[E] = bestRelax;
            chosenType[E] = bestType;
            hybridTE[ToRSIndex(E)] = best;
        }
    }

    vector<int> RelaxationMasks(int boundaryMask) const
    {
        vector<int> attrs;
        for (int a = 0; a < attrCount_; ++a)
            if ((boundaryMask & (1 << a)) != 0)
                attrs.push_back(a);
        int total = 1 << (int)attrs.size();
        vector<int> masks;
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

    // EST^y_E = max_{r not in E} prod_{j in E} deg((x_j ^ x_{p(j,r)}) | ((dE - y) ^ x_j), R_j)
    long long ComputeEST(int E, int effectiveBoundary) const
    {
        long long best = 1;
        for (int r = 0; r < m_; ++r)
        {
            if ((E & (1 << r)) != 0)
                continue;
            long long cur = 1;
            for (int j = 0; j < m_; ++j)
            {
                if ((E & (1 << j)) == 0)
                    continue;
                int nb = nextHop_[j][r];
                int degAttrs = relAttrMask_[j] & relAttrMask_[nb];
                degAttrs |= effectiveBoundary & relAttrMask_[j];
                cur = satMulLL(cur, degree_[j][degAttrs]);
            }
            best = std::max(best, cur);
        }
        return best;
    }

    // Real maximum boundary T^y_E. Returns -1 if some connected component is
    // not free-connex after dropping relaxed boundary attributes y.
    long long ComputeRST(int E, int effectiveBoundary)
    {
        vector<int> nodes;
        for (int r = 0; r < m_; ++r)
            if ((E & (1 << r)) != 0)
                nodes.push_back(r);

        vector<char> seen(m_, 0);
        long long product = 1;
        for (int start : nodes)
        {
            if (seen[start])
                continue;
            // collect connected component within E
            vector<int> comp;
            std::queue<int> q;
            q.push(start);
            seen[start] = 1;
            while (!q.empty())
            {
                int u = q.front();
                q.pop();
                comp.push_back(u);
                for (int w : adj_[u])
                    if ((E & (1 << w)) != 0 && !seen[w])
                    {
                        seen[w] = 1;
                        q.push(w);
                    }
            }
            int compMask = 0;
            long long compRows = 0;
            for (int u : comp)
            {
                compMask |= (1 << u);
                compRows = satAddLL(compRows, (long long)tables_[u].size());
            }

            auto key = std::make_pair(compMask, effectiveBoundary);
            auto cached = rstComponentCache_.find(key);
            long long tc = -1;
            if (cached != rstComponentCache_.end())
            {
                tc = cached->second;
            }
            else if ((int)comp.size() == 1 || compRows <= kMaxExactRSTComponentRows)
            {
                tc = ComponentMaxBoundary(E, comp, effectiveBoundary);
                rstComponentCache_[key] = tc;
            }
            if (tc < 0)
                return -1; // component not free-connex
            product = satMulLL(product, tc);
        }
        return product;
    }

    // Maximum boundary of one connected component. Free-connex iff every
    // remaining boundary edge of the component emanates from a single node v;
    // then root the component at v and group v-tuples by the boundary columns.
    long long ComponentMaxBoundary(int E, const vector<int> &comp, int effectiveBoundary)
    {
        // boundary attributes of the component and which node carries each.
        int boundaryMask = 0;
        int carrier = -1;
        bool single = true;
        for (int u : comp)
        {
            for (int a = 0; a < attrCount_; ++a)
            {
                if ((relAttrMask_[u] & (1 << a)) == 0)
                    continue;
                if ((effectiveBoundary & (1 << a)) == 0)
                    continue;
                // other endpoint of edge a is {a, parent[a]} minus u
                int other = (a == u) ? parent_[a] : a;
                if (other < 0)
                    continue;
                if ((E & (1 << other)) != 0)
                    continue; // internal edge
                boundaryMask |= (1 << a);
                if (carrier == -1)
                    carrier = u;
                else if (carrier != u)
                    single = false;
            }
        }
        if (!single)
            return -1;
        if (carrier == -1)
            carrier = comp[0];

        if ((int)comp.size() == 1)
            return std::max(1LL, degree_[carrier][boundaryMask]);

        // Root the component at carrier; compute per-tuple subtree multiplicity.
        int v = carrier;
        std::unordered_map<int, int> inComp;
        for (int u : comp)
            inComp[u] = 1;

        // BFS order from v within component to get parent-in-comp.
        vector<int> compParent(m_, -1);
        vector<int> bfs;
        {
            std::queue<int> q;
            vector<char> vis(m_, 0);
            q.push(v);
            vis[v] = 1;
            compParent[v] = -1;
            while (!q.empty())
            {
                int u = q.front();
                q.pop();
                bfs.push_back(u);
                for (int w : adj_[u])
                    if ((E & (1 << w)) != 0 && !vis[w])
                    {
                        vis[w] = 1;
                        compParent[w] = u;
                        q.push(w);
                    }
            }
        }

        // delta[u][i] = number of component-subtree join tuples rooted at u-tuple i.
        std::unordered_map<int, vector<long long>> delta;
        for (int u : comp)
            delta[u].assign(tables_[u].size(), 1);

        // process far-from-v first (reverse BFS)
        for (int idx = (int)bfs.size() - 1; idx >= 0; --idx)
        {
            int u = bfs[idx];
            int pu = compParent[u];
            if (pu == -1)
                continue;
            // edge attribute between u and pu
            int a = (parent_[u] == pu) ? u : pu;
            int colChildSide = (parent_[u] == pu) ? joinColInChild_[u] : joinColInParent_[pu];
            int colParentSide = (parent_[u] == pu) ? joinColInParent_[u] : joinColInChild_[pu];
            // group u tuples by join value, sum delta[u]
            std::unordered_map<int, long long> byKey;
            byKey.reserve(tables_[u].size() * 2 + 1);
            for (int i = 0; i < (int)tables_[u].size(); ++i)
            {
                int key = tables_[u][i][colChildSide];
                byKey[key] = satAddLL(byKey[key], delta[u][i]);
            }
            // multiply into pu tuples
            auto &dpu = delta[pu];
            for (int i = 0; i < (int)tables_[pu].size(); ++i)
            {
                int key = tables_[pu][i][colParentSide];
                auto it = byKey.find(key);
                long long add = (it == byKey.end()) ? 0 : it->second;
                dpu[i] = satMulLL(dpu[i], add);
            }
        }

        // boundary columns are columns of v for attributes in boundaryMask
        vector<int> bcols;
        for (int a = 0; a < attrCount_; ++a)
            if ((boundaryMask & (1 << a)) != 0)
                bcols.push_back(attrColumn_[v][a]);

        std::unordered_map<std::string, long long> groupSum;
        groupSum.reserve(tables_[v].size() * 2 + 1);
        long long best = 0;
        const auto &dv = delta[v];
        for (int i = 0; i < (int)tables_[v].size(); ++i)
        {
            std::string key;
            for (int col : bcols)
            {
                key += std::to_string(tables_[v][i][col]);
                key += '#';
            }
            long long s = satAddLL(groupSum[key], dv[i]);
            groupSum[key] = s;
            if (s > best)
                best = s;
        }
        return std::max(1LL, best);
    }

    // Global sensitivity: GS = max_i (product of input sizes of all relations
    // except i) -- a safe, instance-independent smooth upper bound ceiling.
    long long GlobalSensitivity() const
    {
        long long best = 0;
        for (int i = 0; i < m_; ++i)
        {
            long long prod = 1;
            for (int j = 0; j < m_; ++j)
                if (j != i)
                    prod = satMulLL(prod, (long long)tables_[j].size());
            best = std::max(best, prod);
        }
        return std::max(1LL, best);
    }
};

// Convenience entry point. Returns S(I) (smooth local sensitivity), or 0 if the
// shape is not a single-attribute-edge tree join we can handle.
inline long long computeRRSSensitivity(const vector<vector<vector<int>>> &tables,
                                       const vector<int> &parent,
                                       const vector<int> &joinColInParent,
                                       const vector<int> &joinColInChild,
                                       double epsilon, double delta)
{
    return RRSEngine(tables, parent, joinColInParent, joinColInChild, epsilon, delta).Compute();
}

} // namespace rrs
