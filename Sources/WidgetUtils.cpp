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
	if (selectedIndexes.empty())
	{
		return -1;
	}
	if (selectedIndexes.size() > 1)
	{
		QMessageBox::critical( view->parentWidget(), "Multiple items selected",
			"Multiple items are selected. This shouldn't be possible, please create an issue on Github page." );
		return -1;
	}
	return selectedIndexes[0].row();
}

QVector< int > getSelectedItemIdxs( QListView * view )
{
	QVector< int > selected;
	for (QModelIndex & index : view->selectionModel()->selectedIndexes())
		selected.append( index.row() );
	return selected;
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
	if (selectedIndexes.empty())
	{
		return {};
	}
	if (selectedIndexes.size() > 1)
	{
		QMessageBox::critical( view->parentWidget(), "Multiple items selected",
			"Multiple items are selected. This shouldn't be possible, please create an issue on Github page." );
		return {};
	}
	return selectedIndexes[0];
}

bool isSelectedIdx( QTreeView * view, const QModelIndex & index )
{
	return view->selectionModel()->isSelected( index );
}

QModelIndexList getSelectedItemIdxs( QTreeView * view )
{
	return view->selectionModel()->selectedIndexes();
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

QVector< TreePosition > getSelectedItemIDs( QTreeView * view, const DirTreeModel & model )
{
	QVector< TreePosition > itemIDs;
	for (QModelIndex & selectedItemIdx : getSelectedItemIdxs( view ))
		itemIDs.append( model.getNodePosition( selectedItemIdx ) );
	return itemIDs;
}

void selectItemsByID( QTreeView * view, const DirTreeModel & model, const QVector< TreePosition > & itemIDs )
{
	for (const TreePosition & pos : itemIDs)
	{
		QModelIndex itemIdx = model.getNodeByPosition( pos );
		if (itemIdx.isValid())
			selectItemByIdx( view, itemIdx );
	}
}

void updateTreeFromDir( DirTreeModel & model, QTreeView * view, const QString & dir, const PathHelper & pathHelper,
                        std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	// Doing a differential update (deleting only things that were deleted and adding only things that were added)
	// is not worth here. It's too complicated and prone to bugs and its advantages are too small.
	// Instead we just clear everything and then load it from scratch according to the current state of the directory
	// and update selection, scroll bar and dir expansion.

	// note down the current scroll bar position
	auto scrollPos = view->verticalScrollBar()->value();

	// note down the currently selected items
	auto selectedItemIDs = getSelectedItemIDs( view, model );  // empty path when nothing is selected

	deselectSelectedItems( view );

	// note down which directories are expanded
	QHash< QString, bool > expanded;
	model.traverseNodes( [ view, &model, &expanded ]( const QModelIndex & index )
	{
		if (model.isDir( index ))
		{
			expanded[ model.getFSPath( index ) ] = view->isExpanded( index );
		}
	});

	model.startCompleteUpdate();  // this resets the highlighted item pointed to by a mouse cursor

	model.clear();

	model.setBaseDir( dir );  // all new items will be relative to this dir

	fillTreeFromDir( model, QModelIndex(), dir, pathHelper, isDesiredFile );

	model.finishCompleteUpdate();

	// expand those directories that were expanded before
	model.traverseNodes( [ view, &model, &expanded ]( const QModelIndex & index )
	{
		if (model.isDir( index ))
		{
			view->setExpanded( index, expanded[ model.getFSPath( index ) ] );
		}
	});

	// restore the selection so that the same files remain selected
	selectItemsByID( view, model, selectedItemIDs );

	// restore the scroll bar position, so that it doesn't move when an item is selected
	view->verticalScrollBar()->setValue( scrollPos );
}
