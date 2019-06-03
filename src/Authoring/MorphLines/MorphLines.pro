TEMPLATE = app
TARGET = MorphLines
include(../commoninclude.pri)
include($$OUT_PWD/../qtAuthoring-config.pri)
INCLUDEPATH += $$OUT_PWD/..

CONFIG += console nostrictstrings
QT -= gui
DEFINES += _UNICODE QT3DS_AUTHORING _AFXDLL PCRE_STATIC

INCLUDEPATH += \
    ../QT3DSIMP/Qt3DSImportLib \
    ../../Runtime/ogl-runtime/src/render \
    ../../Runtime/ogl-runtime/src/foundation \
    ../../Runtime/ogl-runtime/src/3rdparty/EASTL/UnknownVersion/include

LIBS += \
    $$QMAKE_LIBS_FBX \
    -lws2_32 \
    -lEASTL$$qtPlatformTargetSuffix() \
    -lRpcrt4 \
    -lGdiplus \
    -lUser32 \
    -lUserenv \
    -lWbemuuid \
    -lWinmm \
    -lqt3dsruntimestatic$$qtPlatformTargetSuffix() \
    -lCoreLib$$qtPlatformTargetSuffix() \
    -lCommonLib$$qtPlatformTargetSuffix()

SOURCES += MorphLines.cpp

load(qt_tool)

INSTALLS -= target
