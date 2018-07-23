#include "huawei4gmodule.h"

HUAWEI4GModule::HUAWEI4GModule()
{
    fd = -1;
    bzero(msg, BOXV3_MSG_LENGTH_4G);
    bzero(networkServerVPN, VPNLENGTH);
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
    struct termios options;
    /*获取终端参数，成功返回零；失败返回非零，发生失败接口将设置errno错误标识*/
    if(0 != tcgetattr(fd, &options))
    {
        ret = -EACCES;
        perror("tcgetattr failed.");
    }else
    {
        bzero(&options,sizeof(options));

        options.c_cflag|=(CLOCAL|CREAD);
        options.c_cflag &= ~CSIZE;

        switch(databits)
        {
        case 7:
          options.c_cflag |= CS7;
          break;
        case 8:
          options.c_cflag |= CS8;
          break;
        default:
            ret = -EINVAL;
            perror("Unsupported data bit argument.");
            break;
        }

        if(!ret)
        {
            switch(parity)
            {
            case 'n':
            case 'N':
                options.c_cflag &= ~PARENB;   /* Clear parity enable */
                //options.c_iflag &= ~INPCK;     /* Enable parity checking */
                break;
            case 'o':
            case 'O':
                options.c_cflag |= PARENB;             /*开启奇偶校验 */
                options.c_iflag |= (INPCK | ISTRIP);   /*INPCK打开输入奇偶校验；ISTRIP去除字符的第八个比特*/
                options.c_cflag |= PARODD;             /*启用奇校验(默认为偶校验)*/
                //options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
                //options.c_iflag |= INPCK;             /* Disnable parity checking */
                break;
            case 'e':
            case 'E':
                options.c_cflag |= PARENB;             /*开启奇偶校验 */
                options.c_iflag |= ( INPCK | ISTRIP);  /*打开输入奇偶校验并去除字符第八个比特  */
                options.c_cflag &= ~PARODD;            /*启用偶校验；  */
                //options.c_cflag |= PARENB;     /* Enable parity */
                //options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
                //options.c_iflag |= INPCK;       /* Disnable parity checking */
                break;
            case 'S':
            case 's':  /*as no parity*/
                options.c_cflag &= ~PARENB;
                options.c_cflag &= ~CSTOPB;
                break;
            default:
                ret = -EINVAL;
                perror("Unsupported parity bit argument.");
            }
            if(!ret)
            {
                switch(speed)        /*设置波特率 */
                {
                case 2400:
                    cfsetispeed(&options, B2400);    /*设置输入速度*/
                    cfsetospeed(&options, B2400);    /*设置输出速度*/
                    break;
                case 4800:
                    cfsetispeed(&options, B4800);
                    cfsetospeed(&options, B4800);
                    break;
                case 9600:
                    cfsetispeed(&options, B9600);
                    cfsetospeed(&options, B9600);
                    break;
                case 115200:
                    cfsetispeed(&options, B115200);
                    cfsetospeed(&options, B115200);
                    break;
                default:
                    cfsetispeed(&options, B9600);
                    cfsetospeed(&options, B9600);
                    break;
                }
                /* 设置停止位*/
                switch (stopbits)
                {
                case 1:
                    options.c_cflag &= ~CSTOPB;
                    break;
                case 2:
                    options.c_cflag |= CSTOPB;
                    break;
                default:
                    ret = -EINVAL;
                    perror("Unsupported stop bit argument.");
                }
                if(!ret)
                {
                    /* Set input parity option */
                    if (parity != 'n')
                        options.c_iflag |= INPCK;
                    tcflush(fd,TCIFLUSH);
                    options.c_cc[VTIME] = 0; /* 设置超时0 seconds*/
                    options.c_cc[VMIN] = 0; /* Update the options and do it NOW */
                    if(0 != tcsetattr(fd,TCSANOW,&options))
                    {
                        ret = -EPERM;
                        perror("tcsetattr failed.");
                    }else
                    {
                        ;
                        //options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
                        //options.c_lflag  &= ~( ECHO | ECHOE | ISIG);
                        //options.c_oflag  &= ~OPOST;   /*Output*/
                    }
                }
            }
        }
    }
    return ret;
}

int HUAWEI4GModule::communicationCheck(char* cmd, char* checkString)
{
    int ret = 0;
    int cmdLen = 0;

    /*check for arguments*/
    if(!cmd)
    {
        ret = -EINVAL;
        ERR_RECORDER("cmd buf is null.");
    }else
    {
        cmdLen = strlen(cmd);
        /*The shortest cmd is "AT".*/
        if(cmdLen<2 || cmdLen >= BOXV3_MSG_LENGTH_4G-1)
        {
            ret = -EINVAL;
            ERR_RECORDER("cmdLen is invalid.");
        }else
        {
            /*cp cmd to buf, there must be have a sign '\r' in the end of the cmd*/
            bzero(msg, BOXV3_MSG_LENGTH_4G);
            strncpy(msg, cmd, cmdLen);
            strncat(msg, "\r", 1);
            cmdLen++;
            /*write cmd*/
            ret = write(fd, msg, cmdLen);
            if(-1 == ret)
            {
                ret = -EBADR;
                ERR_RECORDER("write cmd failed.");
            }else if(0 == ret)
            {
                ret = -EAGAIN;
                ERR_RECORDER("write none cmd.");
            }else
            {
                bzero(msg, BOXV3_MSG_LENGTH_4G);
                /*wait for 4G module ack for the cmd*/
                sleep(2);
                ret = read(fd, msg, BOXV3_MSG_LENGTH_4G);
                if(ret < 0)
                {
                    ret = -EBADR;
                    ERR_RECORDER("read cmd fadeback failed.");
                }else if(0 == ret)
                {
                    ret = -EAGAIN;
                    ERR_RECORDER("read none fadeback.");
                }else
                {
                    if(NULL != checkString)
                    {
                        if(!strstr(msg, checkString))
                        {
                            ret = -ENODATA;
                            ERR_RECORDER("read no match specific data.");
                        }else
                        {
                            ret = 0;
                        }

                    }else
                    {
                        ret = 0;
                    }
                }
            }
        }
    }

    showBuf(msg, BOXV3_MSG_LENGTH_4G);
    return ret;
}

int HUAWEI4GModule::initSerialPortFor4GModule(int* fd, char* nodePath, int baudRate)
{
    int ret = 0;
    struct termios serialAttr;

    if(!fd || !nodePath)
    {
        ret = -EINVAL;
        ERR_RECORDER("init arguments failed.");
    }else
    {
        *fd = open(nodePath, O_RDWR);
        if(*fd < 0)
        {
            ret = -errno;
            ERR_RECORDER("Unable to open device");
        }else
        {
            ret = setSerialPortNodeProperty(fd, 8, 1, 'N', BOXV3_BAUDRATE_UART)
        }
    }

    /*some sets are wrong*/
    if((0 != ret)&&(*fd > 0))
    {
        ret = -EAGAIN;
        close(*fd);
        *fd = -1;
    }

    return ret;
}




int HUAWEI4GModule::waitWriteableForFd(int fd, struct timeval* tm)
{
    int ret = 0;
    fd_set writefds;

    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    ret = select(fd+1, NULL, &writefds, NULL, tm) < 0;
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
int HUAWEI4GModule::sendCMDofAT(int fd, char *cmdStr, int len)
{
    int ret = 0, i = 0;

    strncat(cmdStr, AT_CMD_SUFFIX, strlen(AT_CMD_SUFFIX));
    len += strlen(AT_CMD_SUFFIX);

    for(i = 0; i < len; i++)
    {
        ret = waitWriteableForFd(fd, NULL);
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

int HUAWEI4GModule::recvMsgFromATModule(int fd)
{
    int ret=0, i=0;
    char ch = 0;
    //a line length
    char buf[1024];
    int bufIndex = 0;

    //read 10 line
    for(i=0; i<=10; i++)
    {
        ret = 0;
        usleep(500);
        fd_set readfds;

        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);

        if(select(fd + 1, &readfds, NULL, NULL, NULL) < 0)
        {
            ret = -errno;
            ERR_RECORDER(strerror(errno));
        }else
        {
            bzero(buf, sizeof(buf));
            bufIndex = 0;
            if(FD_ISSET(fd, &readfds))
            {
                while(1 == read(fd, &ch, 1))
                {
                    sprintf(&buf[bufIndex++], "%c", ch);
                    if (bufIndex >= 1023 )
                    {
                        break;
                    }
                }
            }
        }
        printf("###############read:%d\n", i);
        showBuf(buf, 1024);
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
        ret = initSerialPortFor4GModule(&fd, (char*)BOXV3_NODEPATH_4G, B9600);
        if(!ret)
        {
            ret = sendCMDofAT(fd, cmd, strlen(cmd));
            if(!ret)
            {
                ret = recvMsgFromATModule(fd);
            }
        }
        close(fd);
    }

    return ret;
}

int HUAWEI4GModule::dialingForNetwork(void)
{
    int ret = 0;

    //china mobile
    ret = communicationCheck((char*)"AT^NDISDUP=1,1,\"CMNET\"", (char*)"OK");
    //china unicom
    //china telecom
    if(!ret)
    {
        ret = communicationCheck((char*)"AT^NDISSTATQRY?", (char*)"1,");
        if(!ret)
        {
            printf("Dialing for China Mobile success!.\n");
        }else
        {
            printf("Dialing for China Mobile failed!.\n");
        }
    }
    return ret;
}


int HUAWEI4GModule::checkSIMCard(void)
{
    int ret = 0;
    /*2. if the card is there and works well, if do not works well, we should use "AT^RESET" to reset the card.*/
    /*
     * AT+CIMI
     * 460042983702403
     *
     * OK
     *
     */
    ret = communicationCheck((char*)"AT+CIMI", (char*)"OK");
    if(!ret)
    {
        printf("SIM card works well.\n");
    }else
    {
        printf("SIM card looks bad.\n");

        printf("Try to reset, and check with 'AT+CIMI' again...\n");
        ret = init4GModule(true);
        if(!ret)
        {
            init4GModule(false);
            ret = checkSIMCard();
            if(!ret)
            {
                printf("SIM card works well.\n");
            }else
            {
                printf("SIM card looks bad.\n");
            }
        }else
        {
            printf("Reset the 4G module failed.\n");
        }
    }

    return ret;
}

int HUAWEI4GModule::checkGeneralInfo(void)
{
    int ret = 0;
    /*1. if the 4g usb module is there and works well.*/
    /*
     * ATI
     * Manufacturer: Huawei Technologies Co., Ltd.
     * Model: ME909s-821
     * Revision: 11.617.05.00.00
     * IMEI: 867012032172634
     * +GCAP: +CGSM,+DS,+ES
     *
     * OK
     */
    ret = communicationCheck((char*)"ATI", (char*)"OK");
    if(!ret)
    {
        printf("4G module works well.\n");
    }else
    {
        printf("4G module couldn't found in sys!\n");
    }

    return ret;
}

int HUAWEI4GModule::init4GModule(bool reset)
{
    int ret = 0;

    ret = communicationCheck((char*)"ATE0", (char*)"OK");
    if(!ret)
    {
        printf("Disable 4G module debug echo mode success.\n");
    }else
    {
        printf("Disable 4G module debug echo mode failed.\n");
    }

    if(reset)
    {
        ret = communicationCheck((char*)"AT^RESET", (char*)"OK");
        if(!ret)
        {
            printf("4G module is reseting, please wait...\n");
            sleep(20);
            printf("Reset 4G module success.\n");
        }else
        {
            printf("Reset 4G module failed.\n");
        }
    }

    return ret;
}

int HUAWEI4GModule::tryCommunicateWith4GModule(char* nodePath, int retryCnt)
{
    volatile int ret = 0;
    int i=0;
    bool resetFlag = false;

    if(retryCnt<0) retryCnt=0;

    if(!nodePath)
    {
        ret = -EPERM;
        perror("nodePath couldn't be NULL.");
    }else
    {
        /*
         * O_RDWR 读、写打开;
         * O_NOCTTY 如果是终端设备，则不将此设备分配作为此进程的控制终端;
         * O_NONBLOCK 本次打开操作和后续的I / O操作设置非阻塞方式
         */
        fd = open(nodePath, O_RDWR|O_NOCTTY|O_NONBLOCK);
        if(fd < 0)
        {
            ret = -EINVAL;
            perror(nodePath);
        }else
        {
            if(setSerialPortNodeProperty(fd, 8, 1, 'N', BOXV3_BAUDRATE_UART))
            {
                ret = -EIO;
                fd = -1;
                perror("Set Parity Error\n");
            }else
            {
                resetFlag = false;
                for(i=0; i<=retryCnt; i++)
                {
                    ret = init4GModule(resetFlag);
                    if(!ret)
                        ret = checkGeneralInfo();
                    if(!ret)
                        ret = checkSIMCard();
                    if(!ret)
                        ret = dialingForNetwork();

                    resetFlag = true;
                }
            }
            close(fd);
            fd = -1;
        }
    }

    return ret;
}
