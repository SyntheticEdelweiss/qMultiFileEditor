QT += \
    core    \
    gui     \
    widgets

TARGET = qMultiFileEditor
TEMPLATE = app
CONFIG += console c++17
CONFIG += warn_on
win32: !contains(CONFIG, build_all): CONFIG -= debug_and_release

contains(CONFIG, warn_off): DEFINES += QT_NO_DEPRECATED_WARNINGS

CONFIG(debug, debug|release) {
    BUILD_TYPE = debug
    DEFINES *= DEBUG_BUILD
}
CONFIG(release, debug|release) {
    BUILD_TYPE = release
    DEFINES *= RELEASE_BUILD
    QMAKE_CXXFLAGS += -O2
}

DESTDIR     = $$PWD/bin
UI_DIR      = $$PWD/ui
#TRANSLATIONS = $$PWD/translations/lang_ru.ts

# if (PWD != build_dir), such as the case with shadow build
!equals(PWD, $${OUT_PWD}) {
    TARGET      = $${TARGET}.$${BUILD_TYPE}
} else {
    TARGET      = $${TARGET}.$${BUILD_TYPE}
    MOC_DIR     = $$PWD/build/$$BUILD_TYPE
    OBJECTS_DIR = $$PWD/build/$$BUILD_TYPE
    RCC_DIR     = $$PWD/build/$$BUILD_TYPE
}


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
