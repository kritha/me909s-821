#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include "threaddialing.h"
#include "global.h"
#include "mainwindow.h"

static QString cmdLineParser(QCoreApplication& a)
{
//#define USE_OPTARGS
    QString value;
    QCoreApplication::setApplicationVersion(BOXV3CHECKAPP_VERSION);
#ifdef USE_OPTARGS
    QCommandLineOption optArgs("f");
    optArgs.setDescription("force option(like RD_ONLY for read devicenode only.)");
    optArgs.setValueName("ForceOptions");
#endif
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addVersionOption();
    parser.addHelpOption();
#ifdef USE_OPTARGS
    parser.addOption(optArgs);
#endif
    parser.process(a);

#ifdef USE_OPTARGS
    value = parser.value(optArgs);
#endif

    return value.contains("RD_ONLY") ? value:QString();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    threadDialing dialing;

    //display
    QObject::connect(&dialing, &threadDialing::signalDisplay, &w, &MainWindow::slotDisplay, Qt::QueuedConnection);

    int ret = 0;
    int fd = -1;
    char nodePath[128] = {};

    cmdLineParser(a);

    if(argc < 2)
    {
        dialing.start();
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
