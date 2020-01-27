#pragma once

#include <random>
#include <mutex>

class Rng
{
private:
    inline static std::mutex _mutex;
    inline static std::mt19937 _mt;
    inline static std::uniform_int_distribution _dist = std::uniform_int_distribution();

    Rng() = default;

public:
    static int next();

    static int next(int min, int max);

    static void seed(int seed);

    static std::mt19937 engine();
};


