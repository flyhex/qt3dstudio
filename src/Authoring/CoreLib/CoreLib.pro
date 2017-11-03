TEMPLATE = lib
TARGET = CoreLib
CONFIG += staticlib nostrictstrings
include(../commoninclude.pri)
include($$OUT_PWD/../qtAuthoring-config.pri)
INCLUDEPATH += $$OUT_PWD/..

DEFINES += _UNICODE QT3DS_AUTHORING _AFXDLL \
    PCRE_STATIC DRIVE_DESIGN_STUDIO \
    DISABLE_MESH_OPTIMIZATION DOM_INCLUDE_TINYXML NO_ZAE COLLADA_DOM_SUPPORT141 NO_BOOST \
    _AMD64_

QT += widgets

macos:DEFINES += WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T QT3DS_LITTLE_ENDIAN

linux: DEFINES += WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T

INCLUDEPATH += \
    ../Client/Code/Core \
    ../Client/Code/Core/Utility \
    ../Client/Code/Core/Types \
    ../Client/Code/Core/Commands \
    ../Client/Code/Core/Core \
    ../Client/Code/Core/Doc \
    ../Client/Code/Core/Doc/ClientDataModelBridge \
    ../Client/Code/Shared \
    ../Client/Code/Shared/Log \
    ../Client/Code/Core/Timer \
    ../Client/Code/Core/VirtualAssets \
    ../QT3DSIMP/Qt3DSImportLib \
    ../QT3DSDM/Systems \
    ../QT3DSDM/Systems/Cores \
    ../QT3DSIMP/Qt3DSImportSGTranslation \
    ../Studio/DragAndDrop \
    ../Studio/Render \
    ../Studio/Workspace \
    ../Studio/_Win/DragNDrop \
    ../Studio/Utils \
    ../Build \
    ../Common/Code/Thread \
    ../Common/Code/IO \
    ../Common/Code \
    ../Common/Code/Exceptions \
    ../Common/Code/_Win32/Include \
    ../Common/Code/Graph \
    ../Common/Code/EulerAngles \
    ../Common/Code/Serialize \
    ../../Runtime/Source/DataModel/Include \
    ../../Runtime/Source/Qt3DSRender/Include \
    ../../Runtime/Source/Qt3DSFoundation/Include \
    ../../Runtime/Source/Qt3DSRuntimeRender/Include \
    ../../Runtime/Source/Qt3DSRuntimeRender/GraphObjects \
    ../../Runtime/Source/Qt3DSRuntimeRender/ResourceManager \
    ../../Runtime/Source/Qt3DSStateApplication/Application \
    ../../3rdparty/Lua/UnknownVersion/src \
    ../../3rdparty/EASTL/UnknownVersion/include \
    $$QMAKE_INCDIR_FBX \
    ../../3rdparty/ColladaDOM/1.4.0/dom/include \
    ../../3rdparty/ColladaDOM/1.4.0/dom/include/1.4 \
    ../../3rdparty/color \
    ..

PRECOMPILED_HEADER = ../Client/Code/Core/StdAfx.h

SOURCES += \
    ../Client/Code/Core/StdAfx.cpp \
    ../Client/Code/Core/Q3DStudioNVFoundation.cpp \
    ../Client/Code/Core/Types/BoundingBox.cpp \
    ../Client/Code/Core/Types/CachedMatrix.cpp \
    ../Client/Code/Core/Types/Frustum.cpp \
    ../Client/Code/Core/Types/Matrix.cpp \
    ../Client/Code/Core/Types/Pixel.cpp \
    ../Client/Code/Core/Types/Plane.cpp \
    ../Client/Code/Core/Types/Rotation3.cpp \
    ../Client/Code/Core/Types/Qt3DSColor.cpp \
    ../Client/Code/Core/Types/Vector2.cpp \
    ../Client/Code/Core/Types/Vector3.cpp \
    ../Client/Code/Core/Utility/BuildConfigParser.cpp \
    ../Client/Code/Core/Utility/ColorConversion.cpp \
    ../Client/Code/Core/Utility/CoreUtils.cpp \
    ../Client/Code/Core/Utility/cpuid.cpp \
    ../Client/Code/Core/Utility/DataModelObjectReferenceHelper.cpp \
    ../Client/Code/Core/Utility/HotKeys.cpp \
    ../Client/Code/Core/Utility/OptimizedArithmetic.cpp \
    ../Client/Code/Core/Utility/PathConstructionHelper.cpp \
    ../Client/Code/Core/Utility/StudioClipboard.cpp \
    ../Client/Code/Core/Utility/StudioObjectTypes.cpp \
    ../Client/Code/Core/Utility/StudioPreferences.cpp \
    ../Client/Code/Core/Utility/StudioProjectVariables.cpp \
    ../Client/Code/Core/Utility/TestCmdUtils.cpp \
    ../Client/Code/Core/Commands/Cmd.cpp \
    ../Client/Code/Core/Commands/CmdActivateSlide.cpp \
    ../Client/Code/Core/Commands/CmdBatch.cpp \
    ../Client/Code/Core/Commands/CmdDataModel.cpp \
    ../Client/Code/Core/Commands/CmdLocateReference.cpp \
    ../Client/Code/Core/Commands/CmdStack.cpp \
    ../Client/Code/Core/Core/Core.cpp \
    ../Client/Code/Core/Core/Dispatch.cpp \
    ../Client/Code/Core/Doc/ComposerEditorInterface.cpp \
    ../Client/Code/Core/Doc/Doc.cpp \
    ../Client/Code/Core/Doc/DocumentBufferCache.cpp \
    ../Client/Code/Core/Doc/DocumentEditor.cpp \
    ../Client/Code/Core/Doc/DynamicLua.cpp \
    ../Client/Code/Core/Doc/GraphUtils.cpp \
    ../Client/Code/Core/Doc/IComposerSerializer.cpp \
    ../Client/Code/Core/Doc/PathImportTranslator.cpp \
    ../Client/Code/Core/Doc/RelativePathTools.cpp \
    ../Client/Code/Core/Doc/StudioProjectSettings.cpp \
    ../Client/Code/Core/Doc/Qt3DSDMStudioSystem.cpp \
    ../Client/Code/Core/Doc/ClientDataModelBridge/ClientDataModelBridge.cpp \
    ../Client/Code/Core/Timer/Timer.cpp \
    ../Client/Code/Core/VirtualAssets/PlaybackClock.cpp \
    ../Client/Code/Core/VirtualAssets/VClockPolicy.cpp \
    ../QT3DSIMP/Qt3DSImportLib/Qt3DSImport.cpp \
    ../QT3DSIMP/Qt3DSImportLib/Qt3DSImportComposerTypes.cpp \
    ../QT3DSIMP/Qt3DSImportLib/Qt3DSImportLibPrecompile.cpp \
    ../QT3DSIMP/Qt3DSImportLib/Qt3DSImportMesh.cpp \
    ../QT3DSIMP/Qt3DSImportLib/Qt3DSImportMeshBuilder.cpp \
    ../QT3DSIMP/Qt3DSImportLib/Qt3DSImportMeshStudioOnly.cpp \
    ../QT3DSIMP/Qt3DSImportLib/Qt3DSImportPath.cpp \
    ../QT3DSIMP/Qt3DSImportLib/Qt3DSImportPerformImport.cpp \
    ../QT3DSIMP/Qt3DSImportSGTranslation/Qt3DSImportColladaSGTranslation.cpp \
    ../QT3DSIMP/Qt3DSImportSGTranslation/Qt3DSImportFbxSGTranslation.cpp \
    ../QT3DSIMP/Qt3DSImportSGTranslation/Qt3DSImportSceneGraphTranslation.cpp \
    ../Client/Code/Core/Utility/q3dsdirsystem.cpp \
    ../Client/Code/Core/Utility/q3dsdirwatcher.cpp

HEADERS += \
    ../Client/Code/Core/Utility/q3dsdirsystem.h \
    ../Client/Code/Core/Utility/q3dsdirwatcher.h
