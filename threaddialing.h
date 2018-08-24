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

#include <iostream>
using namespace std;
#include "tinyxml2.h"
using namespace tinyxml2;

class threadDialing: public QThread
{
    Q_OBJECT
public:
    threadDialing();
    ~threadDialing();
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


    void initOutputFiles(void);
    void clearSimStatesOfOutputFiles(char simSt = 0);
    int parseConfigFile(char *nodePath, int bufLen, char* key);
    int tryBestToCleanSerialIO(int fd);
    int sendCMDandCheckRecvMsg(int fd, char *cmd, checkStageLTE key, int retryCnt, int RDndelay);
    int parseATcmdACKbyLineOrSpecialCmd(dialingInfo_t &info, char *buf, int len, checkStageLTE e);
    int recvMsgFromATModuleAndParsed(int fd, checkStageLTE key, int nsec);

    char *getKeyLineFromBuf(char *dst, int len, char *buf, char *key);
    char *cutAskFromKeyLine(char* keyLine, int keyLineLen, const char *srcLine, int argIndex, const char firstToken = ' ');
    int checkIP(char emergencyFlag);
    int checkInternetAccess(char emergencyFlag = 0);
    int getNativeNetworkInfo(QString ifName, QString &ipString);
    void showDialingResult(dialingInfo_t& info);

    int createLogFile(QString dirFullPath);
    int writeLogLTE(checkStageLTE c, QString logStr=QString());
    int tryCreateDefaultXMLConfigFile(const char* xmlPath);
    int queryWholeXMLConfigFile(const char* xmlPath);
public slots:
    int slotMonitorTimerHandler(void);
    int slotRunDialing(checkStageLTE currentStage = STAGE_INITENV);
signals:
    void signalDisplay(char stage, QString result);
protected:
    int fd;
    char nodePath[BOXV3_NODEPATH_LENGTH];
    QMutex mutexDial;
    QTimer monitorTimer;
    QMutex mutexInfo;
    dialingInfo_t dialingInfo_tmp;
    dialingInfo_t dialingInfo;
    apnNodeInfo_t* apnNodeList;
    apnNodeInfo_t* newApnNodeAndInit(apnNodeInfo_t **head, QMutex& m);
    int releaseNodeList(apnNodeInfo_t* head);
    apnNodeInfo_t *parseWholeXMLConfigFileAndGenerateNodeList(const char* xmlPath, apnNodeInfo_t **head, QMutex& m);
    int getApnNodeListFromConfigFile(const char* xmlPath = APNNODE_XML_CONFIG_FILE);

    void run(void);
};

#endif // THREADDIALING_H
