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
    _UNICODE

win32: PlatformSpecificDir = Windows
macos: PlatformSpecificDir = Macos
linux|integrity: PlatformSpecificDir = Linux
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

linux {
    CONFIG += egl
    DEFINES+=_LINUX QT3DS_OS_LINUX _LINUXPLATFORM QT3DS_NO_X11 \
        QT3DS_VIEWER_EXPORTS WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T

    contains(QT_ARCH, x86_64) {
        BINDIR = $$PWD/../Bin/Linux64
    } else {
        BINDIR = $$PWD/../Bin/Linux32
    }
    QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-local-typedefs
}

linux-clang {
    DEFINES += __STRICT_ANSI__
}

macos {
    DEFINES += _MACOSX _LINUXPLATFORM WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T

    contains(QT_ARCH, x86_64) {
        BINDIR = $$PWD/../Bin/Macosx64
    } else {
        BINDIR = $$PWD/../Bin/Macosx32
    }

    INCLUDEPATH += /usr/local/include

    QMAKE_CXXFLAGS += -Wno-unused-local-typedef
    QMAKE_CFLAGS += -Wno-unused-local-typedef
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
        WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T LUA_BUILD_AS_DLL KDWIN NOMINMAX

    win32-msvc {
        QMAKE_CXXFLAGS += /MP /d2Zi+
        QMAKE_CFLAGS += /MP /d2Zi+
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
