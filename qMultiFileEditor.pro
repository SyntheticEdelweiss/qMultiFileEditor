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
        MulticolorDelegate.cpp \
        Utils.cpp \
        main.cpp \
        MultiFileEditor.cpp

HEADERS += \
        MultiFileEditor.h \
        MulticolorDelegate.h \
        Utils.h

FORMS += \
        MultiFileEditor.ui

RESOURCES += \
    Icons.qrc
