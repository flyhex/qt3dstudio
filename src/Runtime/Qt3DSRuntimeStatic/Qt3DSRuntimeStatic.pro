TEMPLATE = lib
TARGET = qt3dsruntimestatic
CONFIG += staticlib
include(../commoninclude.pri)

!boot2qt:!integrity:!ios {
    RESOURCES += ../res.qrc
}

linux {
    DEFINES += _POSIX_C_SOURCE=199309L
    QMAKE_LFLAGS += -lrt
}

DEFINES += QT3DS_BUILDING_LIBRARY

QT += qml
QT += quick-private

# Foundation
SOURCES += \
    ../Source/foundation/ConvertUTF.cpp \
    ../Source/foundation/EASTL_new.cpp \
    ../Source/foundation/FileTools.cpp \
    ../Source/foundation/IOStreams.cpp \
    ../Source/foundation/Qt3DSLogging.cpp \
    ../Source/foundation/Qt3DSFoundation.cpp \
    ../Source/foundation/Qt3DSMathUtils.cpp \
    ../Source/foundation/Qt3DSPerfTimer.cpp \
    ../Source/foundation/Qt3DSSystem.cpp \
    ../Source/foundation/Socket.cpp \
    ../Source/foundation/StringTable.cpp \
    ../Source/foundation/XML.cpp \
    ../Source/foundation/TrackingAllocator.cpp \
    ../Source/runtimerender/q3dsqmlrender.cpp \
    ../Source/engine/Qt3DSRenderRuntimeBinding.cpp \
    ../Source/engine/Qt3DSRenderRuntimeBindingImplRenderer.cpp \
    ../Source/engine/Qt3DSRenderRuntimeBindingImplTranslation.cpp \
    ../Source/engine/Qt3DSTegraApplication.cpp \
    ../Source/engine/Qt3DSTegraInputEngine.cpp \
    ../Source/runtime/Qt3DSActivationManager.cpp \
    ../Source/runtime/Qt3DSAnimationSystem.cpp \
    ../Source/runtime/Qt3DSApplication.cpp \
    ../Source/runtime/Qt3DSAttributeHashes.cpp \
    ../Source/runtime/Qt3DSComponentManager.cpp \
    ../Source/runtime/Qt3DSElementSystem.cpp \
    ../Source/runtime/Qt3DSEventCallbacks.cpp \
    ../Source/runtime/Qt3DSInputEngine.cpp \
    ../Source/runtime/Qt3DSLogicSystem.cpp \
    ../Source/runtime/Qt3DSCommandHelper.cpp \
    ../Source/runtime/Qt3DSElementHelper.cpp \
    ../Source/runtime/Qt3DSOutputMemoryStream.cpp \
    ../Source/runtime/Qt3DSParametersSystem.cpp \
    ../Source/runtime/Qt3DSPresentation.cpp \
    ../Source/runtime/Qt3DSPresentationFrameData.cpp \
    ../Source/runtime/Qt3DSQmlElementHelper.cpp \
    ../Source/runtime/Qt3DSQmlEngine.cpp \
    ../Source/runtime/Qt3DSSlideSystem.cpp \
    ../Source/runtime/Qt3DSTimePolicy.cpp \
    ../Source/runtime/q3dsvariantconfig.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderCamera.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderDefaultMaterial.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderDynamicObject.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderEffect.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderImage.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderLayer.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderLight.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderLightmaps.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderModel.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderNode.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderPath.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderPresentation.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderScene.cpp \
    ../Source/runtimerender/graphobjects/Qt3DSRenderText.cpp \
    ../Source/runtimerender/rendererimpl/Qt3DSRenderableObjects.cpp \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImpl.cpp \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderData.cpp \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderHelper.cpp \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderPreparationData.cpp \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImplShaders.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderBufferLoader.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderBufferManager.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderImageBatchLoader.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTexture.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTextureBMP.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTextureDDS.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTextureGIF.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTextureHDR.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderPrefilterTexture.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderResourceBufferObjects.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderResourceManager.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderResourceTexture2D.cpp \
    ../Source/runtimerender/Qt3DSOffscreenRenderManager.cpp \
    ../Source/runtimerender/Qt3DSOldNBustedRenderPlugin.cpp \
    ../Source/runtimerender/Qt3DSOnscreenTextRenderer.cpp \
    ../Source/runtimerender/Qt3DSQtTextRenderer.cpp \
    ../Source/runtimerender/Qt3DSRenderClippingFrustum.cpp \
    ../Source/runtimerender/Qt3DSRenderCustomMaterialShaderGenerator.cpp \
    ../Source/runtimerender/Qt3DSRenderCustomMaterialSystem.cpp \
    ../Source/runtimerender/Qt3DSRenderDefaultMaterialShaderGenerator.cpp \
    ../Source/runtimerender/Qt3DSRenderDynamicObjectSystem.cpp \
    ../Source/runtimerender/Qt3DSRenderEffectSystem.cpp \
    ../Source/runtimerender/Qt3DSRendererUtil.cpp \
    ../Source/runtimerender/Qt3DSRenderEulerAngles.cpp \
    ../Source/runtimerender/Qt3DSRenderGpuProfiler.cpp \
    ../Source/runtimerender/Qt3DSRenderGraphObjectSerializer.cpp \
    ../Source/runtimerender/Qt3DSRenderImageScaler.cpp \
    ../Source/runtimerender/Qt3DSRenderInputStreamFactory.cpp \
    ../Source/runtimerender/Qt3DSRenderPathManager.cpp \
    ../Source/runtimerender/Qt3DSRenderPixelGraphicsRenderer.cpp \
    ../Source/runtimerender/Qt3DSRenderPixelGraphicsTypes.cpp \
    ../Source/runtimerender/Qt3DSRenderPlugin.cpp \
    ../Source/runtimerender/Qt3DSRenderRay.cpp \
    ../Source/runtimerender/Qt3DSRenderRenderList.cpp \
    ../Source/runtimerender/Qt3DSRenderShaderCache.cpp \
    ../Source/runtimerender/Qt3DSRenderShaderCodeGenerator.cpp \
    ../Source/runtimerender/Qt3DSRenderShaderCodeGeneratorV2.cpp \
    ../Source/runtimerender/Qt3DSRenderShadowMap.cpp \
    ../Source/runtimerender/Qt3DSRenderSubpresentation.cpp \
    ../Source/runtimerender/Qt3DSRenderTextTextureAtlas.cpp \
    ../Source/runtimerender/Qt3DSRenderTextTextureCache.cpp \
    ../Source/runtimerender/Qt3DSRenderTextureAtlas.cpp \
    ../Source/runtimerender/Qt3DSRenderThreadPool.cpp \
    ../Source/runtimerender/Qt3DSRenderUIPLoader.cpp \
    ../Source/runtimerender/Qt3DSRenderUIPSharedTranslation.cpp \
    ../Source/runtimerender/Qt3DSRenderWidgets.cpp \
    ../Source/runtimerender/Qt3DSTextRenderer.cpp \
    ../Source/system/Qt3DSAssert.cpp \
    ../Source/system/Qt3DSBoundingBox.cpp \
    ../Source/system/Qt3DSColor.cpp \
    ../Source/system/Qt3DSCubicRoots.cpp \
    ../Source/system/Qt3DSDataLogger.cpp \
    ../Source/system/Qt3DSDLLManager.cpp \
    ../Source/system/Qt3DSEulerAngles.cpp \
    ../Source/system/Qt3DSFile.cpp \
    ../Source/system/Qt3DSFileStream.cpp \
    ../Source/system/Qt3DSFunctionWrappers.cpp \
    ../Source/system/Qt3DSMatrix.cpp \
    ../Source/system/Qt3DSMemory.cpp \
    ../Source/system/Qt3DSMemoryFilter.cpp \
    ../Source/system/Qt3DSMemoryHeap.cpp \
    ../Source/system/Qt3DSMemoryManager.cpp \
    ../Source/system/Qt3DSMemoryPool.cpp \
    ../Source/system/Qt3DSMemoryProbe.cpp \
    ../Source/system/Qt3DSMemoryStatistics.cpp \
    ../Source/system/Qt3DSMemoryTracker.cpp \
    ../Source/system/Qt3DSTimer.cpp \
    ../Source/system/Qt3DSTypes.cpp \
    ../Source/system/Qt3DSVector3.cpp \
    ../Source/uipparser/Qt3DSUIPParserActionHelper.cpp \
    ../Source/uipparser/Qt3DSUIPParserImpl.cpp \
    ../Source/uipparser/Qt3DSUIPParserObjectRefHelper.cpp \
    ../Source/runtimerender/Qt3DSRenderContextCore.cpp \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTextureKTX.cpp \
    ../Source/runtimerender/Qt3DSDistanceFieldRenderer.cpp \
    ../Source/runtimerender/Qt3DSFontDatabase.cpp \
    ../Source/runtimerender/Qt3DSDistanceFieldGlyphCacheManager.cpp \
    ../Source/runtimerender/Qt3DSDistanceFieldGlyphCache.cpp

HEADERS += \
    ../Source/foundation/ConvertUTF.h \
    ../Source/foundation/FileTools.h \
    ../Source/foundation/IOStreams.h \
    ../Source/foundation/Qt3DSLogging.h \
    ../Source/foundation/Qt3DSFoundation.h \
    ../Source/foundation/Qt3DSMathUtils.h \
    ../Source/foundation/Qt3DSPerfTimer.h \
    ../Source/foundation/Qt3DSSystem.h \
    ../Source/foundation/Socket.h \
    ../Source/foundation/StringTable.h \
    ../Source/foundation/XML.h \
    ../Source/foundation/AutoDeallocatorAllocator.h \
    ../Source/foundation/FastAllocator.h \
    ../Source/foundation/PoolingAllocator.h \
    ../Source/foundation/PreAllocatedAllocator.h \
    ../Source/foundation/Qt3DS.h \
    ../Source/foundation/Qt3DSAllocator.h \
    ../Source/foundation/Qt3DSAllocatorCallback.h \
    ../Source/foundation/Qt3DSAssert.h \
    ../Source/foundation/Qt3DSAtomic.h \
    ../Source/foundation/Qt3DSBasicTemplates.h \
    ../Source/foundation/Qt3DSBounds3.h \
    ../Source/foundation/Qt3DSBroadcastingAllocator.h \
    ../Source/foundation/Qt3DSContainers.h \
    ../Source/foundation/Qt3DSDataRef.h \
    ../Source/foundation/Qt3DSDiscriminatedUnion.h \
    ../Source/foundation/Qt3DSFastIPC.h \
    ../Source/foundation/Qt3DSFlags.h \
    ../Source/foundation/Qt3DSFPU.h \
    ../Source/foundation/Qt3DSIndexableLinkedList.h \
    ../Source/foundation/Qt3DSIntrinsics.h \
    ../Source/foundation/Qt3DSInvasiveLinkedList.h \
    ../Source/foundation/Qt3DSInvasiveSet.h \
    ../Source/foundation/Qt3DSIPC.h \
    ../Source/foundation/Qt3DSMat33.h \
    ../Source/foundation/Qt3DSMat44.h \
    ../Source/foundation/Qt3DSMath.h \
    ../Source/foundation/Qt3DSMemoryBuffer.h \
    ../Source/foundation/Qt3DSMutex.h \
    ../Source/foundation/Qt3DSNoCopy.h \
    ../Source/foundation/Qt3DSOption.h \
    ../Source/foundation/Qt3DSPlane.h \
    ../Source/foundation/Qt3DSPool.h \
    ../Source/foundation/Qt3DSPreprocessor.h \
    ../Source/foundation/Qt3DSQuat.h \
    ../Source/foundation/Qt3DSRefCounted.h \
    ../Source/foundation/Qt3DSSemaphore.h \
    ../Source/foundation/Qt3DSSimpleTypes.h \
    ../Source/foundation/Qt3DSStringTokenizer.h \
    ../Source/foundation/Qt3DSSync.h \
    ../Source/foundation/Qt3DSThread.h \
    ../Source/foundation/Qt3DSTime.h \
    ../Source/foundation/Qt3DSTransform.h \
    ../Source/foundation/Qt3DSUnionCast.h \
    ../Source/foundation/Qt3DSUtilities.h \
    ../Source/foundation/Qt3DSVec2.h \
    ../Source/foundation/Qt3DSVec3.h \
    ../Source/foundation/Qt3DSVec4.h \
    ../Source/foundation/Qt3DSVersionNumber.h \
    ../Source/foundation/SerializationTypes.h \
    ../Source/foundation/StrConvertUTF.h \
    ../Source/foundation/StringConversion.h \
    ../Source/foundation/StringConversionImpl.h \
    ../Source/foundation/TaggedPointer.h \
    ../Source/foundation/ThreadSafeQueue.h \
    ../Source/foundation/TrackingAllocator.h \
    ../Source/foundation/Utils.h \
    ../Source/runtimerender/q3dsqmlrender.h \
    ../Source/engine/Qt3DSRenderRuntimeBinding.h \
    ../Source/engine/Qt3DSRenderRuntimeBindingImpl.h \
    ../Source/engine/Qt3DSTegraApplication.h \
    ../Source/engine/Qt3DSTegraInputEngine.h \
    ../Source/runtime/Qt3DSActivationManager.h \
    ../Source/runtime/Qt3DSAnimationSystem.h \
    ../Source/runtime/Qt3DSApplication.h \
    ../Source/runtime/Qt3DSAttributeHashes.h \
    ../Source/runtime/Qt3DSComponentManager.h \
    ../Source/runtime/Qt3DSElementSystem.h \
    ../Source/runtime/Qt3DSEventCallbacks.h \
    ../Source/runtime/Qt3DSInputEngine.h \
    ../Source/runtime/Qt3DSLogicSystem.h \
    ../Source/runtime/Qt3DSCommandHelper.h \
    ../Source/runtime/Qt3DSElementHelper.h \
    ../Source/runtime/Qt3DSOutputMemoryStream.h \
    ../Source/runtime/Qt3DSParametersSystem.h \
    ../Source/runtime/Qt3DSPresentation.h \
    ../Source/runtime/Qt3DSPresentationFrameData.h \
    ../Source/runtime/Qt3DSQmlElementHelper.h \
    ../Source/runtime/Qt3DSQmlEngine.h \
    ../Source/runtime/Qt3DSSlideSystem.h \
    ../Source/runtime/Qt3DSTimePolicy.h \
    ../Source/runtime/Qt3DSApplicationValues.h \
    ../Source/runtime/Qt3DSIComponentManager.h \
    ../Source/runtime/Qt3DSIInputSystem.h \
    ../Source/runtime/Qt3DSInputDefs.h \
    ../Source/runtime/Qt3DSInputEventTypes.h \
    ../Source/runtime/Qt3DSIStateful.h \
    ../Source/runtime/Qt3DSIText.h \
    ../Source/runtime/Qt3DSKernelTypes.h \
    ../Source/runtime/q3dsvariantconfig_p.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderCamera.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderCustomMaterial.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderDefaultMaterial.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderDynamicObject.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderEffect.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderGraphObject.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderImage.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderLayer.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderLight.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderLightmaps.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderMaterialDirty.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderModel.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderNode.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderPath.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderPathSubPath.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderPresentation.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderReferencedMaterial.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderScene.h \
    ../Source/runtimerender/graphobjects/Qt3DSRenderText.h \
    ../Source/runtimerender/Qt3DSOffscreenRenderKey.h \
    ../Source/runtimerender/Qt3DSOffscreenRenderManager.h \
    ../Source/runtimerender/Qt3DSOldNBustedRenderPlugin.h \
    ../Source/runtimerender/Qt3DSRender.h \
    ../Source/runtimerender/Qt3DSRenderableImage.h \
    ../Source/runtimerender/Qt3DSRenderClippingFrustum.h \
    ../Source/runtimerender/Qt3DSRenderCustomMaterialRenderContext.h \
    ../Source/runtimerender/Qt3DSRenderCustomMaterialShaderGenerator.h \
    ../Source/runtimerender/Qt3DSRenderCustomMaterialSystem.h \
    ../Source/runtimerender/Qt3DSRenderDefaultMaterialShaderGenerator.h \
    ../Source/runtimerender/Qt3DSRenderDynamicObjectSystem.h \
    ../Source/runtimerender/Qt3DSRenderDynamicObjectSystemCommands.h \
    ../Source/runtimerender/Qt3DSRenderDynamicObjectSystemUtil.h \
    ../Source/runtimerender/Qt3DSRenderEffectSystem.h \
    ../Source/runtimerender/Qt3DSRenderer.h \
    ../Source/runtimerender/Qt3DSRendererUtil.h \
    ../Source/runtimerender/Qt3DSRenderEulerAngles.h \
    ../Source/runtimerender/Qt3DSRenderGraphObjectPickQuery.h \
    ../Source/runtimerender/Qt3DSRenderGraphObjectSerializer.h \
    ../Source/runtimerender/Qt3DSRenderGraphObjectTypes.h \
    ../Source/runtimerender/Qt3DSRenderImageScaler.h \
    ../Source/runtimerender/Qt3DSRenderImageTextureData.h \
    ../Source/runtimerender/Qt3DSRenderInputStreamFactory.h \
    ../Source/runtimerender/Qt3DSRenderMaterialHelpers.h \
    ../Source/runtimerender/Qt3DSRenderMaterialShaderGenerator.h \
    ../Source/runtimerender/Qt3DSRenderMesh.h \
    ../Source/runtimerender/Qt3DSRenderPathManager.h \
    ../Source/runtimerender/Qt3DSRenderPathMath.h \
    ../Source/runtimerender/Qt3DSRenderPathRenderContext.h \
    ../Source/runtimerender/Qt3DSRenderPixelGraphicsRenderer.h \
    ../Source/runtimerender/Qt3DSRenderPixelGraphicsTypes.h \
    ../Source/runtimerender/Qt3DSRenderPlugin.h \
    ../Source/runtimerender/Qt3DSRenderPluginCInterface.h \
    ../Source/runtimerender/Qt3DSRenderPluginGraphObject.h \
    ../Source/runtimerender/Qt3DSRenderPluginPropertyValue.h \
    ../Source/runtimerender/Qt3DSRenderProfiler.h \
    ../Source/runtimerender/Qt3DSRenderRay.h \
    ../Source/runtimerender/Qt3DSRenderRenderList.h \
    ../Source/runtimerender/Qt3DSRenderRotationHelper.h \
    ../Source/runtimerender/Qt3DSRenderShaderCache.h \
    ../Source/runtimerender/Qt3DSRenderShaderCodeGenerator.h \
    ../Source/runtimerender/Qt3DSRenderShaderCodeGeneratorV2.h \
    ../Source/runtimerender/Qt3DSRenderShaderKeys.h \
    ../Source/runtimerender/Qt3DSRenderShadowMap.h \
    ../Source/runtimerender/Qt3DSRenderString.h \
    ../Source/runtimerender/Qt3DSRenderSubpresentation.h \
    ../Source/runtimerender/Qt3DSRenderSubPresentationHelper.h \
    ../Source/runtimerender/Qt3DSRenderTaggedPointer.h \
    ../Source/runtimerender/Qt3DSRenderTessModeValues.h \
    ../Source/runtimerender/Qt3DSRenderTextTextureAtlas.h \
    ../Source/runtimerender/Qt3DSRenderTextTextureCache.h \
    ../Source/runtimerender/Qt3DSRenderTextTypes.h \
    ../Source/runtimerender/Qt3DSRenderTextureAtlas.h \
    ../Source/runtimerender/Qt3DSRenderThreadPool.h \
    ../Source/runtimerender/Qt3DSRenderUIPLoader.h \
    ../Source/runtimerender/Qt3DSRenderUIPSharedTranslation.h \
    ../Source/runtimerender/Qt3DSRenderWidgets.h \
    ../Source/runtimerender/Qt3DSTextRenderer.h \
    ../Source/runtimerender/rendererimpl/Qt3DSRenderableObjects.h \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImpl.h \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderData.h \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderHelper.h \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImplLayerRenderPreparationData.h \
    ../Source/runtimerender/rendererimpl/Qt3DSRendererImplShaders.h \
    ../Source/runtimerender/rendererimpl/Qt3DSVertexPipelineImpl.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderBufferLoader.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderBufferManager.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderImageBatchLoader.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTexture.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTextureDDS.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTextureFreeImageCompat.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderPrefilterTexture.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderResourceBufferObjects.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderResourceManager.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderResourceTexture2D.h \
    ../Source/system/Qt3DSArray.h \
    ../Source/system/Qt3DSAssert.h \
    ../Source/system/Qt3DSAudioPlayer.h \
    ../Source/system/Qt3DSBasicPluginDLL.h \
    ../Source/system/Qt3DSBezierEval.h \
    ../Source/system/Qt3DSBoundingBox.h \
    ../Source/system/Qt3DSCircularArray.h \
    ../Source/system/Qt3DSColor.h \
    ../Source/system/Qt3DSConfig.h \
    ../Source/system/Qt3DSCubicRoots.h \
    ../Source/system/Qt3DSCubicRootsImpl.h \
    ../Source/system/Qt3DSDataLogger.h \
    ../Source/system/Qt3DSDataLogger.hpp \
    ../Source/system/Qt3DSDataLoggerEnums.h \
    ../Source/system/Qt3DSDataLoggerViewer.h \
    ../Source/system/Qt3DSDLLManager.h \
    ../Source/system/Qt3DSEGLTimer.h \
    ../Source/system/Qt3DSEndian.h \
    ../Source/system/Qt3DSEulerAngles.h \
    ../Source/system/Qt3DSFile.h \
    ../Source/system/Qt3DSFileStream.h \
    ../Source/system/Qt3DSFixedArray.h \
    ../Source/system/Qt3DSFNDTimer.h \
    ../Source/system/Qt3DSFunctionWrappers.h \
    ../Source/system/Qt3DSHash.h \
    ../Source/system/Qt3DSIFileStream.h \
    ../Source/system/Qt3DSIStream.h \
    ../Source/system/Qt3DSITimer.h \
    ../Source/system/Qt3DSMacros.h \
    ../Source/system/Qt3DSMatrix.h \
    ../Source/system/Qt3DSMemory.h \
    ../Source/system/Qt3DSMemoryFilter.h \
    ../Source/system/Qt3DSMemoryHeap.h \
    ../Source/system/Qt3DSMemoryManager.h \
    ../Source/system/Qt3DSMemoryPool.h \
    ../Source/system/Qt3DSMemoryProbe.h \
    ../Source/system/Qt3DSMemorySettings.h \
    ../Source/system/Qt3DSMemoryStatistics.h \
    ../Source/system/Qt3DSMemoryTracker.h \
    ../Source/system/Qt3DSPlatformSpecific.h \
    ../Source/system/Qt3DSTimer.h \
    ../Source/system/Qt3DSTypes.h \
    ../Source/system/Qt3DSVector3.h \
    ../Source/system/Qt3DSArray.inl \
    ../Source/system/Qt3DSCircularArray.inl \
    ../Source/system/Qt3DSFixedArray.inl \
    ../Source/state/Qt3DSState.h \
    ../Source/state/Qt3DSStateTypes.h \
    ../Source/state/Qt3DSStateScriptContext.h \
    ../Source/state/Qt3DSStateVisualBindingContextCommands.h \
    ../Source/uipparser/Qt3DSIPresentation.h \
    ../Source/uipparser/Qt3DSUIPParser.h \
    ../Source/uipparser/Qt3DSUIPParserActionHelper.h \
    ../Source/uipparser/Qt3DSUIPParserImpl.h \
    ../Source/uipparser/Qt3DSUIPParserObjectRefHelper.h \
    ../Source/runtime/Qt3DSCommandEventTypes.h \
    ../Source/runtime/Qt3DSEvent.h \
    ../Source/runtime/Qt3DSFrameworkTypes.h \
    ../Source/runtime/Qt3DSInputFrame.h \
    ../Source/runtime/Qt3DSIScene.h \
    ../Source/runtime/Qt3DSIScriptBridge.h \
    ../Source/runtime/Qt3DSPickFrame.h \
    ../Source/runtime/Qt3DSRuntimeFactory.h \
    ../Source/runtime/Qt3DSSceneManager.h \
    ../Source/engine/Qt3DSEGLInfo.h \
    ../Source/engine/Qt3DSEGLWindowSystem.h \
    ../Source/engine/Qt3DSPluginDLL.h \
    ../Source/engine/Qt3DSWindowSystem.h \
    ../Source/runtimerender/Qt3DSRenderContextCore.h \
    ../Source/runtimerender/Qt3DSRenderLightConstantProperties.h \
    ../Source/runtimerender/resourcemanager/Qt3DSRenderLoadedTextureKTX.h \
    ../Source/runtimerender/Qt3DSDistanceFieldRenderer.h \
    ../Source/runtimerender/Qt3DSFontDatabase_p.h \
    ../Source/runtimerender/Qt3DSDistanceFieldGlyphCacheManager_p.h \
    ../Source/runtimerender/Qt3DSDistanceFieldGlyphCache_p.h

win32 {
SOURCES += \
    ../Source/foundation/windows/Qt3DSWindowsAtomic.cpp \
    ../Source/foundation/windows/Qt3DSWindowsFPU.cpp \
    ../Source/foundation/windows/Qt3DSWindowsMutex.cpp \
    ../Source/foundation/windows/Qt3DSWindowsSemaphore.cpp \
    ../Source/foundation/windows/Qt3DSWindowsSync.cpp \
    ../Source/foundation/windows/Qt3DSWindowsThread.cpp \
    ../Source/foundation/windows/Qt3DSWindowsTime.cpp

HEADERS += \
    ../Source/foundation/windows/Qt3DSWindowsAoS.h \
    ../Source/foundation/windows/Qt3DSWindowsFile.h \
    ../Source/foundation/windows/Qt3DSWindowsInclude.h \
    ../Source/foundation/windows/Qt3DSWindowsInlineAoS.h \
    ../Source/foundation/windows/Qt3DSWindowsIntrinsics.h \
    ../Source/foundation/windows/Qt3DSWindowsString.h \
    ../Source/foundation/windows/Qt3DSWindowsTrigConstants.h
}

macos: SOURCES += \
    ../Source/foundation/macos/Qt3DSUnixAtomic.cpp \
    ../Source/foundation/macos/Qt3DSUnixFPU.cpp \
    ../Source/foundation/macos/Qt3DSUnixMutex.cpp \
    ../Source/foundation/macos/Qt3DSUnixSemaphore.cpp \
    ../Source/foundation/macos/Qt3DSUnixSync.cpp \
    ../Source/foundation/linux/Qt3DSLinuxThread.cpp \
    ../Source/foundation/macos/Qt3DSUnixTime.cpp

linux|integrity|qnx {
SOURCES += \
    ../Source/foundation/linux/Qt3DSLinuxAtomic.cpp \
    ../Source/foundation/linux/Qt3DSLinuxFPU.cpp \
    ../Source/foundation/linux/Qt3DSLinuxMutex.cpp \
    ../Source/foundation/linux/Qt3DSLinuxSemaphore.cpp \
    ../Source/foundation/linux/Qt3DSLinuxSync.cpp \
    ../Source/foundation/linux/Qt3DSLinuxThread.cpp \
    ../Source/foundation/linux/Qt3DSLinuxTime.cpp

HEADERS += \
    ../Source/foundation/linux/Qt3DSLinuxAoS.h \
    ../Source/foundation/linux/Qt3DSLinuxFile.h \
    ../Source/foundation/linux/Qt3DSLinuxInlineAoS.h \
    ../Source/foundation/linux/Qt3DSLinuxIntrinsics.h \
    ../Source/foundation/linux/Qt3DSLinuxString.h \
    ../Source/foundation/linux/Qt3DSLinuxTrigConstants.h
}

# Libs
SOURCES += \
    ../Source/platformspecific/$$PlatformSpecificDir/libs/nv_math/nv_math.cpp \
    ../Source/platformspecific/$$PlatformSpecificDir/libs/nv_math/nv_matrix.cpp \
    ../Source/platformspecific/$$PlatformSpecificDir/libs/nv_math/nv_quat.cpp

# RenderBase
SOURCES += \
    ../Source/render/Qt3DSRenderAtomicCounterBuffer.cpp \
    ../Source/render/Qt3DSRenderAttribLayout.cpp \
    ../Source/render/Qt3DSRenderBaseTypes.cpp \
    ../Source/render/Qt3DSRenderComputeShader.cpp \
    ../Source/render/Qt3DSRenderConstantBuffer.cpp \
    ../Source/render/Qt3DSRenderContext.cpp \
    ../Source/render/Qt3DSRenderDataBuffer.cpp \
    ../Source/render/Qt3DSRenderDepthStencilState.cpp \
    ../Source/render/Qt3DSRenderDrawIndirectBuffer.cpp \
    ../Source/render/Qt3DSRenderFragmentShader.cpp \
    ../Source/render/Qt3DSRenderFrameBuffer.cpp \
    ../Source/render/Qt3DSRenderGeometryShader.cpp \
    ../Source/render/Qt3DSRenderImageTexture.cpp \
    ../Source/render/Qt3DSRenderIndexBuffer.cpp \
    ../Source/render/Qt3DSRenderInputAssembler.cpp \
    ../Source/render/Qt3DSRenderOcclusionQuery.cpp \
    ../Source/render/Qt3DSRenderPathFontSpecification.cpp \
    ../Source/render/Qt3DSRenderPathFontText.cpp \
    ../Source/render/Qt3DSRenderPathRender.cpp \
    ../Source/render/Qt3DSRenderPathSpecification.cpp \
    ../Source/render/Qt3DSRenderProgramPipeline.cpp \
    ../Source/render/Qt3DSRenderQueryBase.cpp \
    ../Source/render/Qt3DSRenderRasterizerState.cpp \
    ../Source/render/Qt3DSRenderRenderBuffer.cpp \
    ../Source/render/Qt3DSRenderSampler.cpp \
    ../Source/render/Qt3DSRenderShaderProgram.cpp \
    ../Source/render/Qt3DSRenderStorageBuffer.cpp \
    ../Source/render/Qt3DSRenderSync.cpp \
    ../Source/render/Qt3DSRenderTessellationShader.cpp \
    ../Source/render/Qt3DSRenderTexture2D.cpp \
    ../Source/render/Qt3DSRenderTexture2DArray.cpp \
    ../Source/render/Qt3DSRenderTextureBase.cpp \
    ../Source/render/Qt3DSRenderTextureCube.cpp \
    ../Source/render/Qt3DSRenderTimerQuery.cpp \
    ../Source/render/Qt3DSRenderVertexBuffer.cpp \
    ../Source/render/Qt3DSRenderVertexShader.cpp

HEADERS += \
    ../Source/render/Qt3DSRenderAtomicCounterBuffer.h \
    ../Source/render/Qt3DSRenderAttribLayout.h \
    ../Source/render/Qt3DSRenderBaseTypes.h \
    ../Source/render/Qt3DSRenderComputeShader.h \
    ../Source/render/Qt3DSRenderConstantBuffer.h \
    ../Source/render/Qt3DSRenderContext.h \
    ../Source/render/Qt3DSRenderDataBuffer.h \
    ../Source/render/Qt3DSRenderDepthStencilState.h \
    ../Source/render/Qt3DSRenderDrawable.h \
    ../Source/render/Qt3DSRenderDrawIndirectBuffer.h \
    ../Source/render/Qt3DSRenderFragmentShader.h \
    ../Source/render/Qt3DSRenderFrameBuffer.h \
    ../Source/render/Qt3DSRenderGeometryShader.h \
    ../Source/render/Qt3DSRenderImageTexture.h \
    ../Source/render/Qt3DSRenderIndexBuffer.h \
    ../Source/render/Qt3DSRenderInputAssembler.h \
    ../Source/render/Qt3DSRenderOcclusionQuery.h \
    ../Source/render/Qt3DSRenderPathFontSpecification.h \
    ../Source/render/Qt3DSRenderPathFontText.h \
    ../Source/render/Qt3DSRenderPathRender.h \
    ../Source/render/Qt3DSRenderPathSpecification.h \
    ../Source/render/Qt3DSRenderProgramPipeline.h \
    ../Source/render/Qt3DSRenderQueryBase.h \
    ../Source/render/Qt3DSRenderRasterizerState.h \
    ../Source/render/Qt3DSRenderRenderBuffer.h \
    ../Source/render/Qt3DSRenderSampler.h \
    ../Source/render/Qt3DSRenderShader.h \
    ../Source/render/Qt3DSRenderShaderConstant.h \
    ../Source/render/Qt3DSRenderShaderProgram.h \
    ../Source/render/Qt3DSRenderStorageBuffer.h \
    ../Source/render/Qt3DSRenderSync.h \
    ../Source/render/Qt3DSRenderTessellationShader.h \
    ../Source/render/Qt3DSRenderTexture2D.h \
    ../Source/render/Qt3DSRenderTexture2DArray.h \
    ../Source/render/Qt3DSRenderTextureBase.h \
    ../Source/render/Qt3DSRenderTextureCube.h \
    ../Source/render/Qt3DSRenderTimerQuery.h \
    ../Source/render/Qt3DSRenderVertexBuffer.h \
    ../Source/render/Qt3DSRenderVertexShader.h \
    ../Source/render/glg/Qt3DSGLImplObjects.h

# Render
SOURCES += \
    ../Source/render/backends/gl/Qt3DSOpenGLExtensions.cpp \
    ../Source/render/backends/gl/Qt3DSRenderBackendGL3.cpp \
    ../Source/render/backends/gl/Qt3DSRenderBackendGL4.cpp \
    ../Source/render/backends/gl/Qt3DSRenderBackendGLBase.cpp \
    ../Source/render/backends/gl/Qt3DSRenderContextGL.cpp \
    ../Source/render/backends/software/Qt3DSRenderBackendNULL.cpp \
    ../Source/render/backends/gl/Q3DSRenderBackendGLES2.cpp

HEADERS += \
    ../Source/render/backends/Qt3DSRenderBackend.h \
    ../Source/render/backends/gl/Qt3DSOpenGLPrefix.h \
    ../Source/render/backends/gl/Qt3DSOpenGLUtil.h \
    ../Source/render/backends/gl/Qt3DSOpenGLExtensions.h \
    ../Source/render/backends/gl/Qt3DSRenderBackendGL3.h \
    ../Source/render/backends/gl/Qt3DSRenderBackendGL4.h \
    ../Source/render/backends/gl/Qt3DSRenderBackendGLBase.h \
    ../Source/render/backends/gl/Qt3DSRenderBackendInputAssemblerGL.h \
    ../Source/render/backends/gl/Qt3DSRenderBackendRenderStatesGL.h \
    ../Source/render/backends/gl/Qt3DSRenderBackendShaderProgramGL.h \
    ../Source/render/backends/software/Qt3DSRenderBackendNULL.h \
    ../Source/render/backends/gl/Q3DSRenderBackendGLES2.h

# DataModel
SOURCES += \
    ../Source/datamodel/Qt3DSMetadata.cpp \
    ../../Authoring/QT3DSIMP/Qt3DSImportLib/Qt3DSImportMesh.cpp \
    ../../Authoring/QT3DSIMP/Qt3DSImportLib/Qt3DSImportPath.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMMetaData.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMXML.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMStringTable.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMComposerTypeDefinitions.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMValue.cpp \
    ../../Authoring/QT3DSDM/Systems/Cores/SimpleDataCore.cpp

HEADERS += \
    ../Source/datamodel/Qt3DSMetadata.h \
    ../Source/datamodel/DocumentResourceManagerScriptParser.h \
    ../../Authoring/QT3DSIMP/Qt3DSImportLib/Qt3DSImportMesh.h \
    ../../Authoring/QT3DSIMP/Qt3DSImportLib/Qt3DSImportPath.h \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMMetaData.h \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMXML.h \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMStringTable.h \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMHandles.h \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMComposerTypeDefinitions.h \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMValue.h \
    ../../Authoring/QT3DSDM/Systems/Cores/SimpleDataCore.h

# Engine
HEADERS += \
    ../Source/engine/EnginePrefix.h

# Event
SOURCES += \
    ../Source/event/EventFactory.cpp \
    ../Source/event/EventPoller.cpp \
    ../Source/event/EventSystemC.cpp

HEADERS += \
    ../Source/event/EventPollingSystem.h \
    ../Source/event/EventSystem.h \
    ../Source/event/EventSystemC.h

# Render
HEADERS += \
    ../Source/runtimerender/android/DynamicLibLoader.h \
    ../Source/runtimerender/linux/DynamicLibLoader.h \
    ../Source/runtimerender/macos/DynamicLibLoader.h \
    ../Source/runtimerender/qnx/DynamicLibLoader.h \
    ../Source/runtimerender/windows/DynamicLibLoader.h

# Runtime
HEADERS += \
    ../Source/runtime/RuntimePrefix.h \
    ../Source/runtime/q3dsqmlscript.h \
    ../Source/runtime/q3dsqmlbehavior.h

SOURCES += \
    ../Source/runtime/q3dsqmlscript.cpp \
    ../Source/runtime/q3dsqmlbehavior.cpp

# System
HEADERS += \
    ../Source/system/SystemPrefix.h

DISTFILES += \
    ../Source/runtime/Qt3DSAttributeHashes.txt
