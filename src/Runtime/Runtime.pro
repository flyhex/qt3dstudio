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

# Qt3D based runtime needs Qt 5.11 or later
greaterThan(QT_MAJOR_VERSION, 5)|greaterThan(QT_MINOR_VERSION, 10) {
    isEmpty(QT3DS_BUILD_RUNTIME_2): QT3DS_BUILD_RUNTIME_2=$$(QT3DS_BUILD_RUNTIME_2)
    !isEmpty(QT3DS_BUILD_RUNTIME_2) {
        SUBDIRS += qt3d-runtime
    }
}
