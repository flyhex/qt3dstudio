include($$PWD/../../Runtime/commoninclude.pri)

TEMPLATE = app
TARGET = Qt3DViewer
QT += widgets opengl qml quickwidgets quickcontrols2 studio3d-private

INCLUDEPATH += $$PWD/../qmlviewer

FORMS += mainwindow.ui
RESOURCES += Viewer.qrc
RC_FILE += Viewer.rc

SOURCES += \
    $$PWD/../qmlviewer/Qt3DSViewPlugin.cpp \
    $$PWD/../qmlviewer/Qt3DSView.cpp \
    $$PWD/../qmlviewer/Qt3DSRenderer.cpp \
    $$PWD/../qmlviewer/q3dspresentationitem.cpp \
    main.cpp \
    mainwindow.cpp \
    remotedeploymentreceiver.cpp

HEADERS += \
    $$PWD/../qmlviewer/Qt3DSViewPlugin.h \
    $$PWD/../qmlviewer/Qt3DSView.h \
    $$PWD/../qmlviewer/Qt3DSRenderer.h \
    $$PWD/../qmlviewer/q3dspresentationitem.h \
    mainwindow.h \
    remotedeploymentreceiver.h

LIBS += \
    -lqt3dsruntime$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

macos:QMAKE_RPATHDIR += $$PWD/../../../lib

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
