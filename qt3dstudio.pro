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
# Qt3DStudio and Qt3DViewer executables/application bundles are installed to.
# The required Qt libraries are copied into that directory/bundles.
!build_pass|!debug_and_release {
    !mingw:win32|macos {
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
            # The assumption is that we are deploying release build in case both are built
            release {
                QML_FILE_R = QtStudio3D/declarative_qtstudio3d.dll
                QMAKE_EXTRA_TARGETS += deployReleaseQml
                deployTarget.depends += deployReleaseQml
                deployReleaseQml.depends = mkStudioQmlDir
                deployReleaseQml.commands = \
                    $$QMAKE_COPY $$shell_quote($$shell_path($$OUT_PWD/qml/$$QML_FILE_R)) \
                    $$shell_quote($$shell_path(\$(DEPLOY_DIR)/$$QML_FILE_R))
            } else {
                QML_FILE_D = QtStudio3D/declarative_qtstudio3dd.dll
                QMAKE_EXTRA_TARGETS += deployDebugQml
                deployTarget.depends += deployDebugQml
                deployDebugQml.depends += mkStudioQmlDir
                deployDebugQml.commands = \
                    $$QMAKE_COPY $$shell_quote($$shell_path($$OUT_PWD/qml/$$QML_FILE_D)) \
                    $$shell_quote($$shell_path(\$(DEPLOY_DIR)/$$QML_FILE_D))
            }

            # copy QtStudio3D.dll
            release {
                QTSTUDIO3D_LIB = Qt5Studio3D.dll
            } else {
                QTSTUDIO3D_LIB = Qt5Studio3Dd.dll
            }
            QMAKE_EXTRA_TARGETS += copyStudio3D
            deployTarget.depends += copyStudio3D
            copyStudio3D.commands = \
                $$QMAKE_COPY $$shell_quote($$shell_path( \
                    $$OUT_PWD/bin/$$QTSTUDIO3D_LIB)) \
                $$shell_quote($$shell_path($$[QT_INSTALL_BINS]/$$QTSTUDIO3D_LIB))

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
        }

        qtPrepareTool(DEPLOY_TOOL, $$deploytool)

        EXTRA_DEPLOY_OPTIONS = -qmldir=$$shell_quote($$PWD/src/shared/dummyqml)
        deployTarget.commands = \
            $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/Qt3DStudio$${exesuffix}) \
                -qmldir=$$shell_quote($$PWD/src/Authoring/Studio/Palettes) $$EXTRA_DEPLOY_OPTIONS

        macos {
            # Viewer 1.0
            deployTarget.commands += && \
                $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/Qt3DViewer$${exesuffix}) \
                $$EXTRA_DEPLOY_OPTIONS
        }

        greaterThan(QT_MAJOR_VERSION, 5)|greaterThan(QT_MINOR_VERSION, 10) {
            # Viewer 2.0
            win32 {
                # Viewer 2.0 has similar issues with dependent library naming as viewer 1.0,
                # but it only has one library that is causing problems and no qml (so far),
                # so lets just copy the problem lib over to where windeployqt can find it,
                # i.e. under Qt's bin dir. This pollutes the Qt's bin dir a bit, but as the main
                # use case for this is gathering installer content in CI after everything is
                # already built, this shouldn't be a problem.
                release {
                    RUNTIME2_LIB = Qt53DStudioRuntime2.dll
                } else {
                    RUNTIME2_LIB = Qt53DStudioRuntime2d.dll
                }
                QMAKE_EXTRA_TARGETS += copyRuntime2
                deployTarget.depends += copyRuntime2
                copyRuntime2.commands = \
                    $$QMAKE_COPY $$shell_quote($$shell_path( \
                        $$OUT_PWD/src/Runtime/qt3d-runtime/bin/$$RUNTIME2_LIB)) \
                    $$shell_quote($$shell_path($$[QT_INSTALL_BINS]/$$RUNTIME2_LIB))
            }

            deployTarget.commands += && \
                $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/q3dsviewer$${exesuffix}) \
                $$EXTRA_DEPLOY_OPTIONS
        }
    } else {
        # Create a dummy target for other platforms
        deployTarget.commands = @echo deployqt target is not supported for this platform.
    }
    deployTarget.target = deployqt
    QMAKE_EXTRA_TARGETS += deployTarget
}
