//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: miscellaneous utilities
//======================================================================================================================

#include "Utils.hpp"

#include <QListView>
#include <QMessageBox>


//======================================================================================================================
//  list view helpers

int getSelectedItemIdx( QListView * view )   // this function is for single selection lists
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedRows();
	if (selectedIndexes.empty()) {
		return -1;
	}
	if (selectedIndexes.size() > 1) {
		QMessageBox::critical( view->parentWidget(), "Multiple items selected",
			"Multiple items are selected. This shouldn't be happening and it is a bug. Please create an issue on Github page." );
		return -1;
	}
	return selectedIndexes[0].row();
}

void selectItemByIdx( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	view->selectionModel()->select( modelIndex, QItemSelectionModel::Select );
	view->selectionModel()->setCurrentIndex( modelIndex, QItemSelectionModel::NoUpdate );
}

void deselectItemByIdx( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	view->selectionModel()->select( modelIndex, QItemSelectionModel::Deselect );
}

void deselectSelectedItems( QListView * view )
{
	for (QModelIndex & index : view->selectionModel()->selectedIndexes())
		view->selectionModel()->select( index, QItemSelectionModel::Deselect );
}

void changeSelectionTo( QListView * view, int index )
{
	deselectSelectedItems( view );
	selectItemByIdx( view, index );
}


//======================================================================================================================

QString getMapNumber( QString mapName )
{
	if (mapName.startsWith('E')) {
		return mapName[1]+QString(' ')+mapName[3];
	} else {
		return mapName.mid(3,2);
	}
}
