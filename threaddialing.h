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
#include <QDateTime>
#include <QTimer>
#include <QMutex>

#include "global.h"

class threadDialing: public QThread
{
    Q_OBJECT
public:
    threadDialing();
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
    int checkInternetAccess(char emergencyFlag = 0);
    int getNativeNetworkInfo(QString ifName, QString &ipString);
    void showDialingResult(dialingResult_t& info);
public slots:
    int slotAlwaysRecvMsgForDebug(void);
    int slotStartDialing(char resetFlag);
    int slotMonitorTimerHandler(void);
signals:
    //void signalDialingEnd(QString str);
    void signalDisplay(char stage, QString result);
private:
    int fd;
    char nodePath[BOXV3_NODEPATH_LENGTH];
    dialingResult_t dialingResult;
protected:
    void run(void);
    QMutex mutexDial;
    QTimer monitorTimer;
    int isDialing;
};

#endif // THREADDIALING_H
