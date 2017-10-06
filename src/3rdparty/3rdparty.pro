TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    Lua \
    EASTL

!boot2qt:!android:!integrity {
    SUBDIRS += \
        pcre \
        ColladaDOM/TinyXML \
        ColladaDOM
}
