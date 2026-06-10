#pragma once
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;
using Table = vector<vector<int>>;

struct DOPaddingStats
{
    long long exactRows = 0;
    long long sensitivity = 1;
    int protectedRows = 0;
    long long protectedRowsExact = 0;
    long long paddingRowsExact = 0;
    double factor = 0.0;
    bool rrsAvailable = false;
    bool rrsTooExpensive = false; // RRS supported for this shape but intractable
                                  // at the given (epsilon, delta); raise epsilon.
    long long rrsSEST = 0;
    long long rrsSRRS = 0;
    long long rrsSHybrid = 0;
    long long rrsGS = 0;
    long long rrsMaxK = 0;
    double rrsEstLeaves = 0.0;
    long long rrsMaxEstTE = 0;
    int rrsMaxEstSubset = 0;
    long long rrsMaxRRSTE = 0;
    int rrsMaxRRSSubset = 0;
    long long rrsMaxHybridTE = 0;
    int rrsMaxHybridSubset = 0;
};

// Performs bottom-up semi-join over a join tree.
//
// tables           : tables[i] is the relation for node i
// parent           : parent[i] is the parent node id of i; -1 means root
// root             : id of the root node
// joinColInParent  : joinColInParent[i] is the column index in tables[parent[i]]
//                    used to join with tables[i]; unused for root
// joinColInChild   : joinColInChild[i] is the column index in tables[i]
//                    used to join with tables[parent[i]]; unused for root
//
// Calls ParallelSemiJoin(tables[p], joinColInParent[c], tables[c], joinColInChild[c])
// for each parent-child pair in bottom-up order (leaves first, root last).
Table bottomUpSemiJoin(vector<Table> &tables, const vector<int> &parent, int root,
                      const vector<int> &joinColInParent, const vector<int> &joinColInChild,
                      int protectedOutputRows = 0);

long long computeAcyclicJoinOutputSize(vector<Table> tables, const vector<int> &parent, int root,
                                       const vector<int> &joinColInParent, const vector<int> &joinColInChild);

DOPaddingStats computeDOPaddingStats(vector<Table> tables, const vector<int> &parent, int root,
                                     const vector<int> &joinColInParent, const vector<int> &joinColInChild,
                                     int explicitOutputRows = 0,
                                     double epsilon = 1.0,
                                     double delta = 1e-9);

int computeDOProtectedOutputSize(vector<Table> tables, const vector<int> &parent, int root,
                                 const vector<int> &joinColInParent, const vector<int> &joinColInChild,
                                 int explicitOutputRows = 0,
                                 double epsilon = 1.0,
                                 double delta = 1e-9);

double getLastBottomUpParallelMs();
double getLastBottomUpSemiJoinMs();
double getLastBottomUpRootExpandMs();
