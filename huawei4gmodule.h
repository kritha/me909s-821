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

#include <string>
#include <map>

using namespace std;

#define BOXV3_UARTNODE_DIR "/dev"
#define BOXV3_NODENAME_PREFIX_4G    "ttyUSB"
#define BOXV3_NODEPATH_4G   "/dev/ttyUSB0"
#define BOXV3_BAUDRATE_UART 9600
#define BOXV3_MSG_LENGTH_4G    1024

class HUAWEI4GModule
{
private:
    string uartNodePath;
    map<string, string> cmdMapAT;
    int fd;
    char msg[BOXV3_MSG_LENGTH_4G];
public:
    HUAWEI4GModule();
    //HUAWEI4GModule(string nodePath(const char* s));
    void showBuf(char* buf, int len);
    int setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed);
    int initUartPort(int *fd, char *nodePath);
    int tryCommunicateWith4GModule(int fd, char *buf, int cmdLen, int len);
    int communicationCheck(char *cmd, char *checkString);
    int initUartAndTryCommunicateWith4GModule(char *nodePath);
    int parseMsgFrom4GModule(char *data);
    int tryCommunicateWith4GAndParse(int fd);

    void initCmdStringMap(void);
    int startCheck(char* nodePath);
};

#endif // HUAWEI4GMODULE_H
