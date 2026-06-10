#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>

using namespace std;



class ObliCompact {
public:
    // Swap function
    void Swap(vector<vector<int>> &D, int i, int j, bool b);

    // Offcompact function
    void offcompact(vector<vector<int>> &D, vector<int> &M, int left, int right, int z);

    // ORCompact function
    void ORCompact(vector<vector<int>> &D, vector<int> &M, int left, int right);

    // Serial ORCompact function for baselines that should not use the
    // optimized parallel compact implementation.
    void ORCompactSerial(vector<vector<int>> &D, vector<int> &M, int left, int right);
};

