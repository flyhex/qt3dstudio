TEMPLATE = lib
TARGET = CommonLib
CONFIG += staticlib nostrictstrings
include($$PWD/commoninclude.pri)

QT += widgets

DEFINES += _UNICODE QT3DS_AUTHORING _AFXDLL \
    PCRE_STATIC BOOST_SIGNALS_NO_DEPRECATION_WARNING _AMD64_ _WINSOCK_DEPRECATED_NO_WARNINGS

INCLUDEPATH += \
    Common/Code/_Win32 \
    Common/Code/Thread \
    Common/Code/IO \
    Common/Code \
    Common/Code/Exceptions \
    Common/Code/_Win32/Include \
    Common/Code/Report \
    Common/Code/Serialize \
    QT3DSDM \
    QT3DSDM/Systems \
    $$PWD/../Runtime/Source/Qt3DSFoundation/Include \
    ../3rdparty/EASTL/UnknownVersion/include \
    ../3rdparty/color

PRECOMPILED_HEADER += Common/Code/_Win32/stdafx.h

include(CommonLib.pri)
