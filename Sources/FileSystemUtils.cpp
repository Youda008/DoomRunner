//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  5.4.2020
// Description: utilities concerning paths, directories and files
//======================================================================================================================

#include "FileSystemUtils.hpp"

#include "DirTreeModel.hpp"


//======================================================================================================================

void fillTreeFromDir( DirTreeModel & model, const QModelIndex & parent, const QString & dir, const PathContext & pathContext,
                      std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	if (dir.isEmpty())  // dir is not set -> leave the list empty
		return;

	QDir dir_( dir );
	if (!dir_.exists())  // dir is invalid -> leave the list empty
		return;

	// directories first
	QDirIterator dirIt1( dir_ );
	while (dirIt1.hasNext())
	{
		QString entryPath = pathContext.convertPath( dirIt1.next() );
		QFileInfo entry( entryPath );
		if (entry.isDir())
		{
			QString dirName = dirIt1.fileName();  // we need the original entry name including "." and "..", entry is already converted
			if (dirName != "." && dirName != "..")
			{
				QModelIndex dirItem = model.addNode( parent, dirName, NodeType::DIR );
				fillTreeFromDir( model, dirItem, entry.filePath(), pathContext, isDesiredFile );
			}
		}
	}

	// files second
	QDirIterator dirIt2( dir_ );
	while (dirIt2.hasNext())
	{
		QString entryPath = pathContext.convertPath( dirIt2.next() );
		QFileInfo entry( entryPath );
		if (!entry.isDir())
		{
			if (isDesiredFile( entry ))
			{
				model.addNode( parent, entry.fileName(), NodeType::FILE );
			}
		}
	}
}
