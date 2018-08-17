#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include "threaddialing.h"
#include "threadltenetmonitor.h"
#include "global.h"
#include "mainwindow.h"

static QString cmdLineParser(QCoreApplication& a)
{
    QString value;
    QCoreApplication::setApplicationVersion(BOXV3CHECKAPP_VERSION);
    QCommandLineOption optArgs("f");
    optArgs.setDescription("force option(like RD_ONLY for read devicenode only.)");
    optArgs.setValueName("ForceOptions");

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(optArgs);
    parser.process(a);

    value = parser.value(optArgs);

    return value.contains("RD_ONLY") ? value:QString();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    threadDialing dialing;
    //threadLTENetMonitor monitor;

    //QObject::connect(&monitor, &threadLTENetMonitor::signalStartDialing, &dialing, &threadDialing::slotStartDialing, Qt::QueuedConnection);
    /*this connect for prevent multiple access dialing process at the same time*/
    //QObject::connect(&dialing, &threadDialing::signalDialingEnd, &monitor, &threadLTENetMonitor::slotDialingEnd, Qt::QueuedConnection);

    //display
    QObject::connect(&dialing, &threadDialing::signalDisplay, &w, &MainWindow::slotDisplay, Qt::QueuedConnection);
    //QObject::connect(&monitor, &threadLTENetMonitor::signalDisplay, &w, &MainWindow::slotDisplay, Qt::QueuedConnection);


    int ret = 0;
    int fd = -1;
    char nodePath[128] = {};

    cmdLineParser(a);

    if(argc < 2)
    {
        dialing.start();
        //monitor.start();
        w.showFullScreen();
    }else
    {
        ret = dialing.initUartAndTryCommunicateWith4GModule_ForTest(&fd, nodePath, (char*)BOXV3_NODEPATH_LTE, argv[1]);
        if(ret)
        {
            dialing.showErrInfo(errInfo);
        }
        return 0;
    }

    return a.exec();
}
