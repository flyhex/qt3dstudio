load(qt_build_config)
load(qt_build_paths)

# Common defines across platforms that should be checked if/where they are used
# as we remove dependencies to see where we can reduce
DEFINES += \
    QT3DS_FOUNDATION_NO_EXPORTS \
    _CRT_SECURE_NO_DEPRECATE \
    _CRT_NONSTDC_NO_DEPRECATE \
    QT3DS_GRAPHICS_API_GL \
    QT3DS_NO_SEARCH_PATH \
    QT3DS_RENDER_ENABLE_LOAD_UIP \
    FONT_NO_RES_DIR \
    _TEGRA_NO_LOG_FILE \
    EA_COMPILER_HAS_INTTYPES \
    EASTL_ALLOCATOR_COPY_ENABLED \
    UNICODE \
    _UNICODE \
    NO_BOOST

win32: PlatformSpecificDir = Windows
macos: PlatformSpecificDir = Macos
linux|integrity|qnx: PlatformSpecificDir = Linux
android: PlatformSpecificDir = Android/jni

integrity: {
    DEFINES += _LINUX
    DEFINES += _INTEGRITYPLATFORM
}

INCLUDEPATH += $$PWD/Runtime/Source/PlatformSpecific/$$PlatformSpecificDir

THIRDPARTY_DIR = $$(QT3DSTUDIO_3RDPARTY_DIR)
isEmpty(THIRDPARTY_DIR) {
    THIRDPARTY_DIR = $$PWD/3rdparty
}

contains(QT_ARCH, x86_64) {
    DEFINES += QT3DS_PROCESSOR_X64
}

CONFIG(debug, debug|release) {
    DEFINES += _DEBUG
} else {
    DEFINES += NDEBUG
}

linux|qnx {
    CONFIG += egl
    DEFINES+=_LINUX QT3DS_OS_LINUX _LINUXPLATFORM QT3DS_NO_X11 \
        QT3DS_VIEWER_EXPORTS WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T
    QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-local-typedefs
}

linux-clang {
    DEFINES += __STRICT_ANSI__
}

macos {
    DEFINES += _MACOSX _LINUXPLATFORM WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T
    INCLUDEPATH += /usr/local/include
}

clang {
# Suppress large number of warnings from the Qt 3D Studio Code.
# Suppressions have been reported in JIRA with these bugs:
# QT3DS-2214 -Wno-unused-local-typedefs
# QT3DS-2216 -Wno-inconsistent-missing-override
# QT3DS-2222 -Wno-reorder
# QT3DS-2223 -Wno-format
# QT3DS-2224 -Wno-unused-function
# QT3DS-2227 -Wno-unused-value
# QT3DS-2229 -Wno-delete-non-virtual-dtor
# QT3DS-2234 -Wno-unused-variable
# QT3DS-2235 -Wno-overloaded-virtual
# QT3DS-2237 -Wno-unused-private-field
# QT3DS-2238 -Wno-comment
# QT3DS-2240 -Wno-enum-compare
# QT3DS-2241 -Wno-int-to-pointer-cast
# QT3DS-2242 -Wno-int-to-void-pointer-cast
# QT3DS-2243 -Wno-switch
# QT3DS-2244 -Wno-unused-lambda-capture
# QT3DS-2245 -Wno-sometimes-uninitialized
# QT3DS-2246 -Wno-deprecated-declarations
# QT3DS-2247 -Wno-pointer-bool-conversion
# QT3DS-2248 -Wno-self-assign
# QT3DS-2249 -Wno-tautological-compare
# QT3DS-2250 -Wno-uninitialized
# QT3DS-2251 -Wno-tautological-constant-out-of-range-compare
# QT3DS-2253 -Wno-infinite-recursion
    QMAKE_CXXFLAGS_WARN_ON = -Wall \
        -Wno-unknown-pragmas \
        -Wno-unused-local-typedefs \
        -Wno-inconsistent-missing-override \
        -Wno-reorder \
        -Wno-format \
        -Wno-unused-function \
        -Wno-unused-value \
        -Wno-delete-non-virtual-dtor \
        -Wno-unused-variable \
        -Wno-overloaded-virtual \
        -Wno-unused-private-field \
        -Wno-comment \
        -Wno-enum-compare \
        -Wno-int-to-pointer-cast \
        -Wno-int-to-void-pointer-cast \
        -Wno-switch \
        -Wno-unused-lambda-capture \
        -Wno-sometimes-uninitialized \
        -Wno-deprecated-declarations \
        -Wno-pointer-bool-conversion \
        -Wno-self-assign \
        -Wno-uninitialized \
        -Wno-tautological-compare \
        -Wno-tautological-constant-out-of-range-compare \
        -Wno-infinite-recursion
    QMAKE_CFLAGS_WARN_ON = -Wall \
        -Wno-unknown-pragmas \
        -Wno-unused-local-typedefs \
        -Wno-inconsistent-missing-override \
        -Wno-reorder \
        -Wno-format \
        -Wno-unused-function \
        -Wno-unused-value \
        -Wno-delete-non-virtual-dtor \
        -Wno-unused-variable \
        -Wno-overloaded-virtual \
        -Wno-unused-private-field \
        -Wno-comment \
        -Wno-enum-compare \
        -Wno-int-to-pointer-cast \
        -Wno-int-to-void-pointer-cast \
        -Wno-switch \
        -Wno-unused-lambda-capture \
        -Wno-sometimes-uninitialized \
        -Wno-deprecated-declarations \
        -Wno-pointer-bool-conversion \
        -Wno-self-assign \
        -Wno-uninitialized \
        -Wno-tautological-compare \
        -Wno-tautological-constant-out-of-range-compare \
        -Wno-infinite-recursion

# Suppress the huge pile of Qt related warnings on "direct access in function
# from file to global weak symbol". These arise when not using devbuilds of Qt.
    QMAKE_CXXFLAGS += -fvisibility=hidden
    QMAKE_CFLAGS += -fvisibility=hidden
}

android {
    QMAKE_CXXFLAGS -= -fstack-protector-strong
    QMAKE_CFLAGS -= -fstack-protector-strong
    # TODO: Should be done using this instead of copying the GLES headers, but including this
    # causes lots of conflicting definitions in signal.h for some reason. Feel free to fix it if
    # you know how. After this works, GLES3 and GLES2 folders can be deleted from
    # 3rdparty/RuntimePlatformSpecific/Android/jni
#    INCLUDEPATH += $$(ANDROID_NDK_ROOT)/sysroot/usr/include
#    DEFINES += __BITS_PER_LONG=32
}

win32 {
    DEFINES += QT3DS_OS_WINDOWS _WIN32 _PCPLATFORM \
        WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T KDWIN NOMINMAX

    win32-msvc {
        QMAKE_CXXFLAGS += /MP /d2Zi+
        QMAKE_CFLAGS += /MP /d2Zi+
        # Warning C4251 = needs to have dll-interface to be used by clients of class,
        #                 it comes from a lot of Qt headers, so disabling it.
        QMAKE_CXXFLAGS_WARN_ON += -wd4251 #needs to have dll-interface to be used by clients
    }

    CONFIG(debug, debug|release) {
        win32-msvc {
            QMAKE_CXXFLAGS += /Od
            QMAKE_CFLAGS += /Od
        }
    } else {
        win32-msvc {
            QMAKE_CXXFLAGS += /Ox
            QMAKE_CFLAGS += /Ox
        }
    }

    contains(QT_ARCH, x86_64) {
        DEFINES += _WIN64
    }
}

BINDIR = $$MODULE_BASE_OUTDIR/bin
LIBDIR = $$MODULE_BASE_OUTDIR/lib
LIBS += -L"$$LIBDIR"
!testcase {
    contains(TEMPLATE, app) {
        DESTDIR = $$BINDIR
    } else {
        DESTDIR = $$LIBDIR
        win32: DLLDESTDIR = $$BINDIR
    }
}
