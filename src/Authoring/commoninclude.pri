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

INCLUDEPATH += $$PWD/../Runtime/ogl-runtime/src

win32 {
INCLUDEPATH += \
    $$PWD/../Runtime/ogl-runtime/src/platformspecific/windows/libs \
    $$PWD/../Runtime/ogl-runtime/src/3rdparty/platformspecific/Windows/PlatformLibs \
    $$PWD/../Runtime/ogl-runtime/src/3rdparty/platformspecific/Windows/Qt3DSLibs
}

linux {
INCLUDEPATH += \
    $$PWD/../Runtime/ogl-runtime/src/3rdparty/platformspecific/Linux/PlatformLibs \
    $$PWD/../Runtime/ogl-runtime/src/platformspecific/Linux/libs \
    $$PWD/../Runtime/ogl-runtime/src/3rdparty/platformspecific/Linux/Qt3DSLibs
}

macos {
INCLUDEPATH += \
    $$PWD/../Runtime/ogl-runtime/src/3rdparty/platformspecific/Macos/PlatformLibs \
    $$PWD/../Runtime/ogl-runtime/src/platformspecific/macos/libs \
    $$PWD/../Runtime/ogl-runtime/src/3rdparty/platformspecific/Macos/Qt3DSLibs

}
android {
INCLUDEPATH += \
    $$PWD/../Runtime/ogl-runtime/src/platformspecific/Android/jni/libs
    $$PWD/../Runtime/ogl-runtime/src/platformspecific/Android/jni/libs/nv_thread
}
