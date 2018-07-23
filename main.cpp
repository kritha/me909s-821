#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include "huawei4gmodule.h"
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
    int ret = 0;
    QCoreApplication a(argc, argv);
    cmdLineParser(a);

    HUAWEI4GModule module4G;
#if 0
    ret = module4G.tryCommunicateWith4GModule((char*)BOXV3_NODEPATH_4G, 1);
    if(ret)
    {
        module4G.showErrInfo(errInfo);
    }
#else
    if(argc < 2)
    {
        perror("Too few argument.");
    }else
    {
        ret = module4G.initUartAndTryCommunicateWith4GModule_ForTest((char*)BOXV3_NODEPATH_4G, argv[1]);
        if(ret)
        {
            module4G.showErrInfo(errInfo);
        }
    }
#endif
    return 0;
}

