#pragma once

#include "Types.h"

template<typename T>
class UnionFind
{
private:
    map<T, int> _elements;
    vector<T> _parents;
    vector<T> _heights;

public:
    explicit UnionFind(T max);

    [[nodiscard]] int find(T element) const;

    int join(T element1, T element2);

    [[nodiscard]] vector<vector<T>> groups() const;
};

#include "UnionFind.ipp"