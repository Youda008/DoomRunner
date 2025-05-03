#-------------------------------------------------
#
# Project created by QtCreator 2019-05-13T14:15:57
#
#-------------------------------------------------

TARGET = DoomRunner

TEMPLATE = app
QT += core gui widgets network
CONFIG -= qml_debug


#-- compiler options -----------------------------

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


#-- sources --------------------------------------

INCLUDEPATH += Sources

HEADERS += \
	Sources/DataModels/AModelItem.hpp \
	Sources/DataModels/GenericListModel.hpp \
	Sources/DataModels/ModelCommon.hpp \
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
	Sources/Utils/ContainerUtils.hpp \
	Sources/Utils/DoomRunnerPacks.hpp \
	Sources/Utils/EnumTraits.hpp \
	Sources/Utils/ErrorHandling.hpp \
	Sources/Utils/EventFilters.hpp \
	Sources/Utils/ExeReader.hpp \
	Sources/Utils/FileInfoCache.hpp \
	Sources/Utils/FileSystemUtils.hpp \
	Sources/Utils/JsonUtils.hpp \
	Sources/Utils/LangUtils.hpp \
	Sources/Utils/MiscUtils.hpp \
	Sources/Utils/OSUtils.hpp \
	Sources/Utils/PathCheckUtils.hpp \
	Sources/Utils/StandardOutput.hpp \
	Sources/Utils/StringUtils.hpp \
	Sources/Utils/TimeStats.hpp \
	Sources/Utils/TypeTraits.hpp \
	Sources/Utils/WADReader.hpp \
	Sources/Utils/WidgetUtils.hpp \
	Sources/Utils/WindowsUtils.hpp \
	Sources/Widgets/ExtendedListView.hpp \
	Sources/Widgets/ExtendedTreeView.hpp \
	Sources/Widgets/ExtendedViewCommon.hpp \
	Sources/Widgets/ExtendedViewCommon.impl.hpp \
	Sources/Widgets/RightClickableLabel.hpp \
	Sources/Widgets/RightClickableButton.hpp \
	Sources/Widgets/SearchPanel.hpp \
	Sources/CommonTypes.hpp \
	Sources/DoomFiles.hpp \
	Sources/EngineTraits.hpp \
	Sources/Essential.hpp \
	Sources/MainWindow.hpp \
	Sources/OptionsSerializer.hpp \
	Sources/Themes.hpp \
	Sources/UpdateChecker.hpp \
	Sources/UserData.hpp \
	Sources/Version.hpp \

SOURCES += \
	Sources/DataModels/GenericListModel.cpp \
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
	Sources/Utils/ContainerUtils.cpp \
	Sources/Utils/DoomRunnerPacks.cpp \
	Sources/Utils/ErrorHandling.cpp \
	Sources/Utils/EventFilters.cpp \
	Sources/Utils/ExeReader.cpp \
	Sources/Utils/FileInfoCache.cpp \
	Sources/Utils/FileSystemUtils.cpp \
	Sources/Utils/LangUtils.cpp \
	Sources/Utils/JsonUtils.cpp \
	Sources/Utils/MiscUtils.cpp \
	Sources/Utils/OSUtils.cpp \
	Sources/Utils/PathCheckUtils.cpp \
	Sources/Utils/StandardOutput.cpp \
	Sources/Utils/StringUtils.cpp \
	Sources/Utils/TypeTraitsTest.cpp \
	Sources/Utils/WADReader.cpp \
	Sources/Utils/WidgetUtils.cpp \
	Sources/Utils/WindowsUtils.cpp \
	Sources/Widgets/ExtendedListView.cpp \
	Sources/Widgets/ExtendedTreeView.cpp \
	Sources/Widgets/RightClickableLabel.cpp \
	Sources/Widgets/RightClickableButton.cpp \
	Sources/Widgets/SearchPanel.cpp \
	Sources/CommonTypes.cpp \
	Sources/DoomFiles.cpp \
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


# To set an icon for the exe file we need to use the Windows resource system, see https://doc.qt.io/qt-6/appicon.html
win32: RC_ICONS += Resources/DoomRunner.ico
macx: ICON = Resources/DoomRunner.icns


#-- build type variables -------------------------

CONFIG(debug, debug|release) {
	DEFINES += DEBUG
	DEFINES += IS_DEBUG_BUILD=true
} else {
	DEFINES += NDEBUG  # to disable asserts
	DEFINES += IS_DEBUG_BUILD=false
}

win32 {
	DEFINES += IS_WINDOWS=true
	DEFINES += IS_MACOS=false
} else: macx {
	DEFINES += IS_WINDOWS=false
	DEFINES += IS_MACOS=true
} else {
	DEFINES += IS_WINDOWS=false
	DEFINES += IS_MACOS=false
}


#-- libraries ------------------------------------

win32: LIBS += -lole32 -luuid -ldwmapi -lversion


#-- user configuration ---------------------------

# add "CONFIG+=flatpak" to the qmake command to activate this
flatpak {
	DEFINES += FLATPAK_BUILD
	DEFINES += IS_FLATPAK_BUILD=true
} else {
	DEFINES += IS_FLATPAK_BUILD=false
}


#-- deployment -----------------------------------

# add "INSTALL_DIR=/custom/path" to the qmake command to override this default value
isEmpty(INSTALL_DIR): INSTALL_DIR = /usr/bin

unix: !android
{
	target.path = $$INSTALL_DIR
}

!isEmpty(target.path): INSTALLS += target
