#pragma once

#include "Types.h"

#include <boost/algorithm/string.hpp>

class FuzzyMatch
{
private:
    FuzzyMatch() = default;

    static vector<int> find_exact(string const& key, vector<string> const& values)
    {
        vector<int> res;
        for(int i = 0; i < values.size(); i++)
        {
            if(key == values[i])
            {
                res.push_back(i);
            }
        }

        return res;
    }

    static vector<int> find_by_token(string const& key, vector<string> const& values, bool onlyFirstToken)
    {
        vector<int> res;
        for(int i = 0; i < values.size(); i++)
        {
            vector<string> tokens;
            boost::split(tokens, values[i], boost::is_any_of(" "));

            if(onlyFirstToken) tokens.resize(1);

            for(string const& token : tokens)
            {
                if(token == key)
                {
                    res.push_back(i);
                    break;
                }
            }
        }

        return res;
    }

    static vector<int> find_by_first_token(string const& key, vector<string> const& values)
    {
        return find_by_token(key, values, true);
    }

    static vector<int> find_by_any_token(string const& key, vector<string> const& values)
    {
        return find_by_token(key, values, false);
    }

    static vector<int> find_by_substring(string const& key, vector<string> const& values)
    {
        vector<int> res;
        for(int i = 0; i < values.size(); i++)
        {
            if(values[i].rfind(key) != string::npos)
            {
                res.push_back(i);
            }
        }

        return res;
    }

public:
    static vector<int> find(string const& key, vector<string> const& values)
    {
        auto methods = { find_exact, find_by_first_token, find_by_any_token, find_by_substring };

        for(auto method : methods)
        {
            auto res = method(key, values);
            if(!res.empty())
            {
                return res;
            }
        }

        return {};
    }
};


