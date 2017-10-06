TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    Qt3DSRuntimeStatic.pro \
    Qt3DSRuntime.pro

win32 {
SUBDIRS += \
    AttributeHashes.pro
}
