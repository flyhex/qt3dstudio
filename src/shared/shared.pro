TEMPLATE = subdirs

exists ($$(BREAKPAD_SOURCE_DIR)) {
    CONFIG += ordered
    SUBDIRS += qt-breakpad
}
