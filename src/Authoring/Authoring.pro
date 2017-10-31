TEMPLATE = subdirs
CONFIG += ordered

# Optional license handler
isEmpty(TQTC_LICENSE_MANAGING): TQTC_LICENSE_MANAGING=$$(TQTC_LICENSE_MANAGING)
!isEmpty(TQTC_LICENSE_MANAGING): SUBDIRS += licensehandler

SUBDIRS += \
    QT3DSDM \
    CoreLib \
    Common \
    Studio/Qt3DStudio.pro
