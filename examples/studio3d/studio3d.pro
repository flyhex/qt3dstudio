TEMPLATE = subdirs

!package {
    SUBDIRS += \
        qmldynamickeyframes \
        qmlstreamer \
        widgetviewer \
        surfaceviewer \
        surfaceviewer_offscreen

    # Sample project only has the presentation.
    # We do not want to pollute the sample folder with .pro file, so do the install here.
    INSTALLS += sampleProject
    sampleProject.path = $$[QT_INSTALL_EXAMPLES]/studio3d/SampleProject
    sampleProject.files = SampleProject/*
}
