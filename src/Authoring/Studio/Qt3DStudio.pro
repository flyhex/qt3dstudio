TEMPLATE = app
TARGET = Qt3DStudio
include(../commoninclude.pri)
include($$OUT_PWD/../qtAuthoring-config.pri)
include(../../shared/qtsingleapplication/qtsingleapplication.pri)
INCLUDEPATH += $$OUT_PWD/..

CONFIG += nostrictstrings

DEFINES += _UNICODE UNICODE QT3DS_AUTHORING _AFXDLL \
    PCRE_STATIC EASTL_MINMAX_ENABLED=0 \
    EASTL_NOMINMAX=0 DOM_DYNAMIC

win: QMAKE_LFLAGS += /MANIFEST /ENTRY:"wWinMainCRTStartup"

QT += core gui xml openglextensions
QT += qml quick widgets quickwidgets network

INCLUDEPATH += \
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
    Palettes/TimelineGraphicsView \
    Palettes/TimelineGraphicsView/ui \
    Render \
    UI \
    Utils \
    Workspace \
    Workspace/Views \
    . \
    .. \
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
    ../../Runtime/Source/Qt3DSFoundation/Include/foundation \
    ../../Runtime/Source/Qt3DSRuntimeRender/Include \
    ../../Runtime/Source/Qt3DSRuntimeRender/GraphObjects \
    ../../Runtime/Source/Qt3DSRuntimeRender/ResourceManager \
    ../../Runtime/Source/Qt3DSStateApplication/Application \
    ../../Runtime/Source/Qt3DSEvent/InternalInclude \
    ../../3rdparty/EASTL/UnknownVersion/include \
    ../../3rdparty/color \
    ../../QtExtras/qmlstreamer

linux {
    BEGIN_ARCHIVE = -Wl,--whole-archive
    END_ARCHIVE = -Wl,--no-whole-archive
}

STATICRUNTIME = \
    $$BEGIN_ARCHIVE \
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
      -lqt3dsqmlstreamer$$qtPlatformTargetSuffix() \
       $$QMAKE_LIBS_FBX

linux {
    LIBS += \
        -ldl \
        -lEGL
}

win: PRECOMPILED_HEADER = ../Common/Code/Qt3DSCommonPrecompile.h

HEADERS += \
    MainFrm.h \
    Application/AboutDlg.h \
    Application/DataInputDlg.h \
    Application/DataInputListDlg.h \
    Application/DataInputSelectModel.h \
    Application/DataInputSelectView.h \
    Application/DurationEditDlg.h \
    Application/StudioApp.h \
    Application/StudioTutorialWidget.h \
    Application/TimeEditDlg.h \
    Application/TimeEnums.h \
    Controls/ClickableLabel.h \
    Controls/WidgetControl.h \
    DragAndDrop/DropProxy.h \
    Palettes/PaletteManager.h \
    Palettes/Action/ActionContextMenu.h \
    Palettes/Action/ActionModel.h \
    Palettes/Action/ActionView.h \
    Palettes/Action/EventsBrowserView.h \
    Palettes/Action/EventsModel.h \
    Palettes/Action/PropertyModel.h \
    Palettes/BasicObjects/BasicObjectsModel.h \
    Palettes/BasicObjects/BasicObjectsView.h \
    Palettes/Inspector/ChooserModelBase.h \
    Palettes/Inspector/FileChooserModel.h \
    Palettes/Inspector/FileChooserView.h \
    Palettes/Inspector/ImageChooserModel.h \
    Palettes/Inspector/ImageChooserView.h \
    Palettes/Inspector/InspectorControlModel.h \
    Palettes/Inspector/InspectorControlView.h \
    Palettes/Inspector/MeshChooserModel.h \
    Palettes/Inspector/MeshChooserView.h \
    Palettes/Inspector/MouseHelper.h \
    Palettes/Inspector/ObjectBrowserView.h \
    Palettes/Inspector/ObjectListModel.h \
    Palettes/Inspector/TabOrderHandler.h \
    Palettes/Inspector/TextureChooserView.h \
    Palettes/Project/ProjectContextMenu.h \
    Palettes/Project/ProjectFileSystemModel.h \
    Palettes/Project/ProjectView.h \
    Palettes/Slide/SlideContextMenu.h \
    Palettes/Slide/SlideModel.h \
    Palettes/Slide/SlideView.h \
    Palettes/Timeline/Bindings/ITimelineItem.h \
    Palettes/Timeline/Bindings/ITimelineItemBinding.h \
    Palettes/Timeline/Bindings/ITimelineItemProperty.h \
    Palettes/Timeline/Bindings/ITimelineKeyframesManager.h \
    Palettes/Timeline/Bindings/ITimelineTimebar.h \
    Palettes/Timeline/Bindings/IBreadCrumbProvider.h \
    Palettes/TimelineGraphicsView/Keyframe.h \
    Palettes/TimelineGraphicsView/KeyframeManager.h \
    Palettes/TimelineGraphicsView/RowManager.h \
    Palettes/TimelineGraphicsView/RowMover.h \
    Palettes/TimelineGraphicsView/rowtypes.h \
    Palettes/TimelineGraphicsView/SelectionRect.h \
    Palettes/TimelineGraphicsView/TimelineConstants.h \
    Palettes/TimelineGraphicsView/TimelineControl.h \
    Palettes/TimelineGraphicsView/TimelineGraphicsScene.h \
    Palettes/TimelineGraphicsView/TimelineSplitter.h \
    Palettes/TimelineGraphicsView/TimelineWidget.h \
    Palettes/TimelineGraphicsView/ui/InteractiveTimelineItem.h \
    Palettes/TimelineGraphicsView/ui/NavigationBar.h \
    Palettes/TimelineGraphicsView/ui/NavigationBarItem.h \
    Palettes/TimelineGraphicsView/ui/PlayHead.h \
    Palettes/TimelineGraphicsView/ui/RowTimeline.h \
    Palettes/TimelineGraphicsView/ui/RowTimelineContextMenu.h \
    Palettes/TimelineGraphicsView/ui/RowTimelinePropertyGraph.h \
    Palettes/TimelineGraphicsView/ui/RowTree.h \
    Palettes/TimelineGraphicsView/ui/RowTreeContextMenu.h \
    Palettes/TimelineGraphicsView/ui/RowTreeLabelItem.h \
    Palettes/TimelineGraphicsView/ui/Ruler.h \
    Palettes/TimelineGraphicsView/ui/TimelineItem.h \
    Palettes/TimelineGraphicsView/ui/TimelineToolbar.h \
    Palettes/TimelineGraphicsView/ui/TimelineToolbarLabel.h \
    Palettes/TimelineGraphicsView/ui/TreeHeader.h \
    Palettes/TimelineGraphicsView/ui/TreeHeaderView.h \
    PreviewHelper.h \
    remotedeploymentsender.h \
    Render/StudioGradientWidget.h \
    Render/StudioVisualAidWidget.h \
    UI/EditCameraBar.h \
    UI/GLVersionDlg.h \
    UI/InterpolationDlg.h \
    UI/PlayerContainerWnd.h \
    UI/PlayerWnd.h \
    UI/RecentItems.h \
    UI/ResetKeyframeValuesDlg.h \
    UI/SceneView.h \
    UI/StartupDlg.h \
    UI/StudioAppPrefsPage.h \
    UI/StudioPreferencesPropSheet.h \
    UI/StudioProjectSettingsPage.h \
    ../Common/Code/Graph/GraphPosition.h \
    Palettes/Project/EditPresentationIdDlg.h \
    Application/ProjectFile.h \
    Application/PresentationFile.h \
    Palettes/Project/ChooseImagePropertyDlg.h

FORMS += \
    MainFrm.ui \
    Application/TimeEditDlg.ui \
    Application/DataInputDlg.ui \
    Application/DataInputListDlg.ui \
    Application/DurationEditDlg.ui \
    Application/StudioTutorialWidget.ui \
    Application/AboutDlg.ui \
    Palettes/Progress/ProgressDlg.ui \
    UI/StudioAppPrefsPage.ui \
    UI/StudioPreferencesPropSheet.ui \
    UI/StudioProjectSettingsPage.ui \
    UI/InterpolationDlg.ui \
    UI/ResetKeyframeValuesDlg.ui \
    UI/GLVersionDlg.ui \
    UI/StartupDlg.ui \
    Palettes/Project/EditPresentationIdDlg.ui \
    Palettes/Project/ChooseImagePropertyDlg.ui

SOURCES += \
    Application/AboutDlg.cpp \
    Application/DataInputDlg.cpp \
    Application/DataInputListDlg.cpp \
    Application/DataInputSelectModel.cpp \
    Application/DataInputSelectView.cpp \
    Application/DurationEditDlg.cpp \
    Application/MsgRouter.cpp \
    Application/StudioApp.cpp \
    Application/StudioTutorialWidget.cpp \
    Application/TimeEditDlg.cpp \
    Controls/AppFonts.cpp \
    Controls/BufferedRenderer.cpp \
    Controls/ClickableLabel.cpp \
    Controls/Control.cpp \
    Controls/ControlData.cpp \
    Controls/ControlGraph.cpp \
    Controls/OffscreenRenderer.cpp \
    Controls/Renderer.cpp \
    Controls/WidgetControl.cpp \
    Controls/WinRenderer.cpp \
    DragAndDrop/BasicObjectDropSource.cpp \
    DragAndDrop/DropContainer.cpp \
    DragAndDrop/DropProxy.cpp \
    DragAndDrop/DropSource.cpp \
    DragAndDrop/DropTarget.cpp \
    DragAndDrop/ExplorerFileDropSource.cpp \
    DragAndDrop/FileDropSource.cpp \
    DragAndDrop/ProjectDropTarget.cpp \
    DragAndDrop/SceneDropTarget.cpp \
    DragAndDrop/TimelineDropSource.cpp \
    DragAndDrop/TimelineDropTarget.cpp \
    MainFrm.cpp \
    Palettes/Action/ActionContextMenu.cpp \
    Palettes/Action/ActionModel.cpp \
    Palettes/Action/ActionView.cpp \
    Palettes/Action/EventsBrowserView.cpp \
    Palettes/Action/EventsModel.cpp \
    Palettes/Action/PropertyModel.cpp \
    Palettes/BasicObjects/BasicObjectsModel.cpp \
    Palettes/BasicObjects/BasicObjectsView.cpp \
    Palettes/Inspector/ChooserModelBase.cpp \
    Palettes/Inspector/EasyInspectorGroup.cpp \
    Palettes/Inspector/FileChooserModel.cpp \
    Palettes/Inspector/FileChooserView.cpp \
    Palettes/Inspector/GuideInspectable.cpp \
    Palettes/Inspector/ImageChooserModel.cpp \
    Palettes/Inspector/ImageChooserView.cpp \
    Palettes/Inspector/InspectableBase.cpp \
    Palettes/Inspector/InspectorControlModel.cpp \
    Palettes/Inspector/InspectorControlView.cpp \
    Palettes/Inspector/InspectorGroup.cpp \
    Palettes/Inspector/MeshChooserModel.cpp \
    Palettes/Inspector/MeshChooserView.cpp \
    Palettes/Inspector/MouseHelper.cpp \
    Palettes/Inspector/ObjectBrowserView.cpp \
    Palettes/Inspector/ObjectListModel.cpp \
    Palettes/Inspector/Qt3DSDMInspectable.cpp \
    Palettes/Inspector/Qt3DSDMInspectorGroup.cpp \
    Palettes/Inspector/Qt3DSDMInspectorRow.cpp \
    Palettes/Inspector/Qt3DSDMMaterialInspectable.cpp \
    Palettes/Inspector/Qt3DSDMSceneInspectable.cpp \
    Palettes/Inspector/TabOrderHandler.cpp \
    Palettes/Inspector/TextureChooserView.cpp \
    Palettes/PaletteManager.cpp \
    Palettes/Progress/ProgressView.cpp \
    Palettes/Project/ProjectContextMenu.cpp \
    Palettes/Project/ProjectFileSystemModel.cpp \
    Palettes/Project/ProjectView.cpp \
    Palettes/Slide/SlideContextMenu.cpp \
    Palettes/Slide/SlideModel.cpp \
    Palettes/Slide/SlideView.cpp \
    Palettes/Timeline/Bindings/BehaviorTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/EmptyTimelineTimebar.cpp \
    Palettes/Timeline/Bindings/GroupTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/ImageTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/LayerTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/MaterialTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/OffsetKeyframesCommandHelper.cpp \
    Palettes/Timeline/Bindings/PathAnchorPointTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/PathTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMAssetTimelineKeyframe.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMTimelineItemProperty.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMTimelineKeyframe.cpp \
    Palettes/Timeline/Bindings/Qt3DSDMTimelineTimebar.cpp \
    Palettes/Timeline/Bindings/SlideTimelineItemBinding.cpp \
    Palettes/Timeline/Bindings/TimelineBreadCrumbProvider.cpp \
    Palettes/Timeline/Bindings/TimelineTranslationManager.cpp \
    Palettes/TimelineGraphicsView/KeyframeManager.cpp \
    Palettes/TimelineGraphicsView/RowManager.cpp \
    Palettes/TimelineGraphicsView/RowMover.cpp \
    Palettes/TimelineGraphicsView/SelectionRect.cpp \
    Palettes/TimelineGraphicsView/TimelineControl.cpp \
    Palettes/TimelineGraphicsView/TimelineGraphicsScene.cpp \
    Palettes/TimelineGraphicsView/TimelineSplitter.cpp \
    Palettes/TimelineGraphicsView/TimelineWidget.cpp \
    Palettes/TimelineGraphicsView/ui/InteractiveTimelineItem.cpp \
    Palettes/TimelineGraphicsView/ui/NavigationBar.cpp \
    Palettes/TimelineGraphicsView/ui/NavigationBarItem.cpp \
    Palettes/TimelineGraphicsView/ui/PlayHead.cpp \
    Palettes/TimelineGraphicsView/ui/RowTimeline.cpp \
    Palettes/TimelineGraphicsView/ui/RowTimelineContextMenu.cpp \
    Palettes/TimelineGraphicsView/ui/RowTimelinePropertyGraph.cpp \
    Palettes/TimelineGraphicsView/ui/RowTree.cpp \
    Palettes/TimelineGraphicsView/ui/RowTreeContextMenu.cpp \
    Palettes/TimelineGraphicsView/ui/RowTreeLabelItem.cpp \
    Palettes/TimelineGraphicsView/ui/Ruler.cpp \
    Palettes/TimelineGraphicsView/ui/TimelineItem.cpp \
    Palettes/TimelineGraphicsView/ui/TimelineToolbar.cpp \
    Palettes/TimelineGraphicsView/ui/TimelineToolbarLabel.cpp \
    Palettes/TimelineGraphicsView/ui/TreeHeader.cpp \
    Palettes/TimelineGraphicsView/ui/TreeHeaderView.cpp \
    PreviewHelper.cpp \
    remotedeploymentsender.cpp \
    Render/PathWidget.cpp \
    Render/StudioGradientWidget.cpp \
    Render/StudioRenderer.cpp \
    Render/StudioRendererTranslation.cpp \
    Render/StudioRotationWidget.cpp \
    Render/StudioScaleWidget.cpp \
    Render/StudioTranslationWidget.cpp \
    Render/StudioVisualAidWidget.cpp \
    Render/StudioWidget.cpp \
    Render/WGLRenderContext.cpp \
    UI/EditCameraBar.cpp \
    UI/EditorPane.cpp \
    UI/GLVersionDlg.cpp \
    UI/InterpolationDlg.cpp \
    UI/PlayerContainerWnd.cpp \
    UI/PlayerWnd.cpp \
    UI/RecentItems.cpp \
    UI/ResetKeyframeValuesDlg.cpp \
    UI/SceneView.cpp \
    UI/StartupDlg.cpp \
    UI/StudioAppPrefsPage.cpp \
    UI/StudioPreferencesPropSheet.cpp \
    UI/StudioProjectSettingsPage.cpp \
    Utils/ImportUtils.cpp \
    Utils/MouseCursor.cpp \
    Utils/ResourceCache.cpp \
    Utils/StudioUtils.cpp \
    Utils/SystemPreferences.cpp \
    Utils/TickTock.cpp \
    Workspace/Dialogs.cpp \
    Workspace/Views.cpp \
    Palettes/Project/EditPresentationIdDlg.cpp \
    Application/ProjectFile.cpp \
    Application/PresentationFile.cpp \
    Palettes/Project/ChooseImagePropertyDlg.cpp

RESOURCES += \
    MainFrm.qrc \
    qml.qrc \
    images.qrc

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

macos {
    QMAKE_INFO_PLIST = Info.plist
}

# Copy necessary resources
ABS_PRJ_ROOT = $$absolute_path($$PWD/../../..)
macos:ABS_DEST_DIR = $$absolute_path($$BINDIR)/$${TARGET}.app/Contents/Resources
!macos:ABS_DEST_DIR = $$absolute_path($$BINDIR)

copy_content.files = $$PWD/../../../Studio/*
copy_content.path = $$ABS_DEST_DIR
COPIES += copy_content

install_content.files = $$PWD/../../../Studio/*
install_content.path = $$[QT_INSTALL_BINS]
INSTALLS += install_content

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

# Get a unique version identifying integer
STUDIO_MAJOR_VERSION = $$section(MODULE_VERSION, '.', 0, 0)
STUDIO_MINOR_VERSION = $$section(MODULE_VERSION, '.', 1, 1)
STUDIO_PATCH_VERSION = $$section(MODULE_VERSION, '.', 2, 2)
DEFINES += \
    STUDIO_VERSION_NUM=$${STUDIO_MAJOR_VERSION}0$${STUDIO_MINOR_VERSION}0$${STUDIO_PATCH_VERSION}
