requires(!ios)
requires(!winrt)
requires(!tvos)
requires(!watchos)
requires(!msvc2013)

requires(qtHaveModule(widgets))
requires(qtHaveModule(multimedia))
requires(qtHaveModule(quick))
requires(qtHaveModule(qml))
requires(qtHaveModule(opengl))

SUBDIRS += \
    doc

load(qt_parts)
