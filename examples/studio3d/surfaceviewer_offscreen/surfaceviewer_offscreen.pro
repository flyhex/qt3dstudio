include(../examples.pri)

TEMPLATE = app

QT += studio3d

target.path = $$[QT_INSTALL_EXAMPLES]/studio3d/$$TARGET
INSTALLS += target

SOURCES += main.cpp

RESOURCES += surfaceviewer_offscreen.qrc

OTHER_FILES += doc/src/* \
               doc/images/*
