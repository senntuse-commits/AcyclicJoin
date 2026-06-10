#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>

using namespace std;
using Table = vector<vector<int>>;

class ParallelSemiJoin
{
private:
    const Table &R;
    const Table &S;
    int joinCol_1;
    int joinCol_2;
    int valueCol;
    vector<vector<int>> result;
    vector<int> resultValues;

    Table sortTable(Table table, vector<int> keyColumns, bool direction);

public:
    ParallelSemiJoin(const Table &R, int joinCol_1, const Table &S, int joinCol_2, int valueCol);
    void execute();
    void executeValueOnly();
    vector<vector<int>> getResult();
    vector<int> getResultValues();
};
