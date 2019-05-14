include($$PWD/../../Runtime/commoninclude.pri)

TEMPLATE = app
TARGET = Qt3DViewer
QT += qml quickcontrols2 studio3d-private

INCLUDEPATH += $$PWD/../../Runtime/api/studio3dqml
INCLUDEPATH += $$PWD/../../Runtime/api/studio3d

RESOURCES += Viewer.qrc
RC_ICONS = resources/images/3D-studio-viewer.ico

ICON = resources/images/viewer.icns

SOURCES += \
    $$PWD/../../Runtime/api/studio3dqml/q3dsstudio3d.cpp \
    $$PWD/../../Runtime/api/studio3dqml/q3dsrenderer.cpp \
    $$PWD/../../Runtime/api/studio3dqml/q3dspresentationitem.cpp \
    main.cpp \
    viewer.cpp \
    remotedeploymentreceiver.cpp

HEADERS += \
    $$PWD/../../Runtime/api/studio3dqml/q3dsstudio3d.h \
    $$PWD/../../Runtime/api/studio3dqml/q3dsrenderer.h \
    $$PWD/../../Runtime/api/studio3dqml/q3dspresentationitem.h \
    viewer.h \
    remotedeploymentreceiver.h

android: {
SOURCES += \
    $$PWD/../../Runtime/api/studio3d/q3dsviewersettings.cpp \
    $$PWD/../../Runtime/api/studio3d/q3dspresentation.cpp \
    $$PWD/../../Runtime/api/studio3d/q3dsdatainput.cpp

HEADERS += \
    $$PWD/../../Runtime/api/studio3d/q3dsviewersettings.h \
    $$PWD/../../Runtime/api/studio3d/q3dspresentation.h \
    $$PWD/../../Runtime/api/studio3d/q3dsdatainput.h
}

LIBS += \
    -lqt3dsopengl$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

macos:QMAKE_RPATHDIR += @executable_path/../../../../lib

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
