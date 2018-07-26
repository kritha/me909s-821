#include "huawei4gmodule.h"

HUAWEI4GModule::HUAWEI4GModule()
{
    fd = -1;
}

void HUAWEI4GModule::showBuf(char *buf, int len)
{
    buf[len -1] = '\0';
    for(int i=0; i<len; i++)
    {
        printf("%c", buf[i]);
    }
    puts("");
}


void HUAWEI4GModule::showErrInfo(errInfo_t info)
{
    unsigned int i = 0;
    printf("Error:");
    for(i=0; i<strlen(info.errMsgBuf); i++)
    {
        printf("%c", info.errMsgBuf[i]);
    }
    puts("");
    printf("@Function: %s. ", info.funcp);
    printf("@File: %s(line: %d).\n", info.filep, info.line);
}


/* 设置串口数据位，停止位和效验位
 * @fd           文件句柄
 * @databits     数据位(取值7/8)
 * @stopbits     停止位(取值1/2)
 * @parity       效验类型(取值N/E/O/S)
 * exec success: return 0; failed: return !0(with errno)
*/
int HUAWEI4GModule::setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed)
{
    int ret = 0;
    struct termios uartAttr[2];
    /*获取终端参数，成功返回零；失败返回非零，发生失败接口将设置errno错误标识*/
    if(0 != tcgetattr(fd, &uartAttr[0]))
    {
        ret = -EACCES;
        perror("tcgetattr failed.");
    }else
    {
        /* 先将新串口配置清0 */
        bzero(&uartAttr[1], sizeof(struct termios));
        /* CREAD 开启串行数据接收，CLOCAL并打开本地连接模式 */
        uartAttr[1].c_cflag |= (CLOCAL|CREAD);

        //clear data bits field
        uartAttr[1].c_cflag &= ~CSIZE;

        //set data bits field
        switch(databits)
        {
        case 7:
          uartAttr[1].c_cflag |= CS7;
          break;
        case 8:
          uartAttr[1].c_cflag |= CS8;
          break;
        default:
            ret = -EINVAL;
            perror("Unsupported data bit argument.");
            break;
        }
        /* 设置停止位*/
        switch (stopbits)
        {
        case 1:
            uartAttr[1].c_cflag &= ~CSTOPB;
            break;
        case 2:
            uartAttr[1].c_cflag |= CSTOPB;
            break;
        default:
            ret = -EINVAL;
            perror("Unsupported stop bit argument.");
        }

        switch(parity)
        {
        case 'n':
        case 'N':
            uartAttr[1].c_iflag &= ~INPCK;     /* Enable parity checking */
            uartAttr[1].c_cflag &= ~PARENB;   /* Clear parity enable */
            break;
        case 'o':
            uartAttr[1].c_cflag |= PARENB;             /*开启奇偶校验 */
            uartAttr[1].c_iflag |= (INPCK | ISTRIP);   /*INPCK打开输入奇偶校验；ISTRIP去除字符的第八个比特*/
            uartAttr[1].c_cflag |= PARODD;             /*启用奇校验(默认为偶校验)*/
            break;
        case 'O':
            uartAttr[1].c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
            uartAttr[1].c_iflag |= INPCK;             /* Disnable parity checking */
            break;
        case 'e':
            uartAttr[1].c_cflag |= PARENB;             /*开启奇偶校验 */
            uartAttr[1].c_iflag |= ( INPCK | ISTRIP);  /*打开输入奇偶校验并去除字符第八个比特  */
            uartAttr[1].c_cflag &= ~PARODD;            /*启用偶校验；  */
            break;
        case 'E':
            uartAttr[1].c_cflag |= PARENB;     /* Enable parity */
            uartAttr[1].c_cflag &= ~PARODD;   /* 转换为偶效验*/
            uartAttr[1].c_iflag |= INPCK;       /* Disnable parity checking */
            break;
        case 'S':
        case 's':  /*as no parity*/
            uartAttr[1].c_cflag &= ~PARENB;
            uartAttr[1].c_cflag &= ~CSTOPB;
            break;
        default:
            ret = -EINVAL;
            perror("Unsupported parity bit argument.");
            break;
        }
        /* Set input parity option */
        if (parity != 'n')
            uartAttr[1].c_iflag |= INPCK;

        /*设置波特率, default as B9600*/
        switch(speed)
        {
        case 2400:
            cfsetispeed(&uartAttr[1], B2400);    /*设置输入速度*/
            cfsetospeed(&uartAttr[1], B2400);    /*设置输出速度*/
            break;
        case 4800:
            cfsetispeed(&uartAttr[1], B4800);
            cfsetospeed(&uartAttr[1], B4800);
            break;
        case 9600:
            cfsetispeed(&uartAttr[1], B9600);
            cfsetospeed(&uartAttr[1], B9600);
            break;
        case 115200:
            cfsetispeed(&uartAttr[1], B115200);
            cfsetospeed(&uartAttr[1], B115200);
            break;
        default:
            cfsetispeed(&uartAttr[1], B9600);
            cfsetospeed(&uartAttr[1], B9600);
            break;
        }

        if(!ret)
        {
            tcflush(fd, TCIFLUSH);
            uartAttr[1].c_cc[VTIME] = 0; /* 设置超时0 seconds*/
            uartAttr[1].c_cc[VMIN] = 0; /* Update the options and do it NOW */
            ret = tcsetattr(fd, TCSANOW, &uartAttr[1]);
            if(0 != ret)
            {
                perror("tcsetattr newly failed.");
                if(0 != tcsetattr(fd, TCSANOW, &uartAttr[0]))
                {
                    perror("try to move back to oldly uart attr failed.");
                }else
                {
                    printf("try to move back to oldly uart attr success!\n");
                }
            }
        }

    }
    return ret;
}


int HUAWEI4GModule::initSerialPortForTtyLte(int baudRate, int tryCnt, int nsec)
{
    int ret = 0, i = 0;
    struct timeval tm;

    if(tryCnt < 1) tryCnt = 1;

    for(i=0; i<tryCnt; i++)
    {
        //O_NONBLOCK|O_NOCTTY
        fd = open(nodePath, O_RDWR);
        if(fd < 0)
        {
            ret = -EAGAIN;
            ERR_RECORDER("Unable to open device");
        }else
        {
            ret = setSerialPortNodeProperty(fd, 8, 1, 'N', baudRate);
            /*some sets are wrong*/
            if(0 != ret)
            {
                ret = -EAGAIN;
                exitSerialPortFromTtyLte();
            }else
            {
                /*success*/
                break;
            }
        }
        /*delay, and try again*/
        tm.tv_sec = nsec;
        tm.tv_usec = 0;
        if(nsec < 3) tm.tv_sec = 3;
        select(0, NULL, NULL, NULL, &tm);
    }

    return ret;
}

void HUAWEI4GModule::exitSerialPortFromTtyLte()
{
    close(fd);
    fd = -1;
}



int HUAWEI4GModule::waitWriteableForFd(int fd, int nsec)
{
    int ret = 0;
    fd_set writefds;
    struct timeval tm;

    if(nsec < 1) nsec=1;

    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    tm.tv_sec = nsec;
    tm.tv_usec = 0;
    ret = select(fd+1, NULL, &writefds, NULL, &tm) < 0;
    if(ret < 0)
    {
        ERR_RECORDER(NULL);
    }else
    {
        ret = 0;
    }

    return ret;
}

int HUAWEI4GModule::waitReadableForFd(int fd, struct timeval* tm)
{
    int ret = 0;
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    ret = select(fd+1, &readfds, NULL, NULL, tm) < 0;
    if(ret < 0)
    {
        ERR_RECORDER(NULL);
    }else
    {
        ret = 0;
    }

    return ret;
}
int HUAWEI4GModule::sendCMDofAT(int fd, char *cmd, int len)
{
    int ret = 0, i = 0;
    char cmdStr[AT_CMD_LENGTH_MAX] = {};

    strncpy(cmdStr, cmd, len);
    strncat(cmdStr, AT_CMD_SUFFIX, strlen(AT_CMD_SUFFIX));
    len += strlen(AT_CMD_SUFFIX);

    for(i = 0; i < len; i++)
    {
        ret = waitWriteableForFd(fd, 1);
        if(!ret)
        {
            ret = write(fd, &cmdStr[i], 1);
            if(ret <= 0)
            {
                ret = -EIO;
                ERR_RECORDER(strerror(errno));
            }else
            {
                ret = 0;
            }
        }
    }
    return ret;
}

int HUAWEI4GModule::recvMsgFromATModuleAndParsed(parseEnum key, int nsec)
{
    int ret=0, i=0;
    char ch = 0;
    char buf[BUF_TMP_LENGTH] = {};
    int bufIndex = 0;
    fd_set readfds;
    struct timeval tm;

    if(nsec < 1) nsec = 1;

    for(i=0; i<=10; i++)
    {
        //wait a while for hw before read
        usleep(500);
        //use select timeval to monitor read-timeout and read-end
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        tm.tv_sec = nsec;
        tm.tv_usec = 0;
        ret = select(fd+1, &readfds, NULL, NULL, &tm);
        if(ret < 0)
        {
            ERR_RECORDER(strerror(errno));
        }else if(0 == ret)
        {
            break;
        }else
        {
            ret = 0;
            bzero(buf, sizeof(buf));
            bufIndex = 0;
            if(FD_ISSET(fd, &readfds))
            {
                while(1 == read(fd, &ch, 1))
                {
                    if (bufIndex >= (BUF_TMP_LENGTH - 1))
                    {
                        break;
                    }else
                    {
                        sprintf(&buf[bufIndex++], "%c", ch);
                    }
                }
                printf("###############read:%d\n", i);
                showBuf(buf, BUF_TMP_LENGTH);
                //analyse read date if have key and if we wanna parse it
                if(NOTPARSEACK != key)
                {
                    ret = parseATcmdACKbyLine(buf, BUF_TMP_LENGTH, key);
                    DEBUG_PRINTF("parse(key:%d, ret:%d).", key, ret);
                    if(!ret)
                        break;
                    else
                        ret = -ENODATA;
                }
            }else
            {
                ret = -EINVAL;
                ERR_RECORDER("select wrong, no data to read.");
            }
        }
    }

    return ret;
}


int HUAWEI4GModule::initUartAndTryCommunicateWith4GModule_ForTest(char* nodePath, char* cmd)
{
    int ret = 0;

    if(!nodePath)
    {
        ret = -EPERM;
        ERR_RECORDER("nodePath couldn't be NULL.");
    }else
    {
        ret = initSerialPortForTtyLte(BOXV3_BAUDRATE_UART, 1, 3);
        if(!ret)
        {
            ret = sendCMDofAT(fd, cmd, strlen(cmd));
            if(!ret)
            {
                ret = recvMsgFromATModuleAndParsed(PARSEACK_OK, 1);
            }
        }
        close(fd);
    }

    return ret;
}

int HUAWEI4GModule::parseConfigFile(char *key)
{
    int ret = 0;

    if(strstr(key, "NODENAME"))
    {
        strcpy(nodePath, BOXV3_NODEPATH_LTE);
    }
    else
    {
        ret = -ENOMEM;
        ERR_RECORDER(NULL);
    }

    return ret;
}

char* HUAWEI4GModule::getKeyLineFromBuf(char* buf, char* key)
{
    char* token = NULL;
    const char ss[2] = "\n";

    if(!buf || !key)
    {
        ERR_RECORDER(NULL);
        return NULL;
    }

    token = strtok(buf, ss);
    while(token != NULL)
    {
        if(strstr(token, key))
        {
            break;
        }else
        {
            token = strtok(NULL, ss);
        }
    }

    return token;
}

int HUAWEI4GModule::parseATcmdACKbyLine(char* buf, int len, parseEnum e)
{
    int ret = 0;
    char* linep = NULL;

    if(!buf) ret = -EINVAL;
    else
    {
        //buf, must end with a '\0'
        buf[len - 1] = '\0';

        switch(e)
        {
        case PARSEACK_AT:
        case PARSEACK_RESET:
        case PARSEACK_OK:
        {
            if(!strstr(buf, "OK"))
                ret = -ENODATA;
            break;
        }
        case PARSEACK_CPIN:
        {
            if(!strstr(buf, "READY"))
                ret = -ENODATA;
            break;
        }
        case PARSEACK_REG:
        {
            /* 1(或5) 表示数据业务可以使用；
             * 2 、3 、4 表示数据业务不可用。
             */
            linep = getKeyLineFromBuf(buf, (char*)"+CREG");
            if(linep)
            {
                if((!strstr(linep, "1")) && (!strstr(linep, "5")))
                    ret = -ENODATA;
            }else
                ERR_RECORDER(NULL);
            break;
        }
        case PARSEACK_COPS_CH_M:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+COPS");
            if(linep)
            {
                if((!strstr(linep, "CMCC")) && (!strstr(linep, "CHINA MOBILE")))
                    ret = -ENODATA;
            }else
                ERR_RECORDER(NULL);
            break;
        }
        case PARSEACK_COPS_CH_T:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+COPS");
            if(linep)
            {
                if(!strstr(linep, "CHN-CT"))
                    ret = -ENODATA;
            }else
                ERR_RECORDER(NULL);
            break;
        }
        case PARSEACK_COPS_CH_U:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+COPS");
            if(linep)
            {
                if(!strstr(linep, "CHN-UNICOM"))
                    ret = -ENODATA;
            }else
                ERR_RECORDER(NULL);
            break;
        }
        case PARSEACK_NDISSTATQRY:
        {
            linep = getKeyLineFromBuf(buf, (char*)"^NDISSTAT");
            if(linep)
            {
                if(!strstr(linep, "CHN-UNICOM"))
                    ret = -ENODATA;
            }else
                ERR_RECORDER(NULL);
            break;
        }
        default:
        {
            break;
        }
        }
    }

    return ret;
}
int HUAWEI4GModule::huaweiLTEmoduleDialingProcess(void)
{
    volatile int ret = 0, retryCnt = 0;

    retryCnt = 1;
    do{
        DEBUG_PRINTF("retryCnt: %d", retryCnt);
        //0. get nodePath and access permission
        tryAccessDeviceNode();
        //1. check available for module
        sendCMDandCheckRecvMsg((char*)"AT", PARSEACK_AT, 3, 3);

        //2. check slot & access for SIM
        if(sendCMDandCheckRecvMsg((char*)"AT+CPIN?", PARSEACK_CPIN, 2, 2))
        {
            sendCMDandCheckRecvMsg((char*)"AT^SIMSWITCH=1", PARSEACK_OK, 2, 2);
            sendCMDandCheckRecvMsg((char*)"AT^SIMSWITCH=0", PARSEACK_OK, 2, 2);
        }else
        {
            //tried SIMSWITCH many times, but didn't work, so try RESET for the MT
            if(retryCnt <= 0)
            {
                sendCMDandCheckRecvMsg((char*)"AT^RESET", PARSEACK_RESET, 2, 2);
                DEBUG_PRINTF("wait reset for more than 10s...");
                sleep(10);
                //call back or kill itself and restart by sysinit:respawn service
                ret = -EAGAIN;
                continue;
            }else
            {
                break;
            }
        }
    }while(retryCnt-- > 0);

    if(!ret)
    {
        //3. check data service for SIM
        if(sendCMDandCheckRecvMsg((char*)"AT+CREG?", PARSEACK_REG, 2, 2))
        {
                ERR_RECORDER("can't get data service.");
        }

        if(!ret)
        {
            //4. get network operator
            if(!sendCMDandCheckRecvMsg((char*)"AT+COPS?", PARSEACK_COPS_CH_M, 2, 2))
            {
                //CMNET
                ret |= 0x1;
                sendCMDandCheckRecvMsg((char*)"AT^NDISDUP=1,1,\"CMNET\"", NOTPARSEACK, 2, 2);
            }else if(!sendCMDandCheckRecvMsg((char*)"AT+COPS?", PARSEACK_COPS_CH_T, 2, 2))
            {
                //CTNET
                ret |= 0x10;
            }else if(!sendCMDandCheckRecvMsg((char*)"AT+COPS?", PARSEACK_COPS_CH_U, 2, 2))
            {
                //3GNET
                ret = 0x100;
            }
        }

        if(0 == (ret & 0x111))
        {
            ret = -EINVAL;
            ERR_RECORDER("Didn't get network operator info.");
        }else
        {
            //6. check the dialing result
            if(!(ret = sendCMDandCheckRecvMsg((char*)"AT^NDISSTATQRY?", PARSEACK_NDISSTATQRY, 2, 2)))
            {
                char cmdLine[64] = "udhcpc -t 10 -T 1 -A 1 -n -q -i ";
                strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
                system(cmdLine);
            }
        }
    }

    //7. check for the internet access
    if(!ret)
    {
        ret = checkInternetAccess();
    }

    return ret;
}

int HUAWEI4GModule::tryAccessDeviceNode(void)
{
    int ret = 0;
    ret = parseConfigFile((char*)"NODENAME");
    if(!ret)
    {
        ret = initSerialPortForTtyLte(BOXV3_BAUDRATE_UART, 1, 3);
    }
    return ret;
}

/*
 * 描述符:
 * TCIFLUSH     清除输入队列
 * TCOFLUSH     清除输出队列
 * TCIOFLUSH    清除输入、输出队列:
*/
int HUAWEI4GModule::tryBestToCleanSerialIO(void)
{
    int ret = 0;
#if 1
    int clearLine = 0;
    char buf[1024] = {};
    while(read(fd, buf, sizeof(buf)))
    {
        bzero(buf, sizeof(buf));
        usleep(200);
        if(clearLine++ > 1000) break;
    }
#else
    tcflush(fd, TCIOFLUSH);
#endif
    return ret;
}

/*
 * cmd: AT cmd with out suffix
 * key: what msg you wanna get from recved msg after send msg to MT
 * retryCnt: how many times do you wanna do again when this func exec failed.
 * ndelay: how long would you wait each time when recv every signal msg from MT
*/
int HUAWEI4GModule::sendCMDandCheckRecvMsg(char* cmd, parseEnum key, int retryCnt, int ndelay)
{
    int ret = 0, i=0;

    for(i=0; i<retryCnt; i++)
    {
        tryBestToCleanSerialIO();
        ret = sendCMDofAT(fd, cmd, strlen(cmd));
        if(!ret)
        {
            ret = recvMsgFromATModuleAndParsed(key, ndelay);
            if(!ret) break;
        }else
            ERR_RECORDER(NULL);
    }
    return ret;
}

int HUAWEI4GModule::checkInternetAccess(void)
{
    int ret = 0;
    /* ping
     *
     * -4,-6           Force IP or IPv6 name resolution
     * -c CNT          Send only CNT pings
     * -s SIZE         Send SIZE data bytes in packets (default:56)
     * -t TTL          Set TTL
     * -I IFACE/IP     Use interface or IP address as source
     * -W SEC          Seconds to wait for the first response (default:10)
     *                  (after all -c CNT packets are sent)
     * -w SEC          Seconds until ping exits (default:infinite)
     *                  (can exit earlier with -c CNT)
     * -q              Quiet, only displays output at start
     *                  and when finished
    */
    char cmdLine[64] = "ping -c 2 -s 1 -W 2 -w 3 -I ";
    strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
    strncat(cmdLine, " ", 1);
    strncat(cmdLine, INTERNET_ACCESS_POINT, strlen(INTERNET_ACCESS_POINT));

    ret = system(cmdLine);

    DEBUG_PRINTF("ping ret: %d", ret);

    return ret;
}
