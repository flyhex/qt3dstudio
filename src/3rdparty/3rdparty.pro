TEMPLATE = subdirs
CONFIG += ordered

!cross_compile:!qnx {
    SUBDIRS += \
        pcre \
        ColladaDOM/TinyXML \
        ColladaDOM
}
