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
#include <QThreadStorage>

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
    int parseATcmdACKbyLine(char *buf, int len, parseEnum e);
    int generateDialingState(parseEnum e, int ret);
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
    int checkInternetAccess();
signals:
    void signalDialingEnd(QByteArray array);
public slots:
    int slotAlwaysRecvMsgForDebug(void);
    int slotHuaWeiLTEmoduleDialingProcess(char resetFlag);
protected:
    void run();
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
signals:
    void signalStartDialing(char);
    void signalDialingEnd(QByteArray array);
private:
    lteDialing* worker;
};

#endif // THREADDIALING_H
