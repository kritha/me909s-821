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

    THREADDIALING moduleLTE;
    threadLTENetMonitor monitor;

    QObject::connect(&monitor, SIGNAL(signalStartDialing(char)), &moduleLTE, SLOT(slotStartDialing(char)), Qt::QueuedConnection);

    int ret = 0;
    int fd = -1;
    char nodePath[128] = {};

    monitor.start();

    if(argc < 2)
    {
        moduleLTE.start();
    }else
    {
        ret = moduleLTE.initUartAndTryCommunicateWith4GModule_ForTest(&fd, nodePath, (char*)BOXV3_NODEPATH_LTE, argv[1]);
        if(ret)
        {
            moduleLTE.showErrInfo(errInfo);
        }
        return 0;
    }

    return a.exec();
}
