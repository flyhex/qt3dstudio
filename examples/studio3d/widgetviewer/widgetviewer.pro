include(../examples.pri)

TEMPLATE = app
QT += widgets studio3d

TARGET = widgetviewer

target.path = $$[QT_INSTALL_EXAMPLES]/studio3d/$$TARGET
INSTALLS += target

HEADERS += mainwindow.h
FORMS += mainwindow.ui
SOURCES += \
    main.cpp \
    mainwindow.cpp
RESOURCES += widgetviewer.qrc
