TEMPLATE = lib
TARGET = QT3DSDM
CONFIG += staticlib
include(../commoninclude.pri)

DEFINES += QT3DS_AUTHORING _AFXDLL PCRE_STATIC _UNICODE

linux {
    DEFINES += QT3DSDM_META_DATA_NO_SIGNALS
}

INCLUDEPATH += \
    \
    Systems \
    Systems/Cores \
    ../../Runtime/Source/System/Include \
    ../../Runtime/Source/Qt3DSFoundation/Include \
    ../../Runtime/Source/Qt3DSRuntimeRender/Include \
    ../../Runtime/Source/DataModel/Include \
    ../../Runtime/Source/Qt3DSRender/Include \
    ../../3rdparty/EASTL/UnknownVersion/include \
    ../../3rdparty/utf8cpp/2.3.2/source \
    ../../3rdparty/color \
    ..

PRECOMPILED_HEADER = Qt3DSDMPrefix.h

SOURCES += \
    EASTL_new.cpp \
    Systems/ActionSystem.cpp \
    Systems/SignalsImpl.cpp \
    Systems/SlideSystem.cpp \
    Systems/StudioAnimationSystem.cpp \
    Systems/StudioCoreSystem.cpp \
    Systems/StudioFullSystem.cpp \
    Systems/StudioPropertySystem.cpp \
    Systems/Qt3DSDMComposerTypeDefinitions.cpp \
    Systems/Qt3DSDMGuides.cpp \
    Systems/Qt3DSDMMetaData.cpp \
    Systems/Qt3DSDMSignalSystem.cpp \
    Systems/Qt3DSDMStringTable.cpp \
    Systems/Qt3DSDMValue.cpp \
    Systems/Qt3DSDMXML.cpp \
    Systems/Cores/ActionCoreProducer.cpp \
    Systems/Cores/AnimationCoreProducer.cpp \
    Systems/Cores/DataCoreProducer.cpp \
    Systems/Cores/SimpleActionCore.cpp \
    Systems/Cores/SimpleAnimationCore.cpp \
    Systems/Cores/SimpleDataCore.cpp \
    Systems/Cores/SimpleSlideCore.cpp \
    Systems/Cores/SimpleSlideGraphCore.cpp \
    Systems/Cores/SlideCoreProducer.cpp \
    Systems/Cores/SlideGraphCoreProducer.cpp \

HEADERS += \
    Systems/Qt3DSDMDataTypes.h \
    Systems/Qt3DSDMMetaDataTypes.h \
    Systems/Qt3DSDMMetaDataValue.h \
    Systems/Qt3DSDMMetaData.h
