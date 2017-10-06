TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    studio3d \
    qmlviewer

!boot2qt:!integrity {
    SUBDIRS += Qt3DViewer
}
