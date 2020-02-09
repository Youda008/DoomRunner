//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: declaration of shared data structs used accross multiple windows
//======================================================================================================================

#ifndef SHARED_DATA_INCLUDED
#define SHARED_DATA_INCLUDED


#include "Common.hpp"

#include "ItemModels.hpp"

#include <QString>
#include <QList>
#include <QListView>

class PathHelper;


//======================================================================================================================
//  this needs to be in a separate header, so it can be included from multiple windows

struct Engine {
	QString name;
	QString path;
	// used in automatic list updates for selecting the same item as before
	QString getID() const { return path; }
};

struct IWAD {
	QString name;
	QString path;
	// used in automatic list updates for selecting the same item as before
	QString getID() const { return path; }
};

struct Mod {
	QString name;
	QString path;
	bool checked;
};

struct Preset {
	QString name;
	QString selectedEnginePath;  // we store the engine by path, so that it does't break when user renames them or reorders them
	QString selectedConfig;  // we store the engine by name, so that it does't break when user reorders them
	QString selectedIWAD;  // we store the IWAD by name, so that it doesn't break when user reorders them
	//TreePath selectedMapPack;
	QList< Mod > mods;  // this list needs to be kept in sync with mod list widget
};

struct GameplayOptions {
	int32_t flags1 = 0;
	int32_t flags2 = 0;
};

struct CompatibilityOptions {
	int32_t flags1 = 0;
	int32_t flags2 = 0;
};


// useful for debugging purposes, easier to set breakpoint inside than in lambdas
QString makeEngineDispStrFromName( const Engine & engine );
QString makeEngineDispStrWithPath( const Engine & engine );
QString makeIwadDispStrFromName( const IWAD & iwad );
QString makeIwadDispStrWithPath( const IWAD & iwad );


// extracted here, because MainWindow and SetupDialog both want to use it
void updateIWADsFromDir( AItemListModel< IWAD > & iwads, QListView * iwadView, QString iwadDir, bool recursively,
                         PathHelper & pathHelper );


#endif // SHARED_DATA_INCLUDED
