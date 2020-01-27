#include "Rng.h"

int Rng::next()
{
    _mutex.lock();
    int ret = _dist(_mt);
    _mutex.unlock();

    return ret;
}

int Rng::next(int min, int max)
{
    return min + next() % (max - min);
}

void Rng::seed(int seed)
{
    _mutex.lock();
    _mt = std::mt19937(seed);
    _mutex.unlock();
}

std::mt19937 Rng::engine()
{
    return _mt;
}
