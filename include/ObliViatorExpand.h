#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <utility>

using namespace std;

class ObliViatorExpand
{
private:
    vector<vector<int>> T;
    vector<int> M;
    vector<int> Q;
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

    void printVector(const vector<vector<int>> &vec, const vector<string> &headers);

public:
    ObliViatorExpand(vector<vector<int>> T_input, vector<int> M_input);

    void OExpand(int s, int e);

    void OExpandSub(int s, int e, int z);

    vector<vector<int>> getResult()
    {
        return std::move(T);
    }
};
