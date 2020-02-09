//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  14.5.2019
// Description: declaration of shared data structs used accross multiple windows
//======================================================================================================================

#include "SharedData.hpp"

#include "WidgetUtils.hpp"  // updateListFromDir
#include "Utils.hpp"        // PathHelper

#include <QStringBuilder>


//======================================================================================================================

// useful for debug purposes, easier to set breakpoint inside than in lambdas
QString makeEngineDispStrFromName( const Engine & engine )
{
	return engine.name;
}
QString makeEngineDispStrWithPath( const Engine & engine )
{
	return engine.name % "  [" % engine.path % "]";
}
QString makeIwadDispStrFromName( const IWAD & iwad )
{
	return iwad.name;
}
QString makeIwadDispStrWithPath( const IWAD & iwad )
{
	return iwad.name % "  [" % iwad.path % "]";
}

//----------------------------------------------------------------------------------------------------------------------

void updateIWADsFromDir( AItemListModel< IWAD > & iwadModel, QListView * iwadView, QString iwadDir, bool recursively,
                         PathHelper & pathHelper )
{
	updateListFromDir< IWAD >( iwadModel, iwadView, iwadDir, recursively, {"wad", "iwad", "pk3", "ipk3", "pk7", "ipk7"},
		/*makeItemFromFile*/[ pathHelper ]( const QFileInfo & file ) -> IWAD
		{
			return { file.fileName(), pathHelper.convertPath( file.filePath() ) };
		}
	);
}
