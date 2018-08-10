#ifndef THREADLTENETMONITOR_H
#define THREADLTENETMONITOR_H
#include <QThread>
#include <QTimer>
#include <QDateTime>

#include <QDir>
#include <QFile>

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
    int slotCheckInternetAccess(char priority = 0);
    int slotNetStateMonitor(void);
    void slotDialingEnd(QByteArray array);
protected:
    void run(void);
private:
    int createLogFile(QString dirFullPath);
    int writeLogLTE(connectTimeStatus c);
};

#endif // THREADLTENETMONITOR_H
