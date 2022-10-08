//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Qt widget helpers
//======================================================================================================================

#include "WidgetUtils.hpp"




//======================================================================================================================
//  selection manipulation


//----------------------------------------------------------------------------------------------------------------------
//  1D list view helpers


//  current item

int getCurrentItemIndex( QListView * view )
{
	QModelIndex currentIndex = view->selectionModel()->currentIndex();
	return currentIndex.isValid() ? currentIndex.row() : -1;
}

void setCurrentItemByIndex( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	view->selectionModel()->setCurrentIndex( modelIndex, QItemSelectionModel::NoUpdate );
}

void unsetCurrentItem( QListView * view )
{
	view->selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::NoUpdate );
}


//  selected items

bool isSelectedIndex( QListView * view, int index )
{
	return view->selectionModel()->isSelected( view->model()->index( index, 0 ) );
}

bool isSomethingSelected( QListView * view )
{
	return !view->selectionModel()->selectedIndexes().isEmpty();
}

int getSelectedItemIndex( QListView * view )   // this function is for single selection lists
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

QVector< int > getSelectedItemIndexes( QListView * view )
{
	QVector< int > selected;
	for (QModelIndex & index : view->selectionModel()->selectedIndexes())
		selected.append( index.row() );
	return selected;
}

void selectItemByIndex( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	view->selectionModel()->select( modelIndex, QItemSelectionModel::Select );
}

void deselectItemByIndex( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	view->selectionModel()->select( modelIndex, QItemSelectionModel::Deselect );
}

void deselectSelectedItems( QListView * view )
{
	view->selectionModel()->clearSelection();
}


//  high-level control

void selectAndSetCurrentByIndex( QListView * view, int index )
{
	selectItemByIndex( view, index );
	setCurrentItemByIndex( view, index );
}

void deselectAllAndUnsetCurrent( QListView * view )
{
	deselectSelectedItems( view );
	unsetCurrentItem( view );
}

void chooseItemByIndex( QListView * view, int index )
{
	deselectSelectedItems( view );
	selectItemByIndex( view, index );
	setCurrentItemByIndex( view, index );
}


//----------------------------------------------------------------------------------------------------------------------
//  tree view helpers


//  current item

QModelIndex getCurrentItemIndex( QTreeView * view )
{
	return view->selectionModel()->currentIndex();
}

void setCurrentItemByIndex( QTreeView * view, const QModelIndex & index )
{
	view->selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
}

void unsetCurrentItem( QTreeView * view )
{
	view->selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::NoUpdate );
}


//  selected items

bool isSelectedIndex( QTreeView * view, const QModelIndex & index )
{
	return view->selectionModel()->isSelected( index );
}

bool isSomethingSelected( QTreeView * view )
{
	return !view->selectionModel()->selectedIndexes().isEmpty();
}

QModelIndex getSelectedItemIndex( QTreeView * view )   // this function is for single selection lists
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

QModelIndexList getSelectedItemIndexes( QTreeView * view )
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

void selectItemByIndex( QTreeView * view, const QModelIndex & index )
{
	view->selectionModel()->select( index, QItemSelectionModel::Select );
}

void deselectItemByIndex( QTreeView * view, const QModelIndex & index )
{
	view->selectionModel()->select( index, QItemSelectionModel::Deselect );
}

void deselectSelectedItems( QTreeView * view )
{
	view->selectionModel()->clearSelection();
}


//  high-level control

void selectAndSetCurrentByIndex( QTreeView * view, const QModelIndex & index )
{
	selectItemByIndex( view, index );
	setCurrentItemByIndex( view, index );
}

void deselectAllAndUnsetCurrent( QTreeView * view )
{
	deselectSelectedItems( view );
	unsetCurrentItem( view );
}

void chooseItemByIndex( QTreeView * view, const QModelIndex & index )
{
	deselectSelectedItems( view );
	selectItemByIndex( view, index );
	setCurrentItemByIndex( view, index );
}




//======================================================================================================================
//  button actions


bool editItemAtIndex( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	view->setCurrentIndex( modelIndex );
	view->edit( modelIndex );
	// this returns true even when the editor was not opened using openPersistentEditor()
	return view->isPersistentEditorOpen( modelIndex );
}




//======================================================================================================================
//  miscellaneous


void expandParentsOfNode( QTreeView * view, const QModelIndex & index )
{
	for (QModelIndex currentIndex = index; currentIndex.isValid(); currentIndex = currentIndex.parent())
		if (!view->isExpanded( currentIndex ))
			view->expand( currentIndex );
}
