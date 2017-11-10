TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    3rdparty \
    QtExtras \
    Runtime \
    Viewer

!cross_compile:!qnx:!mingw {
    SUBDIRS += Authoring
}
