CONFIG += testcase
TARGET = tst_qtabwidget

QT += widgets widgets-private testlib

INCLUDEPATH += ../

HEADERS +=  
SOURCES += tst_qtabwidget.cpp

win32:!winrt: QMAKE_USE += user32
