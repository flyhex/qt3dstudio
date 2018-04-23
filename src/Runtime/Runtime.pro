TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    Qt3DSRuntimeStatic \
    Qt3DSRuntime

win32 {
SUBDIRS += \
    AttributeHashes
}

# Qt3D based runtime needs Qt 5.11 or later
greaterThan(QT_MAJOR_VERSION, 5)|greaterThan(QT_MINOR_VERSION, 10) {
    SUBDIRS += qt3d-runtime

    # HACK to ensure syncqt is run for qt3d-runtime if one is run for studio
    # This is necessary because CI doesn't create dummy .git directories for submodules
    if(!build_pass|!debug_and_release) {
        # Pro file existence is checked to ensure we don't create dummy .git in empty submodule dir,
        # which happens when studio is cloned without --recursive option
        exists($$PWD/../../.git):!exists($$PWD/qt3d-runtime/.git):exists($$PWD/qt3d-runtime/qt3d-runtime.pro) {
            MAKE_DIR_CMD = $$sprintf($$QMAKE_MKDIR_CMD, $$shell_quote($$shell_path($$PWD/qt3d-runtime/.git)))
            $$system("$$MAKE_DIR_CMD")
        }
    }
}
