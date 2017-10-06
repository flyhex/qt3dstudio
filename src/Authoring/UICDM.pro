TEMPLATE = lib
TARGET = UICDM
CONFIG += staticlib
include($$PWD/commoninclude.pri)
DEFINES += UIC_AUTHORING _AFXDLL \
    PCRE_STATIC BOOST_SIGNALS_NO_DEPRECATION_WARNING BOOST_ALL_NO_LIB \
    _UNICODE

linux {
    DEFINES += UICDM_META_DATA_NO_SIGNALS
}

INCLUDEPATH += \
    UICDM \
    UICDM/Systems \
    UICDM/Systems/Cores \
    $$PWD/../Runtime/Source/System/Include \
    $$PWD/../Runtime/Source/Qt3DSFoundation/Include \
    $$PWD/../Runtime/Source/UICRender/Include \
    $$PWD/../Runtime/Source/RuntimeUICDM/Include \
    $$PWD/../Runtime/Source/Qt3DSRender/Include \
    ../3rdparty/EASTL/UnknownVersion/include \
    ../3rdparty/utf8cpp/2.3.2/source \
    ../3rdparty/color

PRECOMPILED_HEADER = UICDM/UICDMPrefix.h

include(UICDM.pri)
