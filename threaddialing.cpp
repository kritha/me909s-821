#include "threaddialing.h"


threadDialing::threadDialing()
{
    fd = -1;
    bzero(nodePath, BOXV3_NODEPATH_LENGTH);

    initDialingState(dialingInfo);

    moveToThread(this);
}

void threadDialing::showBuf(char *buf, int len)
{
    buf[len -1] = '\0';
    for(int i=0; i<len; i++)
    {
        printf("%c", buf[i]);
    }
    puts("");
}


void threadDialing::showErrInfo(errInfo_t info)
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
int threadDialing::setSerialPortNodeProperty(int fd, int databits, int stopbits, int parity, int speed)
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


int threadDialing::initSerialPortForTtyLte(int*fdp, char* nodePath, int baudRate, int tryCnt, int nsec)
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

void threadDialing::exitSerialPortFromTtyLte(int* fd)
{
    if(*fd > 2)
    {
        close(*fd);
        *fd = -1;
    }
}



int threadDialing::waitWriteableForFd(int fd, int nsec)
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

int threadDialing::waitReadableForFd(int fd, struct timeval* tm)
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
int threadDialing::sendCMDofAT(int fd, char *cmd, int len)
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


int threadDialing::initUartAndTryCommunicateWith4GModule_ForTest(int* fdp, char* nodePath, char* devNodePath, char* cmd)
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
                ret = recvMsgFromATModuleAndParsed(*fdp, STAGE_PARSE_OFF, 1);
                DEBUG_PRINTF("ret:%d\n", ret);
            }
        }
        close(*fdp);
    }

    return ret;
}

char* threadDialing::getKeyLineFromBuf(char* buf, char* key)
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

/**
 * @brief threadDialing::cutAskFromKeyLine
 * NOtes: this func can just be used once for one keyLine, otherwise there will be a segmentfault
 * @param keyLine
 * @param argIndex : 0 - n
 * @return string pointer of the argIndex if it's found, and NULL when failed
 */
char *threadDialing::cutAskFromKeyLine(char *keyLine, int keyLineLen, const char* srcLine, int argIndex)
{
    char* token = NULL;

    if(argIndex < 0) argIndex = 0;

    if(!keyLine || !srcLine)
    {
        DEBUG_PRINTF("NULL argument.");
        ERR_RECORDER(NULL);
    }else
    {
        bzero(keyLine, keyLineLen);
        if(strlen(srcLine) > (unsigned int)keyLineLen)
        {
            strncpy(keyLine, srcLine, keyLineLen);
        }else
        {
            strncpy(keyLine, srcLine, strlen(srcLine));
        }
        //get the delim string after ":"
        strtok(keyLine, ":");
        token = strtok(NULL, ":");
        //get the delim string before ","
        token = strtok(token, ",");
        while((!token) && (--argIndex >= 0))
        {
            token = strtok(NULL, ",");
        }
    }

    return token;
}

int threadDialing::tryAccessDeviceNode(int* fdp, char* nodePath, int nodeLen)
{
    int ret = 0;
    ret = parseConfigFile(nodePath, nodeLen, (char*)"NODENAME");
    if(!ret)
    {
        if(*fdp > 2)
        {
            exitSerialPortFromTtyLte(fdp);
        }
        ret = initSerialPortForTtyLte(fdp, nodePath, BOXV3_BAUDRATE_UART, 1, 3);
    }
    return ret;
}


int threadDialing::parseConfigFile(char* nodePath, int bufLen, char *key)
{
    int ret = 0;
    if(!nodePath) return -EINVAL;

    if((signed)strlen(key) > bufLen) return -EINVAL;

    bzero(nodePath, bufLen);
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

/*
 * 描述符:
 * TCIFLUSH     清除输入队列
 * TCOFLUSH     清除输出队列
 * TCIOFLUSH    清除输入、输出队列:
*/
int threadDialing::tryBestToCleanSerialIO(int fd)
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
int threadDialing::sendCMDandCheckRecvMsg(int fd, char* cmd, checkStageLTE key, int retryCnt, int RDndelay)
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
            //else sleep(1);
        }else
            ERR_RECORDER(NULL);
    }
    return ret;
}

int threadDialing::parseATcmdACKbyLineOrSpecialCmd(dialingInfo_t& info, char* buf, const int len, checkStageLTE e)
{
    int ret = 0;
    char* linep = NULL;
    char* delim = NULL;
    char* linepTmp = NULL;

    mutexInfo.lock();

    linepTmp = (char*)malloc(len);

    if(!buf || (len <= 0))
    {
        ERR_RECORDER(NULL);
        DEBUG_PRINTF();
        ret = -EINVAL;
    }
    else
    {
        if((STAGE_CHECK_IP != e) && (STAGE_CHECK_PING != e))
        {
        //buf, must end with a '\0'
            buf[len-1] = '\0';
        }

        switch(e)
        {
        case STAGE_PARSE_SIMPLE:
        {
            if(!strstr(buf, "OK"))
                ret = -ENODATA;
            break;
        }
        case STAGE_RESET:
        {
            info.reset.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"OK");
            if(linep)
            {
                info.reset.result = STAGE_RESULT_SUCCESS;
                strncpy(info.reset.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
            }else
            {
                info.reset.result = STAGE_RESULT_UNKNOWN;
                bzero(info.reset.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
            }
            break;
        }
        case STAGE_AT:
        {
            info.at.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"OK");
            if(linep)
            {
                info.at.result = STAGE_RESULT_SUCCESS;
                strncpy(info.at.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
            }else
            {
                info.at.result = STAGE_RESULT_UNKNOWN;
                bzero(info.at.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
            }
            break;
        }
        case STAGE_ICCID:
        {
            info.iccid.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"^ICCID:");
            if(linep)
            {
                info.iccid.result = STAGE_RESULT_SUCCESS;
                strncpy(info.iccid.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
            }else
            {
                info.iccid.result = STAGE_RESULT_UNKNOWN;
                bzero(info.iccid.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
            }
            break;
        }
        case STAGE_CPIN:
        {
            info.cpin.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"READY");
            if(linep)
            {
                info.cpin.result = STAGE_RESULT_SUCCESS;
                strncpy(info.cpin.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
            }else
            {
                info.cpin.result = STAGE_RESULT_UNKNOWN;
                bzero(info.cpin.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
            }
            break;
        }
        case STAGE_SYSINFOEX:
        {
            /*
             * <srv_status>：表示系统服务状态。
                    0  无服务, 1  服务受限, 2  服务有效, 3  区域服务受限, 4  省电或休眠状态
             * <srv_domain>：表示系统服务域。
                    0  无服务, 1  仅 CS 服务, 2  仅 PS 服务, 3  PS+CS 服务, 4  CS、PS 均未注册，并处于搜索状态, 255  CDMA（暂不支持）
            */
            info.sysinfoex.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"^SYSINFOEX:");
            if(linep)
            {
                strncpy(info.sysinfoex.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
                //check the service domain
                delim = cutAskFromKeyLine(linepTmp, len, linep, 1);
                switch(atoi(delim))
                {
                case 0:
                {
                    info.sysinfoex.result = STAGE_RESULT_FAILED;
                    ret = -ENOPROTOOPT;
                    ERR_RECORDER(NULL);
                    DEBUG_PRINTF("Error: SIM no service");
                    break;
                }
                default:
                {
                    info.sysinfoex.result = STAGE_RESULT_SUCCESS;
                    ret = 0;
                    DEBUG_PRINTF("Success: SIM in avaliable service domain.");
                    break;
                }
                }
            }else
            {
                info.sysinfoex.result = STAGE_RESULT_UNKNOWN;
                bzero(info.sysinfoex.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
        case STAGE_COPS:
        {
            info.cops.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"+COPS:");
            if(linep)
            {
                info.cops.result = STAGE_RESULT_SUCCESS;
                strncpy(info.cops.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);

                if(strstr(linep, "CMCC") || strstr(linep, "CHINA MOBILE"))
                {
                    info.currentOperator = STAGE_OPERATOR_MOBILE;
                }else if(strstr(linep, "CHN-CT"))
                {
                    info.currentOperator = STAGE_OPERATOR_TELECOM;
                }else if(strstr(linep, "CHN-UNICOM"))
                {
                    info.currentOperator = STAGE_OPERATOR_UNICOM;
                }else
                {
                    info.cops.result = STAGE_RESULT_FAILED;
                    info.currentOperator = STAGE_UNKNOWN;
                    ret = -ENODATA;
                    ERR_RECORDER(NULL);
                    DEBUG_PRINTF("");
                }
            }else
            {
                info.cops.result = STAGE_RESULT_UNKNOWN;
                bzero(info.cops.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
                ERR_RECORDER(NULL);
                DEBUG_PRINTF("no +COPS:");
            }
            break;
        }
        case STAGE_SIMSWITCH:
        {
            info.swit.checkCnt++;
            /*'ret' which is very special at here, which is the num of channel, or -errno with errno*/
            linep = getKeyLineFromBuf(buf, (char*)"^SIMSWITCH:");
            if(linep)
            {
                info.swit.result = STAGE_RESULT_SUCCESS;
                strncpy(info.swit.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
                char* chp = getKeyLineFromBuf(linep, (char*)"1");
                if(chp)
                {//channel 1
                    info.currentSlot = STAGE_SLOT_1;
                }else
                {//channel 0
                    info.currentSlot = STAGE_SLOT_0;
                }
            }else
            {
                info.swit.result = STAGE_RESULT_UNKNOWN;
                bzero(info.swit.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
        case STAGE_CSQ:
        {
            info.csq.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"+CSQ:");
            if(linep)
            {
                info.csq.result = STAGE_RESULT_SUCCESS;
                strncpy(info.csq.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
                //delim = cutAskFromKeyLine(linep, 0);
                //if(atoi(delim) > SIM_CSQ_SIGNAL_MIN)
            }else
            {
                info.csq.result = STAGE_RESULT_UNKNOWN;
                ret = -ENODATA;
                bzero(info.csq.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
            }
            break;
        }
        case STAGE_CHIPTEMP:
        {
            info.chiptemp.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"^CHIPTEMP:");
            if(linep)
            {
                info.chiptemp.result = STAGE_RESULT_SUCCESS;
                strncpy(info.chiptemp.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
                //delim = cutAskFromKeyLine(linep, 5);
                //if(atoi(delim) < SIM_TEMP_VALUE_MAX);
            }else
            {
                info.chiptemp.result = STAGE_RESULT_UNKNOWN;
                ret = -ENODATA;
                bzero(info.chiptemp.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
            }
            break;
        }
        case STAGE_NDISSTATQRY:
        {
            info.qry.checkCnt++;
            linep = getKeyLineFromBuf(buf, (char*)"^NDISSTATQRY:");
            if(linep)
            {
                strncpy(info.qry.meAckMsg, linep, AT_ACK_RESULT_INFO_LENGTH);
                if(!strstr(linep, "1,,,\"IPV4\""))
                {
                    info.qry.result = STAGE_RESULT_FAILED;
                    ret = -ENODATA;
                }else
                {
                    info.qry.result = STAGE_RESULT_SUCCESS;
                    DEBUG_PRINTF("###PARSEACK_NDISSTATQRY###");
                    showBuf(linep, BUF_TMP_LENGTH);
                    DEBUG_PRINTF("###PARSEACK_NDISSTATQRY###");
                }
            }else
            {
                info.qry.result = STAGE_RESULT_UNKNOWN;
                bzero(info.qry.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
        case STAGE_CHECK_IP:
        {
            info.ip.checkCnt++;
            linep = buf;
            if(len > 0)
            {
                info.ip.result = STAGE_RESULT_SUCCESS;
                strncpy(info.ip.meAckMsg, buf, AT_ACK_RESULT_INFO_LENGTH);
            }else
            {
                info.ip.result = STAGE_RESULT_UNKNOWN;
                bzero(info.ip.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
                ret = -ENODATA;
            }
            break;
        }
        case STAGE_CHECK_PING:
        {
            info.ping.checkCnt++;
            linep = buf;

            if(STAGE_RESULT_SUCCESS == len)
            {
                info.ping.result = STAGE_RESULT_SUCCESS;
                strcpy(info.ping.meAckMsg, "OK");
            }else
            {
                info.ping.result = STAGE_RESULT_FAILED;
                ret = -ENODATA;
                strcpy(info.ping.meAckMsg, "Bad.");
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
        //display
        QString displayLine = QString(linep);
        emit signalDisplay(e, displayLine);

    }

    free(linepTmp);
    mutexInfo.unlock();

    return ret;
}

int threadDialing::recvMsgFromATModuleAndParsed(int fd, checkStageLTE key, int nsec)
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
        DEBUG_PRINTF("###rdCnt:%d\n", i);
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
            if(STAGE_PARSE_OFF == key)
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
                if(STAGE_PARSE_OFF != key)
                {
                    ret = parseATcmdACKbyLineOrSpecialCmd(dialingInfo, buf, BUF_TMP_LENGTH, key);
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


int threadDialing::checkIP(char emergencyFlag)
{
    char cmdLine[64] = {};
    int ret = 0;

    /*
BusyBox v1.18.3 (2012-05-04 17:50:00 CST) multi-call binary.

Usage: udhcpc [-fbnqoCR] [-i IFACE] [-r IP] [-s PROG] [-p PIDFILE]
        [-H HOSTNAME] [-V VENDOR] [-x OPT:VAL]... [-O OPT]...

        -i,--interface IFACE    Interface to use (default eth0)
        -p,--pidfile FILE       Create pidfile
        -s,--script PROG        Run PROG at DHCP events (default /usr/share/udhcpc/default.script)
        -t,--retries N          Send up to N discover packets
        -T,--timeout N          Pause between packets (default 3 seconds)
        -A,--tryagain N         Wait N seconds after failure (default 20)
        -f,--foreground         Run in foreground
        -b,--background         Background if lease is not obtained
        -n,--now                Exit if lease is not obtained
        -q,--quit               Exit after obtaining lease
        -R,--release            Release IP on exit
        -S,--syslog             Log to syslog too
        -a,--arping             Use arping to validate offered address
        -O,--request-option OPT Request option OPT from server (cumulative)
        -o,--no-default-options Don't request any options (unless -O is given)
        -r,--request IP         Request this IP address
        -x OPT:VAL              Include option OPT in sent packets (cumulative)
                                Examples of string, numeric, and hex byte opts:
                                -x hostname:bbox - option 12
                                -x lease:3600 - option 51 (lease time)
                                -x 0x3d:0100BEEFC0FFEE - option 61 (client id)
        -F,--fqdn NAME          Ask server to update DNS mapping for NAME
        -H,-h,--hostname NAME   Send NAME as client hostname (default none)
        -V,--vendorclass VENDOR Vendor identifier (default 'udhcp VERSION')
        -C,--clientid-none      Don't send MAC as client identifier
    */

    if(emergencyFlag)
    {
        strcpy(cmdLine, "udhcpc -t 1 -n -q -i ");
        strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
        ret = system(cmdLine);
    }

    if(ret)
    {
        ERR_RECORDER("udhcpc failed.");
    }else
    {
        QString ipString;
        ret = getNativeNetworkInfo(QString(LTE_MODULE_NETNODENAME), ipString);
        if(!ret)
        {
            parseATcmdACKbyLineOrSpecialCmd(dialingInfo, ipString.toLocal8Bit().data(), ipString.length(), STAGE_CHECK_IP);
        }
    }

    return ret;
}

int threadDialing::checkInternetAccess(char emergencyFlag)
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

    char cmdLine[64] = {};
    if(emergencyFlag)
    {
        strcpy(cmdLine, "ping -c 1 -s 1 -W 1 -w 2 -I ");
    }else
    {
        strcpy(cmdLine, "ping -c 2 -s 1 -W 7 -w 10 -I ");
    }
    strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
    strncat(cmdLine, " ", 1);
    strncat(cmdLine, INTERNET_ACCESS_POINT, strlen(INTERNET_ACCESS_POINT));

    ret = system(cmdLine);
    if(ret)
    {
        ERR_RECORDER("ping failed.");
    }else
    {
        parseATcmdACKbyLineOrSpecialCmd(dialingInfo, (char*)"STAGE_RESULT_SUCCESS", STAGE_RESULT_SUCCESS, STAGE_CHECK_PING);
    }

    DEBUG_PRINTF("ping ret: %d", ret);

    return ret;
}


int threadDialing::getNativeNetworkInfo(QString ifName, QString& ipString)
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
    }else
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

void threadDialing::showDialingResult(dialingInfo_t &info)
{
    DEBUG_PRINTF("#######################show begin#######################");
    printf("isDialSuccess:%d\n", info.isDialedSuccess);
    printf("AT:");
    showBuf(info.at.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("CPIN:");
    showBuf(info.cpin.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("SYSINFOEX:");
    showBuf(info.sysinfoex.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("SIMSWITCH:");
    showBuf(info.swit.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("COPS:");
    showBuf(info.cops.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("NDISSTATQRY:");
    showBuf(info.qry.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("CSQ:");
    showBuf(info.csq.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("TEMP:");
    showBuf(info.chiptemp.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("IP:");
    showBuf(info.ip.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    printf("PING:");
    showBuf(info.ping.meAckMsg, AT_ACK_RESULT_INFO_LENGTH);
    DEBUG_PRINTF("#######################show end#######################");
}


int threadDialing::slotMonitorTimerHandler()
{
    static int prescaler = 0, dialingCnt = 0;
    int ret = 0;
    enum checkStageLTE isDialedSuccess;

    //dynamic check
    DEBUG_PRINTF("TimerCnt: %d.", prescaler);

    if((0 == prescaler)||(prescaler/MONITOR_TIMER_CHECK_SPECIAL_MULT))
    {
        mutexInfo.lock();
        isDialedSuccess = dialingInfo.isDialedSuccess;
        mutexInfo.unlock();

        DEBUG_PRINTF("DialingCnt: %d", ++dialingCnt);

        if(STAGE_RESULT_SUCCESS != isDialedSuccess)
        {
            slotRunDialing(STAGE_DEFAULT);
        }else
        {
            slotRunDialing(STAGE_MODE_REFRESH);
        }
        //for display
        sleep(6);
    }


    if(prescaler++ > 10000)
    {
        prescaler = 0;
        dialingCnt = 0;
    }

    return ret;
}

int threadDialing::slotRunDialing(enum checkStageLTE currentStage)
{
    int ret = 0, switchCnt = 0, resetCnt = 0;
    static int successCnt = 0, failedCnt = 0;
    QString notes;
    //volatile enum checkStageLTE currentStage;
    enum checkStageLTE currentMode = STAGE_MODE_DIAL;

    //just one slot be triggered at one time
    if(mutexDial.tryLock())
    {
        //currentStage = beginStage;

        if(currentStage > STAGE_NODE)
        {
            DEBUG_PRINTF("Warning: Invalid STAGE, change stage to STAGE_NODE(%d).", STAGE_NODE);
            currentStage = STAGE_NODE;
        }
looper_stage_branch:
        DEBUG_PRINTF("currentStage: %d.", currentStage);
        /*begin dialing*/
        switch(currentStage)
        {
        case STAGE_RESET:
        {
            if(1 > resetCnt++)
            {
                ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^RESET", STAGE_RESET, 2, 2);
                DEBUG_PRINTF("wait reset for more than 10s...");
                sleep(10);
                //call back or kill itself and restart by sysinit:respawn service
                DEBUG_PRINTF("reset done. Start check MT again.");
            }else
            {
                currentStage = STAGE_RESULT_FAILED;
                goto looper_stage_branch;
            }
        }
        case STAGE_MODE_REFRESH:
        {
            currentMode = STAGE_MODE_REFRESH;
        }
        case STAGE_DEFAULT:
        {
            if(STAGE_MODE_REFRESH != currentMode)
            {
                emit signalDisplay(STAGE_DEFAULT, QString("Dialing..."));
            }else
            {
                emit signalDisplay(STAGE_DEFAULT, QString("Refresh..."));
            }
            //check for the internet access before dialing
            ret = checkInternetAccess(1);
            if(!ret)
            {
                DEBUG_PRINTF("Net access is ok formerly.");
                emit signalDisplay(STAGE_DISPLAY_NOTES, QString("Net access is ok formerly."));
                if(STAGE_MODE_REFRESH != currentMode)
                {
                    currentStage = STAGE_RESULT_SUCCESS;
                    goto looper_stage_branch;
                }
            }
        }
        case STAGE_INITENV:
        {
            //init env
            initDialingState(dialingInfo);
            //emit signalDisplay(STAGE_DISPLAY_INIT, QString());

            if(STAGE_MODE_REFRESH != currentMode)
            {
                notes = "Dialing start: ";
            }else
            {
                notes = "Refresh env start: ";
            }
            notes += QDateTime::currentDateTime().toString("MM/dd[HH:mm:ss]");
            emit signalDisplay(STAGE_DISPLAY_NOTES, notes);
        }
        case STAGE_NODE:
        {
            //0. get nodePath and access permission
            ret = tryAccessDeviceNode(&fd, nodePath, sizeof(nodePath));
            if(ret)
            {
                emit signalDisplay(STAGE_NODE, QString("NOdeviceNode"));
                currentStage = STAGE_RESULT_FAILED;
                goto looper_stage_branch;
            }else
            {
                emit signalDisplay(STAGE_NODE, QString(nodePath));
            }
        }
        case STAGE_AT:
        {
            ret = sendCMDandCheckRecvMsg(fd, (char*)"AT", STAGE_AT, 2, 2);

            if(STAGE_MODE_REFRESH != currentMode)
            {
                if(ret)
                {
                    currentStage = STAGE_RESULT_FAILED;
                    goto looper_stage_branch;
                }else
                {
                    currentStage = STAGE_CPIN;
                    goto looper_stage_branch;
                }
            }
        }
        case STAGE_SIMSWITCH:
        {
            if(3 > switchCnt++)
            {
                ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH?", STAGE_SIMSWITCH, 2, 2);

                if(STAGE_MODE_REFRESH != currentMode)
                {
                    DEBUG_PRINTF("try use \"AT^SIMSWITCH\" to enable SIM card...");
                    DEBUG_PRINTF("currentSlot:%d.", dialingInfo.currentSlot);
                    switch(dialingInfo.currentSlot)
                    {
                    case STAGE_SLOT_0:
                    {
                        sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=1", STAGE_SIMSWITCH, 2, 2);
                        sleep(4);
                        break;
                    }
                    case STAGE_SLOT_1:
                    {
                        sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=0", STAGE_SIMSWITCH, 2, 2);
                        sleep(4);
                        break;
                    }
                    default:
                    {
                        DEBUG_PRINTF("Don't know current SIMSWITCH CHANNEL. Change to channel 0.");
                        sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=0", STAGE_SIMSWITCH, 2, 2);
                        sleep(4);
                        break;
                    }
                    }
                }
            }else
            {
                DEBUG_PRINTF("Warning: \"AT^SIMSWITCH\" failed!");
                /*
                * switch doesn't work that have to reset the MT
                * if MT hasn't been reset
                */
                currentStage = STAGE_RESET;
                goto looper_stage_branch;
            }
        }
        case STAGE_CPIN:
        {
            ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CPIN?", STAGE_CPIN, 2, 2);
            if(STAGE_MODE_REFRESH != currentMode)
            {
                if(ret)
                {
                    currentStage = STAGE_SIMSWITCH;
                    goto looper_stage_branch;
                }
            }
        }
        case STAGE_SYSINFOEX:
        {
            ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^SYSINFOEX", STAGE_SYSINFOEX, 2, 1);

            if(STAGE_MODE_REFRESH != currentMode)
            {
                if(ret)
                {
                    currentStage = STAGE_RESULT_FAILED;
                    goto looper_stage_branch;
                }
            }
        }
        case STAGE_COPS:
        {
            ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", STAGE_COPS, 2, 2);

            if(STAGE_MODE_REFRESH != currentMode)
            {
                DEBUG_PRINTF("currentOperator:%d.", dialingInfo.currentOperator);
                switch(dialingInfo.currentOperator)
                {
                case STAGE_OPERATOR_MOBILE:
                {
                    if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"CMNET\"", STAGE_PARSE_SIMPLE, 2, 2))
                    {
                        DEBUG_PRINTF("CMCC dialing end.");
                    }else
                    {
                        DEBUG_PRINTF("CMCC dialing failed. Or has been dialed success before.");
                    }
                    break;
                }
                case STAGE_OPERATOR_TELECOM:
                {
                    //sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=0,1,\"card\", \"card\"", NOTPARSEACK, 2, 2);
                    //sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=0,1", NOTPARSEACK, 2, 2);
                    if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"CTNET\"", STAGE_PARSE_SIMPLE, 2, 2))
                    {
                        DEBUG_PRINTF("CH-T dialing end.");
                    }else
                    {
                        DEBUG_PRINTF("CH-T dialing failed. Or has been dialed success before.");
                    }
                    break;
                }
                case STAGE_OPERATOR_UNICOM:
                {
                    if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"3GNET\"", STAGE_PARSE_SIMPLE, 2, 2))
                    {
                        DEBUG_PRINTF("CH-U dialing end.");
                    }else
                    {
                        DEBUG_PRINTF("CH-U dialing failed. Or has been dialed success before.");
                    }
                    break;
                }
                default:
                {
                    DEBUG_PRINTF("Didn't dialing at all.");
                    ERR_RECORDER(NULL);
                    break;
                }
                }
            }
        }
        case STAGE_NDISSTATQRY:
        {
            ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISSTATQRY?", STAGE_NDISSTATQRY, 2, 2);
        }
        case STAGE_GET_SPECIAL_DATA:
        {
            //some need to known
            sendCMDandCheckRecvMsg(fd, (char*)"AT+CSQ", STAGE_CSQ, 2, 1);
            sendCMDandCheckRecvMsg(fd, (char*)"AT^CHIPTEMP?", STAGE_CHIPTEMP, 2, 1);
            sendCMDandCheckRecvMsg(fd, (char*)"AT^ICCID?", STAGE_ICCID, 2, 1);
        }
        case STAGE_CHECK_PING:
        {
            if(STAGE_MODE_REFRESH != currentStage)
            {
                ret = checkInternetAccess(1);
            }
        }
        case STAGE_CHECK_IP:
        {
            if(ret)
            {
                ret = checkIP(1);
            }else
            {
                ret = checkIP(0);
            }

            if(ret)
            {
                currentStage = STAGE_RESULT_FAILED;
            }else
            {
                currentStage = STAGE_RESULT_SUCCESS;
            }
            goto looper_stage_branch;
        }
        case STAGE_RESULT_SUCCESS:
        {
            mutexInfo.lock();
            dialingInfo.isDialedSuccess = STAGE_RESULT_SUCCESS;
            successCnt++;
            mutexInfo.unlock();

            notes = QString::number(successCnt);
            if(STAGE_MODE_REFRESH != currentMode)
            {
                notes += "Dialed end(success): ";
            }else
            {
                notes += "Refresh env end(success): ";
            }
            notes += QDateTime::currentDateTime().toString("MM/dd[HH:mm:ss]");
            emit signalDisplay(STAGE_DISPLAY_NOTES, notes);
            break;
        }
        case STAGE_RESULT_FAILED:
        {
            mutexInfo.lock();
            dialingInfo.isDialedSuccess = STAGE_RESULT_FAILED;
            failedCnt++;
            mutexInfo.unlock();

            notes = QString::number(failedCnt);
            if(STAGE_MODE_REFRESH != currentMode)
            {
                notes += "Dialed end(failed): ";
            }else
            {
                notes += "Refresh env end(failed): ";
            }
            notes += QDateTime::currentDateTime().toString("MM/dd[HH:mm:ss]");
            emit signalDisplay(STAGE_DISPLAY_NOTES, notes);
            break;
        }
        default:
        {
            DEBUG_PRINTF("UNKNOWN STAGE.");
            emit signalDisplay(STAGE_DISPLAY_NOTES, QString("UNKNOWN STAGE."));
            break;
        }
        }
        exitSerialPortFromTtyLte(&fd);
        /*end dialing*/
        mutexDial.unlock();
    }else
    {
        DEBUG_PRINTF("Warning: wouldn't exec! It isRunning formerly...");
    }

    return ret;
}

void threadDialing::initDialingState(dialingInfo_t &info)
{
    bzero(&info, sizeof(dialingInfo_t));
}

void threadDialing::run()
{
    QObject::connect(&monitorTimer, &QTimer::timeout, this, &threadDialing::slotMonitorTimerHandler, Qt::QueuedConnection);

    monitorTimer.start(MONITOR_TIMER_CHECK_INTERVAL);

    exec();
}

