#ifndef THREADLTENETMONITOR_H
#define THREADLTENETMONITOR_H
#include <QThread>
#include <QTimer>

#include "global.h"

class threadLTENetMonitor : public QThread
{
    Q_OBJECT
public:
    threadLTENetMonitor();
signals:
    void signalStartDialing(char reset);
    void signalStopDialing(void);
    void signalResumeTimerAgain(int mSec);
public slots:
    int slotCheckInternetAccess();
    int slotNetStateMonitor();
    void slotDialingEnd(QByteArray array);
protected:
    void run(void);
};

#endif // THREADLTENETMONITOR_H
