CONFIG += testcase

TARGET = tst_qaccessibilitylinux
SOURCES += tst_qaccessibilitylinux.cpp

QT += gui-private widgets dbus testlib accessibility_support-private linuxaccessibility_support-private

DBUS_INTERFACES = $$PWD/../../../../src/platformsupport/linuxaccessibility/dbusxml/Bus.xml
