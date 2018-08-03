include($$PWD/../commonplatform.pri)

contains(TEMPLATE, lib) {
    load(qt_helper_lib)
    # qt_helper_lib assumes non-qt lib, so it clears qt from config - reset that
    CONFIG += qt exceptions
    QT += core gui
}

# TODO: Investigate whether these can be moved to commonplatform
win32-msvc {
    QMAKE_CXXFLAGS += /EHsc /GA
    QMAKE_CFLAGS += /EHsc /GA
}

mingw:win32 {
    DEFINES += WIN32
}

INCLUDEPATH += \
    $$PWD/../shared/header \
    $$PWD/../shared/header/foundation


linux|qnx {
QMAKE_CXXFLAGS += -fpermissive
QMAKE_CFLAGS += -fpermissive
INCLUDEPATH += \
    $$PWD/../shared/header/foundation/linux
}

integrity {
INCLUDEPATH += \
    $$PWD/../shared/header/foundation/linux
}
