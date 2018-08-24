#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include "threaddialing.h"
#include "global.h"
#include "mainwindow.h"

static QString cmdLineParser(MainWindow& w, QCoreApplication& a)
{
#define USE_OPTARGS
    QString value;
    QCoreApplication::setApplicationVersion(BOXV3CHECKAPP_VERSION);
#ifdef USE_OPTARGS
    QCommandLineOption displayArgs("ds");
    displayArgs.setDescription("Set display unable.");
    displayArgs.setValueName("off/on");
    QCommandLineOption optArgs("t");
    optArgs.setDescription("try count if dialed failed, default is infinite.");
    optArgs.setValueName("tryCount");
    QCommandLineOption supportArgs("AT");
    supportArgs.setDescription("This soft ware support exec with AT<cmd>(like: .\"/a.out ATI\").");
    supportArgs.setValueName("cmd");
    QCommandLineOption resetArgs("rc");
    resetArgs.setDescription("Set max reset count when one dialing process is weird failed. Default is 0.");
    resetArgs.setValueName("reset cnt");
    QCommandLineOption switchArgs("sc");
    switchArgs.setDescription("Set max switch card slot times before reset option when one dialing process is weird failed. Default is 3.");
    switchArgs.setValueName("switch slot cnt");
#endif
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addVersionOption();
    parser.addHelpOption();
#ifdef USE_OPTARGS
    parser.addOption(optArgs);
    parser.addOption(supportArgs);
    parser.addOption(resetArgs);
    parser.addOption(switchArgs);
    parser.addOption(displayArgs);
#endif
    parser.process(a);

#ifdef USE_OPTARGS
    value = parser.value(displayArgs);
    if(value.contains("on"))
    {
        w.showFullScreen();
    }

    value = parser.value(optArgs);
    if(!value.isEmpty() || !value.isNull())
    {
        gData.tryCount = value.toInt();
    }else
    {
        gData.tryCount = TRY_COUNT_INFINITE_SIGN;
    }
    DEBUG_PRINTF("tryCount:%d.", gData.tryCount);
    value = parser.value(resetArgs);
    if(!value.isEmpty() || !value.isNull())
    {
        gData.resetCnt = value.toInt();
        if(gData.resetCnt < 0) gData.resetCnt = DEFAULT_MAX_RESET_COUNT;
    }else
    {
        gData.resetCnt = DEFAULT_MAX_RESET_COUNT;
    }
    DEBUG_PRINTF("resetCount:%d.", gData.resetCnt);
    value = parser.value(switchArgs);
    if(!value.isEmpty() || !value.isNull())
    {
        gData.switchCnt = value.toInt();
        if(gData.switchCnt < 0) gData.switchCnt = DEFAULT_MAX_SWITCH_SLOT_COUNT;
    }else
    {
        gData.switchCnt = DEFAULT_MAX_SWITCH_SLOT_COUNT;
        /*
        if(0 == gData.resetCnt)
        {
            gData.resetCnt = 1;
        }*/
    }
    DEBUG_PRINTF("switchArgs:%d.", gData.switchCnt);
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


    //parse cmd argc argv...
    cmdLineParser(w, a);

    if(2 == argc)
    {
        //support AT cmd arguments
        ret = dialing.initUartAndTryCommunicateWith4GModule_ForTest(&fd, nodePath, (char*)BOXV3_NODEPATH_LTE, argv[1]);
        if(ret)
        {
            dialing.showErrInfo(gData.errInfo);
        }
        return 0;
    }else
    {
        //normal start
        dialing.start();
    }

    return a.exec();
}
