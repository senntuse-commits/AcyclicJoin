#pragma once

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using Table = std::vector<std::vector<int>>;

struct FlatTables
{
    std::vector<int> data;
    std::vector<int> offsets;
    std::vector<int> rows;
    std::vector<int> cols;
};

inline FlatTables flattenTables(const std::vector<Table> &tables)
{
    FlatTables flat;
    flat.offsets.reserve(tables.size());
    flat.rows.reserve(tables.size());
    flat.cols.reserve(tables.size());

    for (const Table &table : tables)
    {
        flat.offsets.push_back((int)flat.data.size());
        flat.rows.push_back((int)table.size());
        flat.cols.push_back(table.empty() ? 0 : (int)table[0].size());
        for (const auto &row : table)
        {
            flat.data.insert(flat.data.end(), row.begin(), row.end());
        }
    }
    return flat;
}

inline Table arrayToTable(const int *data, int rows, int cols)
{
    Table table(rows, std::vector<int>(cols));
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            table[i][j] = data[i * cols + j];
    return table;
}

inline Table loadData(const std::string &filename)
{
    Table result;
    std::ifstream infile(filename);
    std::string line;

    while (std::getline(infile, line))
    {
        std::istringstream iss(line);
        std::vector<int> row;
        int value = 0;
        while (iss >> value)
            row.push_back(value);
        if (!row.empty())
            result.push_back(row);
    }

    return result;
}
