TEMPLATE = lib
TARGET = CoreLib
CONFIG += staticlib nostrictstrings
include($$PWD/commoninclude.pri)
include($$OUT_PWD/qtAuthoring-config.pri)
DEFINES += _UNICODE UIC_AUTHORING _AFXDLL \
    PCRE_STATIC BOOST_SIGNALS_NO_DEPRECATION_WARNING DRIVE_DESIGN_STUDIO \
    DISABLE_MESH_OPTIMIZATION DOM_INCLUDE_TINYXML NO_ZAE COLLADA_DOM_SUPPORT141 BOOST_ALL_NO_LIB \
    _AMD64_

QT += widgets

macos:DEFINES += WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T UIC_LITTLE_ENDIAN

linux: DEFINES += WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T

INCLUDEPATH += \
    Client/Code/Core \
    Client/Code/Core/Utility \
    Client/Code/Core/Types \
    Client/Code/Core/Commands \
    Client/Code/Core/Core \
    Client/Code/Core/Doc \
    Client/Code/Core/Doc/ClientDataModelBridge \
    Client/Code/Shared \
    Client/Code/Shared/Log \
    Client/Code/Core/Timer \
    Client/Code/Core/VirtualAssets \
    UICIMP/UICImportLib \
    UICDM/Systems \
    UICDM/Systems/Cores \
    UICIMP/UICImportSGTranslation \
    Studio/DragAndDrop \
    Studio/Render \
    Studio/Workspace \
    Studio/_Win/DragNDrop \
    Studio/Utils \
    Build \
    Common/Code/Thread \
    Common/Code/IO \
    Common/Code \
    Common/Code/Exceptions \
    Common/Code/_Win32/Include \
    Common/Code/Graph \
    Common/Code/EulerAngles \
    Common/Code/Serialize \
    $$PWD/../Runtime/Source/RuntimeUICDM/Include \
    $$PWD/../Runtime/Source/Qt3DSRender/Include \
    $$PWD/../Runtime/Source/Qt3DSFoundation/Include \
    $$PWD/../Runtime/Source/UICRender/Include \
    $$PWD/../Runtime/Source/UICRender/GraphObjects \
    $$PWD/../Runtime/Source/UICRender/ResourceManager \
    $$PWD/../Runtime/Source/UICState/Application \
    ../3rdparty/Lua/UnknownVersion/src \
    ../3rdparty/EASTL/UnknownVersion/include \
    $$QMAKE_INCDIR_FBX \
    ../3rdparty/ColladaDOM/1.4.0/dom/include \
    ../3rdparty/ColladaDOM/1.4.0/dom/include/1.4 \
    ../3rdparty/color

PRECOMPILED_HEADER = Client/Code/Core/StdAfx.h

include(CoreLib.pri)
