#pragma once
#include <vector>
#include <queue>
#include <algorithm>
#include <iostream>

using namespace std;
using Table = vector<vector<int>>;

pair<vector<Table>, long long> TwoPhaseFilter(vector<Table> &tables, const vector<int> &parent, int root,
                             const vector<int> &joinColInParent, const vector<int> &joinColInChild, const vector<int> &tableKeys);

Table JoinByObliViator(vector<Table> &tables, const vector<int> &parent, int root,
                             const vector<int> &joinColInParent, const vector<int> &joinColInChild, const vector<int> &tableKeys,const int out_size);                        

double getLastObliViatorJoinParallelMs();
double getLastTwoPhaseFilterParallelMs();
double getLastTwoPhaseFilterUpMs();
double getLastTwoPhaseFilterMaterializeMs();
double getLastTwoPhaseFilterDownMs();
