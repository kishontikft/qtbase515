CONFIG += testcase
TARGET = tst_qhttpnetworkconnection
SOURCES  += tst_qhttpnetworkconnection.cpp
requires(qtConfig(private_tests))

QT = core-private network-private testlib

CONFIG += unsupported/testserver
QT_TEST_SERVER_LIST = apache2
