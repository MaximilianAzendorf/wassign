#include "common.h"

#include "../src/UnionFind.h"

TEST_CASE("Union find constructor works", UNIT)
{
    REQUIRE(UnionFind<int>(0).size() == 0);
    REQUIRE(UnionFind<int>(1).size() == 1);
    REQUIRE(UnionFind<int>(5).size() == 5);

    REQUIRE_THROWS(UnionFind<int>(-1).size() == 5);
}

TEST_CASE("Union find works", UNIT)
{
    const int MAX = 4;
    UnionFind<int> uf(MAX);

    SECTION("Initial state is correct (find)")
    {
        for(int i = 0; i < MAX; i++)
        {
            for(int j = i + 1; j < MAX; j++)
            {
                REQUIRE(uf.find(i) != uf.find(j));
            }
        }
    }

    SECTION("Initial state is correct (groups)")
    {
        vector<bool> found(MAX);
        auto groups = uf.groups();

        for(int i = 0; i < MAX; i++)
        {
            REQUIRE(groups[i].size() == 1);
            REQUIRE(!found[groups[i].front()]);

            found[groups[i].front()] = true;
        }
    }

    SECTION("Join works")
    {
        uf.join(0, 1);
        uf.join(1, 3);
        REQUIRE(uf.find(0) == uf.find(1));
        REQUIRE(uf.find(1) == uf.find(3));
        REQUIRE(uf.find(1) != uf.find(2));

        bool foundJoinedGroup = false;
        for(auto group : uf.groups())
        {
            if(group.size() == 3)
            {
                REQUIRE(!foundJoinedGroup);
                foundJoinedGroup = true;
                std::sort(group.begin(), group.end());
                REQUIRE(group[0] == 0);
                REQUIRE(group[1] == 1);
                REQUIRE(group[2] == 3);
            }
        }

        REQUIRE(foundJoinedGroup);
    }
}