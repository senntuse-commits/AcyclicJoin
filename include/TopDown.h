#pragma once
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;
using Table = vector<vector<int>>;

Table topDown(vector<Table> &tables, const vector<int> &parent, int root,
              const vector<int> &joinColInParent, const vector<int> &joinColInChild, const vector<int> &tableKeys);

double getLastTopDownParallelMs();

void getRank(Table &table, int joinCols, bool isLeafNode);
vector<int> getRankValues(Table &table, int joinCols, bool isLeafNode, int sortThreads = 0);

Table subProcess(const Table& parent, int joinColInParent, Table &child, int joinColInChild,
                 const vector<int> &childRank, const vector<int> &parentIdx,
                 const vector<int> &scFlat, const vector<int> &pcFlat,
                 int siblingCount, int c, bool isLeafNode, int childNum, int keyCols);

Table sortTable(Table table, vector<int> keyColumns, bool direction, int sortThreads = 0);
