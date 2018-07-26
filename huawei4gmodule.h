#ifndef HUAWEI4GMODULE_H
#define HUAWEI4GMODULE_H

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

#include "global.h"

using namespace std;


class HUAWEI4GModule
{
private:
    int fd;
    char nodePath[128];
public:
    HUAWEI4GModule();
    void showBuf(char* buf, int len);
    void showErrInfo(errInfo_t info);
    int setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed);
    int initUartAndTryCommunicateWith4GModule_ForTest(char *nodePath, char *cmd);
    int parseConfigFile(char* key);
    int parseATcmdACKbyLine(char *buf, int len, parseEnum e);
    int tryAccessDeviceNode(void);

    int initSerialPortForTtyLte(int baudRate, int tryCnt, int nsec);
    int waitWriteableForFd(int fd, int nsec);
    int waitReadableForFd(int fd, timeval *tm);
    int sendCMDofAT(int fd, char *cmd, int len);
    int recvMsgFromATModuleAndParsed(parseEnum key, int nsec);
    void exitSerialPortFromTtyLte();
    int sendCMDandCheckRecvMsg(char *cmd, parseEnum key, int retryCnt, int ndelay);
    int tryBestToCleanSerialIO();
    int huaweiLTEmoduleDialingProcess();
    char *getKeyLineFromBuf(char *buf, char *key);
    int checkInternetAccess();
};

#endif // HUAWEI4GMODULE_H
