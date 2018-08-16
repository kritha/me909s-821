#include "threaddialing.h"


threadDialing::threadDialing()
{
    fd = -1;
    isDialing = 0;
    bzero(nodePath, BOXV3_NODEPATH_LENGTH);

    initDialingState(dialingResult);

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

int threadDialing::recvMsgFromATModuleAndParsed(int fd, parseEnum key, int nsec)
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
                ret = recvMsgFromATModuleAndParsed(*fdp, NOTPARSEACK, 1);
                DEBUG_PRINTF("ret:%d\n", ret);
            }
        }
        close(*fdp);
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
char *threadDialing::cutAskFromKeyLine(char *keyLine, int argIndex)
{
    char* token = NULL;

    if(argIndex < 0) argIndex = 0;

    if(!keyLine)
    {
        DEBUG_PRINTF("NULL argument.");
        ERR_RECORDER(NULL);
    }else
    {
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

void threadDialing::initDialingState(dialingResult_t &info)
{
    bzero(&info, sizeof(dialingResult_t));
}

int threadDialing::parseATcmdACKbyLineOrSpecialCmd(dialingResult_t& info, char* buf, int len, parseEnum e)
{
    int ret = 0;
    char* linep = NULL;
    char* delim = NULL;

    if(!buf || (len <= 0))
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
                emit signalDisplay(STAGE_AT, QString(info.atAck));
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
        case PARSEACK_ICCID:
        {
            linep = getKeyLineFromBuf(buf, (char*)"^ICCID:");
            if(linep)
            {
                strncpy(info.iccidAck, linep, sizeof(info.iccidAck));
                emit signalDisplay(STAGE_ICCID, QString(info.iccidAck));
            }else
            {
                bzero(info.iccidAck, sizeof(info.iccidAck));
                ret = -ENODATA;
            }
            break;
        }
        case PARSEACK_CPIN:
        {
            linep = getKeyLineFromBuf(buf, (char*)"READY");
            if(linep)
            {
                strncpy(info.cpinAck, linep, sizeof(info.cpinAck));
                emit signalDisplay(STAGE_CPIN, QString(info.cpinAck));
            }else
            {
                bzero(info.cpinAck, sizeof(info.cpinAck));
                ret = -ENODATA;
            }
            break;
        }
#if 0
        /*ChinaT: +REG:0, 2   but is works well. So drop it*/
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
#endif
        case PARSEACK_SYSINFOEX:
        {
            /*
             * <srv_status>：表示系统服务状态。
                        0  无服务
                        1  服务受限
                        2  服务有效
                        3  区域服务受限
                        4  省电或休眠状态
             * <srv_domain>：表示系统服务域。
                    0  无服务
                    1  仅 CS 服务
                    2  仅 PS 服务
                    3  PS+CS 服务
                    4  CS、PS 均未注册，并处于搜索状态
                    255  CDMA（暂不支持）
            */
            linep = getKeyLineFromBuf(buf, (char*)"^SYSINFOEX:");
            if(linep)
            {
                strncpy(info.sysinfoexAck, linep, sizeof(info.sysinfoexAck));
                emit signalDisplay(STAGE_SYSINFOEX, QString(info.sysinfoexAck));
#if 0
                //check the service status
                delim = cutAskFromKeyLine(linep, 0);
                switch(atoi(delim))
                {
                case 0:
                case 1:
                case 3:
                case 4:
                {
                    ret = -ENOPROTOOPT;
                    ERR_RECORDER(NULL);
                    DEBUG_PRINTF("Error: SIM no service");
                    break;
                }
                case 2:
                {
                    ret = 0;
                    DEBUG_PRINTF("Success: SIM in avaliable service status.");
                    break;
                }
                default:
                {
                    ret = -ENODATA;
                    ERR_RECORDER(NULL);
                    DEBUG_PRINTF("Error: SIM unknown service status.");
                    break;
                }
                }
#endif
                //check the service domain
                delim = cutAskFromKeyLine(linep, 1);
                switch(atoi(delim))
                {
                case 0:
                {
                    ret = -ENOPROTOOPT;
                    ERR_RECORDER(NULL);
                    DEBUG_PRINTF("Error: SIM no service");
                    break;
                }
                default:
                {
                    ret = 0;
                    DEBUG_PRINTF("Success: SIM in avaliable service domain.");
                    break;
                }
                }
            }else
            {
                bzero(info.sysinfoexAck, sizeof(info.sysinfoexAck));
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
                emit signalDisplay(STAGE_COPS, QString(info.copsAck));

                if(strstr(linep, "CMCC") || strstr(linep, "CHINA MOBILE"))
                {//CMNET
                    info.privateCh = PARSEACK_COPS_CH_M;
                    //emit signalDisplay(STAGE_OPERATOR, QString("CMNET"));
                }else if(strstr(linep, "CHN-CT"))
                {//CTNET
                    info.privateCh = PARSEACK_COPS_CH_T;
                    //emit signalDisplay(STAGE_OPERATOR, QString("CTNET"));
                }else if(strstr(linep, "CHN-UNICOM"))
                {//3GNET
                    info.privateCh = PARSEACK_COPS_CH_U;
                    //emit signalDisplay(STAGE_OPERATOR, QString("3GNET"));
                }else
                {
                    info.privateCh = 0;
                    ret = -ENODATA;
                    ERR_RECORDER(NULL);
                    DEBUG_PRINTF("");
                }
            }else
            {
                bzero(info.copsAck, sizeof(info.copsAck));
                ret = -ENODATA;
                ERR_RECORDER(NULL);
                DEBUG_PRINTF("no +COPS:");
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
                    info.privateCh = 1;
                }else
                {//channel 0
                    info.privateCh = 0;
                }
            }else
            {
                bzero(info.switchAck, sizeof(info.switchAck));
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
#if 0
        case PARSEACK_COPS_CH_M:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+COPS:");
            if(linep)
            {
                strncpy(info.copsAck, linep, sizeof(info.copsAck));
                if((!strstr(linep, "CMCC")) && (!strstr(linep, "CHINA MOBILE")))
                {
                    ret = -ENODATA;
                    ERR_RECORDER(NULL);
                }else
                {
                    emit signalDisplayDialingStage(STAGE_OPERATOR, QString(info.copsAck));
                }
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
                {
                    ret = -ENODATA;
                    ERR_RECORDER(NULL);
                }else
                {
                    emit signalDisplayDialingStage(STAGE_OPERATOR, QString(info.copsAck));
                }
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
                {
                    ret = -ENODATA;
                }else
                {
                    emit signalDisplayDialingStage(STAGE_OPERATOR, QString(info.copsAck));
                }
            }else
            {
                bzero(info.copsAck, sizeof(info.copsAck));
                ret = -ENODATA;
                ERR_RECORDER(NULL);
            }
            break;
        }
#endif
        case PARSEACK_CSQ:
        {
            linep = getKeyLineFromBuf(buf, (char*)"+CSQ:");
            if(linep)
            {
                strncpy(info.csqAck, linep, sizeof(info.csqAck));
                //delim = cutAskFromKeyLine(linep, 0);
                //if(atoi(delim) > SIM_CSQ_SIGNAL_MIN)
                emit signalDisplay(STAGE_NDISSTATQRY, QString(info.csqAck));
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
                emit signalDisplay(STAGE_TEMP, QString(info.tempAck));
                //delim = cutAskFromKeyLine(linep, 5);
                //if(atoi(delim) < SIM_TEMP_VALUE_MAX);
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
                    emit signalDisplay(STAGE_NDISDUP, QString(info.qryAck));
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
            emit signalDisplay(STAGE_NET, QString(info.ipinfo));
            break;
        }
        case SPECIAL_PARSE_PING_RESULT:
        {
            if(1 == len)
            {
                strcpy(info.pingAck, "OK");
                info.isDialedOk = 1;
            }else
            {
                ret = -ENODATA;
                info.isDialedOk = 0;
                strcpy(info.pingAck, "Not good.");
            }
            emit signalDisplay(STAGE_PING_RESULT, QString(info.pingAck));
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

int threadDialing::tryAccessDeviceNode(int* fdp, char* nodePath, int nodeLen)
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
int threadDialing::sendCMDandCheckRecvMsg(int fd, char* cmd, parseEnum key, int retryCnt, int RDndelay)
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
        parseATcmdACKbyLineOrSpecialCmd(dialingResult, (char*)"!ret", 1, SPECIAL_PARSE_PING_RESULT);
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

void threadDialing::showDialingResult(dialingResult_t &info)
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
    printf("SYSINFOEX:");
    showBuf(info.sysinfoexAck, sizeof(info.sysinfoexAck));
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


int threadDialing::slotAlwaysRecvMsgForDebug()
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


int threadDialing::slotStartDialing(char resetFlag)
{
    char probeCntFlag = 0, currentChannel = 0;
    int ret = 0;
    QString ipString;
    QString notes;


    //stop to monitor the normal net
    if(monitorTimer.isActive()) monitorTimer.stop();

    mutexDial.lock();
    if(isDialing != 0)
    {
        ret = -EAGAIN;
        mutexDial.unlock();
        DEBUG_PRINTF("someont isDialing... return!");
        return ret;
    }else
    {
        isDialing = 1;
        mutexDial.unlock();
    }


    DEBUG_PRINTF();
looper_check_stage_devicenode:
    notes = "Dialing start: ";
    //notes += QDateTime::currentDateTime().toString("yyyy/MM/dd-HH:mm:ss");
    notes += QDateTime::currentDateTime().toString("MM/dd[HH:mm:ss]");
    emit signalDisplay(STAGE_DISPLAY_NOTES, notes);

    //init env
    emit signalDisplay(STAGE_DISPLAY_INIT, NULL);
    initDialingState(dialingResult);

    //0. get nodePath and access permission
    ret = tryAccessDeviceNode(&fd, nodePath, sizeof(nodePath));
    if(ret)
    {
        ret = -ENODEV;
        emit signalDisplay(STAGE_NODE, QString("NOdeviceNode"));
        printf("Error: Can't access MT device node.");
    }else
    {
        emit signalDisplay(STAGE_NODE, QString(nodePath));
        DEBUG_PRINTF("Success: access MT device node.");


        //check for the internet access before dialing
        ret = checkInternetAccess(1);
        if(!ret)
        {
            DEBUG_PRINTF("Net access is ok formerly.");
            emit signalDisplay(STAGE_DISPLAY_NOTES, QString("Net access is ok formerly."));
            goto looper_dialing_exit;
        }

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
///looper_check_stage_switchslot:
            if(4 > probeCntFlag)
            {
                if(!(probeCntFlag++/2))
                {
                    DEBUG_PRINTF("try use \"AT^SIMSWITCH\" to enable SIM card...");

                    ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH?", PARSEACK_SWITCH_CHANNEL, 2, 2);
                    currentChannel = dialingResult.privateCh;
                    DEBUG_PRINTF("dialingResult.privateCh:%d.", dialingResult.privateCh);
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
                        DEBUG_PRINTF("Don't know current SIMSWITCH CHANNEL. Change to channel 0.");
                        sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=0", PARSEACK_SWITCH_CHANNEL, 2, 2);
                        sleep(4);
                        break;
                    }
                    }
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
//looper_check_stage_simslot:
            ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CPIN?", PARSEACK_CPIN, 2, 2);
            if(!ret)
            {
                DEBUG_PRINTF("Success: found a SIM card");
            }else
            {
                ret = -EAGAIN;
                DEBUG_PRINTF("Warning: \"AT+CPIN?\" failed!");
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

        //ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CREG?", PARSEACK_REG, 1, 2);
        ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^SYSINFOEX", PARSEACK_SYSINFOEX, 2, 1);
        if(ret)
        {
            DEBUG_PRINTF("Error: the SIM card has no data service.");
            ret = -ENODATA;
        }else
        {
            DEBUG_PRINTF("Success: the SIM card has data service.");

            ret = 0;
            //4 get network operator
//looper_check_stage_dialingoperator:
            ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", PARSEACK_COPS, 2, 2);
            DEBUG_PRINTF("dialingResult.privateCh:%d.", dialingResult.privateCh);
            switch(dialingResult.privateCh)
            {
            case PARSEACK_COPS_CH_M:
            {
                if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"CMNET\"", PARSEACK_OK, 2, 2))
                {
                    DEBUG_PRINTF("CMCC dialing end.");
                }else
                {
                    DEBUG_PRINTF("CMCC dialing failed. Or has been dialed success before.");
                }
                break;
            }
            case PARSEACK_COPS_CH_T:
            {
                //sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=0,1,\"card\", \"card\"", NOTPARSEACK, 2, 2);
                //sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=0,1", NOTPARSEACK, 2, 2);
                if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"CTNET\"", PARSEACK_OK, 2, 2))
                {
                    DEBUG_PRINTF("CH-T dialing end.");
                }else
                {
                    DEBUG_PRINTF("CH-T dialing failed. Or has been dialed success before.");
                }
                break;
            }
            case PARSEACK_COPS_CH_U:
            {
                if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"3GNET\"", PARSEACK_OK, 2, 2))
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


    DEBUG_PRINTF();
    //7.1 check for the internet access
//looper_check_stage_dialingresult:
    if(0 == ret)
    {
        ret = checkInternetAccess();
    }

looper_dialing_exit:
    if(fd > 0)
    {
        //get some other info: signalquality & temperature
        sendCMDandCheckRecvMsg(fd, (char*)"AT^ICCID?", PARSEACK_ICCID, 2, 1);
        sendCMDandCheckRecvMsg(fd, (char*)"AT+CSQ", PARSEACK_CSQ, 2, 1);
        sendCMDandCheckRecvMsg(fd, (char*)"AT^CHIPTEMP?", PARSEACK_TEMP, 2, 1);
    }

    showDialingResult(dialingResult);

    //emit signalDialingEnd(ipString);
    //7.0 parse if has a ip
    if(1)
    {
        getNativeNetworkInfo(QString((char*)LTE_MODULE_NETNODENAME), ipString);
        parseATcmdACKbyLineOrSpecialCmd(dialingResult, ipString.toLocal8Bit().data(), ipString.length(), SPECIAL_PARSE_IP_INFO);
    }
    if(ipString.isEmpty())
    {
        notes = "end failed: ";
    }else
    {
        notes = "end success: ";
    }
    //notes += QDateTime::currentDateTime().toString("yyyy/MM/dd-HH:mm:ss");
    notes += QDateTime::currentDateTime().toString("MM/dd[HH:mm:ss]");
    emit signalDisplay(STAGE_DISPLAY_NOTES, notes);

    mutexDial.lock();
    isDialing = 0;
    mutexDial.unlock();

    //start normal net monitor
    monitorTimer.start(MONITOR_TIMER_CHECK_INTERVAL);

    return ret;
}

int threadDialing::slotMonitorTimerHandler()
{
    QString ipString;
    int ret = 0;
    static int prescaler = 0;
    DEBUG_PRINTF();

    //dynamic check
    if(mutexDial.tryLock())
    {
        if(!isDialing)
        {
            dialingResult.timerCnt++;
            emit signalDisplay(STAGE_DISPLAY_NSEC, QString::number(dialingResult.timerCnt));


            if(prescaler > 10000) prescaler = 0;
            if(prescaler++ / 10)
                {

                //if has a ip
                if(1)
                {
                    getNativeNetworkInfo(QString((char*)LTE_MODULE_NETNODENAME), ipString);
                    parseATcmdACKbyLineOrSpecialCmd(dialingResult, ipString.toLocal8Bit().data(), ipString.length(), SPECIAL_PARSE_IP_INFO);
                }
                //check for the internet access
                checkInternetAccess(1);

                if(fd > 0)
                {
                    //COPS
                    sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", PARSEACK_COPS, 2, 1);
                    //ICCID
                    sendCMDandCheckRecvMsg(fd, (char*)"AT^ICCID?", PARSEACK_ICCID, 2, 1);

                    //CSQ

                    sendCMDandCheckRecvMsg(fd, (char*)"AT+CSQ", PARSEACK_CSQ, 2, 1);

                    //temperature
                    ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^CHIPTEMP?", PARSEACK_TEMP, 2, 1);

                    //SIM slot
                    ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CPIN?", PARSEACK_CPIN, 2, 2);

                    //net access
                    ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISSTATQRY?", PARSEACK_NDISSTATQRY, 2, 2);
                }
            }
            DEBUG_PRINTF("timerCnt: %ld", dialingResult.timerCnt);
        }else
        {
            DEBUG_PRINTF("isDialing... return");
        }

        mutexDial.unlock();

        DEBUG_PRINTF("timerRet:%d", ret);
        if(ret || (fd<0))
        {
            monitorTimer.stop();
            this->slotStartDialing(0);
        }


    }else
    {
        DEBUG_PRINTF("Warning: timer try lock failed at this time.");
    }

    return ret;
}

int threadDialing::slotRunDialing(char beginStage)
{
    int ret = 0, switchCnt = 0, resetCnt = 0;
    QString notes;

    //0.0 stop to monitor the normal net
    if(monitorTimer.isActive()) monitorTimer.stop();

    //just one slot be triggered at one time
    if(mutexDial.tryLock())
    {
        if(beginStage > STAGE_NODE) beginStage = STAGE_NODE;

        do{
            /*begin dialing*/
            switch(beginStage)
            {
            case STAGE_RESET:
            {
                if(1 > resetCnt++)
                {
                    ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^RESET", PARSEACK_RESET, 2, 2);
                    DEBUG_PRINTF("wait reset for more than 10s...");
                    sleep(10);
                    //call back or kill itself and restart by sysinit:respawn service
                    DEBUG_PRINTF("reset done. Start check MT again.");
                }else
                {
                    beginStage = STAGEEND_FAILED;
                    continue;
                }
            }
            case STAGE_REFRESH_BASE_INFO:
            case STAGE_DEFAULT:
            {
                //check for the internet access before dialing
                ret = checkInternetAccess(1);
                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    if(!ret)
                    {
                        DEBUG_PRINTF("Net access is ok formerly.");
                        emit signalDisplay(STAGE_DISPLAY_NOTES, QString("Net access is ok formerly."));
                        beginStage = STAGEEND_SUCCESS;
                        continue;
                    }
                }
            }
            case STAGE_INITENV:
            {
                //init env
                emit signalDisplay(STAGE_DISPLAY_INIT, QString());
                initDialingState(dialingResult);

                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    notes = "Dialing start: ";
                    notes += QDateTime::currentDateTime().toString("MM/dd[HH:mm:ss]");
                    emit signalDisplay(STAGE_DISPLAY_NOTES, notes);
                }
            }
            case STAGE_NODE:
            {
                //0. get nodePath and access permission
                ret = tryAccessDeviceNode(&fd, nodePath, sizeof(nodePath));

                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    if(ret)
                    {
                        emit signalDisplay(STAGE_NODE, QString("NOdeviceNode"));
                        beginStage = STAGEEND_FAILED;
                        continue;
                    }else
                    {
                        emit signalDisplay(STAGE_NODE, QString(nodePath));
                    }
                }else
                {
                    emit signalDisplay(STAGE_NODE, QString(nodePath));
                }
            }
            case STAGE_AT:
            {
                ret = sendCMDandCheckRecvMsg(fd, (char*)"AT", PARSEACK_AT, 2, 2);

                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    if(ret)
                    {
                        beginStage = STAGEEND_FAILED;
                        continue;
                    }else
                    {
                        beginStage = STAGE_CPIN;
                        continue;
                    }
                }
            }
            case STAGE_SIMSWITCH:
            {
                if(3 > switchCnt++)
                {
                    ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH?", PARSEACK_SWITCH_CHANNEL, 2, 2);

                    if(STAGE_REFRESH_BASE_INFO != beginStage)
                    {
                        DEBUG_PRINTF("try use \"AT^SIMSWITCH\" to enable SIM card...");
                        DEBUG_PRINTF("dialingResult.privateCh:%d.", dialingResult.privateCh);
                        switch(dialingResult.privateCh)
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
                            DEBUG_PRINTF("Don't know current SIMSWITCH CHANNEL. Change to channel 0.");
                            sendCMDandCheckRecvMsg(fd, (char*)"AT^SIMSWITCH=0", PARSEACK_SWITCH_CHANNEL, 2, 2);
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
                    beginStage = STAGE_RESET;
                    continue;
                }
            }
            case STAGE_CPIN:
            {
                ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+CPIN?", PARSEACK_CPIN, 2, 2);
                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    if(ret)
                    {
                        beginStage = STAGE_SIMSWITCH;
                        continue;
                    }
                }
            }
            case STAGE_SYSINFOEX:
            {
                ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^SYSINFOEX", PARSEACK_SYSINFOEX, 2, 1);

                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    if(ret)
                    {
                        beginStage = STAGEEND_FAILED;
                        continue;
                    }
                }
            }
            case STAGE_COPS:
            {
                ret = sendCMDandCheckRecvMsg(fd, (char*)"AT+COPS?", PARSEACK_COPS, 2, 2);

                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    DEBUG_PRINTF("dialingResult.privateCh:%d.", dialingResult.privateCh);
                    switch(dialingResult.privateCh)
                    {
                    case PARSEACK_COPS_CH_M:
                    {
                        if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"CMNET\"", PARSEACK_OK, 2, 2))
                        {
                            DEBUG_PRINTF("CMCC dialing end.");
                        }else
                        {
                            DEBUG_PRINTF("CMCC dialing failed. Or has been dialed success before.");
                        }
                        break;
                    }
                    case PARSEACK_COPS_CH_T:
                    {
                        //sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=0,1,\"card\", \"card\"", NOTPARSEACK, 2, 2);
                        //sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=0,1", NOTPARSEACK, 2, 2);
                        if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"CTNET\"", PARSEACK_OK, 2, 2))
                        {
                            DEBUG_PRINTF("CH-T dialing end.");
                        }else
                        {
                            DEBUG_PRINTF("CH-T dialing failed. Or has been dialed success before.");
                        }
                        break;
                    }
                    case PARSEACK_COPS_CH_U:
                    {
                        if(!sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISDUP=1,1,\"3GNET\"", PARSEACK_OK, 2, 2))
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
                ret = sendCMDandCheckRecvMsg(fd, (char*)"AT^NDISSTATQRY?", PARSEACK_NDISSTATQRY, 2, 2);

                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    if(ret)
                    {
                        beginStage = STAGEEND_FAILED;
                        continue;
                    }
                }
            }
            case STAGE_PING_RESULT:
            {

                if(STAGE_REFRESH_BASE_INFO != beginStage)
                {
                    ret = checkInternetAccess(0);
                }
            }
            case STAGEEND_SUCCESS:
            {
                break;
            }
            case STAGEEND_FAILED:
            default:
            {
                break;
            }
            }
        }while(0);


        exitSerialPortFromTtyLte(&fd);
        /*end dialing*/
        mutexDial.unlock();
    }else
    {
        DEBUG_PRINTF("Warning: break! It isRunning formerly...");
    }

    //0.1
    if(!monitorTimer.isActive()) monitorTimer.start(MONITOR_TIMER_CHECK_INTERVAL);

    return ret;
}

void threadDialing::run()
{
    QObject::connect(&monitorTimer, &QTimer::timeout, this, &threadDialing::slotMonitorTimerHandler, Qt::QueuedConnection);

    this->slotStartDialing(0);

    exec();
}

