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
#include <QCoreApplication>

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

    int tryAccessDeviceNode(int* fdp, char *nodePath, int nodeLen);
    int initSerialPortForTtyLte(int* fd, char *nodePath, int baudRate, int tryCnt, int nsec);
    int waitWriteableForFd(int fd, int nsec);
    int waitReadableForFd(int fd, timeval *tm);
    int sendCMDofAT(int fd, char *cmd, int len);
    void exitSerialPortFromTtyLte(int* fd);


    int parseConfigFile(char *nodePath, int bufLen, char* key);
    int tryBestToCleanSerialIO(int fd);
    int sendCMDandCheckRecvMsg(int fd, char *cmd, checkStageLTE key, int retryCnt, int RDndelay);
    int parseATcmdACKbyLineOrSpecialCmd(dialingInfo_t &info, char *buf, int len, checkStageLTE e);
    int recvMsgFromATModuleAndParsed(int fd, checkStageLTE key, int nsec);

    char *getKeyLineFromBuf(char *buf, char *key);
    char *cutAskFromKeyLine(char* keyLine, int keyLineLen, const char *srcLine, int argIndex);
    int checkIP(char emergencyFlag);
    int checkInternetAccess(char emergencyFlag = 0);
    int getNativeNetworkInfo(QString ifName, QString &ipString);
    void showDialingResult(dialingInfo_t& info);
public slots:
    int slotMonitorTimerHandler(void);
    int slotRunDialing(checkStageLTE currentStage = STAGE_DEFAULT);
signals:
    void signalDisplay(char stage, QString result);
protected:
    QTimer monitorTimer;
    QMutex mutexDial;
    //QMutex mutexMoniHandler;

    int fd;
    char nodePath[BOXV3_NODEPATH_LENGTH];

    dialingInfo_t dialingInfo;
    QMutex mutexInfo;
    void initDialingState(dialingInfo_t &info);

    void run(void);
};

#endif // THREADDIALING_H
