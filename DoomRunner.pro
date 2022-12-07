#-------------------------------------------------
#
# Project created by QtCreator 2019-05-13T14:15:57
#
#-------------------------------------------------

QT       += core gui widgets network

TARGET = DoomRunner
TEMPLATE = app

CONFIG += c++17

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

# Some Qt headers use deprecated C++ features and generate tons of warnings.
# This will silence them and prevent them from shadowing our own potentially important warnings.
QMAKE_CXXFLAGS += -Wno-deprecated-copy
QMAKE_CXXFLAGS += -Wno-attributes

QMAKE_CXXFLAGS += -Wno-comment

INCLUDEPATH += Sources

HEADERS += \
    Sources/Dialogs/AboutDialog.hpp \
    Sources/Dialogs/CompatOptsDialog.hpp \
	Sources/Dialogs/DialogCommon.hpp \
    Sources/Dialogs/EngineDialog.hpp \
    Sources/Dialogs/GameOptsDialog.hpp \
    Sources/Dialogs/NewConfigDialog.hpp \
    Sources/Dialogs/OptionsStorageDialog.hpp \
    Sources/Dialogs/OwnFileDialog.hpp \
    Sources/Dialogs/ProcessOutputWindow.hpp \
    Sources/Dialogs/SetupDialog.hpp \
    Sources/Utils/EventFilters.hpp \
    Sources/Utils/FileSystemUtils.hpp \
    Sources/Utils/JsonUtils.hpp \
    Sources/Utils/LangUtils.hpp \
    Sources/Utils/MiscUtils.hpp \
    Sources/Utils/OSUtils.hpp \
    Sources/Utils/WADReader.hpp \
    Sources/Utils/WidgetUtils.hpp \
    Sources/Widgets/EditableListView.hpp \
    Sources/Widgets/ExtendedTreeView.hpp \
	Sources/Widgets/LabelWithHyperlink.hpp \
    Sources/Widgets/ListModel.hpp \
    Sources/Common.hpp \
    Sources/DoomFileInfo.hpp \
    Sources/EngineTraits.hpp \
    Sources/MainWindow.hpp \
    Sources/OptionsSerializer.hpp \
    Sources/Themes.hpp \
    Sources/UpdateChecker.hpp \
    Sources/UserData.hpp \
    Sources/Version.hpp \

SOURCES += \
    Sources/Dialogs/AboutDialog.cpp \
    Sources/Dialogs/CompatOptsDialog.cpp \
	Sources/Dialogs/DialogCommon.cpp \
    Sources/Dialogs/EngineDialog.cpp \
    Sources/Dialogs/GameOptsDialog.cpp \
    Sources/Dialogs/NewConfigDialog.cpp \
    Sources/Dialogs/OptionsStorageDialog.cpp \
    Sources/Dialogs/OwnFileDialog.cpp \
    Sources/Dialogs/ProcessOutputWindow.cpp \
    Sources/Dialogs/SetupDialog.cpp \
    Sources/Utils/EventFilters.cpp \
    Sources/Utils/FileSystemUtils.cpp \
    Sources/Utils/LangUtils.cpp \
    Sources/Utils/JsonUtils.cpp \
    Sources/Utils/MiscUtils.cpp \
    Sources/Utils/OSUtils.cpp \
    Sources/Utils/WADReader.cpp \
    Sources/Utils/WidgetUtils.cpp \
    Sources/Widgets/EditableListView.cpp \
    Sources/Widgets/ExtendedTreeView.cpp \
	Sources/Widgets/LabelWithHyperlink.cpp \
    Sources/Widgets/ListModel.cpp \
    Sources/DoomFileInfo.cpp \
    Sources/EngineTraits.cpp \
    Sources/MainWindow.cpp \
    Sources/OptionsSerializer.cpp \
    Sources/Themes.cpp \
    Sources/UpdateChecker.cpp \
    Sources/UserData.cpp \
    Sources/Version.cpp \
    Sources/main.cpp \

FORMS += \
    Forms/AboutDialog.ui \
    Forms/CompatOptsDialog.ui \
    Forms/EngineDialog.ui \
    Forms/GameOptsDialog.ui \
    Forms/MainWindow.ui \
    Forms/NewConfigDialog.ui \
    Forms/OptionsStorageDialog.ui \
    Forms/ProcessOutputWindow.ui \
    Forms/SetupDialog.ui \

RESOURCES += \
    Resources/Resources.qrc

DISTFILES += \
    Resources/DoomRunner.ico

win32:RC_ICONS += Resources/DoomRunner.ico

win32:LIBS += -lole32 -luuid -ldwmapi

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target
