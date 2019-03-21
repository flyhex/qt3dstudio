include($$PWD/../commonplatform.pri)

contains(TEMPLATE, lib) {
    load(qt_helper_lib)
    # qt_helper_lib assumes non-qt lib, so it clears qt from config - reset that
    CONFIG += qt exceptions
}
QT += core gui openglextensions

DEFINES += COMPILED_FROM_DSP \
    QT3DSDM_USE_NVLOG QT3DSDM_META_DATA_NO_SIGNALS \
    QT3DS_AUTOTESTS_ENABLED

INCLUDEPATH += \
    $$PWD/Source \
    $$PWD/Source/datamodel \
    $$PWD/Source/runtime \
    $$PWD/Source/system \
    $$PWD/Source/engine \
    $$PWD/Source/foundation \
    $$PWD/Source/render \
    $$PWD/Source/render/gl2 \
    $$PWD/Source/render/gl3 \
    $$PWD/Source/render/glg \
    $$PWD/Source/uipparser \
    $$PWD/Source/state \
    $$PWD/Source/stateapplication \
    $$PWD/Source/stateapplication/editor \
    $$PWD/Source/stateapplication/debugger \
    $$PWD/Source/event \
    $$PWD/Source/viewer \
    $$PWD/Source/viewer/perflog \
    $$PWD/../Authoring/QT3DSIMP/Qt3DSImportLib \
    $$PWD/../Authoring/QT3DSDM \
    $$PWD/../Authoring/QT3DSDM/Systems \
    $$PWD/../Authoring/QT3DSDM/Systems/Cores \
    $$PWD/../3rdparty/EASTL/UnknownVersion/include \
    $$PWD/../3rdparty/utf8cpp/2.3.2/source \
    $$PWD/../3rdparty/color \
    $$PWD/../3rdparty/RuntimePlatformSpecific/$$PlatformSpecificDir/PlatformLibs \
    $$PWD/../QtExtras/qmlstreamer \
    $$PWD/Source/runtimerender \
    $$PWD/Source/runtimerender/graphobjects \
    $$PWD/Source/runtimerender/resourcemanager \
    $$PWD/../Viewer/studio3d

# TODO: Investigate whether these can be moved to commonplatform
win32-msvc {
    CONFIG += nostrictstrings
    QMAKE_CXXFLAGS += /EHsc /GA
    QMAKE_CFLAGS += /EHsc /GA
}

win32 {
INCLUDEPATH += \
    $$PWD/Source/platformspecific/windows/libs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Windows/Qt3DSLibs
}

linux|qnx {
QMAKE_CXXFLAGS += -fpermissive
QMAKE_CFLAGS += -fpermissive
INCLUDEPATH += \
    $$PWD/Source/foundation/linux \
    $$PWD/Source/platformspecific/linux/libs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Linux/Qt3DSLibs
}

integrity {
INCLUDEPATH += \
    $$PWD/Source/foundation/linux \
    $$PWD/Source/platformspecific/linux/libs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Linux/Qt3DSLibs
}

macos {
INCLUDEPATH += \
  $$PWD/../3rdparty/RuntimePlatformSpecific/Macos/Qt3DSLibs \
  $$PWD/Source/platformspecific/macos/libs

}

android {
INCLUDEPATH += \
    $$PWD/Source/platformspecific/android/jni/libs/nv_thread \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Android/jni/Qt3DSLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Android/jni
}
