TEMPLATE = subdirs

BREAKPAD_SOURCE_DIR = $$(BREAKPAD_SOURCE_DIR)

!isEmpty(BREAKPAD_SOURCE_DIR):exists($$BREAKPAD_SOURCE_DIR) {
    CONFIG += ordered
    SUBDIRS += qt-breakpad
}
