include($$PWD/../../Runtime/commoninclude.pri)

TEMPLATE = app
TARGET = Qt3DViewer
QT += qml quickcontrols2 studio3d-private

INCLUDEPATH += $$PWD/../qmlviewer

RESOURCES += Viewer.qrc
RC_ICONS = resources/images/3D-studio-viewer.ico

ICON = resources/images/viewer.icns

SOURCES += \
    $$PWD/../qmlviewer/Qt3DSView.cpp \
    $$PWD/../qmlviewer/Qt3DSRenderer.cpp \
    $$PWD/../qmlviewer/q3dspresentationitem.cpp \
    main.cpp \
    viewer.cpp \
    remotedeploymentreceiver.cpp

HEADERS += \
    $$PWD/../qmlviewer/Qt3DSView.h \
    $$PWD/../qmlviewer/Qt3DSRenderer.h \
    $$PWD/../qmlviewer/q3dspresentationitem.h \
    viewer.h \
    remotedeploymentreceiver.h

android: {
SOURCES += \
    $$PWD/../studio3d/q3dsviewersettings.cpp \
    $$PWD/../studio3d/q3dspresentation.cpp \
    $$PWD/../studio3d/q3dsdatainput.cpp

HEADERS += \
    $$PWD/../studio3d/q3dsviewersettings.h \
    $$PWD/../studio3d/q3dspresentation.h \
    $$PWD/../studio3d/q3dsdatainput.h
}

LIBS += \
    -lqt3dsruntime$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

macos:QMAKE_RPATHDIR += @executable_path/../../../../lib

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
