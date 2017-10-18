TEMPLATE = app
TARGET = FBXLineExporter
include($$PWD/commoninclude.pri)
include($$OUT_PWD/qtAuthoring-config.pri)
CONFIG += console
QT -= gui
DEFINES += _UNICODE QT3DS_AUTHORING _AFXDLL PCRE_STATIC

INCLUDEPATH += \
    $$PWD/../Runtime/Source/Qt3DSFoundation/Include \
    $$QMAKE_INCDIR_FBX \
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
    -lqt3dsruntimestatic$$qtPlatformTargetSuffix()

include(FBXLineExporter.pri)

load(qt_tool)

INSTALLS -= target
