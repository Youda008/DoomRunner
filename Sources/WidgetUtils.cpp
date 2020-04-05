//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  6.2.2020
// Description: Qt widget helpers
//======================================================================================================================

#include "WidgetUtils.hpp"

#include <QTreeView>


//======================================================================================================================
//  list view helpers

int getSelectedItemIdx( QListView * view )   // this function is for single selection lists
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
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

bool isSelectedIdx( QListView * view, int index )
{
	return view->selectionModel()->isSelected( view->model()->index( index, 0 ) );
}

bool isSomethingSelected( QListView * view )
{
	return !view->selectionModel()->selectedIndexes().isEmpty();
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
//  tree view helpers

QModelIndex getSelectedItemIdx( QTreeView * view )   // this function is for single selection lists
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.empty()) {
		return {};
	}
	if (selectedIndexes.size() > 1) {
		QMessageBox::critical( view->parentWidget(), "Multiple items selected",
			"Multiple items are selected. This shouldn't be happening and it is a bug. Please create an issue on Github page." );
		return {};
	}
	return selectedIndexes[0];
}

bool isSomethingSelected( QTreeView * view )
{
	return !view->selectionModel()->selectedIndexes().isEmpty();
}

void selectItemByIdx( QTreeView * view, const QModelIndex & index )
{
	view->selectionModel()->select( index, QItemSelectionModel::Select );
	view->selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
}

void deselectSelectedItems( QTreeView * view )
{
	for (QModelIndex & index : view->selectionModel()->selectedIndexes())
		view->selectionModel()->select( index, QItemSelectionModel::Deselect );
}

void changeSelectionTo( QTreeView * view, const QModelIndex & index )
{
	deselectSelectedItems( view );
	selectItemByIdx( view, index );
}

TreePath getSelectedItemID( QTreeView * view, const TreeModel & model )
{
	QModelIndex selectedItemIdx = getSelectedItemIdx( view );
	return model.getItemPath( selectedItemIdx );  // if nothing is selected, this path will be empty
}

bool selectItemByID( QTreeView * view, const TreeModel & model, const TreePath & itemID )
{
	QModelIndex newItemIdx = model.getItemByPath( itemID );  // empty or non-existing path will produce invalid index
	if (newItemIdx.isValid()) {
		selectItemByIdx( view, newItemIdx );
		return true;
	}
	return false;
}

void fillTreeFromDir( TreeModel & model, const QModelIndex & parent, const QString & dir, const QVector< QString > & fileSuffixes )
{
	QDir dir_( dir );
	if (!dir_.exists())
		return;

	// directories first
	QDirIterator dirIt1( dir_ );
	while (dirIt1.hasNext()) {
		dirIt1.next();
		QFileInfo entry = dirIt1.fileInfo();
		if (entry.isDir()) {
			QString dirName = entry.fileName();
			if (dirName != "." && dirName != "..") {
				QModelIndex dirItem = model.addItem( parent, dirName );
				fillTreeFromDir( model, dirItem, entry.filePath(), fileSuffixes );
			}
		}
	}

	// files second
	QDirIterator dirIt2( dir_ );
	while (dirIt2.hasNext()) {
		dirIt2.next();
		QFileInfo entry = dirIt2.fileInfo();
		if (!entry.isDir()) {
			if (fileSuffixes.isEmpty() || fileSuffixes.contains( entry.suffix().toLower() )) {
				model.addItem( parent, entry.fileName() );
			}
		}
	}
}

void updateTreeFromDir( TreeModel & model, QTreeView * view, const QString & dir, const QVector< QString > & fileSuffixes )
{
	if (dir.isEmpty())
		return;

	// write down the currently selected item
	auto selectedItemID = getSelectedItemID( view, model );  // empty path when nothing is selected

	deselectSelectedItems( view );

	model.startCompleteUpdate();  // TODO: perform only diff update, instead of clear-all/insert-all, so that
	                              //       it doesn't always reset highlighting of the item pointer to by a mouse cursor
	model.clear();

	fillTreeFromDir( model, QModelIndex(), dir, fileSuffixes );

	model.finishCompleteUpdate();

	// expand all the directories
	model.traverseItems( [ view, &model ]( const QModelIndex & index ) {
		if (!model.isLeaf( index )) {  // it has children -> it is a directory
			view->setExpanded( index, true );
		}
	});

	// update the selection so that the same file remains selected
	selectItemByID( view, model, selectedItemID );
}
