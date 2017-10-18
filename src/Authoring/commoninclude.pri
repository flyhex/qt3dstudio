include($$PWD/../commonplatform.pri)

contains(TEMPLATE, lib) {
    load(qt_helper_lib)
    # qt_helper_lib assumes non-qt lib, so it clears qt from config - reset that
    CONFIG += qt exceptions
    QT += core gui
}

# TODO: Investigate whether these can be moved to commonplatform
win32 {
    QMAKE_CXXFLAGS += /EHsc /GA
    QMAKE_CFLAGS += /EHsc /GA
}

INCLUDEPATH += $$PWD/../Runtime/Source

win32 {
INCLUDEPATH += \
    $$PWD/../Runtime/Source/PlatformSpecific/Windows/PlatformLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Windows/PlatformLibs \
    $$PWD/../Runtime/Source/PlatformSpecific/Windows/Qt3DSLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Windows/Qt3DSLibs
}

linux {
INCLUDEPATH += \
    $$PWD/../Runtime/Source/PlatformSpecific/Linux/PlatformLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Linux/PlatformLibs \
    $$PWD/../Runtime/Source/PlatformSpecific/Linux/Qt3DSLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Linux/Qt3DSLibs
}

macos {
INCLUDEPATH += \
    $$PWD/../Runtime/Source/PlatformSpecific/Macos/PlatformLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Macos/PlatformLibs \
    $$PWD/../Runtime/Source/PlatformSpecific/Macos/Qt3DSLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Macos/Qt3DSLibs

}
android {
INCLUDEPATH += \
    $$PWD/../Runtime/Source/PlatformSpecific/Android/jni/Qt3DSLibs
    $$PWD/../Runtime/Source/PlatformSpecific/Android/jni/Qt3DSLibs/nv_thread
}
