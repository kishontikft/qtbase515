QT += network

QNETWORK_SRC = $$QT_SOURCE_TREE/src/network

INCLUDEPATH += $$QNETWORK_SRC

win32: QMAKE_USE += ws2_32
