include($$PWD/../../Runtime/commoninclude.pri)

QT += qml quick opengl studio3d-private
CONFIG += plugin

qtHaveModule(multimedia) {
DEFINES += PLATFORM_HAS_QT_MULTIMEDIA_LIB
QT += multimedia
}

TARGET = qtstudio3d
TARGETPATH = QtStudio3D
IMPORT_VERSION = 1.0

SOURCES += \
    Qt3DSViewPlugin.cpp \
    Qt3DSView.cpp \
    Qt3DSRenderer.cpp \
    q3dspresentationitem.cpp

HEADERS += \
    Qt3DSViewPlugin.h \
    Qt3DSView.h \
    Qt3DSRenderer.h \
    q3dspresentationitem.h

LIBS += \
    -lqt3dsruntime$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

QMAKE_DOCS = $$absolute_path($$PWD/../../../doc/qt3dstudio.qdocconf)

OTHER_FILES += \
  qmldir \
  $$PWD/../../../doc/src/06-qml-reference/*

load(qml_plugin)
