TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    3rdparty \
    QtExtras \
    Runtime \
    Viewer \
    shared

!cross_compile:!qnx:!mingw {
    SUBDIRS += Authoring
}
