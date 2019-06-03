TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    3rdparty \
    Runtime \
    shared

!cross_compile:!qnx:!mingw {
    SUBDIRS += Authoring
}
