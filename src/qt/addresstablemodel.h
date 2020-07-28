#ifndef ADDRESSTABLEMODEL_H
#define ADDRESSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class AddressTablePriv;
class CWallet;
class WalletModel;

/**
   Qt model of the address book in the core. This allows views to access and modify the address book.
 */
class AddressTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AddressTableModel(CWallet *wallet, WalletModel *parent = 0);
    ~AddressTableModel();

    enum ColumnIndex {
        Label = 0,   /**< User specified label */
        Address = 1  /**< Bitcoin address */
    };

    enum RoleIndex {
        TypeRole = Qt::UserRole /**< Type of address (#Send or #Receive) */
    };

    /** Return status of edit/insert operation */
    enum EditStatus {
        OK,
        INVALID_ADDRESS,   /**< Unparseable address */
        DUPLICATE_ADDRESS,  /**< Address already in address book */
        WALLET_UNLOCK_FAILURE, /**< Wallet could not be unlocked to create new receiving address */
        KEY_GENERATION_FAILURE /**< Generating a new public key for a receiving address failed */
    };

    static const QString Send; /**< Specifies send address */
    static const QString Receive; /**< Specifies receive address */

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const Q