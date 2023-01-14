#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>

#include "netbase.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(netbase_tests)

BOOST_AUTO_TEST_CASE(netbase_networks)
{
    BOOST_CHECK(CNetAddr("127.0.0.1").GetNetwork()                              == NET_UNROUTABLE);
    BOOST_CHECK(CNetAddr("::1").GetNetwork()                                    == NET_UNROUTABLE);
    BOOST_CHECK(CNetAddr("8.8.8.8").GetNetwork()                                == NET_IPV4);
    BOOST_CHECK(CNetAddr("2001::8888").GetNetwork()                             == NET_IPV6);
    BOOST_CHECK(CNetAddr("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").GetNetwork() == NET_TOR);
}

BOOST_AUTO_TEST_CASE(netbase_properties)
{
    BOOST_CHECK(CNetAddr("127.0.0.1").IsIPv4());
    BOOST_CHECK(CNetAddr("::FFFF:192.168.1.1").IsIPv4());
    BOOST_CHECK(CNetAddr("::1").IsIPv6());
    BOOST_CHECK(CNetAddr("10.0.0.1").IsRFC1918());
    BOOST_CHECK(CNetAddr("192.168.1.1").IsRFC1918());
    BOOST_CHECK(CNetAddr("172.31.255.255").IsRFC1918());
    BOOST_CHECK(CNetAddr("2001:0DB8::").IsRFC3849());
    BOOST_CHECK(CNetAddr("169.254.1.1").IsRFC3927());
    BOOST_CHECK(CNetAddr("2002::1").IsRFC3964());
    BOOST_CHECK(CNetAddr("FC00::").IsRFC4193());
    BOOST_CHECK(CNetAddr("2001::2").IsRFC4380());
    BOOST_CHECK(CNetAddr("2001:10::").IsRFC4843());
    BOOST_CHECK(CNetAddr("FE80::").IsRFC4862());
    BOOST_CHECK(CNetAddr("64:FF9B::").IsRFC6052());
    BOOST_CHECK(CNetAddr("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").IsTor());
    BOOST_CHECK(CNetAddr("127.0.0.1").IsLocal());
    BOOST_CHECK(CNetAddr("::1").IsLocal());
    BOOST_CHECK(CNetAddr("8.8.8.8").IsRoutable());
    BOOST_CHECK(CNetAddr("2001::1").IsRoutable());
    BOOST_CHECK(CNetAddr("127.0.0.1").IsValid());
}

bool static TestSplitHost(string test, string host, int port)
{
    string hostOut;
    int portOut = -1