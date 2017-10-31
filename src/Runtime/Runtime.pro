TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    Qt3DSRuntimeStatic \
    Qt3DSRuntime \
    Qt3DSCompiler

win32 {
SUBDIRS += \
    AttributeHashes
}
