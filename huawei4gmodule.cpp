#include "huawei4gmodule.h"

HUAWEI4GModule::HUAWEI4GModule()
{
    fd = -1;
    bzero(msg, BOXV3_MSG_LENGTH_4G);
}

void HUAWEI4GModule::showBuf(char *buf, int len)
{
    for(int i=0; i<len; i++)
    {
        printf("%c", buf[i]);
    }
    puts("");

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

int HUAWEI4GModule::initUartPort(int* fd, char* nodePath)
{
    int ret = 0;
    if(!nodePath || !fd)
    {
        ret = -EPERM;
        perror("UartPath can't be null!\n");
    }else
    {
        *fd = open(nodePath, O_RDWR|O_NOCTTY|O_NONBLOCK);
        if(*fd < 0)
        {
            ret = -EINVAL;
            perror(nodePath);
        }else
        {
            if(setSerialPortNodeProperty(*fd, 8, 1, 'N', BOXV3_BAUDRATE_UART))
            {
                ret = -EIO;
                perror("Set Parity Error\n");
                close(*fd);
                *fd = -1;
            }
        }
    }

    return ret;
}

int HUAWEI4GModule::tryCommunicateWith4GModule(int fd, char* buf, int cmdLen, int len)
{
    int ret = 0;
    fd_set readfds;
    struct timeval timeout;

    if(!buf)
    {
        ret = -EINVAL;
    }else
    {
        if(cmdLen+1 > len)
        {
            ret = -ENOMEM;
            perror("tryCommunicateWith4GModule didn't have enough buf to opt.");
        }else
        {
            strcat(buf, "\r");
            ret = write(fd, buf, cmdLen+1);
            if(-1 == ret)
            {
                ret = -EIO;
            }else
            {
                /*wait for 4G module answer the cmd*/
                FD_ZERO(&readfds);
                FD_SET(fd, &readfds);
                timeout.tv_sec = 7;
                timeout.tv_usec = 0;
                ret = select(fd+1, &readfds, NULL, NULL, &timeout);
                if(ret<0)
                {
                    ret = -EIO;
                    perror("select 4g module failed.");
                }else if(0 == ret)
                {
                    ret = -EAGAIN;
                    perror("select 4g module overtime.");
                }else
                {
                    ret = 0;
                    bzero(buf, len);
                    read(fd, buf, len);
                }
            }
        }
    }

    close(fd);
    return ret;
}

int HUAWEI4GModule::communicationCheck(char* cmd, char* checkString)
{
    int ret = 0;
    int cmdLen = 0;
    /*check for arguments*/
    if(!cmd || !checkString)
    {
        ret = -EINVAL;
    }else
    {
        cmdLen = strlen(cmd);
        if(cmdLen<=2 || cmdLen >= BOXV3_MSG_LENGTH_4G-1)
        {
            ret = -EINVAL;
        }else
        {
            /*cp cmd to buf, there must be have a sign '\r' in the end of the cmd*/
            bzero(msg, BOXV3_MSG_LENGTH_4G);
            strncpy(msg, cmd, cmdLen);
            strncat(msg, "\r", 1);
            /*write cmd*/
            ret = write(fd, msg, cmdLen+1);
            if(-1 == ret)
            {
                ret = -EBADR;
            }else if(0 == ret)
            {
                ret = -EAGAIN;
            }else
            {
                bzero(msg, BOXV3_MSG_LENGTH_4G);
                /*wait for 4G module ack for the cmd*/
                sleep(3);
                ret = read(fd, msg, BOXV3_MSG_LENGTH_4G);
                if(ret < 0)
                {
                    ret = -EBADR;
                }else if(0 == ret)
                {
                    ret = -EAGAIN;
                }else
                {
                    if(!strstr(msg, checkString))
                    {
                        ret = -ENODATA;
                    }else
                    {
                        ret = 0;
                    }
                }
            }
        }
    }
    return ret;
}

int HUAWEI4GModule::initUartAndTryCommunicateWith4GModule(char* nodePath)
{
    int ret = 0;

    if(!nodePath)
    {
        ret = -EPERM;
        perror("nodePath couldn't be NULL.");
    }else
    {
        /*O_RDWR 读、写打开, O_NOCTTY 如果是终端设备，则不将此设备分配作为此进程的控制终端, O_NONBLOCK 本次打开操作和后续的I / O操作设置非阻塞方式*/
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
                bzero(msg, BOXV3_MSG_LENGTH_4G);
                ret = communicationCheck((char*)"ATI", (char*)"OK");
                if(0 == ret)
                {
                    printf("module ok");
                }else
                {
                    printf("module failed.");
                }
            }
            close(fd);
        }
    }

    return ret;
}

int HUAWEI4GModule::parseMsgFrom4GModule(char* data)
{
    int ret = 0;
    if(!data)
    {
        ret = -EINVAL;
    }else
    {
        printf("%s\n", data);
    }

    return ret;
}

int HUAWEI4GModule::tryCommunicateWith4GAndParse(int fd)
{
    int ret = 0;
    char buf[1024] = "ATI";

    ret = tryCommunicateWith4GModule(fd, buf, strlen(buf), sizeof(buf));
    if(ret)
    {
        ret = -EIO;
        perror("tryCommunicateWith4GModule failed.");
    }else
    {
        if(!strstr(buf, "OK"))
        {
            ret = -ENODATA;
            perror("tryCommunicateWith4GModule didn't get avaliable data.");
        }else
        {
            //parseMsgFrom4GModule(buf);
        }

        //printf
        parseMsgFrom4GModule(buf);
    }
    return ret;
}


void HUAWEI4GModule::initCmdStringMap(void)
{

    cmdMapAT.insert(pair<string,string>("i", "ATI"));
    //该指令用来读取或IMSI，查询前可能需要输入PIN码
    cmdMapAT.insert(pair<string,string>("cimi", "AT+CIMI"));
}

int HUAWEI4GModule::getDeviceNode(char* dirName)
{
    char tmpNodePath[32] = {};
    int ret = 0, tmpFd = -1;
    DIR *dir = NULL;
    struct dirent *ptr = NULL;
    char buf[BOXV3_MSG_LENGTH_4G] = {};

    dir = opendir(dirName);
    if(!dir)
    {
        ret = -ENODEV;
        fprintf(stderr, "opendir(%s) failed!", dirName);
        perror("");
    }else
    {
        while(NULL != (ptr = readdir(dir)))
        {
            if(strstr(ptr->d_name, BOXV3_NODENAME_PREFIX_4G))
            {
              bzero(tmpNodePath, sizeof(tmpNodePath));
              strcpy(tmpNodePath, BOXV3_UARTNODE_DIR);
              strcat(tmpNodePath, "/");
              strcat(tmpNodePath, ptr->d_name);
              printf("tmpNodePath:%s\n", tmpNodePath);
#if 0
              if(!initUartPort(&tmpFd, tmpNodePath))
              {
                if(tryCommunicateWith4GAndParse(tmpFd))
                {
                    ret = -EIO;
                    perror("tryCommunicateWith4GAndParse failed.");
                }
              }
#else
              ret = initUartAndTryCommunicateWith4GModule(&tmpFd, tmpNodePath, buf, sizeof(buf));
#endif
            }
        }
        closedir(dir);
    }
    return ret;
}

int HUAWEI4GModule::startCheck(char *nodePath)
{
    int ret = 0, fd = -1;
    char buf[BOXV3_MSG_LENGTH_4G] = {};

    ret = initUartAndTryCommunicateWith4GModule(&fd, nodePath, buf, sizeof(buf));

    return ret;
}
