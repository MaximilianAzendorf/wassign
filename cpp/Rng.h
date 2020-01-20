#pragma once

#include <random>
#include <mutex>

class Rng
{
private:
    static std::mutex _mutex;
    static std::mt19937 _mt;
    inline static std::uniform_int_distribution _dist = std::uniform_int_distribution();

    Rng() = default;

public:
    static int next()
    {
        _mutex.lock();
        int ret = _dist(_mt);
        _mutex.unlock();

        return ret;
    }

    static int next(int min, int max)
    {
        return min + next() % (max - min);
    }

    static int seed(int seed)
    {
        _mutex.lock();
        _mt = std::mt19937(seed);
        _mutex.unlock();
    }

    static std::mt19937 engine()
    {
        return _mt;
    }
};


