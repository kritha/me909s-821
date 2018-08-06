#include "threadltenetmonitor.h"

threadLTENetMonitor::threadLTENetMonitor()
{

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

int threadLTENetMonitor::slotNetStateMonitor()
{
    DEBUG_PRINTF();
    int ret = 0;
    ret = slotCheckInternetAccess();
    if(ret)
    {
        DEBUG_PRINTF("###########LTE net's access failedd.");
        emit signalStartDialing(0);
    }else
    {
        DEBUG_PRINTF("###########LTE net's access looks good.");
    }

    return ret;
}

void threadLTENetMonitor::run()
{
    QTimer checkTimer;
    QObject::connect(&checkTimer, SIGNAL(timeout()), this, SLOT(slotNetStateMonitor()), Qt::QueuedConnection);

    checkTimer.start(TIMEINTERVAL_LTE_NET_CHECK);

    exec();
}
