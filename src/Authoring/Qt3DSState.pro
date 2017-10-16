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
    $$PWD/../Runtime/Source/Qt3DSStateApplication/Include \
    $$PWD/../Runtime/Source/Qt3DSStateApplication/Editor \
    $$PWD/../Runtime/Source/Qt3DSStateApplication/Debugger \
    $$PWD/../Runtime/Source/Qt3DSState/Include \
    $$PWD/../Runtime/Source/Qt3DSState/InternalInclude \
    $$PWD/../Runtime/Source/Qt3DSEvent/Include \
    $$PWD/../Runtime/Source/Qt3DSEvent/InternalInclude \
    ../3rdparty/EASTL/UnknownVersion/include \
    ../3rdparty/Lua/UnknownVersion/src

include(Qt3DSState.pri)
