TARGET = QtStudio3D

include($$PWD/../../Runtime/commoninclude.pri)
QT += opengl widgets qml

qtHaveModule(multimedia) {
DEFINES += PLATFORM_HAS_QT_MULTIMEDIA_LIB
QT += multimedia
}
CONFIG += console
DEFINES += QTSTUDIO3D_EXPORTS

LIBS += \
    -lqt3dsruntime$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

include(studio3d.pri)

load(qt_module)

OTHER_FILES += $$PWD/../../../doc/src/12-cpp-reference/*
