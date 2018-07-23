QT += core
QT -= gui

TARGET = checkAppFor4G
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    huawei4gmodule.cpp \
    global.c

HEADERS += \
    huawei4gmodule.h \
    global.h

