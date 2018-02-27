TEMPLATE = app
TARGET = Qt3DCompiler
include(../commoninclude.pri)
CONFIG += console

SOURCES += Qt3DSCompilerMain.cpp \
           ../Source/Runtime/Source/Qt3DSInputEngine.cpp \
           ../Source/Qt3DSFoundation/Source/foundation/Qt3DSLogging.cpp

linux|mingw {
    BEGIN_ARCHIVE = -Wl,--whole-archive
    END_ARCHIVE = -Wl,--no-whole-archive
}

boot2qt: {
    RESOURCES += ../res.qrc
    DEFINES += EMBEDDED_LINUX # TODO: Is there a compile-time flag for boot2qt?
}

integrity:ios {
    RESOURCES += ../res.qrc
}

STATICRUNTIME = \
    $$BEGIN_ARCHIVE \
    -lEASTL$$qtPlatformTargetSuffix() \
    -lLua$$qtPlatformTargetSuffix() \
    $$END_ARCHIVE

# On non-windows systems link the whole static archives and do not put them
# in the prl file to prevent them being linked again by targets that depend
# upon this shared library
!win32 {
    QMAKE_LFLAGS += $$STATICRUNTIME
    LIBS += -lqt3dsruntime$$qtPlatformTargetSuffix() \
            -lqt3dsqmlstreamer$$qtPlatformTargetSuffix()

} else {
    LIBS += \
        $$STATICRUNTIME \
        -lqt3dsruntime$$qtPlatformTargetSuffix() \
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

load(qt_tool)
CONFIG -= relative_qt_rpath
