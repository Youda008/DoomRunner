#-------------------------------------------------
#
# Project created by QtCreator 2019-05-13T14:15:57
#
#-------------------------------------------------

QT       += core gui widgets network

TARGET = DoomRunner
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
#DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_CXXFLAGS += -Wno-deprecated-declarations  # commenting-out the above doesn't work

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17

# Some Qt headers use deprecated C++ features and generate tons of warnings.
# This will silence them and prevent them from shadowing our own potentially important warnings.
QMAKE_CXXFLAGS += -Wno-deprecated-copy

SOURCES += \
    Sources/AboutDialog.cpp \
    Sources/CompatOptsDialog.cpp \
    Sources/DirTreeModel.cpp \
    Sources/DoomUtils.cpp \
    Sources/EditableListView.cpp \
    Sources/EngineDialog.cpp \
    Sources/EventFilters.cpp \
    Sources/FileSystemUtils.cpp \
    Sources/GameOptsDialog.cpp \
    Sources/JsonUtils.cpp \
    Sources/LangUtils.cpp \
    Sources/ListModel.cpp \
    Sources/MainWindow.cpp \
    Sources/OSUtils.cpp \
    Sources/SetupDialog.cpp \
    Sources/UpdateChecker.cpp \
    Sources/UserData.cpp \
    Sources/Version.cpp \
    Sources/WidgetUtils.cpp \
    Sources/main.cpp

HEADERS += \
    Sources/AboutDialog.hpp \
    Sources/Common.hpp \
    Sources/CompatOptsDialog.hpp \
    Sources/DirTreeModel.hpp \
    Sources/DoomUtils.hpp \
    Sources/EditableListView.hpp \
    Sources/EngineDialog.hpp \
    Sources/EventFilters.hpp \
    Sources/FileSystemUtils.hpp \
    Sources/GameOptsDialog.hpp \
    Sources/JsonUtils.hpp \
    Sources/LangUtils.hpp \
    Sources/ListModel.hpp \
    Sources/MainWindow.hpp \
    Sources/OSUtils.hpp \
    Sources/SetupDialog.hpp \
    Sources/UpdateChecker.hpp \
    Sources/UserData.hpp \
    Sources/Version.hpp \
    Sources/WidgetUtils.hpp

FORMS += \
    Forms/AboutDialog.ui \
    Forms/CompatOptsDialog.ui \
    Forms/EngineDialog.ui \
    Forms/GameOptsDialog.ui \
    Forms/MainWindow.ui \
    Forms/SetupDialog.ui

DISTFILES += \
    Resources/DoomRunner.ico \
    Resources/DoomRunner2.ico

win32:RC_ICONS += Resources/DoomRunner.ico

unix: LIBS += -lX11 -lXrandr

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
