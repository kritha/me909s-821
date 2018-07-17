#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include "huawei4gmodule.h"

#define BOXV3CHECKAPP_VERSION "V0.0.2"

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

    HUAWEI4GModule module4G;
    //module4G.getDeviceNode((char*)BOXV3_UARTNODE_DIR);
    module4G.startCheck((char*)BOXV3_NODEPATH_4G);

    //return a.exec();
    return 0;
}

