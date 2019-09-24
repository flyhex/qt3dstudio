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

OTHER_FILES += \
    "Studio/Build Configurations/*" \
    "Studio/Build Configurations/Viewer/*" \
    "src/Runtime/ogl-runtime/Studio/Content/Behavior Library/*" \
    "src/Runtime/ogl-runtime/Studio/Content/Effect Library/*" \
    "src/Runtime/ogl-runtime/Studio/Content/Material Library/*"

load(qt_parts)

# 'deployqt' target can be used to automatically copy necessary Qt libraries needed by studio
# applications. DEPLOY_DIR environment variable must point to the directory where
# Qt3DStudio and Qt3DViewer executables/application bundles are installed to.
# The required Qt libraries are copied into that directory/bundles.
!build_pass|!debug_and_release {
    win32|macos {
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
                QML_FILE_R = QtStudio3D/OpenGL/declarative_qtstudio3dopengl.dll
                QMAKE_EXTRA_TARGETS += deployReleaseQml
                deployTarget.depends += deployReleaseQml
                deployReleaseQml.depends = mkStudioQmlDir
                deployReleaseQml.commands = \
                    $$QMAKE_COPY $$shell_quote($$shell_path($$OUT_PWD/src/Runtime/ogl-runtime/qml/$$QML_FILE_R)) \
                    $$shell_quote($$shell_path(\$(DEPLOY_DIR)/$$QML_FILE_R))
            } else {
                QML_FILE_D = QtStudio3D/OpenGL/declarative_qtstudio3dopengld.dll
                QMAKE_EXTRA_TARGETS += deployDebugQml
                deployTarget.depends += deployDebugQml
                deployDebugQml.depends += mkStudioQmlDir
                deployDebugQml.commands = \
                    $$QMAKE_COPY $$shell_quote($$shell_path($$OUT_PWD/src/Runtime/ogl-runtime/qml/$$QML_FILE_D)) \
                    $$shell_quote($$shell_path(\$(DEPLOY_DIR)/$$QML_FILE_D))
            }

            # copy QtStudio3D.dll
            release {
                QTSTUDIO3D_LIB = Qt5Studio3D$${QT_LIBINFIX}.dll
            } else {
                QTSTUDIO3D_LIB = Qt5Studio3D$${QT_LIBINFIX}d.dll
            }
            QMAKE_EXTRA_TARGETS += copyStudio3D
            deployTarget.depends += copyStudio3D
            copyStudio3D.commands = \
                $$QMAKE_COPY $$shell_quote($$shell_path( \
                    $$OUT_PWD/src/Runtime/ogl-runtime/bin/$$QTSTUDIO3D_LIB)) \
                $$shell_quote($$shell_path($$[QT_INSTALL_BINS]/$$QTSTUDIO3D_LIB))

            QMLDIR_FILE = QtStudio3D/OpenGL/qmldir
            QMAKE_EXTRA_TARGETS += deployQmldir
            deployTarget.depends += deployQmldir
            deployQmldir.depends += mkStudioQmlDir
            deployQmldir.commands = \
                $$QMAKE_COPY $$shell_quote($$shell_path($$OUT_PWD/src/Runtime/ogl-runtime/qml/$$QMLDIR_FILE)) \
                $$shell_quote($$shell_path(\$(DEPLOY_DIR)/$$QMLDIR_FILE))

            QMAKE_EXTRA_TARGETS += mkStudioQmlDir
            mkStudioQmlDir.commands = \
                $$sprintf($$QMAKE_MKDIR_CMD, $$shell_quote($$shell_path(\$(DEPLOY_DIR)/QtStudio3D/OpenGL)))
        }

        qtPrepareTool(DEPLOY_TOOL, $$deploytool)

        EXTRA_DEPLOY_OPTIONS = -qmldir=$$shell_quote($$PWD/src/shared/dummyqml)
        deployTarget.commands = \
            $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/Qt3DStudio$${exesuffix}) \
                -qmldir=$$shell_quote($$PWD/src/Authoring/Qt3DStudio/Palettes) $$EXTRA_DEPLOY_OPTIONS

        macos {
            # Viewer 1.0
            deployTarget.commands += && \
                $$DEPLOY_TOOL $$shell_quote(\$(DEPLOY_DIR)/Qt3DViewer$${exesuffix}) \
                $$EXTRA_DEPLOY_OPTIONS
        }
    } else {
        # Create a dummy target for other platforms
        deployTarget.commands = @echo deployqt target is not supported for this platform.
    }
    deployTarget.target = deployqt
    QMAKE_EXTRA_TARGETS += deployTarget
}
