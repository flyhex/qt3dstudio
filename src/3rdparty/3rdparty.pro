TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    EASTL

!cross_compile:!qnx {
    SUBDIRS += \
        pcre \
        ColladaDOM/TinyXML \
        ColladaDOM
}
