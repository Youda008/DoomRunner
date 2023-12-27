//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Qt widget helpers
//======================================================================================================================

#include "WidgetUtils.hpp"

#include <QPalette>
#include <QApplication>
#include <QTableWidget>


namespace wdg {



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
	view->scrollTo( modelIndex, QListView::ScrollHint::EnsureVisible );
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
		reportLogicError( view->parentWidget(), "Multiple items selected", "Multiple items are selected." );
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
	view->scrollTo( index, QListView::ScrollHint::EnsureVisible );
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
		reportLogicError( view->parentWidget(), "Multiple items selected", "Multiple items are selected." );
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


//----------------------------------------------------------------------------------------------------------------------
//  row-oriented table view helpers


//  current item

int getCurrentRowIndex( QTableView * view )
{
	QModelIndex currentIndex = view->selectionModel()->currentIndex();
	return currentIndex.isValid() ? currentIndex.row() : -1;
}

void setCurrentRowByIndex( QTableView * view, int rowIndex )
{
	QModelIndex modelIndex = view->model()->index( rowIndex, 0 );
	view->selectionModel()->setCurrentIndex( modelIndex, QItemSelectionModel::NoUpdate );
}

void unsetCurrentRow( QTableView * view )
{
	view->selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::NoUpdate );
}


//  selected items

bool isSelectedRow( QTableView * view, int rowIndex )
{
	return view->selectionModel()->isSelected( view->model()->index( rowIndex, 0 ) );
}

bool isSomethingSelected( QTableView * view )
{
	return !view->selectionModel()->selectedIndexes().isEmpty();
}

int getSelectedRowIndex( QTableView * view )   // this function is for single selection tables
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.empty())
	{
		return -1;
	}
	// there can be multiple cells selected if the whole row is selected
	return selectedIndexes[0].row();
}

QVector< int > getSelectedRowIndexes( QTableView * view )
{
	QVector< int > selected;
	for (QModelIndex & index : view->selectionModel()->selectedIndexes())
		selected.append( index.row() );
	return selected;
}

void selectRowByIndex( QTableView * view, int rowIndex )
{
	QModelIndex firstModelIndex = view->model()->index( rowIndex, 0 );
	QModelIndex lastModelIndex = view->model()->index( rowIndex, view->model()->columnCount() - 1 );
	QItemSelection selection( firstModelIndex, lastModelIndex );
	view->selectionModel()->select( selection, QItemSelectionModel::Select );
}

void deselectRowByIndex( QTableView * view, int rowIndex )
{
	QModelIndex modelIndex = view->model()->index( rowIndex, 0 );
	view->selectionModel()->select( modelIndex, QItemSelectionModel::Deselect );
}

void deselectSelectedRows( QTableView * view )
{
	view->selectionModel()->clearSelection();
}


//  high-level control

void selectAndSetCurrentRowByIndex( QTableView * view, int rowIndex )
{
	selectRowByIndex( view, rowIndex );
	setCurrentRowByIndex( view, rowIndex );
}

void deselectAllAndUnsetCurrentRow( QTableView * view )
{
	deselectSelectedRows( view );
	unsetCurrentRow( view );
}

void chooseItemByIndex( QTableView * view, int index )
{
	deselectSelectedRows( view );
	selectRowByIndex( view, index );
	setCurrentRowByIndex( view, index );
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

bool editCellAtIndex( QTableView * view, int row, int column )
{
	QModelIndex modelIndex = view->model()->index( row, column );
	view->setCurrentIndex( modelIndex );
	view->edit( modelIndex );
	// this returns true even when the editor was not opened using openPersistentEditor()
	return view->isPersistentEditorOpen( modelIndex );
}

int appendRow( QTableWidget * widget )
{
	int newRowIdx = widget->rowCount();

	widget->insertRow( newRowIdx );
	for (int columnIdx = 0; columnIdx < widget->columnCount(); ++columnIdx)
		widget->setItem( newRowIdx, columnIdx, new QTableWidgetItem({}) );

	selectAndSetCurrentRowByIndex( widget, newRowIdx );

	return newRowIdx;
}

int deleteSelectedRow( QTableWidget * widget )
{
	int selectedIdx = getSelectedRowIndex( widget );
	if (selectedIdx < 0)
	{
		if (widget->rowCount() > 0)
			reportUserError( widget->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	deselectAllAndUnsetCurrentRow( widget );

	widget->removeRow( selectedIdx );

	// try to select some nearest row, so that user can click 'delete' repeatedly to delete all of them
	if (selectedIdx < widget->rowCount())
	{
		selectAndSetCurrentRowByIndex( widget, selectedIdx );
	}
	else  // ................................................................ if the deleted row was the last one,
	{
		if (selectedIdx > 0)  // ............................................ and not the only one,
		{
			selectAndSetCurrentRowByIndex( widget, selectedIdx - 1 );  // ... select the previous
		}
	}

	return selectedIdx;
}

void swapTableRows( QTableWidget * widget, int row1, int row2 )
{
	for (int column = 0; column < widget->columnCount(); ++column)
	{
		QTableWidgetItem * item1 = widget->takeItem( row1, column );
		QTableWidgetItem * item2 = widget->takeItem( row2, column );
		widget->setItem( row1, column, item2 );
		widget->setItem( row2, column, item1 );
	}
}




//======================================================================================================================
//  miscellaneous


void expandParentsOfNode( QTreeView * view, const QModelIndex & modelIindex )
{
	for (QModelIndex currentIndex = modelIindex; currentIndex.isValid(); currentIndex = currentIndex.parent())
		if (!view->isExpanded( currentIndex ))
			view->expand( currentIndex );
}

void scrollToItemAtIndex( QAbstractItemView * view, const QModelIndex & modelIndex )
{
	view->scrollTo( modelIndex, QListView::ScrollHint::PositionAtCenter );
}

void scrollToCurrentItem( QAbstractItemView * view )
{
	QModelIndex currentIndex = view->currentIndex();
	scrollToItemAtIndex( view, currentIndex );
}

void scrollToItemAtIndex( QListView * view, int index )
{
	QModelIndex modelIndex = view->model()->index( index, 0 );
	scrollToItemAtIndex( view, modelIndex );
}

void setTextColor( QWidget * widget, QColor color )
{
	QPalette palette = widget->palette();
	palette.setColor( QPalette::Text, color );
	widget->setPalette( palette );
}

void restoreColors( QWidget * widget )
{
	widget->setPalette( qApp->palette() );
}



} // namespace wdg
