/*
 * Copyright 2020 Maximilian Azendorf
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FuzzyMatch.h"

vector<int> FuzzyMatch::find_exact(string const& key, vector<string> const& values)
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

vector<int> FuzzyMatch::find_by_token(string const& key, vector<string> const& values, bool onlyFirstToken)
{
    vector<int> res;
    for(int i = 0; i < values.size(); i++)
    {
        string value = values[i];

        vector<string> tokens;
        int pos = 0;
        while((pos = value.find(' ')) != string::npos)
        {
            tokens.push_back(value.substr(0, pos));
            value.erase(0, pos + 1);
        }

        tokens.push_back(value);

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

vector<int> FuzzyMatch::find_by_first_token(string const& key, vector<string> const& values)
{
    return find_by_token(key, values, true);
}

vector<int> FuzzyMatch::find_by_any_token(string const& key, vector<string> const& values)
{
    return find_by_token(key, values, false);
}

vector<int> FuzzyMatch::find_by_substring(string const& key, vector<string> const& values)
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

vector<int> FuzzyMatch::find(string const& key, vector<string> const& values)
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
