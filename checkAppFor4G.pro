QT += core
#QT -= gui
QT += gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = checkAppFor4G
#CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    global.c \
    threaddialing.cpp \
    mainwindow.cpp \
    tinyxml2.cpp

HEADERS += \
    global.h \
    threaddialing.h \
    mainwindow.h \
    tinyxml2.h

FORMS += \
    mainwindow.ui

