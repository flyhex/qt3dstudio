TEMPLATE = lib
TARGET = qt3dsstate
CONFIG += staticlib console
include($$PWD/commoninclude.pri)
DEFINES += QT3DS_STATE_EXPORTS
INCLUDEPATH += \
    $$PWD/../Runtime/Source/Qt3DSFoundation/Include \
    $$PWD/../Runtime/Source/Runtime/Include \
    $$PWD/../Runtime/Source/System/Include \
    $$PWD/../Runtime/Source/Qt3DSRender/Include \
    $$PWD/../Runtime/Source/UICState/Include \
    $$PWD/../Runtime/Source/UICState/Editor \
    $$PWD/../Runtime/Source/UICState/Debugger \
    $$PWD/../Runtime/Source/Qt3DSState/Include \
    $$PWD/../Runtime/Source/Qt3DSState/InternalInclude \
    $$PWD/../Runtime/Source/UICEvent/Include \
    $$PWD/../Runtime/Source/UICEvent/InternalInclude \
    ../3rdparty/EASTL/UnknownVersion/include \
    ../3rdparty/Lua/UnknownVersion/src

include(Qt3DSState.pri)
