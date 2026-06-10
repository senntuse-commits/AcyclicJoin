#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <climits>
#include <cstdint>

class BitonicSorter
{
private:
    int length;
    std::vector<std::vector<int>> array;
    std::vector<__uint128_t> lex_keys;
    std::vector<int> keyCols; // The specified sort column index.
    bool dire;                // Sort direction: true for ascending, false for descending.

    void bitonic_sort(int lowbound, int len, bool direction);

    void bitonic_merge(int lowbound, int len, bool direction);

    void compare_and_swap(int i, int j, bool direction);

    void computeLexKeys();

    void pad_array(); // Pad to the next power-of-two length by adding elements at the end.

public:
    BitonicSorter(std::vector<std::vector<int>> arr, std::vector<int> keyColumns, bool direction);

    void Sorter();

    void printData();

    std::vector<std::vector<int>> getSortedData();
};