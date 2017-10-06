defineTest(qtConfLibrary_fbx) {
    libs = $$eval($${1}.libs)
    compiler = $$eval($${1}.compiler)
    includedir =

    CONFIG(debug, debug|release) {
        isEmpty(libs) {
            libs = $$eval($${1}.debuglibs)
        } else {
            build = /debug
        }
    } else {
        isEmpty(libs) {
            libs = $$eval($${1}.releaselibs)
        } else {
            build = /release
        }
    }

    contains(QT_ARCH, x86_64) {
        !macos: architecture = /x64
    } else {
        !macos: architecture = /x86
    }

    fbx_sdk = $$getenv(FBXSDK)

    isEmpty(fbx_sdk) {
        # Expect FBX libs and headers to be in our 3rdparty folder, as that's
        # the only place we look for FBX in the build
        THIRDPARTY_DIR = $$getenv(QT3DSTUDIO_3RDPARTY_DIR)
        isEmpty(THIRDPARTY_DIR) {
            THIRDPARTY_DIR = $$_PRO_FILE_PWD_/src/3rdparty
        }
        exists($$THIRDPARTY_DIR/FBX/2016.1.2) {
            fbx_sdk = $$THIRDPARTY_DIR/FBX/2016.1.2
        }
    }

    !isEmpty(fbx_sdk) {
        includedir += $${fbx_sdk}/include
        libs += "-L$${fbx_sdk}/lib/$${compiler}$${architecture}$${build}"
    }

    $${1}.libs = $$val_escape(libs)
    $${1}.includedir = $$val_escape(includedir)

    export($${1}.libs)
    export($${1}.includedir)

    return(true)
}

