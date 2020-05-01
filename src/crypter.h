// Copyright (c) 2009-2012 The Bitcoin Developers
// Copyright (c) 2011-2012 Litecoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef __CRYPTER_H__
#define __CRYPTER_H__

#include "allocators.h" /* for SecureString */
#include "key.h"
#include "serialize.h"

const unsigned int WALLET_CRYPTO_KEY_SIZE = 32;
const unsigned int WALLET_CRYPTO_SALT_SIZE = 8;

/*
Private key encryption is done based on a CMasterKey,
which holds a salt and random encryption key.

CMasterKeys are encrypted using AES-256-CBC using a key
derived using derivation method nDerivationMethod
(0 == EVP_sha512()) and derivation iterations nDeriveIterations.
vchOtherDerivationParameters is provided for alternative algorithms
which may require more parameters (such as scrypt).

Wallet Private Keys are then encrypted using AES-256-CBC
with the double-sha256 of the public key as the IV, and the
master key's key as the encryption key (see keystore.[ch]).
*/

/** Master key for wallet encryption */
class CMasterKey
{
public:
    std::vector<unsigned char> vchCryptedKey;
    std::vector<unsigned char> vchSalt;
    // 0 = EVP_sha512()
    // 1 = scrypt()
    unsigned int nDerivationMethod;
    unsig