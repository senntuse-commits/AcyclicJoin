#pragma once

#include <vector>

using namespace std;
using Table = vector<vector<int>>;

Table SemiJoin(const Table &R, const Table &S, int joinColR, int joinColS);

Table ReduceByKey(const Table &R, int keyCol);

Table Annotate(const Table &R, const Table &S, int keyColR, int keyColS);

Table Augment(const Table &R, const Table &S, int joinColR, int joinColS);

Table MultiNumber(Table R, int keyCol);

Table Expand(const Table &R, int tau);

Table RelaxedTwoWay(const Table &R, const Table &S, int joinColR, int joinColS, int tau);

Table RelaxedJoin(vector<Table> tables,
                  const vector<int> &parent,
                  int root,
                  const vector<int> &joinColInParent,
                  const vector<int> &joinColInChild,
                  int tau);

double getLastRelaxedUpFilterMs();
double getLastRelaxedDownFilterMs();
double getLastRelaxedJoinMs();
