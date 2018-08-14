#include "threadltenetmonitor.h"

threadLTENetMonitor::threadLTENetMonitor()
{
    moveToThread(this);
}


int threadLTENetMonitor::slotCheckInternetAccess(char priority)
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
    if(priority)
    {
        strcpy(cmdLine, "ping -c 1 -s 1 -W 1 -w 1 -I ");
    }else
    {
        strcpy(cmdLine, "ping -c 1 -s 1 -W 7 -w 10 -I ");
    }
    strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
    strncat(cmdLine, " ", 1);
    strncat(cmdLine, INTERNET_ACCESS_POINT, strlen(INTERNET_ACCESS_POINT));

    ret = system(cmdLine);
    if(ret)
        ERR_RECORDER("ping failed.");

    DEBUG_PRINTF("ping ret: %d", ret);

    return ret;
}

int threadLTENetMonitor::slotNetStateMonitor(void)
{
    volatile static char failedCnt = 0, firstFlag = 0;
    DEBUG_PRINTF();
    int ret = 0;

    if(!firstFlag)
    {
        firstFlag = 1;
        ret = slotCheckInternetAccess(1);
        if(ret)
        {
            failedCnt++;
            DEBUG_PRINTF("###########LTE net's access failed at first time.");
            //writeLogLTE(LTE_DISCONNECTED);
            emit signalStartDialing(0);
        }else
        {
            writeLogLTE(LTE_CONNECTED);
        }
    }
    else
    {
        ret = slotCheckInternetAccess(0);
        if(ret)
        {
            failedCnt++;
            DEBUG_PRINTF("###########LTE net's access failed.");
            writeLogLTE(LTE_DISCONNECTED);
            if(failedCnt > NET_ACCESS_FAILEDCNT_MAX)
            {
                DEBUG_PRINTF("Warning: Net access failed so many times that have to restart dialing!");
                failedCnt = 0;
                emit signalStartDialing(0);
            }
        }else
        {
            writeLogLTE(LTE_CONNECTED);
            DEBUG_PRINTF("###########LTE net's access looks good.");
        }
    }



    return ret;
}

void threadLTENetMonitor::slotDialingEnd(QString str)
{
    DEBUG_PRINTF("ipStr: %s.", str.toLocal8Bit().data());
    emit signalResumeTimerAgain(TIMEINTERVAL_LTE_NET_CHECK);
}

void threadLTENetMonitor::run()
{
    QTimer checkTimer;


    //0. create log file
    this->createLogFile(NET_ACCESS_LOG_DIR);

    QObject::connect(&checkTimer, &QTimer::timeout, this, &threadLTENetMonitor::slotNetStateMonitor);

    /*connect below to prevent to access dialing process thread multiple times at the same time*/
    QObject::connect(this, &threadLTENetMonitor::signalStartDialing, &checkTimer, &QTimer::stop);
    QObject::connect(this, SIGNAL(signalResumeTimerAgain(int)), &checkTimer, SLOT(start(int)));

    //1. imediately executed the monitor
    this->slotNetStateMonitor();
    //2. then monitor is executable by timer
    checkTimer.start(TIMEINTERVAL_LTE_NET_CHECK);

    exec();
}

int threadLTENetMonitor::createLogFile(QString dirFullPath)
{
    int ret = 0;

    if(dirFullPath.isEmpty())
    {
        ret = -EINVAL;
        ERR_RECORDER(NULL);
        DEBUG_PRINTF();
    }else
    {
        //clean log dir
        QString cmd("rm -rf ");
        cmd += dirFullPath;
        system(cmd.toLocal8Bit().data());
        //create log dir
        cmd = QString("mkdir -p ");
        cmd += dirFullPath;
        system(cmd.toLocal8Bit().data());
        //create log file
        dirFullPath += "/";
        dirFullPath += NET_ACCESS_LOG_FILENAME;
        cmd = QString("touch ");
        cmd += dirFullPath;
        ret = system(cmd.toLocal8Bit().data());
#if 0
        //write item info
        cmd = QString("echo 'connect\t\t\tdisconnect\t\t\tconnectMax\n' > ");
        cmd += dirFullPath;
        system(cmd.toLocal8Bit().data());
#endif
    }

    return ret;
}

int threadLTENetMonitor::writeLogLTE(connectTimeStatus c)
{
    int ret = 0;
    QString cmd;
    static enum connectTimeStatus lastTimeStatus;

    switch(c)
    {
    case LTE_CONNECTED:
    {
        if(LTE_CONNECTED != lastTimeStatus)
        {
            //-e 表示开启转义, "\c"表示不换行
            //cmd = QString("echo -e ");
            cmd = QString("echo ");
            cmd += QDateTime::currentDateTime().toString("yyyyMMdd-hh:mm:ss");
            //cmd += QString("(connected) \"\\c\" >> ");
            cmd += QString(" connected >> ");
            cmd += QString(NET_ACCESS_LOG_DIR);
            cmd += QString("/");
            cmd += QString(NET_ACCESS_LOG_FILENAME);
            system(cmd.toLocal8Bit().data());
            DEBUG_PRINTF("---cmd: %s", cmd.toLocal8Bit().data());

            cmd = "net well: ";
            cmd += QDateTime::currentDateTime().toString("MM/dd[HH:mm:ss]");
            emit signalDisplay(STAGE_DISPLAY_NOTES, cmd);
        }else
        {
            DEBUG_PRINTF("Already has been written LTE_CONNECTED.");
        }
        break;
    }
    case LTE_DISCONNECTED:
    {
        if(LTE_DISCONNECTED != lastTimeStatus)
        {
            cmd = QString("echo ");
            cmd += QDateTime::currentDateTime().toString("yyyyMMdd-hh:mm:ss");
            cmd += QString(" disconnected >> ");
            cmd += QString(NET_ACCESS_LOG_DIR);
            cmd += QString("/");
            cmd += QString(NET_ACCESS_LOG_FILENAME);
            system(cmd.toLocal8Bit().data());
            DEBUG_PRINTF("%s", cmd.toLocal8Bit().data());

            cmd = "net bad: ";
            cmd += QDateTime::currentDateTime().toString("MM/dd[HH:mm:ss]");
            emit signalDisplay(STAGE_DISPLAY_NOTES, cmd);
        }else
        {
            DEBUG_PRINTF("Already has been written LTE_DISCONNECTED.");
            break;
        }
    }
    case LTE_CONNECTED_UPTIME:
    {
        break;
    }
    default:
    {
        break;
    }
    }

    lastTimeStatus = c;

    return ret;
}
