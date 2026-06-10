#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>

using namespace std;

struct AggTreeNode
{
    int lpx, px;
    int rsx, sx;
    int l, c;

    AggTreeNode(int lpx, int px, int rsx, int sx, int l, int c)
        : lpx(lpx), px(px), rsx(rsx), sx(sx), l(l), c(c) {}

    AggTreeNode() : lpx(0), px(0), rsx(0), sx(0), l(0), c(0) {}
};

class Aggtree
{
private:
    vector<int> D;      // Data matrix
    vector<int> B;      // Flag matrix
    vector<int> result; // Result vector
    int exclusive;      // Exclusive flag

public:
    Aggtree(const vector<int> &D, const vector<int> &B, int exclusive);
    vector<int> PrefixTreeRun();
    vector<int> SuffixTreeRun();
    vector<int> SuffixProductRun();
    vector<int> FullRun();
};
