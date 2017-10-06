TEMPLATE = app
TARGET = AttributeHashes
CONFIG += console

win32 {
LIBS += \
    -lws2_32
}

include(commoninclude.pri)
include(AttributeHashes.pri)

load(qt_tool)

INSTALLS -= target
