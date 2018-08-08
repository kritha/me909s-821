#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include "threaddialing.h"
#include "threadltenetmonitor.h"
#include "global.h"

static void cmdLineParser(QCoreApplication& a)
{
    QCoreApplication::setApplicationVersion(BOXV3CHECKAPP_VERSION);
    QCommandLineOption optAdmin("admin");
    optAdmin.setDescription("Administrator's options(like: anything)");
    optAdmin.setValueName("wishes");

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(optAdmin);
    parser.process(a);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    cmdLineParser(a);

    threadDialing dialing;
    threadLTENetMonitor monitor;

    QObject::connect(&monitor, &threadLTENetMonitor::signalStartDialing, &dialing, &threadDialing::slotStartDialing);
    /*this connect for prevent multiple access dialing process at the same time*/
    QObject::connect(&dialing, &threadDialing::signalDialingEnd, &monitor, &threadLTENetMonitor::slotDialingEnd);

    int ret = 0;
    int fd = -1;
    char nodePath[128] = {};

#if 0 //read for debug
    moduleLTE.slotAlwaysRecvMsgForDebug();
    return a.exec();
#endif

    if(argc < 2)
    {
        monitor.start();
    }else
    {
        ret = dialing.getWorkerObject()->initUartAndTryCommunicateWith4GModule_ForTest(&fd, nodePath, (char*)BOXV3_NODEPATH_LTE, argv[1]);
        if(ret)
        {
            dialing.getWorkerObject()->showErrInfo(errInfo);
        }
        return 0;
    }

    return a.exec();
}
