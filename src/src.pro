TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    3rdparty \
    QtExtras \
    Runtime

!cross_compile:!qnx:!mingw {
    SUBDIRS += Authoring
}
