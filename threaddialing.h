#ifndef THREADDIALING_H
#define THREADDIALING_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>

#include <QObject>
#include <QThread>
#include <QNetworkInterface>

#include "global.h"

using namespace std;


class lteDialing : public QObject
{
    Q_OBJECT
public:
    lteDialing();
    void showBuf(char* buf, int len);
    void showErrInfo(errInfo_t info);
    int setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed);
    int initUartAndTryCommunicateWith4GModule_ForTest(int* fdp, char* nodePath, char* devNodePath, char* cmd);
    int parseConfigFile(char *nodePath, int bufLen, char* key);
    void initDialingState(dialingResult_t& info);
    int parseATcmdACKbyLineOrSpecialCmd(dialingResult_t &info, char *buf, int len, parseEnum e);
    int tryAccessDeviceNode(int* fdp, char *nodePath, int nodeLen);

    int initSerialPortForTtyLte(int* fd, char *nodePath, int baudRate, int tryCnt, int nsec);
    int waitWriteableForFd(int fd, int nsec);
    int waitReadableForFd(int fd, timeval *tm);
    int sendCMDofAT(int fd, char *cmd, int len);
    int recvMsgFromATModuleAndParsed(int fd, parseEnum key, int nsec);
    void exitSerialPortFromTtyLte(int* fd);
    int sendCMDandCheckRecvMsg(int fd, char *cmd, parseEnum key, int retryCnt, int RDndelay);
    int tryBestToCleanSerialIO(int fd);
    char *getKeyLineFromBuf(char *buf, char *key);
    char *cutAskFromKeyLine(char* keyLine, int argIndex);
    int checkInternetAccess();
    int getNativeNetworkInfo(QString ifName, QString &ipString);
    void showDialingResult(dialingResult_t& info);
signals:
    void signalDialingEnd(QByteArray array);
    void signalDisplayDialingStage(char stage, QString result);
public slots:
    int slotAlwaysRecvMsgForDebug(void);
    int slotHuaWeiLTEmoduleDialingProcess(char resetFlag);
private:
    dialingResult_t dialingResult;
};

class threadDialing: public QObject
{
    Q_OBJECT
    QThread workerThread;
public:
    threadDialing();
    ~threadDialing();

    lteDialing* getWorkerObject();
public slots:
    void handleResults(const QByteArray array);
    void slotStartDialing(char reset);
    void slotDisplayDialingStage(char stage, QString result);
signals:
    void signalStartDialing(char);
    void signalDialingEnd(QByteArray array);
    void signalDisplayDialingStage(char stage, QString result);
private:
    lteDialing* worker;
};

#endif // THREADDIALING_H
