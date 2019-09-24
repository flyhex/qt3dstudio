TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += ogl-runtime

# HACK to ensure syncqt is run for ogl-runtime if one is run for studio
# This is necessary because CI doesn't create dummy .git directories for submodules
if(!build_pass|!debug_and_release) {
    # Pro file existence is checked to ensure we don't create dummy .git in empty submodule dir,
    # which happens when studio is cloned without --recursive option
    exists($$PWD/../../.git):!exists($$PWD/ogl-runtime/.git):exists($$PWD/ogl-runtime/ogl-runtime.pro) {
        MAKE_DIR_CMD = $$sprintf($$QMAKE_MKDIR_CMD, $$shell_quote($$shell_path($$PWD/ogl-runtime/.git)))
        $$system("$$MAKE_DIR_CMD")
    }
}
