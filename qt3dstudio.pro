requires(qtHaveModule(widgets))
requires(qtHaveModule(multimedia))
requires(qtHaveModule(quick))
requires(qtHaveModule(qml))
requires(qtHaveModule(opengl))

SUBDIRS += \
    doc

load(qt_parts)
