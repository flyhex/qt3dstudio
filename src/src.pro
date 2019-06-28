TEMPLATE = subdirs
CONFIG += ordered

# When building the desktop authoring tool, skip embedded/mobile targets
!cross_compile:!qnx:!android:!ios:!integrity {
    SUBDIRS += \
        Runtime \
        3rdparty \
        shared \
        Authoring
}
