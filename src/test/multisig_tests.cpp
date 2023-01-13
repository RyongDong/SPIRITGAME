#include <boost/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/assign/list_inserter.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include <openssl/ec.h>
#include <openssl/err.h>

#include "keystore.h"
#include "main.h"
#include "script.h"
#include "wallet.h"

using namespace std;
using namespace boost::assign;

typedef vector<unsigned char> valtype;

extern uint256 SignatureHash(CScript scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);
extern bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn,
                         bool fValidatePayToScriptHash, int nHashType);

BOOST_AUTO_TEST_SUITE(multisig_tests)

CScript
sign_multisig(CScript scriptPubKey, vector<CKey> keys, CTransaction transaction, int whichIn)
{
    uint256 hash = SignatureHash(scriptPubKey, transaction, whichIn, SIGHASH_ALL);

    CScript result;
    result << OP_0; // CHECKMULTISIG bug workaround
    BOOST_FOREACH(C