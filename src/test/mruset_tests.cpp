#include <boost/test/unit_test.hpp>

using namespace std;

#include "mruset.h"
#include "util.h"

#define NUM_TESTS 16
#define MAX_SIZE 100

class mrutester
{
private:
    mruset<int> mru;
    std::set<int> set;

public:
    mrutester() { mru.max_size(MAX_SIZE); }
    int size() const { return set.size(); }

    void insert(int n)
    {
        mru.insert(n);
        set.insert(n);
        BOOST_CHECK(mru == set);
    }
};

BOOST_AUTO_TEST_SUITE(mruset_tests)

// Test that an mruset behaves like a set, as long as no more than MAX_SIZE elements are in it
BOOST_AUTO_TEST_CASE(mruset_like_set)
{

    for (int nTest=0; nTest<NUM_TESTS; nTest++)
    {
        mrutester tester;
        while (tester.size() < MAX_SIZE)
            tester.insert(GetRandInt(2 * MAX_SIZE));
    }

}

// Test that an mruset's size never exceeds its max_size
BOOST_AUTO_TEST_CASE(mruset_limited_size)
{
    for (int nTest=0; nTest<NUM_TESTS; nT