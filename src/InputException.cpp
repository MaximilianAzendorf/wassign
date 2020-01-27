#include "InputException.h"

#include <utility>

InputException::InputException(string msg)
        : _msg(std::move(msg))
{
}

string InputException::message() const
{
    return _msg;
}

const char *InputException::what() const noexcept
{
    return _msg.c_str();
}
