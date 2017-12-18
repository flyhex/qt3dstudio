include($$PWD/../commonplatform.pri)

contains(TEMPLATE, lib) {
    load(qt_helper_lib)
    # qt_helper_lib assumes non-qt lib, so it clears qt from config - reset that
    CONFIG += qt exceptions
}
QT += core gui openglextensions

DEFINES += COMPILED_FROM_DSP \
    QT3DSDM_USE_NVLOG QT3DSDM_META_DATA_NO_SIGNALS

INCLUDEPATH += \
    $$PWD/Source \
    $$PWD/Source/DataModel/Include \
    $$PWD/Source/Runtime/Include \
    $$PWD/Source/System/Include \
    $$PWD/Source/Engine/Include \
    $$PWD/Source/Qt3DSFoundation/Include \
    $$PWD/Source/Qt3DSRender/Include \
    $$PWD/Source/Qt3DSRender/Source \
    $$PWD/Source/Qt3DSRender/Source/gl2 \
    $$PWD/Source/Qt3DSRender/Source/gl3 \
    $$PWD/Source/Qt3DSRender/Source/glg \
    $$PWD/Source/UIPParser/Include \
    $$PWD/Source/Qt3DSState/Include \
    $$PWD/Source/Qt3DSStateApplication/Include \
    $$PWD/Source/Qt3DSStateApplication/Editor \
    $$PWD/Source/Qt3DSStateApplication/Debugger \
    $$PWD/Source/Qt3DSEvent/Include \
    $$PWD/Source/Qt3DSEvent/InternalInclude \
    $$PWD/Source/Viewer \
    $$PWD/Source/Viewer/PerfLog \
    $$PWD/../Authoring/QT3DSIMP/Qt3DSImportLib \
    $$PWD/../Authoring/QT3DSDM \
    $$PWD/../Authoring/QT3DSDM/Systems \
    $$PWD/../Authoring/QT3DSDM/Systems/Cores \
    $$PWD/../3rdparty/EASTL/UnknownVersion/include \
    $$PWD/../3rdparty/Lua/UnknownVersion/src \
    $$PWD/../3rdparty/utf8cpp/2.3.2/source \
    $$PWD/../3rdparty/color \
    $$PWD/../3rdparty/RuntimePlatformSpecific/$$PlatformSpecificDir/PlatformLibs \
    $$PWD/../QtExtras/qmlstreamer \
    $$PWD/Source/Qt3DSRuntimeRender/Include \
    $$PWD/Source/Qt3DSRuntimeRender/GraphObjects \
    $$PWD/Source/Qt3DSRuntimeRender/ResourceManager

# TODO: Investigate whether these can be moved to commonplatform
win32-msvc {
    CONFIG += nostrictstrings
    QMAKE_CXXFLAGS += /EHsc /GA
    QMAKE_CFLAGS += /EHsc /GA
}

win32 {
INCLUDEPATH += \
    $$PWD/Source/PlatformSpecific/Windows/Qt3DSLibs
    $$PWD/../3rdparty/RuntimePlatformSpecific/Windows/Qt3DSLibs
}

linux|qnx {
QMAKE_CXXFLAGS += -fpermissive
QMAKE_CFLAGS += -fpermissive
INCLUDEPATH += \
    $$PWD/Source/Qt3DSFoundation/Include/foundation/linux \
    $$PWD/Source/Qt3DSFoundation/Source/foundation/linux \
    $$PWD/Source/PlatformSpecific/Linux/Qt3DSLibs
    $$PWD/../3rdparty/RuntimePlatformSpecific/Linux/Qt3DSLibs
}

integrity {
INCLUDEPATH += \
    $$PWD/Source/Qt3DSFoundation/Include/foundation/linux \
    $$PWD/Source/Qt3DSFoundation/Source/foundation/linux \
    $$PWD/Source/PlatformSpecific/Linux/Qt3DSLibs
    $$PWD/../3rdparty/RuntimePlatformSpecific/Linux/Qt3DSLibs
}

macos {
INCLUDEPATH += \
  $$PWD/../3rdparty/RuntimePlatformSpecific/Macos/Qt3DSLibs \
  $$PWD/Source/PlatformSpecific/Macos/Qt3DSLibs

}

android {
INCLUDEPATH += \
    $$PWD/Source/PlatformSpecific/Android/jni/Qt3DSLibs/nv_thread \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Android/jni/Qt3DSLibs \
    $$PWD/../3rdparty/RuntimePlatformSpecific/Android/jni
}
