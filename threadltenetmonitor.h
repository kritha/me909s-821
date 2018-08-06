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
public slots:
    int slotCheckInternetAccess();
    int slotNetStateMonitor();
protected:
    void run(void);
};

#endif // THREADLTENETMONITOR_H
