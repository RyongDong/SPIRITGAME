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
            return NULL;
        return ptxn;
    }
};

extern CDBEnv bitdb;


/** RAII class that provides access to a Berkeley database */
class CDB
{
protected:
    Db* pdb;
    std::string strFile;
    DbTxn *activeTxn;
    bool fReadOnly;

    explicit CDB(const char* pszFile, const char* pszMode="r+");
    ~CDB() { Close(); }
public:
    void Close();
private:
    CDB(const CDB&);
    void operator=(const CDB&);

protected:
    template<typename K, typename T>
    bool Read(const K& key, T& value)
    {
        if (!pdb)
            return false;

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(&ssKey[0], ssKey.size());

        // Read
        Dbt datValue;
        datValue.set_flags(DB_DBT_MALLOC);
        int ret = pdb->get(activeTxn, &datKey, &datValue, 0);
        memset(datKey.get_data(), 0, datKey.get_size());
        if (datValue.get_data() == NULL)
            return false;

        // Unserialize value
        try {
            CDataStream ssValue((char*)datValue.get_data(), (char*)datValue.get_data() + datValue.get_size(), SER_DISK, CLIENT_VERSION);
            ssValue >> value;
        }
        catch (std::exception &e) {
            return false;
        }

        // Clear and free memory
        memset(datValue.get_data(), 0, datValue.get_size());
        free(datValue.get_data());
        return (ret == 0);
    }

    template<typename K, typename T>
    bool Write(const K& key, const T& value, bool fOverwrite=true)
    {
        if (!pdb)
            return false;
        if (fReadOnly)
            assert(!"Write called on database in read-only mode");

        // Key
        CDataStream ssKey(SER_DISK, CLIENT_VERSION);
        ssKey.reserve(1000);
        ssKey << key;
        Dbt datKey(&ssKey[0], ssKey.size());

        // Value
        CDataStream ssValue(SER_DISK, CLIENT_VERSION);
        ssValue.reserve(10000);
        ssValue << value;
        Dbt datValue(&ssValue[0], ssValue.size());

        // Write
        int ret = pdb->put(activeTxn, &datKey, &datValue, (fOverwrite ? 0 : DB_NOOVERWRITE));

        // Clear memory in case it was a private 