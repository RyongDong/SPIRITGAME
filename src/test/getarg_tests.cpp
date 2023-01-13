#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#include "util.h"

BOOST_AUTO_TEST_SUITE(getarg_tests)

static void
ResetArgs(const std::string& strArg)
{
    std::vector<std::string> vecArg;
    boost::split(vecArg, strArg, boost::is_space(), boost::token_compress_on);

    // Insert dummy executable name:
    vecArg.insert(vecArg.begin(), "testbitcoin");

    // Convert to char*:
    std::vector<const char*> vecChar;
    BOOST_FOREACH(std::string& s, vecArg)
        vecChar.push_back(s.c_str());

    ParseParameters(vecChar.size(), &vecChar[0]);
}

BOOST_AUTO_TEST_CASE(boolarg)
{
    ResetArgs("-foo");
    BOOST_CHECK(GetBoolArg("-foo"));
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    BOOST_CHECK(!GetBoolArg("-fo"));
    BOOST_CHECK(!GetBoolArg("-fo", false));
    BOOST_CHECK(GetBoolArg("-fo", true));

    BOOST_CHECK(!GetBoolArg("-fooo"));
    BOOST_CHECK(!GetBoolArg("-fooo", false));
    BOOST_CHECK(GetBoolArg("-fooo", true));

    ResetArgs("-foo=0");
    BOOST_CHECK(!GetBoolArg("-foo"));
    BOOST_CHECK(!GetBoolArg("-foo", false));
    BOOST_CHECK(!GetBoolArg("-foo", true));

    ResetArgs("-foo=1");
    BOOST_CHECK(GetBoolArg("-foo"));
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    // New 0.6 feature: auto-map -nosomething to !-something:
    ResetArgs("-nofoo");
    BOOST_CHECK(!GetBoolArg("-foo"));
    BOOST_CHECK(!GetBoolArg("-foo", false));
    BOOST_CHECK(!GetBoolArg("-foo", true));

    ResetArgs("-nofoo=1");
    BOOST_CHECK(!GetBoolArg("-foo"));
    BOOST_CHECK(!GetBoolArg("-foo", false));
    BOOST_CHECK(!GetBoolArg("-foo", true));

    ResetArgs("-foo -nofoo");  // -foo should win
    BOOST_CHECK(GetBoolArg("-foo"));
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    ResetArgs("-foo=1 -nofoo=1");  // -foo should win
    BOOST_CHECK(GetBoolArg("-foo"));
    BOOST_CHECK(GetBoolArg("-foo", false));
    BOOST_CHECK(GetBoolArg("-foo", true));

    ResetArgs("-foo=0 -nofoo=0");  // -foo should win
    BOOST_CHECK(!GetBoolArg("-foo"));
    BOOST_CHECK(!GetBoolA