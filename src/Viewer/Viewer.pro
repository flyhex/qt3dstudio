TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    studio3d \
    qmlviewer

!boot2qt:!integrity:!qnx {
    SUBDIRS += Qt3DViewer
}
