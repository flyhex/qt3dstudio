TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    Lua \
    EASTL

!boot2qt:!android:!integrity:!qnx {
    SUBDIRS += \
        pcre \
        ColladaDOM/TinyXML \
        ColladaDOM
}
