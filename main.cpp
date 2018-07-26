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
#if 1
    ret = module4G.checkInternetAccess();
    if(ret)
    {
        ret = module4G.huaweiLTEmoduleDialingProcess();
        if(ret)
        {
            module4G.showErrInfo(errInfo);
        }
    }else
    {
        //Everythig is alerady works well.
    }
#else
    if(argc < 2)
    {
        perror("Too few argument.");
    }else
    {
        ret = module4G.initUartAndTryCommunicateWith4GModule_ForTest((char*)BOXV3_NODEPATH_LTE, argv[1]);
        if(ret)
        {
            module4G.showErrInfo(errInfo);
        }
    }
#endif
    return 0;
}

