load(qt_build_config)
load(qt_build_paths)

# Common defines across platforms that should be checked if/where they are used
# as we remove dependencies to see where we can reduce
DEFINES += \
    BOOST_SIGNALS_NO_DEPRECATION_WARNING \
    QT3DS_FOUNDATION_NO_EXPORTS \
    _CRT_SECURE_NO_DEPRECATE \
    _CRT_NONSTDC_NO_DEPRECATE \
    UIC_FPMODEL_HARDFP \
    UIC_GRAPHICS_API_GL \
    UIC_NO_SEARCH_PATH \
    UIC_RENDER_ENABLE_LOAD_UIP \
    FONT_NO_RES_DIR \
    _TEGRA_NO_LOG_FILE \
    EA_COMPILER_HAS_INTTYPES \
    EASTL_ALLOCATOR_COPY_ENABLED \
    OPENKODE \
    KD \
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
    DEFINES += UIC_PROCESSOR_X64
}

CONFIG(debug, debug|release) {
    DEFINES += _DEBUG
} else {
    DEFINES += NDEBUG
}

linux {
    CONFIG += egl
    DEFINES+=_LINUX UIC_OS_LINUX _LINUXPLATFORM QT3DS_NO_X11 \
        UICVIEWER_EXPORTS WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T

    contains(QT_ARCH, x86_64) {
        BINDIR = $$PWD/../Bin/Linux64
    } else {
        BINDIR = $$PWD/../Bin/Linux32
    }
    QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-local-typedefs
}

integrity {
    exists($$THIRDPARTY_DIR/boost/1.65.0) {
        INCLUDEPATH += $$THIRDPARTY_DIR/boost/1.65.0
    } else {
        INCLUDEPATH += $$THIRDPARTY_DIR/boost/1.55.0
    }
}

macos {
    DEFINES += _MACOSX _LINUXPLATFORM WIDE_IS_DIFFERENT_TYPE_THAN_CHAR16_T

    BOOSTDIR = /usr/local/lib
    BOOSTSIGNALLIB = boost_signals-mt
    BOOSTSYSTEMLIB = boost_system-mt
    BOOSTFILESYSTEMLIB = boost_filesystem-mt

    contains(QT_ARCH, x86_64) {
        BINDIR = $$PWD/../Bin/Macosx64
    } else {
        BINDIR = $$PWD/../Bin/Macosx32
    }

    INCLUDEPATH += /usr/local/include
    # macOS uses GLog from Homebrew, the thirdparty one doesn't work
    # which also means we use Boost from Homebrew

    QMAKE_CXXFLAGS += -Wno-unused-local-typedef
    QMAKE_CFLAGS += -Wno-unused-local-typedef
}

linux:!android {
    BOOSTDIR = /usr/lib/x86_64-linux-gnu/
    BOOSTSIGNALLIB = boost_signals
    BOOSTSYSTEMLIB = boost_system
    BOOSTFILESYSTEMLIB = boost_filesystem
    exists($$THIRDPARTY_DIR/boost/1.65.0) {
        INCLUDEPATH += $$THIRDPARTY_DIR/boost/1.65.0
    } else {
        INCLUDEPATH += $$THIRDPARTY_DIR/boost/1.55.0
    }
}

android {
    QMAKE_CXXFLAGS -= -fstack-protector-strong
    QMAKE_CFLAGS -= -fstack-protector-strong
    exists($$THIRDPARTY_DIR/boost/1.65.0) {
        INCLUDEPATH += $$THIRDPARTY_DIR/boost/1.65.0
    } else {
        INCLUDEPATH += $$THIRDPARTY_DIR/boost/1.55.0
    }
    # TODO: Should be done using this instead of copying the GLES headers, but including this
    # causes lots of conflicting definitions in signal.h for some reason. Feel free to fix it if
    # you know how. After this works, GLES3 and GLES2 folders can be deleted from
    # 3rdparty/RuntimePlatformSpecific/Android/jni
#    INCLUDEPATH += $$(ANDROID_NDK_ROOT)/sysroot/usr/include
#    DEFINES += __BITS_PER_LONG=32
}

win32 {
    DEFINES += UIC_OS_WINDOWS _WIN32 _PCPLATFORM \
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

    exists($$THIRDPARTY_DIR/boost/1.65.0) {
        contains(QT_ARCH, x86_64) {
            BOOSTDIR = $$THIRDPARTY_DIR/boost/1.65.0/msvc14/lib64
        } else {
            BOOSTDIR = $$THIRDPARTY_DIR/boost/1.65.0/msvc14/lib
        }

        CONFIG(debug, debug|release) {
            BOOSTSIGNALLIB = libboost_signals-vc140-mt-gd-1_65
            BOOSTSYSTEMLIB = libboost_system-vc140-mt-gd-1_65
            BOOSTFILESYSTEMLIB = libboost_filesystem-vc140-mt-gd-1_65
        } else {
            BOOSTSIGNALLIB = libboost_signals-vc140-mt-1_65
            BOOSTSYSTEMLIB = libboost_system-vc140-mt-1_65
            BOOSTFILESYSTEMLIB = libboost_filesystem-vc140-mt-1_65
        }
        INCLUDEPATH += $$THIRDPARTY_DIR/boost/1.65.0
    } else {
        contains(QT_ARCH, x86_64) {
            BOOSTDIR = $$THIRDPARTY_DIR/boost/1.55.0/msvc14/lib64
        } else {
            BOOSTDIR = $$THIRDPARTY_DIR/boost/1.55.0/msvc14/lib
        }

        CONFIG(debug, debug|release) {
            BOOSTSIGNALLIB = libboost_signals-vc120-mt-gd-1_55
            BOOSTSYSTEMLIB = libboost_system-vc120-mt-gd-1_55
            BOOSTFILESYSTEMLIB = libboost_filesystem-vc120-mt-gd-1_55
        } else {
            BOOSTSIGNALLIB = libboost_signals-vc120-mt-1_55
            BOOSTSYSTEMLIB = libboost_system-vc120-mt-1_55
            BOOSTFILESYSTEMLIB = libboost_filesystem-vc120-mt-1_55
        }
        INCLUDEPATH += $$THIRDPARTY_DIR/boost/1.55.0
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
