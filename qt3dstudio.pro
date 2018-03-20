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

            # windeployqt will get confused when deploying viewer 1.0, because some of studio
            # libraries it links detect as Qt libraries due to their naming scheme.
            # Since viewer and studio have mostly identical Qt dependencies and both are deployed
            # to the same directory, we can just omit deployment of viewer in windows and
            # deploy the missing qml bits separately.

            # Viewer 1.0 needs the studio qml plugin
            debug_and_release|CONFIG(release, debug|release) {
                QML_FILE_R = QtStudio3D/declarative_qtstudio3d.dll
                QMAKE_EXTRA_TARGETS += deployReleaseQml
                deployTarget.depends += deployReleaseQml
                deployReleaseQml.depends = mkStudioQmlDir
                deployReleaseQml.commands = \
                    $$QMAKE_COPY $$shell_quote($$shell_path($$OUT_PWD/qml/$$QML_FILE_R)) \
                    $$shell_quote($$shell_path(\$(DEPLOY_DIR)/$$QML_FILE_R))
            }
            debug_and_release|CONFIG(debug, debug|release) {
                QML_FILE_D = QtStudio3D/declarative_qtstudio3dd.dll
                QMAKE_EXTRA_TARGETS += deployDebugQml
                deployTarget.depends += deployDebugQml
                deployDebugQml.depends += mkStudioQmlDir
                deployDebugQml.commands = \
                    $$QMAKE_COPY $$shell_quote($$shell_path($$OUT_PWD/qml/$$QML_FILE_D)) \
                    $$shell_quote($$shell_path(\$(DEPLOY_DIR)/$$QML_FILE_D))
            }
            QMLDIR_FILE = QtStudio3D/qmldir
            QMAKE_EXTRA_TARGETS += deployQmldir
            deployTarget.depends += deployQmldir
            deployQmldir.depends += mkStudioQmlDir
            deployQmldir.commands = \
                $$QMAKE_COPY $$shell_quote($$shell_path($$OUT_PWD/qml/$$QMLDIR_FILE)) \
                $$shell_quote($$shell_path(\$(DEPLOY_DIR)/$$QMLDIR_FILE))

            QMAKE_EXTRA_TARGETS += mkStudioQmlDir
            mkStudioQmlDir.commands = \
                $$sprintf($$QMAKE_MKDIR_CMD, $$shell_quote($$shell_path(\$(DEPLOY_DIR)/QtStudio3D)))

            # Deploy viewer 1.0 qml dependencies when deploying studio
            EXTRA_DEPLOY_OPTIONS = -qmldir=$$shell_quote($$PWD/src/Viewer/Qt3DViewer)
        }

        qtPrepareTool(DEPLOY_TOOL, $$deploytool)

        deployTarget.target = deployqt
        deployTarget.commands = \
            $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/Qt3DStudio$${exesuffix}) \
                -qmldir=$$shell_quote($$PWD/src/Authoring/Studio/Palettes) $$EXTRA_DEPLOY_OPTIONS

        macos {
            deployTarget.commands += && \
                $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/Qt3DViewer$${exesuffix}) \
                -qmldir=$$shell_quote($$PWD/src/Viewer/Qt3DViewer)
        }

        QMAKE_EXTRA_TARGETS += deployTarget
    }
}
