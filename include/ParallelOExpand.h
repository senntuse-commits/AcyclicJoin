#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <iomanip>
#include <cmath>
#include <climits>
#include <omp.h>

using namespace std;

class SOExpand
{
private:
    bool isTruncate;            // whether to truncate the result
    vector<vector<int>> T;      // the origin table
    vector<int> M;              // the expand times
    vector<int> Q;              // the position of each element in the table
    vector<vector<int>> result; // the result

    int D1, D2;

    template <typename T>
    void Swap(T &D, int i, int j, bool b)
    {
        if (b)
        {
            swap(D[i], D[j]);
        }
        else
        {
            D[i] = D[i];
            D[j] = D[j];
        }
    }

    bool SubSOExpand(vector<vector<int>> &T, vector<int> &Q, int start, int end, int sep);

    void printVector(const vector<vector<int>> &vec, const vector<string> &headers);

public:
    SOExpand(vector<vector<int>> T, vector<int> M, bool isTruncate = true);

    void Expand();

    vector<vector<int>> getResult();
};
