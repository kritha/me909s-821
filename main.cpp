#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include "threaddialing.h"
#include "global.h"
#include "mainwindow.h"

static QString cmdLineParser(QCoreApplication& a)
{
#define USE_OPTARGS
    QString value;
    QCoreApplication::setApplicationVersion(BOXV3CHECKAPP_VERSION);
#ifdef USE_OPTARGS
    QCommandLineOption optArgs("t");
    optArgs.setDescription("try count if dialed failed, default is infinite.");
    optArgs.setValueName("tryCount");
    QCommandLineOption supportArgs("AT");
    supportArgs.setDescription("This soft ware support exec with AT<cmd>(like: .\"/a.out ATI\").");
    supportArgs.setValueName("cmd");
#endif
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addVersionOption();
    parser.addHelpOption();
#ifdef USE_OPTARGS
    parser.addOption(optArgs);
    parser.addOption(supportArgs);
#endif
    parser.process(a);

#ifdef USE_OPTARGS
    value = parser.value(optArgs);
    if(!value.isEmpty() || !value.isNull())
    {
        tryCount = value.toInt();
    }else
    {
        tryCount = TRY_COUNT_INFINITE_SIGN;
    }
    DEBUG_PRINTF("tryCount:%d.", tryCount);
#endif

    return value;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    int fd = -1;
    char nodePath[128] = {};

    QApplication a(argc, argv);

    MainWindow w;
    threadDialing dialing;

    //display
    QObject::connect(&dialing, &threadDialing::signalDisplay, &w, &MainWindow::slotDisplay, Qt::QueuedConnection);

    //deal with emergency situation
    signal(SIGINT, emergency_sighandler);
    signal(SIGKILL, emergency_sighandler);
    signal(SIGQUIT, emergency_sighandler);

    //parse cmd argc argv...
    cmdLineParser(a);

    if(2 == argc)
    {
        //support AT cmd arguments
        ret = dialing.initUartAndTryCommunicateWith4GModule_ForTest(&fd, nodePath, (char*)BOXV3_NODEPATH_LTE, argv[1]);
        if(ret)
        {
            dialing.showErrInfo(errInfo);
        }
        return 0;
    }else
    {
        //normal start
        dialing.start();
        w.showFullScreen();
    }

    return a.exec();
}
