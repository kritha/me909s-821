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

#include <string>
#include <map>

using namespace std;

//#define BOXV3_UARTNODE_DIR "/dev"
//#define BOXV3_NODENAME_PREFIX_4G    "ttyUSB"
#define BOXV3_NODEPATH_4G   "/dev/ttyUSB0"
#define BOXV3_BAUDRATE_UART 9600
#define BOXV3_MSG_LENGTH_4G    1024

#define AT_CMD_SUFFIX   "\r"
#define CHINAMOBILE "CMNET"
#define VPNLENGTH   32

class HUAWEI4GModule
{
private:
    int fd;
    char msg[BOXV3_MSG_LENGTH_4G];
    char networkServerVPN[VPNLENGTH];
public:
    HUAWEI4GModule();
    //HUAWEI4GModule(string nodePath(const char* s));
    void showBuf(char* buf, int len);
    void showErrInfo(errInfo_t info);
    int setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed);
    int communicationCheck(char *cmd, char *checkString);
    int initUartAndTryCommunicateWith4GModule_ForTest(char *nodePath, char *cmd);
    int tryCommunicateWith4GModule(char *nodePath, int retryCnt);
    int checkGeneralInfo();
    int checkSIMCard(void);
    int init4GModule(bool reset);
    int dialingForNetwork();

    int initSerialPortFor4GModule(int *fd, char *nodePath, int baudRate);
    int waitWriteableForFd(int fd, timeval *tm);
    int waitReadableForFd(int fd, timeval *tm);
    int sendCMDofAT(int fd, char *cmdStr, int len);
    int recvMsgFromATModule(int fd);
};

#endif // HUAWEI4GMODULE_H
