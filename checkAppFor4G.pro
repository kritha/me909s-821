QT += core
QT -= gui

TARGET = checkAppFor4G
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    global.c \
    threaddialing.cpp \
    threadltenetmonitor.cpp

HEADERS += \
    global.h \
    threaddialing.h \
    threadltenetmonitor.h

