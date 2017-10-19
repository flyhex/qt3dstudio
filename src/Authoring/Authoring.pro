TEMPLATE = subdirs
CONFIG += ordered

# Optional license handler
isEmpty(TQTC_LICENSE_MANAGING): TQTC_LICENSE_MANAGING=$$(TQTC_LICENSE_MANAGING)
!isEmpty(TQTC_LICENSE_MANAGING): SUBDIRS += licensehandler.pro

SUBDIRS += \
    QT3DSDM.pro \
    Qt3DSState.pro \
    CoreLib.pro \
    CommonLib.pro \
    Q3DStudio.pro \
    PresentationCompiler.pro

win32 {
    SUBDIRS += MorphLines.pro
    qtConfig(qt3dstudio-fbx): SUBDIRS += FBXLineExporter.pro
}
