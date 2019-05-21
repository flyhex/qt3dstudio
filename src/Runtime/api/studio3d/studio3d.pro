TARGET = QtStudio3D

include($$PWD/../../commoninclude.pri)
QT += opengl widgets qml

qtHaveModule(multimedia) {
DEFINES += PLATFORM_HAS_QT_MULTIMEDIA_LIB
QT += multimedia
}
CONFIG += console

LIBS += \
    -lqt3dsopengl$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

HEADERS += \
    q3dsdataoutput.h \
    q3dsdataoutput_p.h \
    q3dswidget.h \
    q3dswidget_p.h \
    q3dssurfaceviewer.h \
    q3dssurfaceviewer_p.h \
    qstudio3dglobal.h \
    viewerqmlstreamproxy_p.h \
    q3dsviewersettings.h \
    q3dsviewersettings_p.h \
    q3dspresentation.h \
    q3dspresentation_p.h \
    q3dssceneelement.h \
    q3dssceneelement_p.h \
    q3dselement.h \
    q3dselement_p.h \
    studioutils_p.h \
    q3dscommandqueue_p.h \
    q3dsimagesequencegenerator_p.h \
    q3dsimagesequencegeneratorthread_p.h \
    q3dsdatainput.h \
    q3dsdatainput_p.h

SOURCES += q3dswidget.cpp \
           q3dsdataoutput.cpp \
           q3dssurfaceviewer.cpp \
           viewerqmlstreamproxy.cpp \
           q3dsviewersettings.cpp \
           q3dspresentation.cpp \
           q3dssceneelement.cpp \
           q3dselement.cpp \
           studioutils.cpp \
           q3dscommandqueue.cpp \
           q3dsimagesequencegenerator.cpp \
           q3dsimagesequencegeneratorthread.cpp \
           q3dsdatainput.cpp

load(qt_module)

OTHER_FILES += $$PWD/../../../doc/src/12-cpp-reference/*
