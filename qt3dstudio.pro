requires(!ios)
requires(!winrt)
requires(!tvos)
requires(!watchos)
requires(!win32-msvc2013)

requires(qtHaveModule(widgets))
requires(qtHaveModule(multimedia))
requires(qtHaveModule(quick))
requires(qtHaveModule(qml))
requires(qtHaveModule(opengl))

SUBDIRS += \
    doc

load(qt_parts)

# 'deployqt' target can be used to automatically copy necessary Qt libraries needed by studio
# applications. DEPLOY_DIR environment variable must point to the directory where
# Qt3DStudio and Qt3DViewer executables/application bundles are found.
# The required Qt libraries are copied into that directory/bundles.
!build_pass|!debug_and_release {
    macos|win32 {
        macos {
            deploytool = macdeployqt
            exesuffix = .app
        } else:win32 {
            deploytool = windeployqt
            exesuffix = .exe
        }

        qtPrepareTool(DEPLOY_TOOL, $$deploytool)

        install_dir =
        deployTarget.target = deployqt
        deployTarget.commands = \
            $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/Qt3DStudio$${exesuffix}) \
                -qmldir=$$shell_quote($$PWD/src/Authoring/Studio/Palettes) && \
            $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/Qt3DViewer$${exesuffix}) \
                -qmldir=$$shell_quote($$PWD/src/Viewer/Qt3DViewer)
        QMAKE_EXTRA_TARGETS += deployTarget
    }
}
