QT       += core gui
TARGET = $$qtLibraryTarget(platformplugin)
TEMPLATE = lib
CONFIG += plugin

## load mobilityconfig if mobility is available
load(mobilityconfig, true)

# HTML5 Media Support
# We require QtMultimedia
!contains(DEFINES, ENABLE_VIDEO=.) {
    contains(MOBILITY_CONFIG, multimedia) {
        CONFIG += mobility
        MOBILITY += multimedia
        DEFINES -= ENABLE_VIDEO=0
        DEFINES += ENABLE_VIDEO=1
        DEFINES -= ENABLE_QT_MULTIMEDIA=0
        DEFINES += ENABLE_QT_MULTIMEDIA=1
    }
}

DESTDIR = $$[QT_INSTALL_PLUGINS]/webkit

SOURCES += \
    WebPlugin.cpp \
    WebNotificationPresenter.cpp

HEADERS += \
    WebPlugin.h \
    qwebkitplatformplugin.h \
    WebNotificationPresenter.h

!contains(DEFINES, ENABLE_NOTIFICATIONS=.): DEFINES += ENABLE_NOTIFICATIONS=1
