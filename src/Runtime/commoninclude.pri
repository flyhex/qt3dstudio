include($$PWD/../commonplatform.pri)

contains(TEMPLATE, lib) {
    load(qt_helper_lib)
    # qt_helper_lib assumes non-qt lib, so it clears qt from config - reset that
    CONFIG += qt exceptions
}
QT += core gui openglextensions

DEFINES += COMPILED_FROM_DSP \
    UICDM_USE_NVLOG UICDM_META_DATA_NO_SIGNALS

INCLUDEPATH += \
    $$PWD/Source \
    $$PWD/Source/UICRender/Include \
    $$PWD/Source/RuntimeUICDM/Include \
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
    $$PWD/Source/UICState/Include \
    $$PWD/Source/UICState/Editor \
    $$PWD/Source/UICState/Debugger \
    $$PWD/Source/UICEvent/Include \
    $$PWD/Source/UICEvent/InternalInclude \
    $$PWD/Source/UICRender/GraphObjects \
    $$PWD/Source/UICRender/ResourceManager \
    $$PWD/Source/Viewer \
    $$PWD/Source/Viewer/PerfLog \
    $$PWD/../Authoring/UICIMP/UICImportLib \
    $$PWD/../Authoring/UICDM \
    $$PWD/../Authoring/UICDM/Systems \
    $$PWD/../Authoring/UICDM/Systems/Cores \
    $$PWD/../3rdparty/EASTL/UnknownVersion/include \
    ../3rdparty/Lua/UnknownVersion/src \
    ../3rdparty/utf8cpp/2.3.2/source \
    ../3rdparty/color \
    ../3rdparty/RuntimePlatformSpecific/$$PlatformSpecificDir/PlatformLibs \
    $$PWD/../QtExtras/qmlstreamer

# TODO: Investigate whether these can be moved to commonplatform
win32 {
CONFIG += nostrictstrings
QMAKE_CXXFLAGS += /EHsc /GA
QMAKE_CFLAGS += /EHsc /GA
INCLUDEPATH += \
    $$PWD/Source/PlatformSpecific/Windows/Qt3DSLibs
    $$PWD/../3rdparty/RuntimePlatformSpecific/Windows/Qt3DSLibs
}

linux {
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
