//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  6.2.2020
// Description: Qt widget helpers
//======================================================================================================================

#include "WidgetUtils.hpp"

#include "DirTreeModel.hpp"

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

TreePath getSelectedItemID( QTreeView * view, const DirTreeModel & model )
{
	QModelIndex selectedItemIdx = getSelectedItemIdx( view );
	return model.getItemPath( selectedItemIdx );  // if nothing is selected, this path will be empty
}

bool selectItemByID( QTreeView * view, const DirTreeModel & model, const TreePath & itemID )
{
	QModelIndex newItemIdx = model.getItemByPath( itemID );  // empty or non-existing path will produce invalid index
	if (newItemIdx.isValid()) {
		selectItemByIdx( view, newItemIdx );
		return true;
	}
	return false;
}

void fillTreeFromDir( DirTreeModel & model, const QModelIndex & parent, const QString & dir, std::function< bool ( const QFileInfo & file ) > isDesiredFile )
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
				QModelIndex dirItem = model.addItem( parent, dirName, NodeType::DIR );
				fillTreeFromDir( model, dirItem, entry.filePath(), isDesiredFile );
			}
		}
	}

	// files second
	QDirIterator dirIt2( dir_ );
	while (dirIt2.hasNext()) {
		dirIt2.next();
		QFileInfo entry = dirIt2.fileInfo();
		if (!entry.isDir()) {
			if (isDesiredFile( entry )) {
				model.addItem( parent, entry.fileName(), NodeType::FILE );
			}
		}
	}
}

void updateTreeFromDir( DirTreeModel & model, QTreeView * view, const QString & dir, std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	if (dir.isEmpty())
		return;

	// Doing a differential update (deleting only things that were deleted and adding only things that were added)
	// is not worth here. It's too complicated and prone to bugs and its advantages are too small.
	// Instead we just clear everything and then load it from scratch according to the current state of the directory
	// and update selection, scroll bar and dir expansion.

	// note down the current scroll bar position
	auto scrollPos = view->verticalScrollBar()->value();

	// note down the currently selected item
	auto selectedItemID = getSelectedItemID( view, model );  // empty path when nothing is selected

	deselectSelectedItems( view );

	// note down which directories are expanded
	QHash< QString, bool > expanded;
	model.traverseItems( [ view, &model, &expanded ]( const QModelIndex & index ) {
		if (model.isDir( index )) {
			expanded[ model.getItemPath( index ).toString() ] = view->isExpanded( index );
		}
	});

	model.startCompleteUpdate();  // this resets the highlighted item pointed to by a mouse cursor

	model.clear();

	fillTreeFromDir( model, QModelIndex(), dir, isDesiredFile );

	model.finishCompleteUpdate();

	// expand those directories that were expanded before
	model.traverseItems( [ view, &model, &expanded ]( const QModelIndex & index ) {
		if (model.isDir( index )) {
			view->setExpanded( index, expanded[ model.getItemPath( index ).toString() ] );
		}
	});

	// restore the selection so that the same file remains selected
	selectItemByID( view, model, selectedItemID );

	// restore the scroll bar position, so that it doesn't move when an item is selected
	view->verticalScrollBar()->setValue( scrollPos );
}
