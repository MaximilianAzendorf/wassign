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

#include "Options.h"

#include "Version.h"
#include "input/InputException.h"

#include <popl.hpp>
#include <iostream>
#include <utility>

using namespace popl;

int Options::parse_time(string value)
{
    int time = 0;
    int current = 0;

    for(char c : value)
    {
        if(c >= '0' && c <= '9')
        {
            current = current * 10 + (c - '0');
        }
        else if(c >= 'a' && c <= 'z')
        {
            time += current * _timeMultiplier.at(c);
        }
        else
        {
            throw InputException("Unknown time specifier " + value + ".");
        }
    }

    return time;
}

OptionsParseStatus Options::parse_base(int argc, char **argv, bool newOpt, string const& header)
{
    OptionParser op("Allowed options");

    auto helpOpt = op.add<Switch>("h", "help", "Show this help.");
    auto versionOpt = op.add<Switch>("", "version", "Show version.");
    auto inputOpt = op.add<Value<string>>("i", "input", "Specifies an input file.");
    auto outputOpt = op.add<Value<string>>("o", "output", "Specifies an output file.");
    auto verbosityOpt = op.add<Value<int>>("v", "verbosity", "A number between 0 and 3 indicating how much status information should be given.");
    auto anyOpt = op.add<Switch>("a", "any", "Stop after the first found solution.");
    auto prefExpOpt = op.add<Value<double>>("p", "pref-exp", "The preference exponent.");
    auto timeoutOpt = op.add<Value<string>>("t", "timeout", "Sets the optimization timeout.");
    auto csTimeoutOpt = op.add<Value<string>>("", "cs-timeout", "Sets the timeout for attempting to satisfy critical sets of a certain preference level.");
    auto noCsOpt = op.add<Switch>("", "no-cs", "Do not perform critical set analysis");
    auto noCsSimpOpt = op.add<Switch>("", "no-cs-simp", "Do not perform critical set simplification (only relevant if critical set analysis is enabled)");
    auto threadsOpt = op.add<Value<int>>("j", "threads", "Number of threads to use for computation.");
    auto maxNeighborsOpt = op.add<Value<int>>("n", "max-neighbors", "Maximum number of neighbor schedulings that will be explored per hill climbing iteration.");
    auto greedyOpt = op.add<Switch>("g", "greedy", "Do not use the worst-preference scoring as primary score and just use sum-based scoring instead.");

    op.parse(argc, argv);

    if(helpOpt->is_set() && newOpt)
    {
        std::cout << header << std::endl << op << std::endl;
        return OPT_PARSE_EXIT;
    }
    else if(versionOpt->is_set() && newOpt)
    {
        std::cout << WASSIGN_VERSION << std::endl;
        return OPT_PARSE_EXIT;
    }
    else if(!op.non_option_args().empty() || !op.unknown_options().empty())
    {
        return OPT_PARSE_ERROR;
    }
    else
    {
        if(inputOpt->is_set())
        {
            vector<string> inputFiles;
            for (int i = 0; i < inputOpt->count(); i++)
            {
                inputFiles.push_back(inputOpt->value(i));
            }

            set_input_files(inputFiles);
        }

        if(outputOpt->is_set()) set_output_file(outputOpt->value());
        if(verbosityOpt->is_set()) set_verbosity(verbosityOpt->value());
        if(anyOpt->is_set()) set_any(true);
        if(prefExpOpt->is_set()) set_preference_exponent(prefExpOpt->value());
        if(timeoutOpt->is_set()) set_timeout_seconds(parse_time(timeoutOpt->value()));
        if(csTimeoutOpt->is_set()) set_critical_set_timeout_seconds(parse_time(csTimeoutOpt->value()));
        if(noCsOpt->is_set()) set_no_critical_sets(true);
        if(noCsSimpOpt->is_set()) set_no_critical_set_simplification(true);
        if(threadsOpt->is_set()) set_thread_count(threadsOpt->value());
        if(maxNeighborsOpt->is_set()) set_max_neighbors(maxNeighborsOpt->value());
        if(greedyOpt->is_set()) set_greedy(true);

        if(verbosity() > 0 && newOpt)
        {
            std::cerr << header << std::endl;
        }

        return OPT_PARSE_OK;
    }
}

OptionsParseStatus Options::parse_override(int argc, char **argv)
{
    return parse_base(argc, argv, false, "");
}

OptionsParseStatus Options::parse(int argc, char **argv, string const& header, shared_ptr<Options>& outResult)
{
    shared_ptr<Options> result = Options::default_options();

    auto res = result->parse_base(argc, argv, true, header);

    outResult = result;
    return res;
}

shared_ptr<Options> Options::default_options()
{
    return std::make_shared<Options>();
}

int Options::verbosity() const
{
    return _verbosity;
}

vector<string> Options::input_files() const
{
    return _inputFiles;
}

string Options::output_file() const
{
    return _outputFile;
}

int Options::timeout_seconds() const
{
    return _timeout;
}

int Options::critical_set_timeout_seconds() const
{
    return _csTimeout;
}

bool Options::no_critical_sets() const
{
    return _noCs;
}

bool Options::no_critical_set_simplification() const
{
    return _noCsSimp;
}

double Options::preference_exponent() const
{
    return _prefExp;
}

bool Options::any() const
{
    return _any;
}

int Options::thread_count() const
{
    return _threadCount;
}

bool Options::greedy() const
{
    return _greedy;
}

int Options::max_neighbors() const
{
    return _maxNeighbors;
}

void Options::set_verbosity(int verbosity)
{
    _verbosity = verbosity;
}

void Options::set_input_files(vector<string> inputFiles)
{
    _inputFiles = std::move(inputFiles);
}

void Options::set_no_critical_sets(bool noCriticalSets)
{
    _noCs = noCriticalSets;
}

void Options::set_no_critical_set_simplification(bool noCriticalSetSimplification)
{
    _noCsSimp = noCriticalSetSimplification;
}

void Options::set_output_file(string outputFile)
{
    _outputFile = std::move(outputFile);
}

void Options::set_timeout_seconds(int timeoutSeconds)
{
    _timeout = timeoutSeconds;
}

void Options::set_critical_set_timeout_seconds(int csTimeoutSeconds)
{
    _csTimeout = csTimeoutSeconds;
}

void Options::set_preference_exponent(double prefExponent)
{
    _prefExp = prefExponent;
}

void Options::set_any(bool any)
{
    _any = any;
}

void Options::set_thread_count(int threadCount)
{
    _threadCount = threadCount;
}

void Options::set_greedy(bool greedy)
{
    _greedy = greedy;
}

void Options::set_max_neighbors(int maxNeighbors)
{
    _maxNeighbors = maxNeighbors;
}
