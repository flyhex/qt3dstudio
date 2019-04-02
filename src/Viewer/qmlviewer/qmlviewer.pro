include($$PWD/../../Runtime/commoninclude.pri)

QT += qml quick opengl studio3d-private
CONFIG += plugin

qtHaveModule(multimedia) {
DEFINES += PLATFORM_HAS_QT_MULTIMEDIA_LIB
QT += multimedia
}

TARGET = qtstudio3dopengl
TARGETPATH = QtStudio3D/OpenGL
IMPORT_VERSION = 2.4

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
    -lqt3dsopengl$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

OTHER_FILES += \
  qmldir

load(qml_plugin)
