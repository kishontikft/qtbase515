SOURCES += tst_signaldumper.cpp
QT = core testlib-private

macos:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = signaldumper

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
