#include "threaddialing.h"

THREADDIALING::THREADDIALING()
{
}

void THREADDIALING::showBuf(char *buf, int len)
{
    buf[len -1] = '\0';
    for(int i=0; i<len; i++)
    {
        printf("%c", buf[i]);
    }
    puts("");
}


void THREADDIALING::showErrInfo(errInfo_t info)
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
int THREADDIALING::setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed)
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


int THREADDIALING::initSerialPortForTtyLte(int*fdp, char* nodePath, int baudRate, int tryCnt, int nsec)
{
    int ret = 0, i = 0;
    struct timeval tm;

    if(!fdp || !nodePath) return -EINVAL;

    if(tryCnt < 1) tryCnt = 1;

    for(i=0; i<tryCnt; i++)
    {
        //*fdp = open(nodePath, O_RDWR);
        *fdp = open(nodePath, O_RDWR|O_NOCTTY);
        if(*fdp < 0)
        {
            ret = -EAGAIN;
            ERR_RECORDER("Unable to open device");
        }else
        {
            ret = setSerialPortNodeProperty(*fdp, 8, 1, 'N', baudRate);
            /*some sets are wrong*/
            if(0 != ret)
            {
                ret = -EAGAIN;
                exitSerialPortFromTtyLte(fdp);
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

void THREADDIALING::exitSerialPortFromTtyLte(int* fd)
{
    close(*fd);
    *fd = -1;
}



int THREADDIALING::waitWriteableForFd(int fd, int nsec)
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

int THREADDIALING::waitReadableForFd(int fd, struct timeval* tm)
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
int THREADDIALING::sendCMDofAT(int fd, char *cmd, int len)
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

int THREADDIALING::recvMsgFromATModuleAndParsed(int fd, parseEnum key, int nsec)
{
    int ret=0, i=0;
    char ch = 0;
    char buf[BUF_TMP_LENGTH] = {};
    int bufIndex = 0;
    fd_set readfds;
    struct timeval tm;

    if(nsec < 1) nsec = 1;

    DEBUG_PRINTF("####################################");
    for(i=0; i<=10; i++)
    {
        DEBUG_PRINTF("#######iturn:%d\n", i);
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
            if(NOTPARSEACK == key)
            {
                ret = 0;
            }else
            {
                ret = -ETIMEDOUT;
            }
            break;
        }else
        {
            ret = 0;
            bzero(buf, BUF_TMP_LENGTH);
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
                }else
                {
                    DEBUG_PRINTF("key is NOTPARSEACK.");
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


int THREADDIALING::initUartAndTryCommunicateWith4GModule_ForTest(int* fdp, char* nodePath, char* devNodePath, char* cmd)
{
    int ret = 0;

    if(!devNodePath)
    {
        ret = -EPERM;
        ERR_RECORDER("nodePath couldn't be NULL.");
    }else
    {
        strcpy(nodePath, devNodePath);
        ret = initSerialPortForTtyLte(fdp, nodePath, BOXV3_BAUDRATE_UART, 1, 3);
        if(!ret)
        {
            ret = sendCMDofAT(*fdp, cmd, strlen(cmd));
            if(!ret)
            {
                ret = recvMsgFromATModuleAndParsed(*fdp, NOTPARSEACK, 1);
                DEBUG_PRINTF("ret:%d\n", ret);
            }
        }
        close(*fdp);
    }

    return ret;
}

int THREADDIALING::parseConfigFile(char* nodePath, int bufLen, char *key)
{
    int ret = 0;
    if(!nodePath) return -EINVAL;

    if((signed)strlen(key) > bufLen) return -EINVAL;

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

char* THREADDIALING::getKeyLineFromBuf(char* buf, char* key)
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

int THREADDIALING::parseATcmdACKbyLine(char* buf, int len, parseEnum e)
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
        case PARSEACK_RESET:
        case PARSEACK_AT:
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
            {
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
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
            {
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
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
            {
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
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
            {
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
        case PARSEACK_NDISSTATQRY:
        {
            linep = getKeyLineFromBuf(buf, (char*)"^NDISSTAT");
            if(linep)
            {
                if(!strstr(linep, "1,,,\"IPV4\""))
                {
                    ret = -ENODATA;
                }else
                {
                    DEBUG_PRINTF("###PARSEACK_NDISSTATQRY###");
                    showBuf(linep, BUF_TMP_LENGTH);
                    DEBUG_PRINTF("###PARSEACK_NDISSTATQRY###");
                }
            }else
            {
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }

            break;
        }
        default:
        {
            ret = -ENODATA;
            ERR_RECORDER(NULL);
            break;
        }
        }
    }

    return ret;
}
int THREADDIALING::huaweiLTEmoduleDialingProcess(char reset)
{
    char resetFlag = 0, tryOnceFlag = 0;
    int ret = 0, retryCnt = 0, fd = -1;
    char nodePath[128] = {};


    retryCnt = 2;
looper_dialing_init:
    //0. get nodePath and access permission
    ret = tryAccessDeviceNode(&fd, nodePath, sizeof(nodePath));
    if(ret)
    {
        ret = -ENODEV;
        printf("Error: Can't access MT device node.");
    }else
    {
        DEBUG_PRINTF("Success: access MT device node.");
        //1. check available for module
        ret = sendCMDandCheckRecvMsg(fd, (char*)"AT", PARSEACK_AT, 2, 2);
        if(ret)
        {
            ret = -EIO;
            printf("Error: MT doesn't work.");
        }else
        {
            DEBUG_PRINTF("Success: MT is working.");

            /*
             * RESET the MT by user in force
            */
            DEBUG_PRINTF("resetFlag: %d", resetFlag);
            if(reset)
            {
                //if MT hasn't been reset
                if(resetFlag)
                {
                    ret = -EOWNERDEAD;
                    DEBUG_PRINTF("MT couldn't work. MT has been reset, wouldn't reset again.");
                }else
                {
                   resetFlag = 1;
                   ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^RESET", PARSEACK_RESET, 2, 2);
                   DEBUG_PRINTF("wait reset for more than 10s...");
                   sleep(10);
                   //call back or kill itself and restart by sysinit:respawn service
                   DEBUG_PRINTF("reset done. Start check MT again.");
                   goto looper_dialing_init;
                }
            }

            //2. check slot & access for SIM
            do{
                DEBUG_PRINTF("retryCnt: %d", retryCnt);
                ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CPIN?", PARSEACK_CPIN, 2, 2);
                if(!ret)
                {
                    DEBUG_PRINTF("Success: found a SIM card");
                    break;
                }else
                {
                    ret = -EAGAIN;
                    DEBUG_PRINTF("Warning: \"AT+CPIN?\" failed!");
                    if(!tryOnceFlag)
                    {
                        tryOnceFlag = 1;
                        DEBUG_PRINTF("try use \"AT^SIMSWITCH\" to enable SIM card...");
                        sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=1", PARSEACK_OK, 3, 2);
                        sleep(2);
                        sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH?", PARSEACK_OK, 1, 2);
                        sleep(2);
                        sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=0", PARSEACK_OK, 3, 2);
                        sleep(2);
                        continue;
                    }else
                    {
                        DEBUG_PRINTF("Warning: \"AT^SIMSWITCH\" failed!");
                        /*
                         * so many time switch doesn't work that have to reset the MT
                         * if MT hasn't been reset
                         */
                        if(resetFlag)
                        {
                            ret = -EOWNERDEAD;
                            DEBUG_PRINTF("MT couldn't work. MT has been reset, wouldn't reset again.");
                        }else
                        {
                           resetFlag = 1;
                           ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^RESET", PARSEACK_RESET, 2, 2);
                           DEBUG_PRINTF("wait reset for more than 10s...");
                           sleep(10);
                           //call back or kill itself and restart by sysinit:respawn service
                           DEBUG_PRINTF("reset done. Start check MT again.");
                           goto looper_dialing_init;
                        }
                    }
                }
            }while(retryCnt-- > 0);
        }
    }

    if(ret)
    {
        printf("Error: no SIM card.\n");
        if(resetFlag)
        {
            printf("Warning: if a SIM card had inserted in the slot, please plug it again. The slot is loose.\n");
        }
        goto looper_dialing_exit;
    }else
    {
        //3. check data service for SIM
        ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CREG?", PARSEACK_REG, 2, 2);
        if(ret)
        {
            ERR_RECORDER("can't get data service.");
            goto looper_dialing_exit;
        }else
        {
            DEBUG_PRINTF("Success: the SIM card has data service.");
            ret = 0;
            //4.0 check the dialing result
            if(!(ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISSTATQRY?", PARSEACK_NDISSTATQRY, 1, 2)))
            {
                DEBUG_PRINTF("Notices: the SIM card already dialing success.");
                char cmdLine[64] = "udhcpc -t 4 -T 1 -A 1 -n -q -i ";
                strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
                ret = system(cmdLine);
            }else
            {
                ret = 0;
                //4.1 get network operator
                if(!sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", PARSEACK_COPS_CH_M, 2, 2))
                {
                    //CMNET
                    ret |= 0x1;
                    if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"CMNET\"", PARSEACK_OK, 2, 2))
                    {
                        DEBUG_PRINTF("CMCC dialing end.");
                    }else
                    {
                        DEBUG_PRINTF("CMCC dialing failed. Or has been dialed success before.");
                    }

                }else if(!sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", PARSEACK_COPS_CH_T, 2, 2))
                {
                    //CTNET
                    ret |= 0x10;
                }else if(!sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", PARSEACK_COPS_CH_U, 2, 2))
                {
                    //3GNET
                    ret = 0x100;
                }

                if(0 == (ret & 0x111))
                {
                    ret = -EINVAL;
                    ERR_RECORDER("Didn't get network operator info.");
                    goto looper_dialing_exit;
                }else
                {
                    //6. check the dialing result
                    if(!(ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISSTATQRY?", PARSEACK_NDISSTATQRY, 2, 2)))
                    {
                        char cmdLine[64] = "udhcpc -t 10 -T 1 -A 1 -n -q -i ";
                        strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
                        ret = system(cmdLine);
                    }else
                    {
                        DEBUG_PRINTF("Warning: \"AT^NDISSTATQRY?\" failed! Dialing failed.");
                        sendCMDandCheckRecvMsg(fd, (char*)"AT+CSQ", NOTPARSEACK, 2, 1);
                        goto looper_dialing_exit;
                    }
                }
            }
        }
    }

    //7. check for the internet access
    if(!ret)
    {
        ret = checkInternetAccess();
    }

looper_dialing_exit:

    emit signalDialingEnd();
    return ret;
}

int THREADDIALING::tryAccessDeviceNode(int* fdp, char* nodePath, int nodeLen)
{
    int ret = 0;
    ret = parseConfigFile(nodePath, nodeLen, (char*)"NODENAME");
    if(!ret)
    {
        ret = initSerialPortForTtyLte(fdp, nodePath, BOXV3_BAUDRATE_UART, 1, 3);
    }
    return ret;
}

/*
 * 描述符:
 * TCIFLUSH     清除输入队列
 * TCOFLUSH     清除输出队列
 * TCIOFLUSH    清除输入、输出队列:
*/
int THREADDIALING::tryBestToCleanSerialIO(int fd)
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
 * RDndelay: how long would you wait each time when recv every signal msg from MT
*/
int THREADDIALING::sendCMDandCheckRecvMsg(int fd, char* cmd, parseEnum key, int retryCnt, int RDndelay)
{
    int ret = 0, i=0;

    for(i=0; i<retryCnt; i++)
    {
        tryBestToCleanSerialIO(fd);
        ret = sendCMDofAT(fd, cmd, strlen(cmd));
        if(!ret)
        {
            ret = recvMsgFromATModuleAndParsed(fd, key, RDndelay);
            if(!ret) break;
        }else
            ERR_RECORDER(NULL);
    }
    return ret;
}

int THREADDIALING::checkInternetAccess(void)
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
    char cmdLine[64] = "ping -c 2 -s 1 -W 7 -w 10 -I ";
    strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
    strncat(cmdLine, " ", 1);
    strncat(cmdLine, INTERNET_ACCESS_POINT, strlen(INTERNET_ACCESS_POINT));

    ret = system(cmdLine);
    if(ret)
        ERR_RECORDER("ping failed.");

    DEBUG_PRINTF("ping ret: %d", ret);

    return ret;
}

void THREADDIALING::slotSendState(QByteArray tmpArray)
{

}

void THREADDIALING::slotStartDialing(char reset)
{
    if(this->isRunning())
    {
        DEBUG_PRINTF("Dialing thread this->quit()...");
        this->quit();
        DEBUG_PRINTF("Dialing thread this->wait()...");
        this->wait();
    }

    DEBUG_PRINTF("Dialing thread this->start()...");
    this->start();
}

void THREADDIALING::slotStopDialing()
{
    this->quit();
}

int THREADDIALING::slotAlwaysRecvMsgForDebug()
{
    int ret = 0, fd = -1;
    char ch = 0;
    char buf[BUF_TMP_LENGTH] = {};
    int bufIndex = 0;
    fd_set readfds;

    //0. get nodePath and access permission*fdp = open(nodePath, O_RDONLY);
    fd = open(BOXV3_NODEPATH_LTE, O_RDONLY);
    if(fd < 0)
    {
        ret = -EAGAIN;
        ERR_RECORDER("Unable to open device");
    }else
    {
        ret = setSerialPortNodeProperty(fd, 8, 1, 'N', B9600);
        /*some sets are wrong*/
        if(0 != ret)
        {
            ret = -EAGAIN;
            exitSerialPortFromTtyLte(&fd);
        }else
        {
            /*success*/
            while(1)
            {
                //use select timeval to monitor read-timeout and read-end
                FD_ZERO(&readfds);
                FD_SET(fd, &readfds);
                ret = select(fd+1, &readfds, NULL, NULL, NULL);
                if(ret < 0)
                {
                    ERR_RECORDER(strerror(errno));
                }else if(0 == ret)
                {
                    printf("...\n");
                    sleep(1);
                }else
                {
                    bzero(buf, BUF_TMP_LENGTH);
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
                        showBuf(buf, BUF_TMP_LENGTH);
                    }
                }
            }
        }
    }

    close(fd);
    return ret;
}

void THREADDIALING::run()
{
    int ret = 0;
    dialingResult_t dialResultTmp;

    ret = huaweiLTEmoduleDialingProcess(0);

    exec();
}
