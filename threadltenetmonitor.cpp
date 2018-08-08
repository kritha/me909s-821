#include "threadltenetmonitor.h"

threadLTENetMonitor::threadLTENetMonitor()
{
    moveToThread(this);
}


int threadLTENetMonitor::slotCheckInternetAccess(void)
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
    char cmdLine[64] = "ping -c 1 -s 1 -W 7 -w 10 -I ";
    strncat(cmdLine, LTE_MODULE_NETNODENAME, strlen(LTE_MODULE_NETNODENAME));
    strncat(cmdLine, " ", 1);
    strncat(cmdLine, INTERNET_ACCESS_POINT, strlen(INTERNET_ACCESS_POINT));

    ret = system(cmdLine);
    if(ret)
        ERR_RECORDER("ping failed.");

    DEBUG_PRINTF("ping ret: %d", ret);

    return ret;
}

int threadLTENetMonitor::slotNetStateMonitor()
{
    volatile static char failedCnt = 0;
    DEBUG_PRINTF();
    int ret = 0;
    ret = slotCheckInternetAccess();
    if(ret)
    {
        failedCnt++;
        DEBUG_PRINTF("###########LTE net's access failedd.");
        if(failedCnt > NET_ACCESS_FAILEDCNT_MAX)
        {
            DEBUG_PRINTF("Warning: Net access failed so many times that have to restart dialing!");
            failedCnt = 0;
            emit signalStartDialing(0);
        }
    }else
    {
        DEBUG_PRINTF("###########LTE net's access looks good.");
    }

    return ret;
}

void threadLTENetMonitor::slotDialingEnd(QByteArray array)
{
    DEBUG_PRINTF("%s.", array.data());
    emit signalResumeTimerAgain(TIMEINTERVAL_LTE_NET_CHECK);
}

void threadLTENetMonitor::run()
{
    QTimer checkTimer;
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
