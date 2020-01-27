#pragma once

#include "Types.h"

class FuzzyMatch
{
private:
    FuzzyMatch() = default;

    static vector<int> find_exact(string const& key, vector<string> const& values);

    static vector<int> find_by_token(string const& key, vector<string> const& values, bool onlyFirstToken);

    static vector<int> find_by_first_token(string const& key, vector<string> const& values);

    static vector<int> find_by_any_token(string const& key, vector<string> const& values);

    static vector<int> find_by_substring(string const& key, vector<string> const& values);

public:
    static vector<int> find(string const& key, vector<string> const& values);
};


