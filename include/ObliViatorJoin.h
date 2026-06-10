#pragma once
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <omp.h>
#include <unordered_map>
#include <utility>
#ifdef SGX_ENCLAVE_BUILD
#include "sgx_log.h"
#endif

using namespace std;
using Table = vector<vector<int>>;

class ObliViatorJoin
{
private:
    Table T1, T2, result;
    int joinCol_1, joinCol_2; // join key in Table
    int keys_1, keys_2;       // number of attributes for T1 and T2
    int out_size;

    void augmentTables(Table &T);

    Table sortTable(Table table, vector<int> keyColumns, bool direction);

    void fillDimension(Table &T);

    void fetchExpand(const Table &T, Table &ExpandT1, Table &ExpandT2);

    void alignTable(Table &T);

public:
    ObliViatorJoin(Table t1, const int joinCol_1, const int keys_1, Table t2, const int joinCol_2, const int keys_2, const int output_size) : T1(std::move(t1)), T2(std::move(t2)), joinCol_1(joinCol_1), joinCol_2(joinCol_2), keys_1(keys_1), keys_2(keys_2), out_size(output_size)
    {
        if (T1.empty() || T2.empty())
        {
#ifdef SGX_ENCLAVE_BUILD
            sgx_cerr << "Incompatible table schemas." << sgx_endl;
#else
            cerr << "Incompatible table schemas." << endl;
#endif
            return;
        }
    }

    double ms_augment = 0, ms_fillDim = 0, ms_expand = 0, ms_align = 0, ms_result = 0;

    void printTable(const Table &vec, const vector<string> &headers);

    void join();

    Table getResult()
    {
        return std::move(result);
    }
};
