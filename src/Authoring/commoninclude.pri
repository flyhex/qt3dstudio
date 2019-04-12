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

INCLUDEPATH += $$PWD/../Runtime/Source

win32 {
INCLUDEPATH += \
    $$PWD/../Runtime/Source/platformspecific/windows/libs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Windows/PlatformLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Windows/Qt3DSLibs
}

linux {
INCLUDEPATH += \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Linux/PlatformLibs \
    $$PWD/../Runtime/Source/platformspecific/Linux/libs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Linux/Qt3DSLibs
}

macos {
INCLUDEPATH += \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Macos/PlatformLibs \
    $$PWD/../Runtime/Source/platformspecific/macos/libs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Macos/Qt3DSLibs

}
android {
INCLUDEPATH += \
    $$PWD/../Runtime/Source/platformspecific/Android/jni/libs
    $$PWD/../Runtime/Source/platformspecific/Android/jni/libs/nv_thread
}
