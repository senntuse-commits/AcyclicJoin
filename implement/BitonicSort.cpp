#include "../include/BitonicSort.h"
#include "../include/PrimitiveProfile.h"

#ifdef SGX_ENCLAVE_BUILD
#include "sgx_log.h"
#define cout sgx_cout
#define cerr sgx_cerr
#define endl sgx_endl
#endif

BitonicSorter::BitonicSorter(std::vector<std::vector<int>> arr, std::vector<int> keyColumns, bool direction)
{
    array = arr;
    keyCols = keyColumns;
    dire = direction;
    length = arr.size();
}

/*
Compare the values of the specified column between the i-th and j-th elements,
and decide whether to swap them based on the sort direction.
*/
void BitonicSorter::compare_and_swap(int i, int j, bool direction)
{
    if (i >= length || j >= length)
        return;
    bool should_swap = false;
    for (int k = 0; k < keyCols.size(); ++k)
    {
        int col = keyCols[k];
        if (array[i][col] == array[j][col])
            continue;

        should_swap = array[i][col] > array[j][col];
        break;
    }

    if (direction == should_swap)
    {
        std::swap(array[i], array[j]);
    }
    else
    {
        array[i] = array[i];
        array[j] = array[j];
    }
}

/*
Merge bitonic sequence: convert a segment of data into
a monotonic (ascending/descending) sequence.
*/
void BitonicSorter::bitonic_merge(int lowbound, int len, bool direction)
{
    if (len > 1)
    {
        int mid = len / 2;
        for (int i = lowbound; i < lowbound + mid; ++i)
        {
            compare_and_swap(i, i + mid, direction);
        }
        bitonic_merge(lowbound, mid, direction);
        bitonic_merge(lowbound + mid, mid, direction);
    }
}

/*
Bitonic sort: first divide into an ascending and a descending sequence, then merge.
*/
void BitonicSorter::bitonic_sort(int lowbound, int len, bool direction)
{
    if (len > 1)
    {
        int mid = len / 2;
        bitonic_sort(lowbound, mid, true);
        bitonic_sort(lowbound + mid, mid, false);
        bitonic_merge(lowbound, len, direction);
    }
}

/*
Pad the array to a power-of-two length using INT_MAX or INT_MIN to fill the specified column.
*/
void BitonicSorter::pad_array()
{
    int pow2 = 1;
    while (pow2 < length)
        pow2 <<= 1;

    if (pow2 > length)
    {
        std::vector<int> filler(array[0].size(), 0);
        int pad_value = dire ? INT_MAX : INT_MIN;
        filler[keyCols[0]] = pad_value; 

        while (array.size() < pow2)
        {
            array.push_back(filler);
        }
        length = pow2;
    }
}

/* The sorting interface exposed to users */
void BitonicSorter::Sorter()
{
    PrimitiveProfileScope primitiveScope(PrimitiveSort);
    if(array.empty())
        return;
    pad_array();
    bitonic_sort(0, length, dire);

    while (!array.empty() &&
           (array.back()[keyCols[0]] == INT_MAX || array.back()[keyCols[0]] == INT_MIN))
    {
        array.pop_back();
    }
    length = array.size();
}

void BitonicSorter::printData()
{
#ifdef SGX_ENCLAVE_BUILD
    return;
#else
    for (int i = 0; i < length; ++i)
    {
        for (int j = 0; j < array[i].size(); ++j)
        {
            std::cout << array[i][j] << " ";
        }
        std::cout << "\n";
    }
#endif
}

std::vector<std::vector<int>> BitonicSorter::getSortedData()
{
    return array;
}
