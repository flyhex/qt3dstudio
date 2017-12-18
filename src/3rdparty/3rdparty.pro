TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    Lua \
    EASTL

!cross_compile:!qnx {
    SUBDIRS += \
        pcre \
        ColladaDOM/TinyXML \
        ColladaDOM
}
