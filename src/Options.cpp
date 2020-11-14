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

#include "Util.h"
#include "Version.h"
#include "InputException.h"
#include "../deps/popl/include/popl.hpp"

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

OptionsParseStatus Options::parse(int argc, char **argv, string const& header, const_ptr<Options>& outResult)
{
    shared_ptr<Options> result = Options::default_options();

    OptionParser op("Allowed options");

    auto helpOpt = op.add<Switch>("h", "help", "Show this help.");
    auto versionOpt = op.add<Switch>("", "version", "Show version.");
    auto inputOpt = op.add<Value<string>>("i", "input", "Specifies an input file.");
    auto outputOpt = op.add<Value<string>>("o", "output", "Specifies an output file.");
    auto verbosityOpt = op.add<Value<int>>("v", "verbosity", "A number between 0 and 3 indicating how much status information should be given.");
    auto anyOpt = op.add<Switch>("a", "any", "Stop after the first found solution.");
    auto prefExpOpt = op.add<Value<double>>("p", "pref-exp", "The preference exponent.");
    auto timeoutOpt = op.add<Value<string>>("t", "timeout", "Sets the optimization timeout.");
    auto csTimeoutOpt = op.add<Value<string>>("m", "cs-timeout", "Sets the timeout for attempting to satisfy critical sets of a certain preference level.");
    auto noCsOpt = op.add<Switch>("", "no-cs", "Do not perform critical set analysis");
    auto threadsOpt = op.add<Value<int>>("j", "threads", "Number of threads to use for computation.");

    op.parse(argc, argv);

    if(helpOpt->is_set())
    {
        std::cout << header << std::endl << op << std::endl;
        return EXIT;
    }
    else if(versionOpt->is_set())
    {
        std::cout << WASSIGN_VERSION << std::endl;
        return EXIT;
    }
    else if(!op.non_option_args().empty() || !op.unknown_options().empty())
    {
        return ERROR;
    }
    else
    {
        vector<string> inputFiles;
        for(int i = 0; i < inputOpt->count(); i++)
        {
            inputFiles.push_back(inputOpt->value(i));
        }

        result->set_input_files(inputFiles);
        if(outputOpt->is_set()) result->set_output_file(outputOpt->value());
        if(verbosityOpt->is_set()) result->set_verbosity(verbosityOpt->value());
        if(anyOpt->is_set()) result->set_any(true);
        if(prefExpOpt->is_set()) result->set_preference_exponent(prefExpOpt->value());
        if(timeoutOpt->is_set()) result->set_timeout_seconds(parse_time(timeoutOpt->value()));
        if(csTimeoutOpt->is_set()) result->set_critical_set_timeout_seconds(parse_time(csTimeoutOpt->value()));
        if(noCsOpt->is_set()) result->set_no_critical_sets(true);
        if(threadsOpt->is_set()) result->set_thread_count(threadsOpt->value());

        if(result->verbosity() > 0)
        {
            std::cerr << header << std::endl;
        }

        outResult = result;
        return OK;
    }
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

void Options::set_output_file(string outputFile)
{
    _outputFile = outputFile;
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
