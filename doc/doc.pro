TEMPLATE = aux

build_online_docs: \
    QMAKE_DOCS = $$PWD/online/qt3dstudio.qdocconf
else: \
    QMAKE_DOCS = $$PWD/qt3dstudio.qdocconf
