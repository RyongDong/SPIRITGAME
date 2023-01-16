#include <vector>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#include "script.h"
#include "key.h"

using namespace std;

// Helpers:
static std::vector<unsigned char>
Serialize(const CScript& s)
{
    std::vector<unsigned char> sSerialized(s);
    return sSerialized;
}

BOOST_AUTO_TEST_SUITE(sigopcount_tests)

BOOST_AUTO_TEST_