#include "transactiontablemodel.h"
#include "guiutil.h"
#include "transactionrecord.h"
#include "guiconstants.h"
#include "transactiondesc.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "bitcoinunits.h"

#include "wallet.h"
#include "ui_interface.h"

#include <QLocale>
#include <QList>
#include <QColor>
#include <QTimer>
#include <QIcon>
#include <QDateTime>
#include <QtAlgorithms>

// Amount column is right-aligned it contains numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter
    };

// Comparison operator for sort/binary search of model tx list
struct TxLessThan
{
    bool operator()(const TransactionRecord &a, const TransactionRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const TransactionRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const TransactionRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
class TransactionTablePriv
{
public:
    TransactionTablePriv(CWallet *wallet, TransactionTableModel *parent):
            wallet(wallet),
            parent(parent)
    {
    }
    CWallet *wallet;
    TransactionTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<TransactionRecord> cachedWallet;

    /* Query entire wallet anew from core.
     */
    void refreshWallet()
    {
        OutputDebugStringF("refreshWallet\n");
        cachedWallet.clear();
        {
            LOCK(wallet->cs_wallet);
            for(std::map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); ++it)
            {
                if(TransactionRecord::showTransaction(it->second))
                    cachedWallet.append(TransactionRecord::decomposeTransaction(wallet, it->second));
            }
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(const uint256 &hash, int status)
    {
        OutputDebugStringF("updateWallet %s %i\n", hash.ToString().c_str(), status);
        {
            LOCK(wallet->cs_wallet);

            // Find transaction in wallet
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
            bool inWallet = mi != wallet->mapWallet.end();

            // Find bounds of this transaction in model
            QList<TransactionRecord>::iterator lower = qLowerBound(
                cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
            QList<TransactionRecord>::iterator upper = qUpperBound(
                cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
            int lowerIndex = (lower - cachedWallet.begin());
            int upperIndex = (upper - cachedWallet.begin());
            bool inModel = (lower != upper);

            // Determine whether to show transaction or not
            bool showTransaction = (inWallet && TransactionRecord::showTransaction(mi->second));

            if(status == CT_UPDATED)
            {
                if(showTransaction && !inModel)
                    status = CT_NEW; /* Not in model, but want to show, treat as new */
                if(!showTransaction && inModel)
                    status = CT_DELETED; /* In model, but want to hide, treat as deleted */
            }

            OutputDebugStringF("   inWallet=%i inModel=%i Index=%i-%i showTransaction=%i derivedStatus=%i\n",
                     inWallet, inModel, lowerIndex, upperIndex, showTransaction, status);

            switch(status)
            {
            case CT_NEW:
                if(inModel)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_NEW, but transaction is already in model\n");
                    break;
                }
                if(!inWallet)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_NEW, but transaction is not in wallet\n");
                    break;
                }
                if(showTransaction)
                {
                    // Added -- insert at the right position
                    QList<TransactionRecord> toInsert =
                            TransactionRecord::decomposeTransaction(wallet, mi->second);
                    if(!toInsert.isEmpty()) /* only if something to insert */
                    {
                        parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                        int insert_idx = lowerIndex;
                        foreach(const TransactionRecord &rec, toInsert)
                        {
                            cachedWallet.insert(insert_idx, rec);
                            insert_idx += 1;
                        }
                        parent->endInsertRows();
                    }
                }
                break;
            case CT_DELETED:
                if(!inModel)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_DELETED, but transaction is not in model\n");
                    break;
             