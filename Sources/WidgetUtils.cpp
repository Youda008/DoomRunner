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
//  1D list view helpers

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
	view->selectionModel()->clearSelection();
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

QModelIndexList getSelectedRows( QTreeView * view )
{
	QModelIndexList selectedRows;

	// view->selectionModel()->selectedRows()  doesn't work :(

	const QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	for (const QModelIndex & index : selectedIndexes)
		if (index.column() == 0)
			selectedRows.append( index );

	return selectedRows;
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

void deselectItemByIdx( QTreeView * view, const QModelIndex & index )
{
	view->selectionModel()->select( index, QItemSelectionModel::Deselect );
	//view->selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
}

void deselectSelectedItems( QTreeView * view )
{
	view->selectionModel()->clearSelection();
}

void changeSelectionTo( QTreeView * view, const QModelIndex & index )
{
	deselectSelectedItems( view );
	selectItemByIdx( view, index );
}
