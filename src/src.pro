TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    3rdparty \
    QtExtras \
    Runtime \
    Viewer

!boot2qt:!android:!integrity {
    SUBDIRS += Authoring
}
