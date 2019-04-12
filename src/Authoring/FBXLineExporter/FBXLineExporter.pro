TEMPLATE = app
TARGET = FBXLineExporter
include(../commoninclude.pri)
include($$OUT_PWD/../qtAuthoring-config.pri)
INCLUDEPATH += $$OUT_PWD/..

CONFIG += console
QT -= gui
DEFINES += _UNICODE QT3DS_AUTHORING _AFXDLL PCRE_STATIC

INCLUDEPATH += \
    ../../Runtime/Source/foundation \
    $$QMAKE_INCDIR_FBX \
    ../../3rdparty/EASTL/UnknownVersion/include

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
    -lqt3dsruntimestatic$$qtPlatformTargetSuffix()

SOURCES += FBXLineExporter.cpp

load(qt_tool)

INSTALLS -= target
