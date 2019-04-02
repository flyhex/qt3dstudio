TEMPLATE = subdirs
CONFIG += ordered

!macos:!win32: SUBDIRS += \
    qtextras

# TODO: Re-enable these tests after fixing them
#    runtime \
#    viewer \
#    studio3d
