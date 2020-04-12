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

#include <QString>
#include <QVector>

class QFileInfo;
class PathHelper;


//======================================================================================================================
//  data structures

struct Engine {
	QString name;
	QString path;
	QString configDir;
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
	QVector< Mod > mods;  // this list needs to be kept in sync with mod list widget
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

// just to prevent writing the same long lambdas at multiple places
bool isIWAD( const QFileInfo & file );
bool isMapPack( const QFileInfo & file );

class IWADfromFileMaker {
	const PathHelper & pathHelper;
 public:
	IWADfromFileMaker( const PathHelper & pathHelper ) : pathHelper( pathHelper ) {}
	IWAD operator()( const QFileInfo & file );
};


#endif // SHARED_DATA_INCLUDED
