//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  21.5.2019
// Description: specialized list widget accepting drops of file system paths
//              inspired by github.com/Hypnotoad90/RocketLauncher2/blob/master/FileSystemDnDListWidget.h
//======================================================================================================================

#include "FileSystemDnDListWidget.hpp"

#include <QDragEnterEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>
#include <QListView>


void FileSystemDnDListWidget::dragEnterEvent( QDragEnterEvent * event )
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	} else if (event->source() != this) {
		event->setDropAction( Qt::CopyAction );
		event->accept();
	} else {
		QListWidget::dragEnterEvent( event );
	}
}

void FileSystemDnDListWidget::dropEvent( QDropEvent * event )
{
	if (event->mimeData()->hasUrls()) {
		for (const QUrl & droppedUrl : event->mimeData()->urls())
		{
			QString localPath = droppedUrl.toLocalFile();
			if (localPath.isEmpty())
				continue;

			QFileInfo fileInfo( localPath );
			if (!fileInfo.exists())
				continue;

			emit fileSystemPathDropped( fileInfo.absoluteFilePath() );
		}
		event->acceptProposedAction();
	} else if (event->source() != this) {
		emit internalItemDropped( event );
	} else {
		QListWidget::dropEvent( event );
	}
}
