TEMPLATE = app
CONFIG += testcase
include($$PWD/../../../../src/commonplatform.pri)

TARGET = tst_qt3dsqmlstream

QT += testlib quick

SOURCES += tst_qt3dsqmlstream.cpp

INCLUDEPATH += \
    $$PWD/../../../../src/QtExtras/qmlstreamer

LIBS += \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

ANDROID_EXTRA_LIBS = \
  libqt3dsqmlstreamer.so
