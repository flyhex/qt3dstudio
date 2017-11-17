TEMPLATE = app
TARGET = Qt3DStudio
include(../commoninclude.pri)
include($$OUT_PWD/../qtAuthoring-config.pri)
INCLUDEPATH += $$OUT_PWD/..

CONFIG += nostrictstrings

DEFINES += _UNICODE UNICODE QT3DS_AUTHORING _AFXDLL \
    PCRE_STATIC EASTL_MINMAX_ENABLED=0 \
    EASTL_NOMINMAX=0 DOM_DYNAMIC DRIVE_DESIGN_STUDIO

win: QMAKE_LFLAGS += /MANIFEST /ENTRY:"wWinMainCRTStartup"

QT += core gui openglextensions
QT += qml quick widgets quickwidgets network

INCLUDEPATH += \
    _Win/Include \
    _Win/Application \
    _Win/Controls \
    _Win/DragNDrop \
    _Win/Palettes \
    _Win/Palettes/Progress \
    _Win/Palettes/Splash \
    _Win/UI \
    _Win/Utils \
    Application \
    Controls \
    DragAndDrop \
    Palettes \
    Palettes/Action \
    Palettes/Action/ActionParamRow \
    Palettes/BasicObjects \
    Palettes/Inspector \
    Palettes/Master \
    Palettes/Progress \
    Palettes/Project \
    Palettes/Slide \
    Palettes/Timeline \
    Render \
    UI \
    Utils \
    Workspace \
    Workspace/Views \
    . \
    .. \
    English.lproj/Strings \
    ../QT3DSIMP/Qt3DSImportLib \
    ../QT3DSIMP/Qt3DSImportSGTranslation \
    ../QT3DSDM/Systems \
    ../Common/Code/Thread \
    ../Common/Code/IO \
    ../Common/Code \
    ../Common/Code/Exceptions \
    ../Common/Code/_Win32/Include \
    ../Common/Code/_Win32 \
    ../Common/Code/Graph \
    ../Common/Code/Report \
    ../Common/Code/Memory \
    ../Client/Code/Core/Utility \
    ../Client/Code/Core/Types \
    ../Client/Code/Core/Commands \
    ../Client/Code/Core/Core \
    ../Client/Code/Core \
    ../Client/Code/Core/Doc \
    ../Client/Code/Core/Doc/ClientDataModelBridge \
    ../Client/Code/Shared \
    ../Client/Code/Shared/Log \
    ../../Runtime/Source/Qt3DSRender/Include \
    ../../Runtime/Source/Qt3DSFoundation/Include \
    ../../Runtime/Source/Qt3DSRuntimeRender/Include \
    ../../Runtime/Source/Qt3DSRuntimeRender/GraphObjects \
    ../../Runtime/Source/Qt3DSRuntimeRender/ResourceManager \
    ../../Runtime/Source/Qt3DSStateApplication/Application \
    ../../Runtime/Source/Qt3DSEvent/InternalInclude \
    ../../3rdparty/EASTL/UnknownVersion/include \
    ../../3rdparty/color

linux {
    BEGIN_ARCHIVE = -Wl,--whole-archive
    END_ARCHIVE = -Wl,--no-whole-archive
}

STATICRUNTIME = \
    $$BEGIN_ARCHIVE \
    -lLua$$qtPlatformTargetSuffix() \
    -lEASTL$$qtPlatformTargetSuffix() \
    -lpcre$$qtPlatformTargetSuffix() \
    -lTinyXML$$qtPlatformTargetSuffix() \
    -lColladaDOM$$qtPlatformTargetSuffix() \
    -lQT3DSDM$$qtPlatformTargetSuffix() \
    -lCommonLib$$qtPlatformTargetSuffix() \
    -lCoreLib$$qtPlatformTargetSuffix() \
    $$END_ARCHIVE

# On non-windows systems link the whole static archives and do not put them
# in the prl file to prevent them being linked again by targets that depend
# upon this shared library
!win32 {
    QMAKE_LFLAGS += $$STATICRUNTIME
} else {
    DEFINES +=  WIN32_LEAN_AND_MEAN
    LIBS += $$STATICRUNTIME
    !mingw: QMAKE_LFLAGS += /NODEFAULTLIB:tinyxml.lib
}

LIBS += \
      -lqt3dsruntimestatic$$qtPlatformTargetSuffix() \
       $$QMAKE_LIBS_FBX

linux {
    LIBS += \
        -ldl \
        -lEGL
}

win: PRECOMPILED_HEADER = _Win/Studio/stdafx.h

HEADERS += \
    MainFrm.h \
    _Win/Application/AboutDlg.h \
    _Win/UI/EditCameraBar.h \
    _Win/UI/InterpolationDlg.h \
    _Win/UI/StartupDlg.h \
    _Win/UI/RecentItems.h \
    _Win/UI/PlayerWnd.h \
    _Win/UI/PlayerContainerWnd.h \
    _Win/UI/SceneView.h \
    Controls/WidgetControl.h \
    _Win/UI/StudioAppPrefsPage.h \
    _Win/UI/StudioPreferencesPropSheet.h \
    _Win/UI/StudioProjectSettingsPage.h \
    _Win/UI/ResetKeyframeValuesDlg.h \
    _Win/UI/GLVersionDlg.h \
    _Win/DragNDrop/DropProxy.h \
    Palettes/Inspector/ObjectListModel.h \
    Palettes/Inspector/ObjectBrowserView.h \
    _Win/Application/SubPresentationsDlg.h \
    Controls/ButtonControl.h \
    Controls/ToggleButton.h \
    Palettes/Timeline/IBreadCrumbProvider.h \
    Controls/BreadCrumbControl.h \
    Controls/TreeItem.h

FORMS += \
    _Win/UI/timeeditdlg.ui \
    _Win/UI/StudioAppPrefsPage.ui \
    _Win/UI/StudioPreferencesPropSheet.ui \
    _Win/UI/StudioProjectSettingsPage.ui \
    _Win/UI/InterpolationDlg.ui \
    _Win/UI/ResetKeyframeValuesDlg.ui \
    _Win/UI/GLVersionDlg.ui \
    Application/StudioTutorialWidget.ui \
    _Win/UI/TimeLineToolbar.ui

SOURCES += \
    MainFrm.cpp \
    PreviewHelper.cpp \
    remotedeploymentsender.cpp \
    _Win/Application/AboutDlg.cpp \
    _Win/Application/MsgRouter.cpp \
    _Win/Application/StudioApp.cpp \
    _Win/Controls/AppFonts.cpp \
    _Win/Controls/BufferedRenderer.cpp \
    _Win/Controls/OffscreenRenderer.cpp \
    _Win/Controls/WinRenderer.cpp \
    _Win/DragNDrop/DropProxy.cpp \
    _Win/Palettes/PaletteManager.cpp \
    _Win/Palettes/Progress/ProgressView.cpp \
    _Win/Palettes/Splash/SplashView.cpp \
    _Win/UI/EditCameraBar.cpp \
    _Win/UI/EditorPane.cpp \
    _Win/UI/GLVersionDlg.cpp \
    _Win/UI/InterpolationDlg.cpp \
    _Win/UI/PlayerContainerWnd.cpp \
    _Win/UI/PlayerWnd.cpp \
    _Win/UI/RecentItems.cpp \
    _Win/UI/ResetKeyframeValuesDlg.cpp \
    _Win/UI/SceneView.cpp \
    _Win/UI/StartupDlg.cpp \
    _Win/UI/StudioAppPrefsPage.cpp \
    #_Win/UI/StudioPaletteBar.cpp \
    _Win/UI/StudioPreferencesPropSheet.cpp \
    _Win/UI/StudioProjectSettingsPage.cpp \
    _Win/UI/TimeEditDlg.cpp \
    _Win/UI/TimeLineToolbar.cpp \
    _Win/Utils/MouseCursor.cpp \
    _Win/Workspace/Dialogs.cpp \
    _Win/Workspace/Views.cpp \
    Application/StudioTutorialWidget.cpp \
    Controls/BaseMeasure.cpp \
    Controls/BlankControl.cpp \
    Controls/BreadCrumbControl.cpp \
    Controls/ButtonControl.cpp \
    Controls/Control.cpp \
    Controls/ControlData.cpp \
    Controls/ControlGraph.cpp \
    Controls/FloatEdit.cpp \
    Controls/FlowLayout.cpp \
    Controls/InsertionLine.cpp \
    Controls/InsertionOverlay.cpp \
    Controls/LazyFlow.cpp \
    Controls/ListBoxItem.cpp \
    Controls/ListBoxStringItem.cpp \
    Controls/ListLayout.cpp \
    Controls/NameEdit.cpp \
    Controls/OverlayControl.cpp \
    Controls/Renderer.cpp \
    Controls/ScrollController.cpp \
    Controls/Scroller.cpp \
    Controls/ScrollerBackground.cpp \
    Controls/ScrollerBar.cpp \
    Controls/ScrollerButtonControl.cpp \
    Controls/ScrollerThumb.cpp \
    Controls/SIcon.cpp \
    Controls/SplashControl.cpp \
    Controls/SplitBar.cpp \
    Controls/Splitter.cpp \
    Controls/StringEdit.cpp \
    Controls/TextEdit.cpp \
    Controls/TextEditContextMenu.cpp \
    Controls/TextEditInPlace.cpp \
    Controls/TimeEdit.cpp \
    Controls/ToggleButton.cpp \
    Controls/TreeControl.cpp \
    Controls/TreeItem.cpp \
    DragAndDrop/BasicObjectDropSource.cpp \
    DragAndDrop/DropContainer.cpp \
    DragAndDrop/DropSource.cpp \
    DragAndDrop/DropTarget.cpp \
    DragAndDrop/ExplorerFileDropSource.cpp \
    DragAndDrop/FileDropSource.cpp \
    DragAndDrop/ListBoxDropSource.cpp \
    DragAndDrop/ListBoxDropTarget.cpp \
    DragAndDrop/ProjectDropTarget.cpp \
    DragAndDrop/SceneDropTarget.cpp \
    DragAndDrop/TimelineDropSource.cpp \
    DragAndDrop/TimelineDropTarget.cpp \
    Palettes/Action/ActionModel.cpp \
    Palettes/Action/ActionView.cpp \
    Palettes/Action/ActionContextMenu.cpp \
    Palettes/Action/EventsModel.cpp \
    Palettes/Action/EventsBrowserView.cpp \
    Palettes/Action/PropertyModel.cpp \
    Palettes/BasicObjects/BasicObjectsModel.cpp \
    Palettes/BasicObjects/BasicObjectsView.cpp \
    Palettes/Inspector/ChooserModelBase.cpp \
    Palettes/Inspector/EasyInspectorGroup.cpp \
    Palettes/Inspector/GuideInspectable.cpp \
    Palettes/Inspector/InspectableBase.cpp \
    Palettes/Inspector/InspectorGroup.cpp \
    Palettes/Inspector/ImageChooserView.cpp \
    Palettes/Inspector/FileChooserView.cpp \
    Palettes/Inspector/FileChooserModel.cpp \
    Palettes/Inspector/ImageChooserModel.cpp \
    Palettes/Inspector/MeshChooserModel.cpp \
    Palettes/Inspector/MeshChooserView.cpp \
    Palettes/Inspector/TextureChooserView.cpp \
    Palettes/Inspector/Qt3DSDMInspectable.cpp \
    Palettes/Inspector/Qt3DSDMInspectorGroup.cpp \
    Palettes/Inspector/Qt3DSDMInspectorRow.cpp \
    Palettes/Inspector/Qt3DSDMMaterialInspectable.cpp \
    Palettes/Inspector/Qt3DSDMSceneInspectable.cpp \
    Palettes/Inspector/InspectorControlView.cpp \
    Palettes/Inspector/InspectorControlModel.cpp \
    Palettes/Inspector/ObjectListModel.cpp \
    Palettes/Inspector/ObjectBrowserView.cpp \
    Palettes/Inspector/TabOrderHandler.cpp \
    Palettes/Inspector/MouseHelper.cpp \
    Palettes/Project/ProjectView.cpp \
    Palettes/Project/ProjectFileSystemModel.cpp \
    Palettes/Project/ProjectContextMenu.cpp \
    Palettes/Slide/SlideModel.cpp \
    Palettes/Slide/SlideView.cpp \
    Palettes/Slide/SlideContextMenu.cpp \
    Palettes/Timeline/AreaBoundingRect.cpp \
    Palettes/Timeline/AssetTimelineKeyframe.cpp \
    Palettes/Timeline/BaseStateRow.cpp \
    Palettes/Timeline/BaseTimebarlessRow.cpp \
    Palettes/Timeline/BaseTimelineTreeControl.cpp \
    Palettes/Timeline/BlankToggleControl.cpp \
    Palettes/Timeline/ColorBlankControl.cpp \
    Palettes/Timeline/ColorControl.cpp \
    Palettes/Timeline/CommentEdit.cpp \
    Palettes/Timeline/ComponentContextMenu.cpp \
    Palettes/Timeline/FilterToolbar.cpp \
    Palettes/Timeline/KeyframeContextMenu.cpp \
    Palettes/Timeline/Playhead.cpp \
    Palettes/Timeline/PropertyColorControl.cpp \
    Palettes/Timeline/PropertyGraphKeyframe.cpp \
    Palettes/Timeline/PropertyRow.cpp \
    Palettes/Timeline/PropertyTimebarGraph.cpp \
    Palettes/Timeline/PropertyTimebarRow.cpp \
    Palettes/Timeline/PropertyTimelineKeyframe.cpp \
    Palettes/Timeline/PropertyToggleControl.cpp \
    Palettes/Timeline/PropertyTreeControl.cpp \
    Palettes/Timeline/ScalableScroller.cpp \
    Palettes/Timeline/ScalableScrollerBar.cpp \
    Palettes/Timeline/SlideRow.cpp \
    Palettes/Timeline/SlideTimebarRow.cpp \
    Palettes/Timeline/Snapper.cpp \
    Palettes/Timeline/StateRow.cpp \
    Palettes/Timeline/StateRowFactory.cpp \
    Palettes/Timeline/StateTimebarlessRow.cpp \
    Palettes/Timeline/StateTimebarRow.cpp \
    Palettes/Timeline/TimebarControl.cpp \
    Palettes/Timeline/TimebarTip.cpp \
    Palettes/Timeline/TimelineControl.cpp \
    Palettes/Timeline/TimelineFilter.cpp \
    Palettes/Timeline/TimelineKeyframe.cpp \
    Palettes/Timeline/TimelineRow.cpp \
    Palettes/Timeline/TimelineSplitter.cpp \
    Palettes/Timeline/TimelineTimelineLayout.cpp \
    Palettes/Timeline/TimelineTreeLayout.cpp \
    Palettes/Timeline/TimeMeasure.cpp \
    Palettes/Timeline/ToggleBlankControl.cpp \
    Palettes/Timeline/ToggleControl.cpp \
    Palettes/Timeline/ToggleToolbar.cpp \
    Palettes/Timeline/TreeBlankControl.cpp \
    Palettes/Timeline/Bindings/BehaviorTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/EmptyTimelineTimebar.cpp \
    Palettes/Timeline/Bindings/GroupTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/ImageTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/KeyframesManager.cpp \
    Palettes/Timeline/Bindings/LayerTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/MaterialTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/OffsetKeyframesCommandHelper.cpp \
    Palettes/Timeline/Bindings/PathAnchorPointTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/PathTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/SlideTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/TimelineBreadCrumbProvider.cpp \
    Palettes/Timeline/Bindings/TimelineTranslationManager.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMAssetTimelineKeyframe.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMTimelineItemProperty.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMTimelineKeyframe.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMTimelineTimebar.cpp \
    Render/PathWidget.cpp \
    Render/StudioRenderer.cpp \
    Render/StudioRendererTranslation.cpp \
    Render/StudioRotationWidget.cpp \
    Render/StudioScaleWidget.cpp \
    Render/StudioTranslationWidget.cpp \
    Render/StudioWidget.cpp \
    Render/WGLRenderContext.cpp \
    #UI/PaletteState.cpp \
    Utils/CmdLineParser.cpp \
    Utils/ImportUtils.cpp \
    Utils/ResourceCache.cpp \
    Utils/StringLoader.cpp \
    Utils/StudioUtils.cpp \
    Utils/SystemPreferences.cpp \
    Utils/TickTock.cpp \
    Controls/ClickableLabel.cpp \
    Controls/WidgetControl.cpp \
    _Win/Application/SubPresentationsDlg.cpp

HEADERS += \
    _Win/UI/TimeEditDlg.h \
    _Win/UI/TimeLineToolbar.h \
    _Win/Application/StudioApp.h \
    Controls/TextEditContextMenu.h \
    Palettes/Action/ActionModel.h \
    Palettes/Action/ActionContextMenu.h \
    Palettes/Action/ActionView.h \
    Palettes/Action/EventsModel.h \
    Palettes/Action/EventsBrowserView.h \
    Palettes/Action/PropertyModel.h \
    Palettes/BasicObjects/BasicObjectsModel.h \
    Palettes/BasicObjects/BasicObjectsView.h \
    Palettes/Inspector/InspectorControlView.h \
    Palettes/Inspector/InspectorControlModel.h \
    Palettes/Slide/SlideModel.h \
    Palettes/Slide/SlideView.h \
    Palettes/Timeline/ComponentContextMenu.h \
    Palettes/Timeline/KeyframeContextMenu.h \
    Palettes/Timeline/Bindings/IKeyframeSelector.h \
    Palettes/Timeline/Bindings/ITimelineItemBinding.h \
    Palettes/Timeline/Bindings/ITimelineItem.h \
    Palettes/Timeline/Bindings/ITimelineItemProperty.h \
    Palettes/Timeline/Bindings/ITimelineKeyframesManager.h \
    Palettes/Timeline/Bindings/ITimelineTimebar.h \
    Palettes/Timeline/TimelineTimelineLayout.h \
    Palettes/Slide/SlideContextMenu.h \
    Controls/ClickableLabel.h \
    PreviewHelper.h \
    remotedeploymentsender.h \
    Application/StudioTutorialWidget.h \
    Palettes/Inspector/ChooserModelBase.h \
    Palettes/Inspector/ImageChooserView.h \
    Palettes/Inspector/ImageChooserModel.h \
    Palettes/Inspector/MeshChooserModel.h \
    Palettes/Inspector/MeshChooserView.h \
    Palettes/Inspector/FileChooserView.h \
    Palettes/Inspector/FileChooserModel.h \
    Palettes/Inspector/TextureChooserView.h \
    Palettes/Inspector/TabOrderHandler.h \
    Palettes/Inspector/MouseHelper.h \
    Palettes/Project/ProjectView.h \
    Palettes/Project/ProjectFileSystemModel.h \
    Palettes/Project/ProjectContextMenu.h \
    ../Common/Code/Graph/GraphPosition.h

FORMS += \
    MainFrm.ui \
    _Win/Application/AboutDlg.ui \
    _Win/UI/StartupDlg.ui \
    _Win/Application/SubPresentationsDlg.ui \
    _Win/Palettes/Progress/ProgressDlg.ui

RESOURCES += \
    MainFrm.qrc \
    qml.qrc \
    images.qrc \
    qt3dstudio.qrc

PREDEPS_LIBS += \
    qt3dsruntimestatic \
    QT3DSDM \
    CommonLib \
    CoreLib

include(../../utils.pri)
PRE_TARGETDEPS += $$fixLibPredeps($$LIBDIR, PREDEPS_LIBS)

# Bundle FBX for macOS
macos:!isEmpty(QMAKE_LIBS_FBX) {
    fbxlibpath = $$last(QMAKE_LIBS_FBX)
    fbxsdk.files = $$str_member($$fbxlibpath, 2, -1)/libfbxsdk.dylib
    fbxsdk.path = Contents/MacOS
    QMAKE_BUNDLE_DATA += fbxsdk
}

# Copy necessary resources

ABS_PRJ_ROOT = $$absolute_path($$PWD/../../..)
macos:ABS_DEST_DIR = $$absolute_path($$BINDIR)/$${TARGET}.app/Contents
!macos:ABS_DEST_DIR = $$absolute_path($$BINDIR)
macos:RES = "Resources"
!macos:RES = "res"

defineReplace(doReplaceResCopy_copy1) {
    filePath = $$absolute_path($$1)
    macos:filePath = $$replace(filePath, $$ABS_PRJ_ROOT/Studio, $$ABS_DEST_DIR/$$RES)
    !macos:filePath = $$replace(filePath, $$ABS_PRJ_ROOT/Studio, $$ABS_DEST_DIR)
    PRE_TARGETDEPS += $$filePath
    export(PRE_TARGETDEPS)
    return($$system_path($$filePath))
}
defineReplace(doReplaceResCopy_copy2) {
    filePath = $$absolute_path($$1)
    macos:filePath = $$replace(filePath, $$ABS_PRJ_ROOT/Studio, $$ABS_DEST_DIR/$$RES)
    !macos:filePath = $$replace(filePath, $$ABS_PRJ_ROOT/Studio, $$ABS_DEST_DIR)
    PRE_TARGETDEPS += $$filePath
    export(PRE_TARGETDEPS)
    return($$system_path($$filePath))
}

defineReplace(addFilesToResources) {
    # Remove directories from results
    input_files = $$files($$2, true)
    for(input_file, input_files) {
        last_part = $$split(input_file, /)
        last_part = $$last(last_part)
        last_split = $$split(last_part, .)
        split_size = $$size(last_split)
        equals(split_size, 2): $${1}.input_files += $$input_file
    }
    $${1}.input = $${1}.input_files
    $${1}.commands = $(COPY_FILE) "${QMAKE_FILE_IN}" "${QMAKE_FILE_OUT}"
    $${1}.CONFIG = no_link
    $${1}.name = $$1
    $${1}.output_function = doReplaceResCopy_$${1}

    export($${1}.input_files)
    export($${1}.input)
    export($${1}.output_function)
    export($${1}.commands)
    export($${1}.CONFIG)
    export($${1}.name)

    $${1}_install.files = $$2
    $${1}_install.path = $$[QT_INSTALL_BINS]/$$3
    INSTALLS += $${1}_install
    export($${1}_install.files)
    export($${1}_install.path)
    export(INSTALLS)

    return($$1)
}

QMAKE_EXTRA_COMPILERS += $$addFilesToResources("copy1", $$PWD/../../../Studio/Content/*, Content)
QMAKE_EXTRA_COMPILERS += $$addFilesToResources("copy2", "$$PWD/../../../Studio/Build Configurations/*", "Build Configurations")

CONFIG += exceptions

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

RC_ICONS = images/3D-studio.ico
ICON = images/studio.icns

# Extract SHA from git if building sources from git repository
exists($$ABS_PRJ_ROOT/.git) {
    GIT_SHA = $$system(git rev-list --abbrev-commit -n1 HEAD)
}
# Otherwise attempt to extract SHA from .tag file
isEmpty(GIT_SHA):exists($$ABS_PRJ_ROOT/.tag) {
    STUDIO_TAG = $$cat($$ABS_PRJ_ROOT/.tag)
    FIRST_CHAR = $$str_member($$STUDIO_TAG, 0, 0)
    !equals(FIRST_CHAR, "$"): GIT_SHA = $$first(STUDIO_TAG)
}
!isEmpty(GIT_SHA): DEFINES += QT3DSTUDIO_REVISION=$$GIT_SHA
