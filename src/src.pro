TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    3rdparty \
    QtExtras \
    Runtime \
    shared

!cross_compile:!qnx:!mingw {
    SUBDIRS += Authoring
}
