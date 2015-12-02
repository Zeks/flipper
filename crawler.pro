#-------------------------------------------------
#
# Project created by QtCreator 2015-11-29T15:56:44
#
#-------------------------------------------------

QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ffse
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui


QMAKE_CXXFLAGS += -std=c++11

