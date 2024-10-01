#-------------------------------------------------
#
# Project created by QtCreator 2022-11-25T14:12:06
#
#-------------------------------------------------

QT += \
    core    \
    gui     \
    widgets

TARGET = qMultiFileEditor
TEMPLATE = app
CONFIG += console warn_on c++17

DESTDIR     = $$PWD/bin/
MOC_DIR     = $$PWD/moc/
OBJECTS_DIR = $$PWD/obj/
UI_DIR      = $$PWD/ui/
#TRANSLATIONS = $$PWD/translations/lang_ru.ts

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        Utils.cpp \
        main.cpp \
        MultiFileEditor.cpp

HEADERS += \
        MultiFileEditor.h \
        Utils.h

FORMS += \
        MultiFileEditor.ui

RESOURCES += \
    Icons.qrc
