#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <cstdint>
#ifdef SGX_ENCLAVE_BUILD
#include "sgx_log.h"
#endif

using namespace std;

class ParallelBitonicSorter
{
private:
    int length;
    int original_length;
    vector<vector<int>> array;
    vector<int> flatRows;
    vector<__int128_t> lex_keys;
    int rowWidth;
    __int128_t key_base;           // base used in computeLexKeys, stored for pad_array
    vector<int> keyCols; // The specified sort column index.
    bool dire;           // Sort direction: true for ascending, false for descending.

    void computeLexKeys();

    void pad_array(); // Pad to the next power-of-two length by adding elements at the end.

public:
    ParallelBitonicSorter(vector<vector<int>> arr, vector<int> keyColumns, bool direction);

    void Sorter();

    void printData();

    vector<vector<int>> getSortedData();

    void print_int128(__int128_t n)
    {
        if (n == 0)
        {
#ifdef SGX_ENCLAVE_BUILD
            sgx_cout << "0";
#else
            cout << "0";
#endif
            return;
        }

        bool is_negative = n < 0;
        __uint128_t u = is_negative ? (__uint128_t)(-n) : (__uint128_t)(n);

        string s;
        while (u > 0)
        {
            s = char('0' + u % 10) + s;
            u /= 10;
        }

        if (is_negative)
            s = "-" + s;

#ifdef SGX_ENCLAVE_BUILD
        sgx_cout << s;
#else
        cout << s;
#endif
    }
};
