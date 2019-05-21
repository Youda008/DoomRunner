//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  21.5.2019
// Description: specialized list widget accepting drops of file system paths
//              inspired by github.com/Hypnotoad90/RocketLauncher2/blob/master/dndfilesystemlistview.h
//======================================================================================================================

#ifndef FILE_SYSTEM_DND_LIST_WIDGET_INCLUDED
#define FILE_SYSTEM_DND_LIST_WIDGET_INCLUDED


#include "Common.hpp"

#include <QListWidget>


//======================================================================================================================
/** specialized list widget accepting drops of file system paths
  * emits a signal for each valid existing file system path that is dropped here */

class FileSystemDnDListWidget : public QListWidget {

	Q_OBJECT

 public:

	FileSystemDnDListWidget( QWidget * parent ) : QListWidget( parent ) {}

 protected:

	virtual void dragEnterEvent( QDragEnterEvent * event ) override;
	virtual void dropEvent( QDropEvent * event ) override;

 signals:

	void fileSystemPathDropped( const QString & path );
	void internalItemDropped( QDropEvent * event );

};


#endif // FILE_SYSTEM_DND_LIST_WIDGET_INCLUDED
