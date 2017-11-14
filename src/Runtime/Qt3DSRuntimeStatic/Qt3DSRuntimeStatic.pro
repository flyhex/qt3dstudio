TEMPLATE = lib
TARGET = qt3dsruntimestatic
CONFIG += staticlib
include(../commoninclude.pri)

!boot2qt:!integrity:!ios {
    RESOURCES += ../res.qrc
}

linux {
    DEFINES += _POSIX_C_SOURCE=199309L
}

QT += qml

# Foundation
SOURCES += \
    ../Source/Qt3DSFoundation/Source/foundation/ConvertUTF.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/EASTL_new.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/FileTools.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/IOStreams.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/Qt3DSLogging.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/Qt3DSFoundation.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/Qt3DSMathUtils.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/Qt3DSPerfTimer.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/Qt3DSSystem.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/Socket.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/StringTable.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/XML.cpp \
    ../Source/Qt3DSRuntimeRender/Source/q3dsqmlrender.cpp \
    ../Source/Engine/Source/Qt3DSRenderRuntimeBinding.cpp \
    ../Source/Engine/Source/Qt3DSRenderRuntimeBindingImplRenderer.cpp \
    ../Source/Engine/Source/Qt3DSRenderRuntimeBindingImplTranslation.cpp \
    ../Source/Engine/Source/Qt3DSTegraApplication.cpp \
    ../Source/Engine/Source/Qt3DSTegraInputEngine.cpp \
    ../Source/Runtime/Source/Qt3DSActivationManager.cpp \
    ../Source/Runtime/Source/Qt3DSAnimationSystem.cpp \
    ../Source/Runtime/Source/Qt3DSApplication.cpp \
    ../Source/Runtime/Source/Qt3DSAttributeHashes.cpp \
    ../Source/Runtime/Source/Qt3DSBinarySerializerImpl.cpp \
    ../Source/Runtime/Source/Qt3DSComponentManager.cpp \
    ../Source/Runtime/Source/Qt3DSElementSystem.cpp \
    ../Source/Runtime/Source/Qt3DSEventCallbacks.cpp \
    ../Source/Runtime/Source/Qt3DSInputEngine.cpp \
    ../Source/Runtime/Source/Qt3DSLogicSystem.cpp \
    ../Source/Runtime/Source/Qt3DSLuaAxis.cpp \
    ../Source/Runtime/Source/Qt3DSLuaButton.cpp \
    ../Source/Runtime/Source/Qt3DSLuaColor.cpp \
    ../Source/Runtime/Source/Qt3DSLuaCommandHelper.cpp \
    ../Source/Runtime/Source/Qt3DSLuaElementHelper.cpp \
    ../Source/Runtime/Source/Qt3DSLuaEngine.cpp \
    ../Source/Runtime/Source/Qt3DSLuaEventHelper.cpp \
    ../Source/Runtime/Source/Qt3DSLuaHelper.cpp \
    ../Source/Runtime/Source/Qt3DSLuaKeyboard.cpp \
    ../Source/Runtime/Source/Qt3DSLuaMatrix.cpp \
    ../Source/Runtime/Source/Qt3DSLuaRotation.cpp \
    ../Source/Runtime/Source/Qt3DSLuaSceneHelper.cpp \
    ../Source/Runtime/Source/Qt3DSLuaVector.cpp \
    ../Source/Runtime/Source/Qt3DSOutputMemoryStream.cpp \
    ../Source/Runtime/Source/Qt3DSParametersSystem.cpp \
    ../Source/Runtime/Source/Qt3DSPresentation.cpp \
    ../Source/Runtime/Source/Qt3DSPresentationFrameData.cpp \
    ../Source/Runtime/Source/Qt3DSQmlElementHelper.cpp \
    ../Source/Runtime/Source/Qt3DSQmlEngine.cpp \
    ../Source/Runtime/Source/Qt3DSSlideSystem.cpp \
    ../Source/Runtime/Source/Qt3DSTimePolicy.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderCamera.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderDefaultMaterial.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderDynamicObject.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderEffect.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderImage.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderLayer.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderLight.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderLightmaps.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderModel.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderNode.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderPath.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderPresentation.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderScene.cpp \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderText.cpp \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRenderableObjects.cpp \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImpl.cpp \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImplLayerRenderData.cpp \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImplLayerRenderHelper.cpp \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImplLayerRenderPreparationData.cpp \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImplShaders.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderBufferLoader.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderBufferManager.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderImageBatchLoader.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderLoadedTexture.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderLoadedTextureBMP.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderLoadedTextureDDS.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderLoadedTextureGIF.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderLoadedTextureHDR.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderPrefilterTexture.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderResourceBufferObjects.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderResourceManager.cpp \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderResourceTexture2D.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSOffscreenRenderManager.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSOldNBustedRenderPlugin.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSOnscreenTextRenderer.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSQtTextRenderer.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderClippingFrustum.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderCustomMaterialShaderGenerator.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderCustomMaterialSystem.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderDefaultMaterialShaderGenerator.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderDynamicObjectSystem.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderEffectSystem.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRendererUtil.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderEulerAngles.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderGpuProfiler.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderGraphObjectSerializer.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderImageScaler.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderInputStreamFactory.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderPathManager.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderPixelGraphicsRenderer.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderPixelGraphicsTypes.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderPlugin.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderRay.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderRenderList.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderShaderCache.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderShaderCodeGenerator.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderShaderCodeGeneratorV2.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderShadowMap.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderSubpresentation.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderTextTextureAtlas.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderTextTextureCache.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderTextureAtlas.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderThreadPool.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderUIPLoader.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderUIPSharedTranslation.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderWidgets.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSTextRenderer.cpp \
    ../Source/System/Source/Qt3DSAssert.cpp \
    ../Source/System/Source/Qt3DSBoundingBox.cpp \
    ../Source/System/Source/Qt3DSColor.cpp \
    ../Source/System/Source/Qt3DSCubicRoots.cpp \
    ../Source/System/Source/Qt3DSDataLogger.cpp \
    ../Source/System/Source/Qt3DSDLLManager.cpp \
    ../Source/System/Source/Qt3DSEulerAngles.cpp \
    ../Source/System/Source/Qt3DSFile.cpp \
    ../Source/System/Source/Qt3DSFileStream.cpp \
    ../Source/System/Source/Qt3DSFunctionWrappers.cpp \
    ../Source/System/Source/Qt3DSMatrix.cpp \
    ../Source/System/Source/Qt3DSMemory.cpp \
    ../Source/System/Source/Qt3DSMemoryFilter.cpp \
    ../Source/System/Source/Qt3DSMemoryHeap.cpp \
    ../Source/System/Source/Qt3DSMemoryManager.cpp \
    ../Source/System/Source/Qt3DSMemoryPool.cpp \
    ../Source/System/Source/Qt3DSMemoryProbe.cpp \
    ../Source/System/Source/Qt3DSMemoryStatistics.cpp \
    ../Source/System/Source/Qt3DSMemoryTracker.cpp \
    ../Source/System/Source/Qt3DSTimer.cpp \
    ../Source/System/Source/Qt3DSTypes.cpp \
    ../Source/System/Source/Qt3DSVector3.cpp \
    ../Source/Qt3DSStateApplication/Application/Qt3DSStateApplication.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSLuaDatamodelCache.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSLuaSideDebugger.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSSceneGraphRuntimeDebugger.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDataTest.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDebuggedInterpreter.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDebugger.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDebuggerListener.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDebugStreams.cpp \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateLuaTest.cpp \
    ../Source/Qt3DSStateApplication/Source/Qt3DSStateContext.cpp \
    ../Source/Qt3DSStateApplication/Source/Qt3DSStateExecutionContext.cpp \
    ../Source/Qt3DSStateApplication/Source/Qt3DSStateInterpreter.cpp \
    ../Source/Qt3DSStateApplication/Source/Qt3DSStateLuaScriptContext.cpp \
    ../Source/Qt3DSStateApplication/Source/Qt3DSStateVisualBindingContext.cpp \
    ../Source/Qt3DSStateApplication/Source/Qt3DSStateXMLIO.cpp \
    ../Source/UIPParser/Source/Qt3DSUIPParserActionHelper.cpp \
    ../Source/UIPParser/Source/Qt3DSUIPParserImpl.cpp \
    ../Source/UIPParser/Source/Qt3DSUIPParserObjectRefHelper.cpp \
    ../Source/Qt3DSRuntimeRender/Source/Qt3DSRenderContextCore.cpp

HEADERS += \
    ../Source/Qt3DSFoundation/Include/foundation/ConvertUTF.h \
    ../Source/Qt3DSFoundation/Include/foundation/FileTools.h \
    ../Source/Qt3DSFoundation/Include/foundation/IOStreams.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSLogging.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSFoundation.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSMathUtils.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSPerfTimer.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSSystem.h \
    ../Source/Qt3DSFoundation/Include/foundation/Socket.h \
    ../Source/Qt3DSFoundation/Include/foundation/StringTable.h \
    ../Source/Qt3DSFoundation/Include/foundation/XML.h \
    ../Source/Qt3DSFoundation/Include/foundation/AutoDeallocatorAllocator.h \
    ../Source/Qt3DSFoundation/Include/foundation/FastAllocator.h \
    ../Source/Qt3DSFoundation/Include/foundation/PoolingAllocator.h \
    ../Source/Qt3DSFoundation/Include/foundation/PreAllocatedAllocator.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DS.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSAllocator.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSAllocatorCallback.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSAssert.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSAtomic.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSBasicTemplates.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSBounds3.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSBroadcastingAllocator.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSContainers.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSDataRef.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSDiscriminatedUnion.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSFastIPC.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSFlags.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSFPU.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSIndexableLinkedList.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSIntrinsics.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSInvasiveLinkedList.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSInvasiveSet.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSIPC.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSMat33.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSMat44.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSMath.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSMemoryBuffer.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSMutex.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSNoCopy.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSOption.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSPlane.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSPool.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSPreprocessor.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSQuat.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSRefCounted.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSSemaphore.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSSimpleTypes.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSStringTokenizer.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSSync.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSThread.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSTime.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSTransform.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSUnionCast.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSUtilities.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSVec2.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSVec3.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSVec4.h \
    ../Source/Qt3DSFoundation/Include/foundation/Qt3DSVersionNumber.h \
    ../Source/Qt3DSFoundation/Include/foundation/SerializationTypes.h \
    ../Source/Qt3DSFoundation/Include/foundation/StrConvertUTF.h \
    ../Source/Qt3DSFoundation/Include/foundation/StringConversion.h \
    ../Source/Qt3DSFoundation/Include/foundation/StringConversionImpl.h \
    ../Source/Qt3DSFoundation/Include/foundation/TaggedPointer.h \
    ../Source/Qt3DSFoundation/Include/foundation/ThreadSafeQueue.h \
    ../Source/Qt3DSFoundation/Include/foundation/TrackingAllocator.h \
    ../Source/Qt3DSFoundation/Include/foundation/Utils.h \
    ../Source/Qt3DSRuntimeRender/Include/q3dsqmlrender.h \
    ../Source/Engine/Include/Qt3DSRenderRuntimeBinding.h \
    ../Source/Engine/Include/Qt3DSRenderRuntimeBindingImpl.h \
    ../Source/Engine/Include/Qt3DSTegraApplication.h \
    ../Source/Engine/Include/Qt3DSTegraInputEngine.h \
    ../Source/Runtime/Include/Qt3DSActivationManager.h \
    ../Source/Runtime/Include/Qt3DSAnimationSystem.h \
    ../Source/Runtime/Include/Qt3DSApplication.h \
    ../Source/Runtime/Include/Qt3DSAttributeHashes.h \
    ../Source/Runtime/Include/Qt3DSBinarySerializerImpl.h \
    ../Source/Runtime/Include/Qt3DSComponentManager.h \
    ../Source/Runtime/Include/Qt3DSElementSystem.h \
    ../Source/Runtime/Include/Qt3DSEventCallbacks.h \
    ../Source/Runtime/Include/Qt3DSInputEngine.h \
    ../Source/Runtime/Include/Qt3DSLogicSystem.h \
    ../Source/Runtime/Include/Qt3DSLuaAxis.h \
    ../Source/Runtime/Include/Qt3DSLuaButton.h \
    ../Source/Runtime/Include/Qt3DSLuaColor.h \
    ../Source/Runtime/Include/Qt3DSLuaCommandHelper.h \
    ../Source/Runtime/Include/Qt3DSLuaElementHelper.h \
    ../Source/Runtime/Include/Qt3DSLuaEngine.h \
    ../Source/Runtime/Include/Qt3DSLuaEventHelper.h \
    ../Source/Runtime/Include/Qt3DSLuaHelper.h \
    ../Source/Runtime/Include/Qt3DSLuaKeyboard.h \
    ../Source/Runtime/Include/Qt3DSLuaMatrix.h \
    ../Source/Runtime/Include/Qt3DSLuaRotation.h \
    ../Source/Runtime/Include/Qt3DSLuaSceneHelper.h \
    ../Source/Runtime/Include/Qt3DSLuaVector.h \
    ../Source/Runtime/Include/Qt3DSOutputMemoryStream.h \
    ../Source/Runtime/Include/Qt3DSParametersSystem.h \
    ../Source/Runtime/Include/Qt3DSPresentation.h \
    ../Source/Runtime/Include/Qt3DSPresentationFrameData.h \
    ../Source/Runtime/Include/Qt3DSQmlElementHelper.h \
    ../Source/Runtime/Include/Qt3DSQmlEngine.h \
    ../Source/Runtime/Include/Qt3DSSlideSystem.h \
    ../Source/Runtime/Include/Qt3DSTimePolicy.h \
    ../Source/Runtime/Include/Qt3DSApplicationValues.h \
    ../Source/Runtime/Include/Qt3DSBinarySerializationHelper.h \
    ../Source/Runtime/Include/Qt3DSBinarySerializer.h \
    ../Source/Runtime/Include/Qt3DSIComponentManager.h \
    ../Source/Runtime/Include/Qt3DSIInputSystem.h \
    ../Source/Runtime/Include/Qt3DSInputDefs.h \
    ../Source/Runtime/Include/Qt3DSInputEventTypes.h \
    ../Source/Runtime/Include/Qt3DSIStateful.h \
    ../Source/Runtime/Include/Qt3DSIText.h \
    ../Source/Runtime/Include/Qt3DSKernelTypes.h \
    ../Source/Runtime/Include/Qt3DSLuaIncludes.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderCamera.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderCustomMaterial.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderDefaultMaterial.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderDynamicObject.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderEffect.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderGraphObject.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderImage.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderLayer.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderLight.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderLightmaps.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderMaterialDirty.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderModel.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderNode.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderPath.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderPathSubPath.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderPresentation.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderReferencedMaterial.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderScene.h \
    ../Source/Qt3DSRuntimeRender/GraphObjects/Qt3DSRenderText.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSOffscreenRenderKey.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSOffscreenRenderManager.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSOldNBustedRenderPlugin.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRender.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderableImage.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderClippingFrustum.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderCustomMaterialRenderContext.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderCustomMaterialShaderGenerator.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderCustomMaterialSystem.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderDefaultMaterialShaderGenerator.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderDynamicObjectSystem.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderDynamicObjectSystemCommands.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderDynamicObjectSystemUtil.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderEffectSystem.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderer.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRendererUtil.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderEulerAngles.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderGraphObjectPickQuery.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderGraphObjectSerializer.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderGraphObjectTypes.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderImageScaler.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderImageTextureData.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderInputStreamFactory.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderMaterialHelpers.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderMaterialShaderGenerator.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderMesh.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPathManager.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPathMath.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPathRenderContext.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPixelGraphicsRenderer.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPixelGraphicsTypes.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPlugin.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPluginCInterface.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPluginGraphObject.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderPluginPropertyValue.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderProfiler.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderRay.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderRenderList.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderRotationHelper.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderShaderCache.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderShaderCodeGenerator.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderShaderCodeGeneratorV2.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderShaderKeys.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderShadowMap.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderString.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderSubpresentation.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderSubPresentationHelper.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderTaggedPointer.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderTessModeValues.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderTextTextureAtlas.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderTextTextureCache.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderTextTypes.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderTextureAtlas.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderThreadPool.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderUIPLoader.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderUIPSharedTranslation.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderWidgets.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSTextRenderer.h \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRenderableObjects.h \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImpl.h \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImplLayerRenderData.h \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImplLayerRenderHelper.h \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImplLayerRenderPreparationData.h \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSRendererImplShaders.h \
    ../Source/Qt3DSRuntimeRender/RendererImpl/Qt3DSVertexPipelineImpl.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderBufferLoader.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderBufferManager.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderImageBatchLoader.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderLoadedTexture.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderLoadedTextureDDS.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderLoadedTextureFreeImageCompat.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderPrefilterTexture.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderResourceBufferObjects.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderResourceManager.h \
    ../Source/Qt3DSRuntimeRender/ResourceManager/Qt3DSRenderResourceTexture2D.h \
    ../Source/System/Include/Qt3DSArray.h \
    ../Source/System/Include/Qt3DSAssert.h \
    ../Source/System/Include/Qt3DSAudioPlayer.h \
    ../Source/System/Include/Qt3DSBasicPluginDLL.h \
    ../Source/System/Include/Qt3DSBezierEval.h \
    ../Source/System/Include/Qt3DSBoundingBox.h \
    ../Source/System/Include/Qt3DSCircularArray.h \
    ../Source/System/Include/Qt3DSColor.h \
    ../Source/System/Include/Qt3DSConfig.h \
    ../Source/System/Include/Qt3DSCubicRoots.h \
    ../Source/System/Include/Qt3DSCubicRootsImpl.h \
    ../Source/System/Include/Qt3DSDataLogger.h \
    ../Source/System/Include/Qt3DSDataLogger.hpp \
    ../Source/System/Include/Qt3DSDataLoggerEnums.h \
    ../Source/System/Include/Qt3DSDataLoggerViewer.h \
    ../Source/System/Include/Qt3DSDLLManager.h \
    ../Source/System/Include/Qt3DSEGLTimer.h \
    ../Source/System/Include/Qt3DSEndian.h \
    ../Source/System/Include/Qt3DSEulerAngles.h \
    ../Source/System/Include/Qt3DSFile.h \
    ../Source/System/Include/Qt3DSFileStream.h \
    ../Source/System/Include/Qt3DSFixedArray.h \
    ../Source/System/Include/Qt3DSFNDTimer.h \
    ../Source/System/Include/Qt3DSFunctionWrappers.h \
    ../Source/System/Include/Qt3DSHash.h \
    ../Source/System/Include/Qt3DSIFileStream.h \
    ../Source/System/Include/Qt3DSIStream.h \
    ../Source/System/Include/Qt3DSITimer.h \
    ../Source/System/Include/Qt3DSMacros.h \
    ../Source/System/Include/Qt3DSMatrix.h \
    ../Source/System/Include/Qt3DSMemory.h \
    ../Source/System/Include/Qt3DSMemoryFilter.h \
    ../Source/System/Include/Qt3DSMemoryHeap.h \
    ../Source/System/Include/Qt3DSMemoryManager.h \
    ../Source/System/Include/Qt3DSMemoryPool.h \
    ../Source/System/Include/Qt3DSMemoryProbe.h \
    ../Source/System/Include/Qt3DSMemorySettings.h \
    ../Source/System/Include/Qt3DSMemoryStatistics.h \
    ../Source/System/Include/Qt3DSMemoryTracker.h \
    ../Source/System/Include/Qt3DSPlatformSpecific.h \
    ../Source/System/Include/Qt3DSTimer.h \
    ../Source/System/Include/Qt3DSTypes.h \
    ../Source/System/Include/Qt3DSVector3.h \
    ../Source/System/Include/Qt3DSArray.inl \
    ../Source/System/Include/Qt3DSCircularArray.inl \
    ../Source/System/Include/Qt3DSFixedArray.inl \
    ../Source/Qt3DSStateApplication/Application/Qt3DSStateApplication.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSLuaDebugger.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSLuaDebuggerImpl.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSLuaDebuggerProtocol.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSSceneGraphDebugger.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSSceneGraphDebuggerProtocol.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSSceneGraphDebuggerValue.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDebugger.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDebuggerProtocol.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDebuggerValues.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateDebugStreams.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateTest.h \
    ../Source/Qt3DSStateApplication/Debugger/Qt3DSStateTestCommon.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSState.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateContext.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateExecutionContext.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateExecutionTypes.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateIdValue.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateInterpreter.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateLuaScriptContext.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateScriptContext.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateSharedImpl.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateSignalConnection.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateTypes.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateVisualBindingContext.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateVisualBindingContextCommands.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateVisualBindingContextValues.h \
    ../Source/Qt3DSStateApplication/Include/Qt3DSStateXMLIO.h \
    ../Source/UIPParser/Include/Qt3DSIPresentation.h \
    ../Source/UIPParser/Include/Qt3DSUIPParser.h \
    ../Source/UIPParser/Include/Qt3DSUIPParserActionHelper.h \
    ../Source/UIPParser/Include/Qt3DSUIPParserImpl.h \
    ../Source/UIPParser/Include/Qt3DSUIPParserObjectRefHelper.h \
    ../Source/Runtime/Include/Qt3DSCommandEventTypes.h \
    ../Source/Runtime/Include/Qt3DSEvent.h \
    ../Source/Runtime/Include/Qt3DSFrameworkTypes.h \
    ../Source/Runtime/Include/Qt3DSInputFrame.h \
    ../Source/Runtime/Include/Qt3DSIScene.h \
    ../Source/Runtime/Include/Qt3DSIScriptBridge.h \
    ../Source/Runtime/Include/Qt3DSPickFrame.h \
    ../Source/Runtime/Include/Qt3DSRuntimeFactory.h \
    ../Source/Runtime/Include/Qt3DSSceneManager.h \
    ../Source/Engine/Include/Qt3DSEGLInfo.h \
    ../Source/Engine/Include/Qt3DSEGLWindowSystem.h \
    ../Source/Engine/Include/Qt3DSPluginDLL.h \
    ../Source/Engine/Include/Qt3DSWindowSystem.h \
    ../Source/Qt3DSRuntimeRender/Include/Qt3DSRenderContextCore.h

win32 {
SOURCES += \
    ../Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsAtomic.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsFPU.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsMutex.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsSemaphore.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsSync.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsThread.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/windows/Qt3DSWindowsTime.cpp

HEADERS += \
    ../Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsAoS.h \
    ../Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsFile.h \
    ../Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsInclude.h \
    ../Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsInlineAoS.h \
    ../Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsIntrinsics.h \
    ../Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsString.h \
    ../Source/Qt3DSFoundation/Include/foundation/windows/Qt3DSWindowsTrigConstants.h
}

macos: SOURCES += \
    ../Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixAtomic.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixFPU.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixMutex.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixSemaphore.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixSync.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxThread.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/macosx/Qt3DSUnixTime.cpp

linux|integrity {
SOURCES += \
    ../Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxAtomic.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxFPU.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxMutex.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxSemaphore.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxSync.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxThread.cpp \
    ../Source/Qt3DSFoundation/Source/foundation/linux/Qt3DSLinuxTime.cpp

HEADERS += \
    ../Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxAoS.h \
    ../Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxFile.h \
    ../Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxInlineAoS.h \
    ../Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxIntrinsics.h \
    ../Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxString.h \
    ../Source/Qt3DSFoundation/Include/foundation/linux/Qt3DSLinuxTrigConstants.h
}

# Libs
SOURCES += \
    ../Source/PlatformSpecific/$$PlatformSpecificDir/Qt3DSLibs/nv_math/nv_math.cpp \
    ../Source/PlatformSpecific/$$PlatformSpecificDir/Qt3DSLibs/nv_math/nv_matrix.cpp \
    ../Source/PlatformSpecific/$$PlatformSpecificDir/Qt3DSLibs/nv_math/nv_quat.cpp

# RenderBase
SOURCES += \
    ../Source/Qt3DSRender/Source/Qt3DSRenderAtomicCounterBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderAttribLayout.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderComputeShader.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderConstantBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderContext.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderDataBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderDepthStencilState.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderDrawIndirectBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderFragmentShader.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderFrameBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderGeometryShader.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderImageTexture.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderIndexBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderInputAssembler.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderOcclusionQuery.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderPathFontSpecification.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderPathFontText.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderPathRender.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderPathSpecification.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderProgramPipeline.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderQueryBase.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderRasterizerState.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderRenderBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderSampler.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderShaderProgram.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderStorageBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderSync.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderTessellationShader.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderTexture2D.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderTexture2DArray.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderTextureBase.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderTextureCube.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderTimerQuery.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderVertexBuffer.cpp \
    ../Source/Qt3DSRender/Source/Qt3DSRenderVertexShader.cpp

HEADERS += \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderAtomicCounterBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderAttribLayout.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderBaseTypes.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderComputeShader.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderConstantBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderContext.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderDataBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderDepthStencilState.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderDrawable.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderDrawIndirectBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderFragmentShader.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderFrameBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderGeometryShader.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderImageTexture.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderIndexBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderInputAssembler.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderOcclusionQuery.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderPathFontSpecification.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderPathFontText.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderPathRender.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderPathSpecification.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderProgramPipeline.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderQueryBase.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderRasterizerState.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderRenderBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderSampler.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderShader.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderShaderConstant.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderShaderProgram.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderStorageBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderSync.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderTessellationShader.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderTexture2D.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderTexture2DArray.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderTextureBase.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderTextureCube.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderTimerQuery.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderVertexBuffer.h \
    ../Source/Qt3DSRender/Include/render/Qt3DSRenderVertexShader.h \
    ../Source/Qt3DSRender/Include/render/glg/Qt3DSGLImplObjects.h

# Render
SOURCES += \
    ../Source/Qt3DSRender/Source/backends/gl/Qt3DSOpenGLExtensions.cpp \
    ../Source/Qt3DSRender/Source/backends/gl/Qt3DSRenderBackendGL3.cpp \
    ../Source/Qt3DSRender/Source/backends/gl/Qt3DSRenderBackendGL4.cpp \
    ../Source/Qt3DSRender/Source/backends/gl/Qt3DSRenderBackendGLBase.cpp \
    ../Source/Qt3DSRender/Source/backends/gl/Qt3DSRenderContextGL.cpp \
    ../Source/Qt3DSRender/Source/backends/software/Qt3DSRenderBackendNULL.cpp \
    ../Source/Qt3DSRender/Source/backends/gl/Q3DSRenderBackendGLES2.cpp

HEADERS += \
    ../Source/Qt3DSRender/Include/render/backends/Qt3DSRenderBackend.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSOpenGLPrefix.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSOpenGLUtil.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSOpenGLExtensions.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendGL3.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendGL4.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendGLBase.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendInputAssemblerGL.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendRenderStatesGL.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Qt3DSRenderBackendShaderProgramGL.h \
    ../Source/Qt3DSRender/Include/render/backends/software/Qt3DSRenderBackendNULL.h \
    ../Source/Qt3DSRender/Include/render/backends/gl/Q3DSRenderBackendGLES2.h

# DataModel
SOURCES += \
    ../Source/DataModel/Source/Qt3DSMetadata.cpp \
    ../../Authoring/QT3DSIMP/Qt3DSImportLib/Qt3DSImportMesh.cpp \
    ../../Authoring/QT3DSIMP/Qt3DSImportLib/Qt3DSImportPath.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMMetaData.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMXML.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMStringTable.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMHandles.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMComposerTypeDefinitions.cpp \
    ../../Authoring/QT3DSDM/Systems/Qt3DSDMValue.cpp \
    ../../Authoring/QT3DSDM/Systems/Cores/SimpleDataCore.cpp

HEADERS += \
    ../Source/DataModel/Include/Qt3DSMetadata.h \
    ../Source/DataModel/Include/DocumentResourceManagerScriptParser.h \
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
    ../Source/Engine/Include/EnginePrefix.h

SOURCES += \
    ../Source/Engine/Source/EnginePrefix.cpp

# Event
SOURCES += \
    ../Source/Qt3DSEvent/Source/EventFactory.cpp \
    ../Source/Qt3DSEvent/Source/EventPoller.cpp \
    ../Source/Qt3DSEvent/Source/EventPollingSystemLuaBinding.cpp \
    ../Source/Qt3DSEvent/Source/EventSystemC.cpp

HEADERS += \
    ../Source/Qt3DSEvent/InternalInclude/EventPollingSystem.h \
    ../Source/Qt3DSEvent/InternalInclude/EventPollingSystemLuaBinding.h \
    ../Source/Qt3DSEvent/Include/EventSystem.h \
    ../Source/Qt3DSEvent/Include/EventSystemC.h

# Render
HEADERS += \
    ../Source/Qt3DSRuntimeRender/Include/ANDROID/DynamicLibLoader.h \
    ../Source/Qt3DSRuntimeRender/Include/LINUX/DynamicLibLoader.h \
    ../Source/Qt3DSRuntimeRender/Include/OSX/DynamicLibLoader.h \
    ../Source/Qt3DSRuntimeRender/Include/QNX/DynamicLibLoader.h \
    ../Source/Qt3DSRuntimeRender/Include/WINDOWS/DynamicLibLoader.h

# Runtime
HEADERS += \
    ../Source/Runtime/Include/RuntimePrefix.h \
    ../Source/Runtime/Include/q3dsqmlscript.h \
    ../Source/Runtime/Include/q3dsqmlbehavior.h

SOURCES += \
    ../Source/Runtime/Source/RuntimePrefix.cpp \
    ../Source/Runtime/Source/q3dsqmlscript.cpp \
    ../Source/Runtime/Source/q3dsqmlbehavior.cpp

# System
SOURCES += \
    ../Source/System/Source/SystemPrefix.cpp

HEADERS += \
    ../Source/System/Include/SystemPrefix.h

DISTFILES += \
    ../Source/Runtime/Include/Qt3DSAttributeHashes.txt
