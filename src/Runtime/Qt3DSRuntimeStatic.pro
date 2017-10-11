TEMPLATE = lib
TARGET = qt3dsruntimestatic
CONFIG += staticlib
include(commoninclude.pri)

!boot2qt:!integrity {
    RESOURCES += res.qrc
}

linux {
    DEFINES += _POSIX_C_SOURCE=199309L
}

QT += qml

# Foundation
SOURCES += \
    Source/Qt3DSFoundation/Source/foundation/ConvertUTF.cpp \
    Source/Qt3DSFoundation/Source/foundation/EASTL_new.cpp \
    Source/Qt3DSFoundation/Source/foundation/FileTools.cpp \
    Source/Qt3DSFoundation/Source/foundation/IOStreams.cpp \
    Source/Qt3DSFoundation/Source/foundation/Qt3DSLogging.cpp \
    Source/Qt3DSFoundation/Source/foundation/Qt3DSFoundation.cpp \
    Source/Qt3DSFoundation/Source/foundation/Qt3DSMathUtils.cpp \
    Source/Qt3DSFoundation/Source/foundation/Qt3DSPerfTimer.cpp \
    Source/Qt3DSFoundation/Source/foundation/Qt3DSSystem.cpp \
    Source/Qt3DSFoundation/Source/foundation/Socket.cpp \
    Source/Qt3DSFoundation/Source/foundation/StringTable.cpp \
    Source/Qt3DSFoundation/Source/foundation/XML.cpp \
    Source/UICRender/Source/q3dsqmlrender.cpp

HEADERS += \
    Source/Qt3DSFoundation/Include/foundation/ConvertUTF.h \
    Source/Qt3DSFoundation/Include/foundation/FileTools.h \
    Source/Qt3DSFoundation/Include/foundation/IOStreams.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSLogging.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSFoundation.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSMathUtils.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSPerfTimer.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSSystem.h \
    Source/Qt3DSFoundation/Include/foundation/Socket.h \
    Source/Qt3DSFoundation/Include/foundation/StringTable.h \
    Source/Qt3DSFoundation/Include/foundation/XML.h \
    Source/Qt3DSFoundation/Include/foundation/AutoDeallocatorAllocator.h \
    Source/Qt3DSFoundation/Include/foundation/FastAllocator.h \
    Source/Qt3DSFoundation/Include/foundation/PoolingAllocator.h \
    Source/Qt3DSFoundation/Include/foundation/PreAllocatedAllocator.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DS.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSAllocator.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSAllocatorCallback.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSAssert.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSAtomic.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSBasicTemplates.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSBounds3.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSBroadcastingAllocator.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSContainers.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSDataRef.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSDiscriminatedUnion.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSFastIPC.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSFlags.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSFPU.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSIndexableLinkedList.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSIntrinsics.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSInvasiveLinkedList.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSInvasiveSet.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSIPC.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSMat33.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSMat44.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSMath.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSMemoryBuffer.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSMutex.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSNoCopy.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSOption.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSPlane.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSPool.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSPreprocessor.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSQuat.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSRefCounted.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSSemaphore.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSSimpleTypes.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSStringTokenizer.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSSync.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSThread.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSTime.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSTransform.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSUnionCast.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSUtilities.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSVec2.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSVec3.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSVec4.h \
    Source/Qt3DSFoundation/Include/foundation/Qt3DSVersionNumber.h \
    Source/Qt3DSFoundation/Include/foundation/SerializationTypes.h \
    Source/Qt3DSFoundation/Include/foundation/StrConvertUTF.h \
    Source/Qt3DSFoundation/Include/foundation/StringConversion.h \
    Source/Qt3DSFoundation/Include/foundation/StringConversionImpl.h \
    Source/Qt3DSFoundation/Include/foundation/TaggedPointer.h \
    Source/Qt3DSFoundation/Include/foundation/ThreadSafeQueue.h \
    Source/Qt3DSFoundation/Include/foundation/TrackingAllocator.h \
    Source/Qt3DSFoundation/Include/foundation/Utils.h \
    Source/UICRender/Include/q3dsqmlrender.h

win32 {
SOURCES += \
    Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsAtomic.cpp \
    Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsFPU.cpp \
    Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsMutex.cpp \
    Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsSemaphore.cpp \
    Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsSync.cpp \
    Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsThread.cpp \
    Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsTime.cpp

HEADERS += \
    Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsAoS.h \
    Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsFile.h \
    Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsInclude.h \
    Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsInlineAoS.h \
    Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsIntrinsics.h \
    Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsString.h \
    Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsTrigConstants.h
}

macos: SOURCES += \
    Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixAtomic.cpp \
    Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixFPU.cpp \
    Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixMutex.cpp \
    Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixSemaphore.cpp \
    Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixSync.cpp \
    Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxThread.cpp \
    Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixTime.cpp

linux|integrity {
SOURCES += \
    Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxAtomic.cpp \
    Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxFPU.cpp \
    Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxMutex.cpp \
    Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxSemaphore.cpp \
    Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxSync.cpp \
    Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxThread.cpp \
    Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxTime.cpp

HEADERS += \
    Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxAoS.h \
    Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxFile.h \
    Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxInlineAoS.h \
    Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxIntrinsics.h \
    Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxString.h \
    Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxTrigConstants.h
}

# Libs
SOURCES += \
    ../3rdparty/RuntimePlatformSpecific/$$PlatformSpecificDir/Qt3DSLibs/KD/MinKD.c \
    Source/PlatformSpecific/$$PlatformSpecificDir/Qt3DSLibs/nv_math/nv_math.cpp \
    Source/PlatformSpecific/$$PlatformSpecificDir/Qt3DSLibs/nv_math/nv_matrix.cpp \
    Source/PlatformSpecific/$$PlatformSpecificDir/Qt3DSLibs/nv_math/nv_quat.cpp

# RenderBase
SOURCES += \
    Source/Qt3DSRender/Source/Qt3DSRenderAtomicCounterBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderAttribLayout.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderComputeShader.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderConstantBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderContext.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderDataBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderDepthStencilState.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderDrawIndirectBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderFragmentShader.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderFrameBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderGeometryShader.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderImageTexture.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderIndexBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderInputAssembler.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderOcclusionQuery.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderPathFontSpecification.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderPathFontText.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderPathRender.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderPathSpecification.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderProgramPipeline.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderQueryBase.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderRasterizerState.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderRenderBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderSampler.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderShaderProgram.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderStorageBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderSync.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderTessellationShader.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderTexture2D.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderTexture2DArray.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderTextureBase.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderTextureCube.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderTimerQuery.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderVertexBuffer.cpp \
    Source/Qt3DSRender/Source/Qt3DSRenderVertexShader.cpp

HEADERS += \
    Source/Qt3DSRender/Include/render/Qt3DSRenderAtomicCounterBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderAttribLayout.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderBaseTypes.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderComputeShader.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderConstantBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderContext.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderDataBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderDepthStencilState.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderDrawable.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderDrawIndirectBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderFragmentShader.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderFrameBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderGeometryShader.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderImageTexture.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderIndexBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderInputAssembler.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderOcclusionQuery.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderPathFontSpecification.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderPathFontText.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderPathRender.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderPathSpecification.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderProgramPipeline.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderQueryBase.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderRasterizerState.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderRenderBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderSampler.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderShader.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderShaderConstant.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderShaderProgram.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderStorageBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderSync.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderTessellationShader.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderTexture2D.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderTexture2DArray.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderTextureBase.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderTextureCube.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderTimerQuery.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderVertexBuffer.h \
    Source/Qt3DSRender/Include/render/Qt3DSRenderVertexShader.h \
    Source/Qt3DSRender/Include/render/glg/Qt3DSGLImplObjects.h

# Render
SOURCES += \
    Source/Qt3DSRender/Source/backends/gl/Qt3DSOpenGLExtensions.cpp \
    Source/Qt3DSRender/Source/backends/gl/Qt3DSRenderBackendGL3.cpp \
    Source/Qt3DSRender/Source/backends/gl/Qt3DSRenderBackendGL4.cpp \
    Source/Qt3DSRender/Source/backends/gl/Qt3DSRenderBackendGLBase.cpp \
    Source/Qt3DSRender/Source/backends/gl/Qt3DSRenderContextGL.cpp \
    Source/Qt3DSRender/Source/backends/software/Qt3DSRenderBackendNULL.cpp \
    Source/Qt3DSRender/Source/backends/gl/Q3DSRenderBackendGLES2.cpp

HEADERS += \
    Source/Qt3DSRender/Include/render/backends/Qt3DSRenderBackend.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSOpenGLPrefix.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSOpenGLUtil.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSOpenGLExtensions.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendGL3.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendGL4.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendGLBase.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendInputAssemblerGL.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendRenderStatesGL.h \
    Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendShaderProgramGL.h \
    Source/Qt3DSRender/Include/render/backends/software/Qt3DSRenderBackendNULL.h \
    Source/Qt3DSRender/Include/render/backends/gl/Q3DSRenderBackendGLES2.h

# UICDM
SOURCES += \
    Source/RuntimeUICDM/Source/UICMetadata.cpp \
    $$PWD/../Authoring/UICIMP/UICImportLib/UICImportMesh.cpp \
    $$PWD/../Authoring/UICIMP/UICImportLib/UICImportPath.cpp \
    $$PWD/../Authoring/UICDM/Systems/UICDMMetaData.cpp \
    $$PWD/../Authoring/UICDM/Systems/UICDMXML.cpp \
    $$PWD/../Authoring/UICDM/Systems/UICDMStringTable.cpp \
    $$PWD/../Authoring/UICDM/Systems/UICDMHandles.cpp \
    $$PWD/../Authoring/UICDM/Systems/UICDMComposerTypeDefinitions.cpp \
    $$PWD/../Authoring/UICDM/Systems/UICDMValue.cpp \
    $$PWD/../Authoring/UICDM/Systems/Cores/SimpleDataCore.cpp

HEADERS += \
    Source/RuntimeUICDM/Include/UICMetadata.h \
    Source/RuntimeUICDM/Include/DocumentResourceManagerScriptParser.h \
    $$PWD/../Authoring/UICIMP/UICImportLib/UICImportMesh.h \
    $$PWD/../Authoring/UICIMP/UICImportLib/UICImportPath.h \
    $$PWD/../Authoring/UICDM/Systems/UICDMMetaData.h \
    $$PWD/../Authoring/UICDM/Systems/UICDMXML.h \
    $$PWD/../Authoring/UICDM/Systems/UICDMStringTable.h \
    $$PWD/../Authoring/UICDM/Systems/UICDMHandles.h \
    $$PWD/../Authoring/UICDM/Systems/UICDMComposerTypeDefinitions.h \
    $$PWD/../Authoring/UICDM/Systems/UICDMValue.h \
    $$PWD/../Authoring/UICDM/Systems/Cores/SimpleDataCore.h

# Engine
HEADERS += \
    Source/Engine/Include/EnginePrefix.h \
    Source/Engine/Include/UICTegraApplication.h \
    Source/Engine/Include/UICTegraInputEngine.h \
    Source/Engine/Include/UICRenderRuntimeBinding.h \
    Source/Engine/Include/UICRenderRuntimeBindingImpl.h

SOURCES += \
    Source/Engine/Source/EnginePrefix.cpp \
    Source/Engine/Source/UICOSSpecificFunctions.cpp \
    Source/Engine/Source/UICRenderRuntimeBinding.cpp \
    Source/Engine/Source/UICRenderRuntimeBindingImplRenderer.cpp \
    Source/Engine/Source/UICRenderRuntimeBindingImplTranslation.cpp \
    Source/Engine/Source/UICTegraApplication.cpp \
    Source/Engine/Source/UICTegraInputEngine.cpp

# Event
SOURCES += \
    Source/UICEvent/Source/EventFactory.cpp \
    Source/UICEvent/Source/EventPoller.cpp \
    Source/UICEvent/Source/EventPollingSystemLuaBinding.cpp \
    Source/UICEvent/Source/EventSystemC.cpp

HEADERS += \
    Source/UICEvent/InternalInclude/EventPollingSystem.h \
    Source/UICEvent/InternalInclude/EventPollingSystemLuaBinding.h \
    Source/UICEvent/Include/EventSystem.h \
    Source/UICEvent/Include/EventSystemC.h

# Render
SOURCES += \
    Source/UICRender/Source/UICOffscreenRenderManager.cpp \
    Source/UICRender/Source/UICOldNBustedRenderPlugin.cpp \
    Source/UICRender/Source/UICOnscreenTextRenderer.cpp \
    Source/UICRender/Source/UICRenderClippingFrustum.cpp \
    Source/UICRender/Source/UICRenderContext.cpp \
    Source/UICRender/Source/UICRenderCustomMaterialShaderGenerator.cpp \
    Source/UICRender/Source/UICRenderCustomMaterialSystem.cpp \
    Source/UICRender/Source/UICRenderDefaultMaterialShaderGenerator.cpp \
    Source/UICRender/Source/UICRenderDynamicObjectSystem.cpp \
    Source/UICRender/Source/UICRenderEffectSystem.cpp \
    Source/UICRender/Source/UICRendererUtil.cpp \
    Source/UICRender/Source/UICRenderEulerAngles.cpp \
    Source/UICRender/Source/UICRenderGpuProfiler.cpp \
    Source/UICRender/Source/UICRenderGraphObjectSerializer.cpp \
    Source/UICRender/Source/UICRenderImageScaler.cpp \
    Source/UICRender/Source/UICRenderInputStreamFactory.cpp \
    Source/UICRender/Source/UICRenderPathManager.cpp \
    Source/UICRender/Source/UICRenderPixelGraphicsRenderer.cpp \
    Source/UICRender/Source/UICRenderPixelGraphicsTypes.cpp \
    Source/UICRender/Source/UICRenderPlugin.cpp \
    Source/UICRender/Source/UICRenderRay.cpp \
    Source/UICRender/Source/UICRenderRenderList.cpp \
    Source/UICRender/Source/UICRenderShaderCache.cpp \
    Source/UICRender/Source/UICRenderShaderCodeGenerator.cpp \
    Source/UICRender/Source/UICRenderShaderCodeGeneratorV2.cpp \
    Source/UICRender/Source/UICRenderShadowMap.cpp \
    Source/UICRender/Source/UICRenderSubpresentation.cpp \
    Source/UICRender/Source/UICRenderTextTextureAtlas.cpp \
    Source/UICRender/Source/UICRenderTextTextureCache.cpp \
    Source/UICRender/Source/UICRenderTextureAtlas.cpp \
    Source/UICRender/Source/UICRenderThreadPool.cpp \
    Source/UICRender/Source/UICRenderUIPLoader.cpp \
    Source/UICRender/Source/UICRenderUIPSharedTranslation.cpp \
    Source/UICRender/Source/UICRenderWidgets.cpp \
    Source/UICRender/Source/UICTextRenderer.cpp \
    Source/UICRender/Source/UICQtTextRenderer.cpp \
    Source/UICRender/GraphObjects/UICRenderCamera.cpp \
    Source/UICRender/GraphObjects/UICRenderDefaultMaterial.cpp \
    Source/UICRender/GraphObjects/UICRenderDynamicObject.cpp \
    Source/UICRender/GraphObjects/UICRenderEffect.cpp \
    Source/UICRender/GraphObjects/UICRenderImage.cpp \
    Source/UICRender/GraphObjects/UICRenderLayer.cpp \
    Source/UICRender/GraphObjects/UICRenderLight.cpp \
    Source/UICRender/GraphObjects/UICRenderLightmaps.cpp \
    Source/UICRender/GraphObjects/UICRenderModel.cpp \
    Source/UICRender/GraphObjects/UICRenderNode.cpp \
    Source/UICRender/GraphObjects/UICRenderPath.cpp \
    Source/UICRender/GraphObjects/UICRenderPresentation.cpp \
    Source/UICRender/GraphObjects/UICRenderScene.cpp \
    Source/UICRender/GraphObjects/UICRenderText.cpp \
    Source/UICRender/RendererImpl/UICRenderableObjects.cpp \
    Source/UICRender/RendererImpl/UICRendererImpl.cpp \
    Source/UICRender/RendererImpl/UICRendererImplLayerRenderData.cpp \
    Source/UICRender/RendererImpl/UICRendererImplLayerRenderHelper.cpp \
    Source/UICRender/RendererImpl/UICRendererImplLayerRenderPreparationData.cpp \
    Source/UICRender/RendererImpl/UICRendererImplShaders.cpp \
    Source/UICRender/ResourceManager/UICRenderBufferLoader.cpp \
    Source/UICRender/ResourceManager/UICRenderBufferManager.cpp \
    Source/UICRender/ResourceManager/UICRenderImageBatchLoader.cpp \
    Source/UICRender/ResourceManager/UICRenderLoadedTexture.cpp \
    Source/UICRender/ResourceManager/UICRenderLoadedTextureBMP.cpp \
    Source/UICRender/ResourceManager/UICRenderLoadedTextureDDS.cpp \
    Source/UICRender/ResourceManager/UICRenderLoadedTextureGIF.cpp \
    Source/UICRender/ResourceManager/UICRenderLoadedTextureHDR.cpp \
    Source/UICRender/ResourceManager/UICRenderPrefilterTexture.cpp \
    Source/UICRender/ResourceManager/UICRenderResourceBufferObjects.cpp \
    Source/UICRender/ResourceManager/UICRenderResourceManager.cpp \
    Source/UICRender/ResourceManager/UICRenderResourceTexture2D.cpp

HEADERS += \
    Source/UICRender/Include/UICOffscreenRenderKey.h \
    Source/UICRender/Include/UICOffscreenRenderManager.h \
    Source/UICRender/Include/UICOldNBustedRenderPlugin.h \
    Source/UICRender/Include/UICRender.h \
    Source/UICRender/Include/UICRenderableImage.h \
    Source/UICRender/Include/UICRenderClippingFrustum.h \
    Source/UICRender/Include/UICRenderContext.h \
    Source/UICRender/Include/UICRenderCustomMaterialRenderContext.h \
    Source/UICRender/Include/UICRenderCustomMaterialShaderGenerator.h \
    Source/UICRender/Include/UICRenderCustomMaterialSystem.h \
    Source/UICRender/Include/UICRenderDefaultMaterialShaderGenerator.h \
    Source/UICRender/Include/UICRenderDynamicObjectSystem.h \
    Source/UICRender/Include/UICRenderDynamicObjectSystemCommands.h \
    Source/UICRender/Include/UICRenderDynamicObjectSystemUtil.h \
    Source/UICRender/Include/UICRenderEffectSystem.h \
    Source/UICRender/Include/UICRenderer.h \
    Source/UICRender/Include/UICRendererUtil.h \
    Source/UICRender/Include/UICRenderEulerAngles.h \
    Source/UICRender/Include/UICRenderGraphObjectPickQuery.h \
    Source/UICRender/Include/UICRenderGraphObjectSerializer.h \
    Source/UICRender/Include/UICRenderGraphObjectTypes.h \
    Source/UICRender/Include/UICRenderImageScaler.h \
    Source/UICRender/Include/UICRenderImageTextureData.h \
    Source/UICRender/Include/UICRenderInputStreamFactory.h \
    Source/UICRender/Include/UICRenderMaterialHelpers.h \
    Source/UICRender/Include/UICRenderMaterialShaderGenerator.h \
    Source/UICRender/Include/UICRenderMesh.h \
    Source/UICRender/Include/UICRenderPathManager.h \
    Source/UICRender/Include/UICRenderPathMath.h \
    Source/UICRender/Include/UICRenderPathRenderContext.h \
    Source/UICRender/Include/UICRenderPixelGraphicsRenderer.h \
    Source/UICRender/Include/UICRenderPixelGraphicsTypes.h \
    Source/UICRender/Include/UICRenderPlugin.h \
    Source/UICRender/Include/UICRenderPluginCInterface.h \
    Source/UICRender/Include/UICRenderPluginGraphObject.h \
    Source/UICRender/Include/UICRenderPluginPropertyValue.h \
    Source/UICRender/Include/UICRenderProfiler.h \
    Source/UICRender/Include/UICRenderRay.h \
    Source/UICRender/Include/UICRenderRenderList.h \
    Source/UICRender/Include/UICRenderRotationHelper.h \
    Source/UICRender/Include/UICRenderShaderCache.h \
    Source/UICRender/Include/UICRenderShaderCodeGenerator.h \
    Source/UICRender/Include/UICRenderShaderCodeGeneratorV2.h \
    Source/UICRender/Include/UICRenderShaderKeys.h \
    Source/UICRender/Include/UICRenderShadowMap.h \
    Source/UICRender/Include/UICRenderString.h \
    Source/UICRender/Include/UICRenderSubpresentation.h \
    Source/UICRender/Include/UICRenderSubPresentationHelper.h \
    Source/UICRender/Include/UICRenderTaggedPointer.h \
    Source/UICRender/Include/UICRenderTessModeValues.h \
    Source/UICRender/Include/UICRenderTextTextureAtlas.h \
    Source/UICRender/Include/UICRenderTextTextureCache.h \
    Source/UICRender/Include/UICRenderTextTypes.h \
    Source/UICRender/Include/UICRenderTextureAtlas.h \
    Source/UICRender/Include/UICRenderThreadPool.h \
    Source/UICRender/Include/UICRenderUIPLoader.h \
    Source/UICRender/Include/UICRenderUIPSharedTranslation.h \
    Source/UICRender/Include/UICRenderWidgets.h \
    Source/UICRender/Include/UICRenderWindowDimensions.h \
    Source/UICRender/Include/UICTextRenderer.h \
    Source/UICRender/GraphObjects/UICRenderCamera.h \
    Source/UICRender/GraphObjects/UICRenderCustomMaterial.h \
    Source/UICRender/GraphObjects/UICRenderDefaultMaterial.h \
    Source/UICRender/GraphObjects/UICRenderDynamicObject.h \
    Source/UICRender/GraphObjects/UICRenderEffect.h \
    Source/UICRender/GraphObjects/UICRenderGraphObject.h \
    Source/UICRender/GraphObjects/UICRenderImage.h \
    Source/UICRender/GraphObjects/UICRenderLayer.h \
    Source/UICRender/GraphObjects/UICRenderLight.h \
    Source/UICRender/GraphObjects/UICRenderLightmaps.h \
    Source/UICRender/GraphObjects/UICRenderMaterialDirty.h \
    Source/UICRender/GraphObjects/UICRenderModel.h \
    Source/UICRender/GraphObjects/UICRenderNode.h \
    Source/UICRender/GraphObjects/UICRenderPath.h \
    Source/UICRender/GraphObjects/UICRenderPathSubPath.h \
    Source/UICRender/GraphObjects/UICRenderPresentation.h \
    Source/UICRender/GraphObjects/UICRenderReferencedMaterial.h \
    Source/UICRender/GraphObjects/UICRenderScene.h \
    Source/UICRender/GraphObjects/UICRenderText.h \
    Source/UICRender/RendererImpl/UICRenderableObjects.h \
    Source/UICRender/RendererImpl/UICRendererImpl.h \
    Source/UICRender/RendererImpl/UICRendererImplLayerRenderData.h \
    Source/UICRender/RendererImpl/UICRendererImplLayerRenderHelper.h \
    Source/UICRender/RendererImpl/UICRendererImplLayerRenderPreparationData.h \
    Source/UICRender/RendererImpl/UICRendererImplShaders.h \
    Source/UICRender/RendererImpl/UICVertexPipelineImpl.h \
    Source/UICRender/ResourceManager/UICRenderBufferLoader.h \
    Source/UICRender/ResourceManager/UICRenderBufferManager.h \
    Source/UICRender/ResourceManager/UICRenderImageBatchLoader.h \
    Source/UICRender/ResourceManager/UICRenderLoadedTexture.h \
    Source/UICRender/ResourceManager/UICRenderLoadedTextureDDS.h \
    Source/UICRender/ResourceManager/UICRenderLoadedTextureFreeImageCompat.h \
    Source/UICRender/ResourceManager/UICRenderPrefilterTexture.h \
    Source/UICRender/ResourceManager/UICRenderResourceBufferObjects.h \
    Source/UICRender/ResourceManager/UICRenderResourceManager.h \
    Source/UICRender/ResourceManager/UICRenderResourceTexture2D.h \
    Source/UICRender/Include/ANDROID/DynamicLibLoader.h \
    Source/UICRender/Include/LINUX/DynamicLibLoader.h \
    Source/UICRender/Include/OSX/DynamicLibLoader.h \
    Source/UICRender/Include/QNX/DynamicLibLoader.h \
    Source/UICRender/Include/WINDOWS/DynamicLibLoader.h

# Runtime
HEADERS += \
    Source/Runtime/Include/RuntimePrefix.h \
    Source/Runtime/Include/UICActivationManager.h \
    Source/Runtime/Include/UICAnimationSystem.h \
    Source/Runtime/Include/UICApplication.h \
    Source/Runtime/Include/UICAttributeHashes.h \
    Source/Runtime/Include/UICBinarySerializerImpl.h \
    Source/Runtime/Include/UICComponentManager.h \
    Source/Runtime/Include/UICElementSystem.h \
    Source/Runtime/Include/UICEventCallbacks.h \
    Source/Runtime/Include/UICInputEngine.h \
    Source/Runtime/Include/UICLogicSystem.h \
    Source/Runtime/Include/UICLuaAxis.h \
    Source/Runtime/Include/UICLuaButton.h \
    Source/Runtime/Include/UICLuaColor.h \
    Source/Runtime/Include/UICLuaCommandHelper.h \
    Source/Runtime/Include/UICLuaElementHelper.h \
    Source/Runtime/Include/UICLuaEngine.h \
    Source/Runtime/Include/UICLuaEventHelper.h \
    Source/Runtime/Include/UICLuaHelper.h \
    Source/Runtime/Include/UICLuaKeyboard.h \
    Source/Runtime/Include/UICLuaMatrix.h \
    Source/Runtime/Include/UICLuaRotation.h \
    Source/Runtime/Include/UICLuaSceneHelper.h \
    Source/Runtime/Include/UICLuaVector.h \
    Source/Runtime/Include/UICOutputMemoryStream.h \
    Source/Runtime/Include/UICParametersSystem.h \
    Source/Runtime/Include/UICPresentation.h \
    Source/Runtime/Include/UICPresentationFrameData.h \
    Source/Runtime/Include/UICQmlEngine.h \
    Source/Runtime/Include/UICSlideSystem.h \
    Source/Runtime/Include/UICTimePolicy.h \
    Source/Runtime/Include/UICQmlElementHelper.h \
    Source/Runtime/Include/q3dsqmlscript.h

SOURCES += \
    Source/Runtime/Source/RuntimePrefix.cpp \
    Source/Runtime/Source/UICActivationManager.cpp \
    Source/Runtime/Source/UICAnimationSystem.cpp \
    Source/Runtime/Source/UICApplication.cpp \
    Source/Runtime/Source/UICAttributeHashes.cpp \
    Source/Runtime/Source/UICBinarySerializerImpl.cpp \
    Source/Runtime/Source/UICComponentManager.cpp \
    Source/Runtime/Source/UICElementSystem.cpp \
    Source/Runtime/Source/UICEventCallbacks.cpp \
    Source/Runtime/Source/UICInputEngine.cpp \
    Source/Runtime/Source/UICLogicSystem.cpp \
    Source/Runtime/Source/UICLuaAxis.cpp \
    Source/Runtime/Source/UICLuaButton.cpp \
    Source/Runtime/Source/UICLuaColor.cpp \
    Source/Runtime/Source/UICLuaCommandHelper.cpp \
    Source/Runtime/Source/UICLuaElementHelper.cpp \
    Source/Runtime/Source/UICLuaEngine.cpp \
    Source/Runtime/Source/UICLuaEventHelper.cpp \
    Source/Runtime/Source/UICLuaHelper.cpp \
    Source/Runtime/Source/UICLuaKeyboard.cpp \
    Source/Runtime/Source/UICLuaMatrix.cpp \
    Source/Runtime/Source/UICLuaRotation.cpp \
    Source/Runtime/Source/UICLuaSceneHelper.cpp \
    Source/Runtime/Source/UICLuaVector.cpp \
    Source/Runtime/Source/UICOutputMemoryStream.cpp \
    Source/Runtime/Source/UICParametersSystem.cpp \
    Source/Runtime/Source/UICPresentation.cpp \
    Source/Runtime/Source/UICPresentationFrameData.cpp \
    Source/Runtime/Source/UICQmlEngine.cpp \
    Source/Runtime/Source/UICSlideSystem.cpp \
    Source/Runtime/Source/UICTimePolicy.cpp \
    Source/Runtime/Source/UICQmlElementHelper.cpp \
    Source/Runtime/Source/q3dsqmlscript.cpp

# State
SOURCES += \
    Source/UICState/Source/UICStateContext.cpp \
    Source/UICState/Source/UICStateExecutionContext.cpp \
    Source/UICState/Source/UICStateInterpreter.cpp \
    Source/UICState/Source/UICStateLuaScriptContext.cpp \
    Source/UICState/Source/UICStateVisualBindingContext.cpp \
    Source/UICState/Source/UICStateXMLIO.cpp \
    Source/UICState/Debugger/UICLuaDatamodelCache.cpp \
    Source/UICState/Debugger/UICLuaSideDebugger.cpp \
    Source/UICState/Debugger/UICSceneGraphRuntimeDebugger.cpp \
    Source/UICState/Debugger/UICStateDataTest.cpp \
    Source/UICState/Debugger/UICStateDebuggedInterpreter.cpp \
    Source/UICState/Debugger/UICStateDebugger.cpp \
    Source/UICState/Debugger/UICStateDebuggerListener.cpp \
    Source/UICState/Debugger/UICStateDebugStreams.cpp \
    Source/UICState/Debugger/UICStateLuaTest.cpp \
    Source/UICState/Application/UICStateApplication.cpp

HEADERS += \
    Source/UICState/Include/UICState.h \
    Source/UICState/Include/UICStateContext.h \
    Source/UICState/Include/UICStateExecutionContext.h \
    Source/UICState/Include/UICStateExecutionTypes.h \
    Source/UICState/Include/UICStateIdValue.h \
    Source/UICState/Include/UICStateInterpreter.h \
    Source/UICState/Include/UICStateLuaScriptContext.h \
    Source/UICState/Include/UICStateScriptContext.h \
    Source/UICState/Include/UICStateSharedImpl.h \
    Source/UICState/Include/UICStateSignalConnection.h \
    Source/UICState/Include/UICStateTypes.h \
    Source/UICState/Include/UICStateVisualBindingContext.h \
    Source/UICState/Include/UICStateVisualBindingContextCommands.h \
    Source/UICState/Include/UICStateVisualBindingContextValues.h \
    Source/UICState/Include/UICStateXMLIO.h \
    Source/UICState/Application/UICStateApplication.h \
    Source/UICState/Debugger/UICLuaDebugger.h \
    Source/UICState/Debugger/UICLuaDebuggerImpl.h \
    Source/UICState/Debugger/UICLuaDebuggerProtocol.h \
    Source/UICState/Debugger/UICSceneGraphDebugger.h \
    Source/UICState/Debugger/UICSceneGraphDebuggerProtocol.h \
    Source/UICState/Debugger/UICSceneGraphDebuggerValue.h \
    Source/UICState/Debugger/UICStateDebugger.h \
    Source/UICState/Debugger/UICStateDebuggerProtocol.h \
    Source/UICState/Debugger/UICStateDebuggerValues.h \
    Source/UICState/Debugger/UICStateDebugStreams.h \
    Source/UICState/Debugger/UICStateTest.h \
    Source/UICState/Debugger/UICStateTestCommon.h

# System
SOURCES += \
    Source/System/Source/SystemPrefix.cpp \
    Source/System/Source/UICAssert.cpp \
    Source/System/Source/UICBoundingBox.cpp \
    Source/System/Source/UICColor.cpp \
    Source/System/Source/UICCubicRoots.cpp \
    Source/System/Source/UICDataLogger.cpp \
    Source/System/Source/UICDLLManager.cpp \
    Source/System/Source/UICEulerAngles.cpp \
    Source/System/Source/UICFile.cpp \
    Source/System/Source/UICFileStream.cpp \
    Source/System/Source/UICFunctionWrappers.cpp \
    Source/System/Source/UICMatrix.cpp \
    Source/System/Source/UICMemory.cpp \
    Source/System/Source/UICMemoryFilter.cpp \
    Source/System/Source/UICMemoryHeap.cpp \
    Source/System/Source/UICMemoryManager.cpp \
    Source/System/Source/UICMemoryPool.cpp \
    Source/System/Source/UICMemoryProbe.cpp \
    Source/System/Source/UICMemoryStatistics.cpp \
    Source/System/Source/UICMemoryTracker.cpp \
    Source/System/Source/UICSyncPrimitive.cpp \
    Source/System/Source/UICThreadManager.cpp \
    Source/System/Source/UICTimer.cpp \
    Source/System/Source/UICTypes.cpp \
    Source/System/Source/UICVector3.cpp

HEADERS += \
    Source/System/Include/SystemPrefix.h \
    Source/System/Include/UICArray.h \
    Source/System/Include/UICAssert.h \
    Source/System/Include/UICAudioPlayer.h \
    Source/System/Include/UICBasicPluginDLL.h \
    Source/System/Include/UICBezierEval.h \
    Source/System/Include/UICBoundingBox.h \
    Source/System/Include/UICCircularArray.h \
    Source/System/Include/UICColor.h \
    Source/System/Include/UICConfig.h \
    Source/System/Include/UICCubicRoots.h \
    Source/System/Include/UICCubicRootsImpl.h \
    Source/System/Include/UICDataLogger.h \
    Source/System/Include/UICDataLogger.hpp \
    Source/System/Include/UICDataLoggerEnums.h \
    Source/System/Include/UICDataLoggerViewer.h \
    Source/System/Include/UICDLLManager.h \
    Source/System/Include/UICEGLTimer.h \
    Source/System/Include/UICEndian.h \
    Source/System/Include/UICEulerAngles.h \
    Source/System/Include/UICFile.h \
    Source/System/Include/UICFileStream.h \
    Source/System/Include/UICFixedArray.h \
    Source/System/Include/UICFNDTimer.h \
    Source/System/Include/UICFunctionWrappers.h \
    Source/System/Include/UICHash.h \
    Source/System/Include/UICIFileStream.h \
    Source/System/Include/UICIStream.h \
    Source/System/Include/UICITimer.h \
    Source/System/Include/UICMacros.h \
    Source/System/Include/UICMatrix.h \
    Source/System/Include/UICMemory.h \
    Source/System/Include/UICMemoryFilter.h \
    Source/System/Include/UICMemoryHeap.h \
    Source/System/Include/UICMemoryManager.h \
    Source/System/Include/UICMemoryPool.h \
    Source/System/Include/UICMemoryProbe.h \
    Source/System/Include/UICMemorySettings.h \
    Source/System/Include/UICMemoryStatistics.h \
    Source/System/Include/UICMemoryTracker.h \
    Source/System/Include/UICPlatformSpecific.h \
    Source/System/Include/UICSyncPrimitive.h \
    Source/System/Include/UICThreadManager.h \
    Source/System/Include/UICThreadSafeQueue.h \
    Source/System/Include/UICThreadSafeScratchpad.h \
    Source/System/Include/UICTimer.h \
    Source/System/Include/UICTypes.h \
    Source/System/Include/UICVector3.h

# Parser
SOURCES += \
    Source/UIPParser/Source/UICUIPParserActionHelper.cpp \
    Source/UIPParser/Source/UICUIPParserImpl.cpp \
    Source/UIPParser/Source/UICUIPParserObjectRefHelper.cpp

HEADERS += \
    Source/UIPParser/Include/UICIPresentation.h \
    Source/UIPParser/Include/UICUIPParser.h \
    Source/UIPParser/Include/UICUIPParserActionHelper.h \
    Source/UIPParser/Include/UICUIPParserImpl.h \
    Source/UIPParser/Include/UICUIPParserObjectRefHelper.h
