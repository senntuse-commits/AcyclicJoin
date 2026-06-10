#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <omp.h>
#include <utility>

using namespace std;

class DupAggtree
{
private:
    vector<vector<int>> D;
    vector<int> B;
    vector<vector<int>> result;

    // Flat-array variant (slim columns, avoids ~1 GB A_px/A_lpx allocation)
    const vector<int>* D_flat = nullptr;
    int n_rows_flat = 0, n_cols_flat = 0;
    const vector<int>* B_flat = nullptr;

public:
    DupAggtree(const vector<vector<int>> &D, const vector<int> &B);
    // Flat constructor: D_flat is row-major n_rows×n_cols, B_flat same length as D_flat rows
    DupAggtree(const vector<int>& D_flat, int n_rows, int n_cols, const vector<int>& B_flat);
    vector<vector<int>> DupAggtreeRun();
    // Returns flat row-major result array of same shape as D_flat
    vector<int> DupAggtreeRunFlat();
};
