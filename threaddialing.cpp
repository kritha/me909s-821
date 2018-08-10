#include "threaddialing.h"

lteDialing::lteDialing()
{
    initDialingState(dialingResult);
}

void lteDialing::showBuf(char *buf, int len)
{
    buf[len -1] = '\0';
    for(int i=0; i<len; i++)
    {
        printf("%c", buf[i]);
    }
    puts("");
}


void lteDialing::showErrInfo(errInfo_t info)
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
int lteDialing::setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed)
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


int lteDialing::initSerialPortForTtyLte(int*fdp, char* nodePath, int baudRate, int tryCnt, int nsec)
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

void lteDialing::exitSerialPortFromTtyLte(int* fd)
{
    close(*fd);
    *fd = -1;
}



int lteDialing::waitWriteableForFd(int fd, int nsec)
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

int lteDialing::waitReadableForFd(int fd, struct timeval* tm)
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
int lteDialing::sendCMDofAT(int fd, char *cmd, int len)
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

int lteDialing::recvMsgFromATModuleAndParsed(int fd, parseEnum key, int nsec)
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
                    ret = parseATcmdACKbyLineOrSpecialCmd(dialingResult, buf, BUF_TMP_LENGTH, key);
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


int lteDialing::initUartAndTryCommunicateWith4GModule_ForTest(int* fdp, char* nodePath, char* devNodePath, char* cmd)
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

int lteDialing::parseConfigFile(char* nodePath, int bufLen, char *key)
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

char* lteDialing::getKeyLineFromBuf(char* buf, char* key)
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

void lteDialing::initDialingState(dialingResult_t &info)
{
    bzero(&info, sizeof(dialingResult_t));
}

int lteDialing::parseATcmdACKbyLineOrSpecialCmd(dialingResult_t& info, char* buf, int len, parseEnum e)
{
    int ret = 0;
    char* linep = NULL;

    if(!buf)
    {
        ERR_RECORDER(NULL);
        DEBUG_PRINTF();
        ret = -EINVAL;
    }
    else
    {
        if((SPECIAL_PARSE_IP_INFO != e) && (SPECIAL_PARSE_PING_RESULT != e))
        {
        //buf, must end with a '\0'
            buf[len - 1] = '\0';
        }

        switch(e)
        {
        case PARSEACK_OK:
        {
            if(!strstr(buf, "OK"))
                ret = -ENODATA;
            break;
        }
        case PARSEACK_RESET:
        {
            if(!strstr(buf, "OK"))
                ret = -ENODATA;
            break;
        }
        case PARSEACK_AT:
        {

            linep = getKeyLineFromBuf(buf, (char*)"OK");
            if(linep)
            {
                strncpy(info.atAck, linep, sizeof(info.atAck));
            }else
            {
                bzero(info.atAck, sizeof(info.atAck));
                ret = -ENODATA;
            }

            break;
        }
        case PARSEACK_ATI:
        {
            if(!strstr(buf, "OK"))
                ret = -ENODATA;
            break;
        }
        case PARSEACK_CPIN:
        {
            linep = getKeyLineFromBuf(buf, (char*)"READY");
            if(linep)
            {
                strncpy(info.cpinAck, linep, sizeof(info.cpinAck));
            }else
            {
                bzero(info.cpinAck, sizeof(info.cpinAck));
                ret = -ENODATA;
            }
            break;
        }
        case PARSEACK_REG:
        {
            /* 1(或5) 表示数据业务可以使用；
             * 2 、3 、4 表示数据业务不可用。
             *
             * +CREG: <stat>[,<lac>,<ci>[,<AcT>]].

             * <n>:
                0  Disable network registration unsolicited result code +CREG. (default value)
                1  Enable network registration unsolicited result code +CREG: <stat>.
                2  Enable network registration and location information unsolicited result code

             * <stat>:
                0  Not registered, MS is not currently searching for a new operator to register
                with.
                1  Registered, home network.
                2  Not registered, but MS is currently searching for a new operator to register
                with.
                3  Registration denied.
                4  Unknown.
                5  Registered, roaming.
             */
            linep = getKeyLineFromBuf(buf, (char*)"+CREG:");
            if(linep)
            {
                strncpy(info.cregAck, linep, sizeof(info.cregAck));
                if((!strstr(linep, "1")) && (!strstr(linep, "5")))
                {
                    ret = -ENODATA;
                    //debug
                    if(!strstr(linep, "2"))
                    {
                        DEBUG_PRINTF("---debug---reg: 0,2");
                        ret = 0;
                    }
                    //debug end
                }
            }else
            {
                bzero(info.cregAck, sizeof(info.cregAck));
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
        case PARSEACK_COPS:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+COPS:");
            if(linep)
            {
                strncpy(info.copsAck, linep, sizeof(info.copsAck));
            }else
            {
                bzero(info.copsAck, sizeof(info.copsAck));
                ret = -ENODATA;
            }
            break;
        }
        case PARSEACK_SWITCH_CHANNEL:
        {
            /*'ret' which is very special at here, which is the num of channel, or -errno with errno*/
            linep = getKeyLineFromBuf(buf, (char*)"^SIMSWITCH:");
            if(linep)
            {
                strncpy(info.switchAck, linep, sizeof(info.switchAck));
                char* chp = getKeyLineFromBuf(linep, (char*)"1");
                if(chp)
                {//channel 1
                    ret = 1;
                }else
                {//channel 0
                    ret = 0;
                }
            }else
            {
                bzero(info.switchAck, sizeof(info.switchAck));
                ret = -ENODATA;
            }
            break;
        }
        case PARSEACK_COPS_CH_M:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+COPS:");
            if(linep)
            {
                strncpy(info.copsAck, linep, sizeof(info.copsAck));
                if((!strstr(linep, "CMCC")) && (!strstr(linep, "CHINA MOBILE")))
                    ret = -ENODATA;
            }else
            {
                bzero(info.copsAck, sizeof(info.copsAck));
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
        case PARSEACK_COPS_CH_T:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+COPS:");
            if(linep)
            {
                strncpy(info.copsAck, linep, sizeof(info.copsAck));
                if(!strstr(linep, "CHN-CT"))
                    ret = -ENODATA;
            }else
            {
                bzero(info.copsAck, sizeof(info.copsAck));
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
        case PARSEACK_COPS_CH_U:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+COPS:");
            if(linep)
            {
                strncpy(info.copsAck, linep, sizeof(info.copsAck));
                if(!strstr(linep, "CHN-UNICOM"))
                    ret = -ENODATA;
            }else
            {
                bzero(info.copsAck, sizeof(info.copsAck));
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
        case PARSEACK_CSQ:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+CSQ:");
            if(linep)
            {
                strncpy(info.csqAck, linep, sizeof(info.csqAck));
            }else
            {
                ret = -ENODATA;
                bzero(info.csqAck, sizeof(info.csqAck));
            }

            break;
        }
        case PARSEACK_TEMP:
        {
            linep = getKeyLineFromBuf(buf, (char*)"^CHIPTEMP:");
            if(linep)
            {
                strncpy(info.tempAck, linep, sizeof(info.tempAck));
            }else
            {
                ret = -ENODATA;
                bzero(info.tempAck, sizeof(info.tempAck));
            }
            break;
        }
        case PARSEACK_NDISSTATQRY:
        {
            linep = getKeyLineFromBuf(buf, (char*)"^NDISSTATQRY:");
            if(linep)
            {
                strncpy(info.qryAck, linep, sizeof(info.qryAck));
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
                bzero(info.qryAck, sizeof(info.qryAck));
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }

            break;
        }
        case SPECIAL_PARSE_IP_INFO:
        {
            if(len > 0)
            {
                strncpy(info.ipinfo, buf, len);
            }else
            {
                bzero(info.ipinfo, sizeof(info.ipinfo));
                ret = -ENODATA;
            }
            break;
        }
        case SPECIAL_PARSE_PING_RESULT:
        {
            if(!len)
            {
                strcpy(info.pingAck, "OK");
                info.isDialedOk = 1;
            }else
            {
                ret = -ENODATA;
                info.isDialedOk = 0;
                strcpy(info.pingAck, "Not good.");
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

int lteDialing::tryAccessDeviceNode(int* fdp, char* nodePath, int nodeLen)
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
int lteDialing::tryBestToCleanSerialIO(int fd)
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
int lteDialing::sendCMDandCheckRecvMsg(int fd, char* cmd, parseEnum key, int retryCnt, int RDndelay)
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

int lteDialing::checkInternetAccess(void)
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

    parseATcmdACKbyLineOrSpecialCmd(dialingResult, (char*)"ret", ret, SPECIAL_PARSE_PING_RESULT);
    DEBUG_PRINTF("ping ret: %d", ret);

    return ret;
}


int lteDialing::getNativeNetworkInfo(QString ifName, QString& ipString)
{
    int ret = 0;

    QNetworkInterface interface;
    QList<QNetworkAddressEntry> entryList;
    QList<QNetworkInterface> list = QNetworkInterface::allInterfaces();
    int i=0, j = 0;

    if(ifName.isEmpty())
    {
        ret = -EINVAL;
        DEBUG_PRINTF("interface name is empty.");
    }
    else
    {
        //clean ip string
        ipString = QString("");

        for(i = 0; i<list.count(); i++)
        {
            interface = list.at(i);
            entryList= interface.addressEntries();

            if(interface.name() != ifName) continue;
            //qDebug() << "DevName: " << interface.name();
            //qDebug() << "Mac: " << interface.hardwareAddress();

            //20180324log: There's error about it'll has a double scan for same name if you use the entryList.count(), What a fuck?
            if(entryList.isEmpty())
            {
                DEBUG_PRINTF("Error: doesn't get a ip.");
                break;
            }else
            {
                QNetworkAddressEntry entry = entryList.at(j);
                ipString = entry.ip().toString();
                //entry.ip().toIPv4Address());
                DEBUG_PRINTF("IP: %s.", entry.ip().toString().toLocal8Bit().data());
                //qDebug() << "j"<<j<< "Netmask: " << entry.netmask().toString();
                //qDebug() << "j"<<j<< "Broadcast: " << entry.broadcast().toString();
            }
        }

        if(ipString.isEmpty() || ipString.isNull())
        {
            ERR_RECORDER(NULL);
            DEBUG_PRINTF();
            ret = -ENODATA;
        }
        DEBUG_PRINTF();

    }

    return ret;
}

void lteDialing::showDialingResult(dialingResult_t &info)
{
    DEBUG_PRINTF("#######################show begin#######################");
    printf("ret:%d\n", info.isDialedOk);
    printf("stage:%d\n", info.stage);
    printf("AT:");
    showBuf(info.atAck, sizeof(info.atAck));
    printf("ATI");
    showBuf(info.atiAck, sizeof(info.atiAck));
    printf("CPIN:");
    showBuf(info.cpinAck, sizeof(info.cpinAck));
    printf("SWITCH:");
    showBuf(info.switchAck, sizeof(info.switchAck));
    printf("CREG:");
    showBuf(info.cregAck, sizeof(info.cregAck));
    printf("COPS:");
    showBuf(info.copsAck, sizeof(info.copsAck));
    printf("NDISSTATQRY:");
    showBuf(info.qryAck, sizeof(info.qryAck));
    printf("CSQ:");
    showBuf(info.csqAck, sizeof(info.csqAck));
    printf("TEMP:");
    showBuf(info.tempAck, sizeof(info.tempAck));
    printf("IP:");
    showBuf(info.ipinfo, sizeof(info.ipinfo));
    printf("PING:");
    showBuf(info.pingAck, sizeof(info.pingAck));
    DEBUG_PRINTF("#######################show end#######################");
}


int lteDialing::slotAlwaysRecvMsgForDebug()
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
        ret = setSerialPortNodeProperty(fd, 8, 1, 'N', BOXV3_BAUDRATE_UART);
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


int lteDialing::slotHuaWeiLTEmoduleDialingProcess(char resetFlag)
{
    char probeCntFlag = 0, currentChannel = 0;
    int ret = 0, fd = -1;
    char nodePath[128] = {};


looper_check_stage_devicenode:
    initDialingState(dialingResult);
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
//looper_check_stage_ltemodule:
        ret = sendCMDandCheckRecvMsg(fd, (char*)"AT", PARSEACK_AT, 2, 2);
        if(ret)
        {
            ret = -EIO;
            printf("Error: MT doesn't work.");
        }else
        {
            DEBUG_PRINTF("Success: MT is working.");

            //close display the cmd back
            //sendCMDandCheckRecvMsg(fd, (char*)"ATE0", PARSEACK_AT, 2, 1);
            /*
             * RESET the MT by user in force
            */
            if(resetFlag)
            {
               ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^RESET", PARSEACK_RESET, 2, 2);
               DEBUG_PRINTF("wait reset for more than 10s...");
               sleep(10);
               //call back or kill itself and restart by sysinit:respawn service
               DEBUG_PRINTF("reset done. Start check MT again.");
               goto looper_check_stage_devicenode;
            }

            //2. check slot & access for SIM
looper_check_stage_simslot:
            ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CPIN?", PARSEACK_CPIN, 2, 2);
            if(!ret)
            {
                DEBUG_PRINTF("Success: found a SIM card");
            }else
            {
                ret = -EAGAIN;
                DEBUG_PRINTF("Warning: \"AT+CPIN?\" failed!");
                if(3 > probeCntFlag)
                {
                    if(!(probeCntFlag++/2))
                    {
                        DEBUG_PRINTF("try use \"AT^SIMSWITCH\" to enable SIM card...");

                        currentChannel = sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH?", PARSEACK_SWITCH_CHANNEL, 2, 2);
                        switch(currentChannel)
                        {
                        case 0:
                        {
                            sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=1", PARSEACK_SWITCH_CHANNEL, 2, 2);
                            sleep(4);
                            break;
                        }
                        case 1:
                        {
                            sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=0", PARSEACK_SWITCH_CHANNEL, 2, 2);
                            sleep(4);
                            break;
                        }
                        default:
                        {
                            DEBUG_PRINTF("Don't know current SIMSWITCH CHANNEL. Change to channel 1.");
                            sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=1", PARSEACK_SWITCH_CHANNEL, 2, 2);
                            sleep(4);
                            break;
                        }
                        }
                        goto looper_check_stage_simslot;
                    }else
                    {
                        DEBUG_PRINTF("Warning: \"AT^SIMSWITCH\" failed!");
                        /*
                        * switch doesn't work that have to reset the MT
                        * if MT hasn't been reset
                        */

                        ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^RESET", PARSEACK_RESET, 2, 2);
                        DEBUG_PRINTF("wait reset for more than 10s...");
                        sleep(10);
                        //call back or kill itself and restart by sysinit:respawn service
                        DEBUG_PRINTF("reset done. Start check MT again.");
                        goto looper_check_stage_devicenode;
                    }
                }
            }
        }
    }

    if(ret)
    {
        printf("Error: no SIM card.\n");
        goto looper_dialing_exit;
    }else
    {
        //3. check data service for SIM
//looper_check_stage_dataservice:
        ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CREG?", PARSEACK_REG, 2, 2);
        if(ret)
        {
            ERR_RECORDER("can't get data service.");
            goto looper_dialing_exit;
        }else
        {
            ;/*ChinaT: +REG:0, 2   but is works well.*/
        }

        if(1)
        {
            DEBUG_PRINTF("Success: the SIM card has data service.");

            ret = 0;
            //4 get network operator
//looper_check_stage_dialingoperator:
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

            }else if(!sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", PARSEACK_COPS_CH_U, 2, 2))
            {
                //3GNET
                ret |= 0x10;
                if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"3GNET\"", PARSEACK_OK, 2, 2))
                {
                    DEBUG_PRINTF("CH-U dialing end.");
                }else
                {
                    DEBUG_PRINTF("CH-U dialing failed. Or has been dialed success before.");
                }
            }else if(!sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", PARSEACK_COPS_CH_T, 2, 2))
            {
                //CTNET
                ret = 0x100;
                //sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=0,1,\"card\", \"card\"", NOTPARSEACK, 2, 2);
                //sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=0,1", NOTPARSEACK, 2, 2);
                if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"CTNET\"", PARSEACK_OK, 2, 2))
                {
                    DEBUG_PRINTF("CH-T dialing end.");
                }else
                {
                    DEBUG_PRINTF("CH-T dialing failed. Or has been dialed success before.");
                }
            }

//looper_check_stage_getip:
            //6.1 check the dialing result
            if(!(ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISSTATQRY?", PARSEACK_NDISSTATQRY, 2, 2)))
            {
                char cmdLine[64] = "udhcpc -t 10 -T 1 -A 1 -n -q -i ";
                strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
                ret = system(cmdLine);
            }else
            {
                DEBUG_PRINTF("Warning: \"AT^NDISSTATQRY?\" failed! Dialing failed.");
                goto looper_dialing_exit;
            }

        }
    }

    //7.0 parse if has a ip
    if(1)
    {
        QString ipString;
        getNativeNetworkInfo(QString((char*)LTE_MODULE_NETNODENAME), ipString);
        parseATcmdACKbyLineOrSpecialCmd(dialingResult, ipString.toLocal8Bit().data(), ipString.length(), SPECIAL_PARSE_IP_INFO);
    }
    DEBUG_PRINTF();
    //7.1 check for the internet access
//looper_check_stage_dialingresult:
    if(!ret)
    {
        ret = checkInternetAccess();
    }

looper_dialing_exit:
    //get some other info: signalquality & temperature
    sendCMDandCheckRecvMsg(fd, (char*)"AT+CSQ", PARSEACK_CSQ, 2, 1);
    sendCMDandCheckRecvMsg(fd, (char*)"AT^CHIPTEMP?", PARSEACK_TEMP, 2, 1);

    showDialingResult(dialingResult);

    QByteArray result("");
    emit signalDialingEnd(result);

    return ret;
}

threadDialing::threadDialing()
{
    worker = new lteDialing();
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &threadDialing::signalStartDialing, worker, &lteDialing::slotHuaWeiLTEmoduleDialingProcess);
    connect(worker, &lteDialing::signalDialingEnd, this, &threadDialing::handleResults);
    workerThread.start();
}

threadDialing::~threadDialing()
{
    workerThread.quit();
    workerThread.wait();
}

lteDialing *threadDialing::getWorkerObject()
{
    return worker;
}

void threadDialing::handleResults(const QByteArray array)
{
    emit signalDialingEnd(array);
}

void threadDialing::slotStartDialing(char reset)
{
    reset = 0;
    emit signalStartDialing(reset);
}
