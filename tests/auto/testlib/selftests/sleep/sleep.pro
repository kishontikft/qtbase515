SOURCES += tst_sleep.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = sleep

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
