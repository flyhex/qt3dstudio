TEMPLATE = app
TARGET = MorphLines
include($$PWD/commoninclude.pri)
include($$OUT_PWD/qtAuthoring-config.pri)
CONFIG += console nostrictstrings
QT -= gui
DEFINES += _UNICODE QT3DS_AUTHORING _AFXDLL PCRE_STATIC

INCLUDEPATH += \
    QT3DSIMP/Qt3DSImportLib \
    $$PWD/../Runtime/Source/Qt3DSRender/Include \
    $$PWD/../Runtime/Source/Qt3DSFoundation/Include \
    ../3rdparty/EASTL/UnknownVersion/include

LIBS += \
    -L"$$BOOSTDIR" \
    $$QMAKE_LIBS_FBX \
    -lws2_32 \
    -lLua$$qtPlatformTargetSuffix() \
    -lEASTL$$qtPlatformTargetSuffix() \
    -lRpcrt4 \
    -lGdiplus \
    -lUser32 \
    -lUserenv \
    -lWbemuuid \
    -lWinmm \
    -l$$BOOSTSIGNALLIB \
    -l$$BOOSTSYSTEMLIB \
    -l$$BOOSTFILESYSTEMLIB \
    -lqt3dsruntimestatic$$qtPlatformTargetSuffix() \
    -lCoreLib$$qtPlatformTargetSuffix() \
    -lCommonLib$$qtPlatformTargetSuffix()

include(MorphLines.pri)

load(qt_tool)

INSTALLS -= target
