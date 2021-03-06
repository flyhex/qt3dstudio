TEMPLATE = lib
TARGET = CommonLib
CONFIG += staticlib nostrictstrings
include(../commoninclude.pri)

QT += widgets

DEFINES += _UNICODE QT3DS_AUTHORING _AFXDLL PCRE_STATIC _WINSOCK_DEPRECATED_NO_WARNINGS \
            NO_ZAE COLLADA_DOM_SUPPORT141 DOM_INCLUDE_TINYXML \
            DISABLE_MESH_OPTIMIZATION

contains(QMAKE_TARGET.arch, x86_64) {
    DEFINES += _AMD64_
}

mingw {
LIBS += \
    -lqt3dsruntimestatic$$qtPlatformTargetSuffix() \
    -lqt3dsqmlstreamer$$qtPlatformTargetSuffix() \
    -lEASTL$$qtPlatformTargetSuffix() \
    -lpcre$$qtPlatformTargetSuffix() \
    -lTinyXML$$qtPlatformTargetSuffix() \
    -lColladaDOM$$qtPlatformTargetSuffix() \
    -lQT3DSDM$$qtPlatformTargetSuffix()
}

INCLUDEPATH += \
    Code/_Win32 \
    Code/Thread \
    Code/IO \
    Code \
    Code/Exceptions \
    Code/_Win32/Include \
    Code/Report \
    Code/Serialize \
    ../Client/Code/Shared \
    ../Client/Code/Core/Utility \
    ../../Runtime/ogl-runtime/src/dm \
    ../../Runtime/ogl-runtime/src/dm/systems \
    ../Qt3DStudio/Utils \
    ../../Runtime/ogl-runtime/src/foundation \
    ../../Runtime/ogl-runtime/src/3rdparty/EASTL/UnknownVersion/include \
    ../../Runtime/ogl-runtime/src/3rdparty/color \
    ..

PRECOMPILED_HEADER += Code/Qt3DSCommonPrecompile.h

SOURCES += \
    ../../Runtime/ogl-runtime/src/3rdparty/color/CColor.cpp \
    Code/HiResTimer.cpp \
    Code/MasterP.cpp \
    Code/MethProf.cpp \
    Code/Pt.cpp \
    Code/StackTokenizer.cpp \
    Code/StringTokenizer.cpp \
    Code/StrUtilities.cpp \
    Code/Qt3DSAtomic.cpp \
    Code/Qt3DSFileTools.cpp \
    Code/Qt3DSId.cpp \
    Code/Qt3DSMath.cpp \
    Code/Qt3DSMemory.cpp \
    Code/Qt3DSMessageBox.cpp \
    Code/Qt3DSObjectCounter.cpp \
    Code/Qt3DSString.cpp \
    Code/Qt3DSTime.cpp \
    Code/Thread/Mutex.cpp \
    Code/Thread/Thread.cpp \
    Code/Exceptions/XMLException.cpp \
    Code/_Win32/Qt3DSFile.cpp \
    Code/Graph/Graph.cpp \
    Code/InfoDump/CoutSink.cpp \
    Code/InfoDump/DumpFileSink.cpp \
    Code/InfoDump/InfoDump.cpp \
    Code/InfoDump/InfoSink.cpp \
    Code/InfoDump/StrVecSink.cpp \
    Code/IO/BufferedInputStream.cpp \
    Code/IO/BufferedOutputStream.cpp \
    Code/IO/FileInputStream.cpp \
    Code/IO/FileOutputStream.cpp \
    Code/IO/IOStreams.cpp \
    Code/IO/LEndianStreams.cpp \
    Code/IO/MemBuf.cpp \
    Code/IO/MemInputStream.cpp \
    Code/IO/MemOutputStream.cpp \
    Code/IO/Seekable.cpp \
    Code/EulerAngles/EulerAngles.cpp \
    Code/Memory/MemoryObject.cpp \
    Code/Serialize/FormattedInputStream.cpp \
    Code/Serialize/FormattedOutputStream.cpp \

HEADERS = Code/Literals.h
