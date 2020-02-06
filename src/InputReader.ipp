#pragma once

#include "InputReader.h"

using x3::eoi;

template<typename Parser>
bool InputReader::parse(string const& line, Parser parser)
{
    return x3::phrase_parse(line.begin(), line.end(), parser >> eoi, x3::ascii::space);
}

