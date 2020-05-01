// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2012 Litecoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_DB_H
#define BITCOIN_DB_H

#include "main.h"

#include <map>
#include <string>
#include <vector>

#include <db_cxx.h>

class CAddress;
class CAddrMan;
class CBlockLocator;
class CDiskBlockIndex;
class CDiskTxPos;
class CMasterKey;
class COutPoint;
class CTxIndex;
class CWallet;
class CWalletTx;

extern unsigned int nWalletDBUpdated;

void ThreadFlushWalletDB(void* parg);
bool BackupWallet(const CWallet& wallet, const std::string& strDest);


class CDBEnv
{
private:
    bool fDetachDB;
    bool fDbEnvInit;
    boost::filesystem::path pathEnv;

    void EnvShutdown();

public:
    mutable CCriticalSection cs_db;
    DbEnv dbenv;
    std::map<std::string, int> mapFileUseCount;
    std::map<std::string, Db*> mapDb;

    CDBEnv();
    ~CDBEnv();
    bool Open(boost::filesystem::path pathEnv_);
    void Close();
    void Flush(bool fShutdown);
    void CheckpointLSN(std::string strFile);
    void SetDetach(bool fDetachDB_) { fDetachDB = fDetachDB_; }
    bool GetDetach() { return fDetachDB; }

    void CloseDb(const std::string& strFile);

    DbTxn *TxnBegin(int flags=DB_TXN_WRITE_NOSYNC)
    {
        DbTxn* ptxn = NULL;
        int ret = dbenv.txn_begin(NULL, &ptxn, flags);
        if (!ptxn || ret != 0)
        