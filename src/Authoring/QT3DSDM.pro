TEMPLATE = lib
TARGET = QT3DSDM
CONFIG += staticlib
include($$PWD/commoninclude.pri)

DEFINES += QT3DS_AUTHORING _AFXDLL PCRE_STATIC _UNICODE

linux {
    DEFINES += QT3DSDM_META_DATA_NO_SIGNALS
}

INCLUDEPATH += \
    QT3DSDM \
    QT3DSDM/Systems \
    QT3DSDM/Systems/Cores \
    $$PWD/../Runtime/Source/System/Include \
    $$PWD/../Runtime/Source/Qt3DSFoundation/Include \
    $$PWD/../Runtime/Source/Qt3DSRuntimeRender/Include \
    $$PWD/../Runtime/Source/DataModel/Include \
    $$PWD/../Runtime/Source/Qt3DSRender/Include \
    ../3rdparty/EASTL/UnknownVersion/include \
    ../3rdparty/utf8cpp/2.3.2/source \
    ../3rdparty/color

PRECOMPILED_HEADER = QT3DSDM/Qt3DSDMPrefix.h

include(QT3DSDM.pri)
