TEMPLATE = aux

build_online_docs: \
    QMAKE_DOCS = $$PWD/online/qt3dstudio.qdocconf
else: \
    QMAKE_DOCS = $$PWD/qt3dstudio.qdocconf

OTHER_FILES += \
    $$PWD/src/*.qdoc \
    $$PWD/src/03-studio/*.qdoc \
    $$PWD/src/04-viewer/*.qdoc \
    $$PWD/src/05-runtime/*.qdoc \
    $$PWD/src/06-qml-reference/*.qdoc \
    $$PWD/src/07-file-formats/*.qdoc \
    $$PWD/src/08-integrating/*.qdoc \
    $$PWD/src/10-best-practices/*.qdoc \
    $$PWD/src/11-quick-start-guides/*.qdoc \
    $$PWD/src/12-cpp-reference/*.qdoc \
    $$PWD/src/images/*.*
