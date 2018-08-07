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

#include "global.h"

using namespace std;


class THREADDIALING : public QThread
{
    Q_OBJECT
public:
    THREADDIALING();
    void showBuf(char* buf, int len);
    void showErrInfo(errInfo_t info);
    int setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed);
    int initUartAndTryCommunicateWith4GModule_ForTest(int* fdp, char* nodePath, char* devNodePath, char* cmd);
    int parseConfigFile(char *nodePath, int bufLen, char* key);
    int parseATcmdACKbyLine(char *buf, int len, parseEnum e);
    int tryAccessDeviceNode(int* fdp, char *nodePath, int nodeLen);

    int initSerialPortForTtyLte(int* fd, char *nodePath, int baudRate, int tryCnt, int nsec);
    int waitWriteableForFd(int fd, int nsec);
    int waitReadableForFd(int fd, timeval *tm);
    int sendCMDofAT(int fd, char *cmd, int len);
    int recvMsgFromATModuleAndParsed(int fd, parseEnum key, int nsec);
    void exitSerialPortFromTtyLte(int* fd);
    int sendCMDandCheckRecvMsg(int fd, char *cmd, parseEnum key, int retryCnt, int RDndelay);
    int tryBestToCleanSerialIO(int fd);
    int huaweiLTEmoduleDialingProcess(char resetFlag);
    char *getKeyLineFromBuf(char *buf, char *key);
    int checkInternetAccess();
signals:
    void signalSendState(QByteArray tmpArray);
    void signalDialingEnd(QByteArray array=QByteArray(NULL));
public slots:
    void slotSendState(QByteArray tmpArray);
    void slotStartDialing(char reset);
    void slotStopDialing(void);
    int slotAlwaysRecvMsgForDebug(void);
protected:
    void run(void);
};

#endif // THREADDIALING_H
