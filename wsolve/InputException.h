#pragma once

#include "Types.h"

#include <exception>
#include <utility>

class InputException : public std::exception
{
private:
    string _msg;

public:
    InputException(string msg)
        : _msg(std::move(msg))
    {
    }

    [[nodiscard]] string message() const
    {
        return _msg;
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return _msg.c_str();
    }
};


