TEMPLATE = subdirs
CONFIG += ordered

!macos: SUBDIRS += \
    qtextras

# TODO: Re-enable these tests after fixing them
#    runtime \
#    viewer \
#    studio3d
