TEMPLATE = subdirs
CONFIG += ordered

!boot2qt:!integrity:!qnx {
    SUBDIRS += Qt3DViewer
}
