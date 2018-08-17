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
    void signalDisplay(char key, QString notes);
public slots:
    int slotCheckInternetAccess(char priority = 0);
    int slotNetStateMonitor(void);
    void slotDialingEnd(QString str);
protected:
    void run(void);
private:
    int createLogFile(QString dirFullPath);
    int writeLogLTE(checkStageLTE c);
};

#endif // THREADLTENETMONITOR_H
