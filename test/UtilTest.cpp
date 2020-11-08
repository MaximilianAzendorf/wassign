#include "common.h"

#include "../src/Types.h"
#include "../src/Util.h"

#define PREFIX "[Util] "

TEST_CASE(PREFIX "str should work")
{
    REQUIRE(str((int)5) == "5");
    REQUIRE(str((unsigned)5) == "5");
    REQUIRE(str(5.5f).rfind("5.5", 0) == 0);
    REQUIRE(str(5.5).rfind("5.5", 0) == 0);
    REQUIRE(str(5.124, 2) == "5.12");
    REQUIRE(str(seconds(5)) == "00:00:05");
    REQUIRE(str(seconds(3 * 3600 + 26 * 60 + 53)) == "03:26:53");
    REQUIRE(str(secondsf(61.1)) == "00:01:01");
    REQUIRE(str(nanoseconds(123000000000L)) == "00:02:03");
}

TEST_CASE(PREFIX "Riffle shuffle should work")
{
    const int SIZE = 10;

    vector<int> v1(SIZE), v1b;
    vector<int> v2(SIZE), v2b;

    std::iota(v1.begin(), v1.end(), 0);
    std::iota(v2.begin(), v2.end(), SIZE);

    vector<int> res = riffle_shuffle(v1, v2);

    for(int n : res)
    {
        (n < SIZE ? v1b : v2b).push_back(n);
    }

    REQUIRE(v1 == v1b);
    REQUIRE(v2 == v2b);
}