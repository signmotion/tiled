include(../plugin.pri)

DEFINES += WORLD_LIBRARY

greaterThan(QT_MAJOR_VERSION, 4) {
    OTHER_FILES = plugin.json
}

HEADERS += \
    Coord3x3.h \
    world_global.h \
    worldplugin.h

SOURCES += \
    Coord3x3.cpp \
    worldplugin.cpp
