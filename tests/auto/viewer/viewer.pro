TEMPLATE = app
CONFIG += testcase
include($$PWD/../../../src/Runtime/commoninclude.pri)

TARGET = tst_qt3dsviewer
QT += testlib gui quick
RESOURCES += viewer.qrc

HEADERS += \
    tst_qt3dsviewer.h

SOURCES += \
    tst_qt3dsviewer.cpp

LIBS += \
    -lqt3dsruntime$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

ANDROID_EXTRA_LIBS = \
  libqt3dsruntime.so \
  libqt3dsqmlstreamer.so
