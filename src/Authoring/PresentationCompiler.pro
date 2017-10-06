TEMPLATE = app
TARGET = Qt3DCompiler
include($$PWD/../Runtime/commoninclude.pri)
CONFIG += console

SOURCES += \
    $$PWD/../Runtime/Source/UICCompiler/Source/UICCompilerMain.cpp

linux {
    BEGIN_ARCHIVE = -Wl,--whole-archive
    END_ARCHIVE = -Wl,--no-whole-archive
}

STATICRUNTIME = \
    $$BEGIN_ARCHIVE \
    -lEASTL$$qtPlatformTargetSuffix() \
    -lLua$$qtPlatformTargetSuffix() \
    -lqt3dsruntimestatic$$qtPlatformTargetSuffix() \
    $$END_ARCHIVE

# On non-windows systems link the whole static archives and do not put them
# in the prl file to prevent them being linked again by targets that depend
# upon this shared library
!win32 {
    QMAKE_LFLAGS += $$STATICRUNTIME
    LIBS += -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()
} else {
    LIBS += \
        $$STATICRUNTIME \
        -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()
}

win32 {
    LIBS += \
        -lws2_32
}

linux {
    LIBS += \
        -ldl \
        -lEGL
}

PREDEPS_LIBS = qt3dsruntimestatic

include($$PWD/../utils.pri)
PRE_TARGETDEPS += $$fixLibPredeps($$LIBDIR, PREDEPS_LIBS)

load(qt_tool)
CONFIG -= relative_qt_rpath
