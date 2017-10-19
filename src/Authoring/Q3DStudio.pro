TEMPLATE = app
TARGET = Qt3DStudio
include($$PWD/commoninclude.pri)
include($$OUT_PWD/qtAuthoring-config.pri)
CONFIG += nostrictstrings
DEFINES += _UNICODE UNICODE QT3DS_AUTHORING _AFXDLL \
    PCRE_STATIC BOOST_SIGNALS_NO_DEPRECATION_WARNING EASTL_MINMAX_ENABLED=0 \
    EASTL_NOMINMAX=0 DOM_DYNAMIC DRIVE_DESIGN_STUDIO

win: QMAKE_LFLAGS += /MANIFEST /ENTRY:"wWinMainCRTStartup"

QT += core gui openglextensions
QT += qml quick widgets quickwidgets network

INCLUDEPATH += \
    Studio/_Win/Include \
    Studio/_Win/Application \
    Studio/_Win/Controls \
    Studio/_Win/DragNDrop \
    Studio/_Win/Palettes \
    Studio/_Win/Palettes/Progress \
    Studio/_Win/Palettes/Splash \
    Studio/_Win/UI \
    Studio/_Win/Utils \
    Studio/Application \
    Studio/Controls \
    Studio/DragAndDrop \
    Studio/Palettes \
    Studio/Palettes/Action \
    Studio/Palettes/Action/ActionParamRow \
    Studio/Palettes/BasicObjects \
    Studio/Palettes/Inspector \
    Studio/Palettes/Master \
    Studio/Palettes/Progress \
    Studio/Palettes/Project \
    Studio/Palettes/Slide \
    Studio/Palettes/Timeline \
    Studio/Render \
    Studio/UI \
    Studio/Utils \
    Studio/Workspace \
    Studio/Workspace/Views \
    Studio \
    Studio/English.lproj/Strings \
    QT3DSIMP/Qt3DSImportLib \
    QT3DSIMP/Qt3DSImportSGTranslation \
    QT3DSDM/Systems \
    Common/Code/Thread \
    Common/Code/IO \
    Common/Code \
    Common/Code/Exceptions \
    Common/Code/_Win32/Include \
    Common/Code/_Win32 \
    Common/Code/Graph \
    Common/Code/Report \
    Common/Code/Memory \
    Client/Code/Core/Utility \
    Client/Code/Core/Types \
    Client/Code/Core/Commands \
    Client/Code/Core/Core \
    Client/Code/Core \
    Client/Code/Core/Doc \
    Client/Code/Core/Doc/ClientDataModelBridge \
    Client/Code/Shared \
    Client/Code/Shared/Log \
    $$PWD/../Runtime/Source/Qt3DSRender/Include \
    $$PWD/../Runtime/Source/Qt3DSFoundation/Include \
    $$PWD/../Runtime/Source/Qt3DSRuntimeRender/Include \
    $$PWD/../Runtime/Source/Qt3DSRuntimeRender/GraphObjects \
    $$PWD/../Runtime/Source/Qt3DSRuntimeRender/ResourceManager \
    $$PWD/../Runtime/Source/Qt3DSStateApplication/Application \
    $$PWD/../Runtime/Source/Qt3DSEvent/InternalInclude \
    ../3rdparty/EASTL/UnknownVersion/include \
    ../3rdparty/color


LIBS += \
    -L"$$BOOSTDIR"

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
    QMAKE_LFLAGS += /NODEFAULTLIB:tinyxml.lib
}

LIBS += \
      -lqt3dsruntimestatic$$qtPlatformTargetSuffix() \
      -l$$BOOSTSIGNALLIB \
      -l$$BOOSTSYSTEMLIB \
      -l$$BOOSTFILESYSTEMLIB \
       $$QMAKE_LIBS_FBX

linux {
    LIBS += \
        -ldl \
        -lEGL
}

win: PRECOMPILED_HEADER = Studio/_Win/Studio/stdafx.h

include(Q3DStudio.pri)


DISTFILES += \
    Studio/Palettes/BasicObjects/BasicObjectsView.qml \
    Studio/Palettes/BasicObjects/SlideView.qml

FORMS += \
    Studio/MainFrm.ui \
    Studio/_Win/Application/AboutDlg.ui \
    Studio/_Win/UI/StartupDlg.ui \
    Studio/_Win/Application/SubPresentationsDlg.ui \
    Studio/_Win/Palettes/Progress/ProgressDlg.ui

RESOURCES += \
    Studio/MainFrm.qrc \
    qml.qrc \
    Studio/images.qrc

PREDEPS_LIBS += \
    qt3dsruntimestatic \
    QT3DSDM \
    CommonLib \
    CoreLib


# Optional license handler
isEmpty(TQTC_LICENSE_MANAGING): TQTC_LICENSE_MANAGING=$$(TQTC_LICENSE_MANAGING)
!isEmpty(TQTC_LICENSE_MANAGING) {
    DEFINES += USE_LICENSE_HANDLER LICENSE_HANDLER_AS_DLL
    LIBS += -llicensehandler$$qtPlatformTargetSuffix()
    INCLUDEPATH += $$TQTC_LICENSE_MANAGING/studio3d
}

include($$PWD/../utils.pri)
PRE_TARGETDEPS += $$fixLibPredeps($$LIBDIR, PREDEPS_LIBS)

# Copy necessary resources

ABS_PRJ_ROOT = $$absolute_path($$PWD/../..)
macos:ABS_DEST_DIR = $$absolute_path($$BINDIR)/$${TARGET}.app/Contents
!macos:ABS_DEST_DIR = $$absolute_path($$BINDIR)
macos:RES = "Resources"
!macos:RES = "res"

defineReplace(doReplaceResCopy_copy1) {
    filePath = $$absolute_path($$1)
    filePath = $$replace(filePath, $$ABS_PRJ_ROOT/Studio, $$ABS_DEST_DIR)
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

QMAKE_EXTRA_COMPILERS += $$addFilesToResources("copy1", $$PWD/../../Studio/Content/*, Content)
QMAKE_EXTRA_COMPILERS += $$addFilesToResources("copy2", "$$PWD/../../Studio/Build Configurations/*", "Build Configurations")

CONFIG += exceptions
DISTFILES +=

RESOURCES += \
    Studio/qt3dstudio.qrc

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

RC_ICONS = Studio/images/3D-studio.ico
