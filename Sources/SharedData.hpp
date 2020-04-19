//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: data structures and functionality that is used accross multiple windows/dialogs
//======================================================================================================================

#ifndef SHARED_DATA_INCLUDED
#define SHARED_DATA_INCLUDED


#include "Common.hpp"

#include "DirTreeModel.hpp"

#include <QString>
#include <QVector>
#include <QFileInfo>
#include <QDir>


//======================================================================================================================
//  data structures

// constructors from QFileInfo are used in automatic list updates for initializing element from file-system entry
// getID methods are used in automatic list updates for selecting the same item as before

struct Engine {
	QString name;
	QString path;
	QString configDir;

	Engine() {}
	Engine( const QFileInfo & file ) : name( file.fileName() ), path( file.filePath() ), configDir( file.dir().path() ) {}
	QString getID() const { return path; }
};

struct ConfigFile {
	QString fileName;

	ConfigFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
};

struct SaveFile {
	QString fileName;

	SaveFile( const QFileInfo & file ) : fileName( file.fileName() ) {}
};

struct IWAD {
	QString name;
	QString path;

	IWAD() {}
	IWAD( const QFileInfo & file ) : name( file.fileName() ), path( file.filePath() ) {}
	QString getID() const { return path; }
};

struct Mod {
	QString path;
	QString fileName;  ///< cached last part of path, beware of inconsistencies
	bool checked;

	Mod() {}
	Mod( const QFileInfo & file, bool checked = true ) : path( file.filePath() ), fileName( file.fileName() ), checked( checked ) {}
};

struct Preset {
	QString name;
	QString selectedEnginePath;  // we store the engine by path, so that it does't break when user renames them or reorders them
	QString selectedConfig;  // we store the config by name instead of index, so that it does't break when user reorders them
	QString selectedIWAD;  // we store the IWAD by name instead of index, so that it doesn't break when user reorders them
	TreePosition selectedMapPack;
	QString cmdArgs;
	QVector< Mod > mods;  // this list needs to be kept in sync with mod list widget

	Preset() {}
	Preset( const QString & name ) : name( name ) {}
	Preset( const QFileInfo & ) {} // dummy, it's required by the EditableListModel template, but isn't actually used
};

struct GameplayOptions {
	int32_t flags1 = 0;
	int32_t flags2 = 0;
};

struct CompatibilityOptions {
	int32_t flags1 = 0;
	int32_t flags2 = 0;
};


//======================================================================================================================
//  functions

// just to prevent writing the same lambdas at multiple places
bool isIWAD( const QFileInfo & file );
bool isMapPack( const QFileInfo & file );


#endif // SHARED_DATA_INCLUDED
