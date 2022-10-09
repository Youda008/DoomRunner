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

QMAKE_CXXFLAGS += -Wno-comment

SOURCES += \
    Sources/AboutDialog.cpp \
    Sources/CompatOptsDialog.cpp \
    Sources/DoomUtils.cpp \
    Sources/EditableListView.cpp \
    Sources/EngineDialog.cpp \
    Sources/EngineProperties.cpp \
    Sources/EventFilters.cpp \
    Sources/ExtendedTreeView.cpp \
    Sources/FileSystemUtils.cpp \
    Sources/GameOptsDialog.cpp \
    Sources/JsonUtils.cpp \
    Sources/LangUtils.cpp \
    Sources/ListModel.cpp \
    Sources/MainWindow.cpp \
    Sources/MiscUtils.cpp \
    Sources/NewConfigDialog.cpp \
    Sources/OSUtils.cpp \
    Sources/OptionsSerializer.cpp \
    Sources/OptionsStorageDialog.cpp \
    Sources/ProcessOutputWindow.cpp \
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
    Sources/DoomUtils.hpp \
    Sources/EditableListView.hpp \
    Sources/EngineDialog.hpp \
    Sources/EngineProperties.hpp \
    Sources/EventFilters.hpp \
    Sources/ExtendedTreeView.hpp \
    Sources/FileSystemUtils.hpp \
    Sources/GameOptsDialog.hpp \
    Sources/JsonUtils.hpp \
    Sources/LangUtils.hpp \
    Sources/ListModel.hpp \
    Sources/MainWindow.hpp \
    Sources/MiscUtils.hpp \
    Sources/NewConfigDialog.hpp \
    Sources/OSUtils.hpp \
    Sources/OptionsSerializer.hpp \
    Sources/OptionsStorageDialog.hpp \
    Sources/ProcessOutputWindow.hpp \
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
    Forms/NewConfigDialog.ui \
    Forms/OptionsStorageDialog.ui \
    Forms/ProcessOutputWindow.ui \
    Forms/SetupDialog.ui

RESOURCES += \
    Resources/Resources.qrc

DISTFILES += \
    Resources/DoomRunner.ico

win32:RC_ICONS += Resources/DoomRunner.ico

win32:LIBS += -lole32 -luuid

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
