TARGET = qtcrashhandler
QT += network widgets
TEMPLATE = app

INCLUDEPATH += \
    $$(BREAKPAD_SOURCE_DIR)/src \
    qtcrashhandler

SOURCES += \
    main.cpp \
    mainwidget.cpp \
    detaildialog.cpp \
    dumpsender.cpp

HEADERS += \
    mainwidget.h \
    detaildialog.h \
    dumpsender.h

FORMS += \
    mainwidget.ui

DESTDIR = ../../../../bin
