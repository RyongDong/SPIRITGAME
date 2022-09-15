#ifndef MININGPAGE_H
#define MININGPAGE_H

#include "clientmodel.h"

#include <QWidget>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTime>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <QSettings>

// Log types
#define STARTED 0
#define SHARE_SUCCESS 1
#define SHARE_FAIL 2
#define ERROR 3
#define LONGPOLL 4

namespace Ui {
    class MiningPage;
}
class ClientModel;

class MiningPage : public QWidget
{
    Q_OBJECT

public:
    explicit MiningPage(QWidget *parent = 0);
    ~MiningPage();

    bool minerActive;

    QProcess *minerProcess;

    QMap<int, double> threadSpeed;

    QTimer *readTimer;

    int acceptedShares;
    int rejectedShares;

    int roundAcceptedShares;
    int roundRejectedShares;

    int initThreads;

    void setModel(ClientModel *model);

public slots:
    void startPressed();

    void startPoolMining();
    void stop