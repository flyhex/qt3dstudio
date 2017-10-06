SOURCES += main.cpp

win32-msvc {
#workaround for debug and release configurations where the test system
#generates only single Makefile for projects under config.tests and the compiler flags
#contain -MDd also for release. The dynamic lib version works for both cases.
LIBS-= -llibfbxsdk-md
LIBS+= -llibfbxsdk
}

linux {
LIBS += -ldl
}
