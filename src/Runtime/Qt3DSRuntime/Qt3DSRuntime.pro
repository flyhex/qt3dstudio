TEMPLATE = lib
TARGET = qt3dsopengl
VERSION = $$MODULE_VERSION

DEFINES += QT3DS_RUNTIME_EXPORTS

CONFIG += installed
include(../commoninclude.pri)
QT += qml

boot2qt: {
    RESOURCES += ../res.qrc
    DEFINES += EMBEDDED_LINUX # TODO: Is there a compile-time flag for boot2qt?
}

integrity|ios {
    RESOURCES += ../res.qrc
}

SOURCES += \
    ../Source/viewer/Qt3DSAudioPlayerImpl.cpp \
    ../Source/viewer/Qt3DSViewerApp.cpp

HEADERS += \
    ../Source/viewer/qt3dsruntimeglobal.h \
    ../Source/viewer/Qt3DSAudioPlayerImpl.h \
    ../Source/viewer/Qt3DSViewerApp.h \
    ../Source/viewer/Qt3DSViewerTimer.h

linux|qnx|mingw {
    BEGIN_ARCHIVE = -Wl,--whole-archive
    END_ARCHIVE = -Wl,--no-whole-archive
}

STATICRUNTIME = \
    $$BEGIN_ARCHIVE \
    -lqt3dsruntimestatic$$qtPlatformTargetSuffix() \
    -lEASTL$$qtPlatformTargetSuffix() \
    $$END_ARCHIVE

# On non-windows systems link the whole static archives and do not put them
# in the prl file to prevent them being linked again by targets that depend
# upon this shared library
!win32:!CONFIG(static){
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

    RESOURCES += ../platformres.qrc
}

linux {
    LIBS += \
        -ldl \
        -lEGL
}

macos {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
}

PREDEPS_LIBS = qt3dsruntimestatic

include(../../utils.pri)
PRE_TARGETDEPS += $$fixLibPredeps($$LIBDIR, PREDEPS_LIBS)
