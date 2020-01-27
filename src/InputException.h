#pragma once

#include "Types.h"

#include <exception>

class InputException : public std::exception
{
private:
    string _msg;

public:
    explicit InputException(string msg);

    [[nodiscard]] string message() const;

    [[nodiscard]] const char* what() const noexcept override;
};


