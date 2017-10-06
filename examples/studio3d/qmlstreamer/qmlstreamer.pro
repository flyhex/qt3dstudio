TEMPLATE = app

QT += qml quick

integrity: DEFINES += USE_EMBEDDED_FONTS

target.path = $$[QT_INSTALL_EXAMPLES]/studio3d/$$TARGET
INSTALLS += target

SOURCES += main.cpp

RESOURCES += qmlstreamer.qrc

OTHER_FILES += qml/qmlstreamer/* \
               doc/src/* \
               doc/images/*

# Icon in case example is included in installer
exists(example.ico): RC_ICONS = example.ico
