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
#include <QAbstractButton>


namespace wdg {


//======================================================================================================================
// selection manipulation


//----------------------------------------------------------------------------------------------------------------------
// generic view helpers


// current item

QModelIndex getCurrentItemIndex( QAbstractItemView * view )
{
	return view->selectionModel()->currentIndex();
}

void setCurrentItemByIndex( QAbstractItemView * view, const QModelIndex & index )
{
	view->selectionModel()->setCurrentIndex( index, QItemSelectionModel::NoUpdate );
	view->scrollTo( index, QListView::ScrollHint::EnsureVisible );
}

void unsetCurrentItem( QAbstractItemView * view )
{
	view->selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::NoUpdate );
}


// selected items

bool isSelectedIndex( QAbstractItemView * view, const QModelIndex & index )
{
	return view->selectionModel()->isSelected( index );
}

bool isSomethingSelected( QAbstractItemView * view )
{
	return !view->selectionModel()->selectedIndexes().isEmpty();
}

QModelIndex getSelectedItemIndex( QAbstractItemView * view )
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.empty())
	{
		return {};
	}
	if (selectedIndexes.size() > 1)
	{
		reportLogicError( view->parentWidget(), {}, "Multiple items selected", "Multiple items are selected." );
		return {};
	}
	return selectedIndexes[0];
}

QModelIndexList getSelectedItemIndexes( QAbstractItemView * view )
{
	return view->selectionModel()->selectedIndexes();
}

QModelIndexList getSelectedRows( QAbstractItemView * view )
{
	QModelIndexList selectedRows;

	// view->selectionModel()->selectedRows()  doesn't work :(

	const QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	for (const QModelIndex & index : selectedIndexes)
		if (index.column() == 0)
			selectedRows.append( index );

	return selectedRows;
}

void selectItemByIndex( QAbstractItemView * view, const QModelIndex & index )
{
	view->selectionModel()->select( index, QItemSelectionModel::Select );
}

void deselectItemByIndex( QAbstractItemView * view, const QModelIndex & index )
{
	view->selectionModel()->select( index, QItemSelectionModel::Deselect );
}

void deselectSelectedItems( QAbstractItemView * view )
{
	view->selectionModel()->clearSelection();
}


// high-level control

void selectAndSetCurrentByIndex( QAbstractItemView * view, const QModelIndex & index )
{
	selectItemByIndex( view, index );
	setCurrentItemByIndex( view, index );
}

void deselectAllAndUnsetCurrent( QAbstractItemView * view )
{
	deselectSelectedItems( view );
	unsetCurrentItem( view );
}

void chooseItemByIndex( QAbstractItemView * view, const QModelIndex & index )
{
	deselectSelectedItems( view );
	selectItemByIndex( view, index );
	setCurrentItemByIndex( view, index );
}

void selectSetCurrentAndScrollTo( QAbstractItemView * view, const QModelIndex & index )
{
	const auto origHorizontalPos = view->horizontalScrollBar()->value();
	selectAndSetCurrentByIndex( view, index );
	scrollToItemAtIndex( view, index );  // this sets the horizontal scrollbar to middle, so it needs to be set back
	view->horizontalScrollBar()->setValue( origHorizontalPos );
}


//----------------------------------------------------------------------------------------------------------------------
// 1D list view convenience wrappers


static QAbstractItemView * toAbstract( QListView * view )
{
	return static_cast< QAbstractItemView * >( view );
}


// current item

int getCurrentItemIndex( QListView * view )
{
	QModelIndex currentIndex = getCurrentItemIndex( toAbstract( view ) );
	return currentIndex.isValid() ? currentIndex.row() : -1;
}

void setCurrentItemByIndex( QListView * view, int index )
{
	setCurrentItemByIndex( toAbstract( view ), view->model()->index( index, 0 ) );
}


// selected items

bool isSelectedIndex( QListView * view, int index )
{
	return isSelectedIndex( toAbstract( view ), view->model()->index( index, 0 ) );
}

int getSelectedItemIndex( QListView * view )
{
	QModelIndex selectedIndex = getSelectedItemIndex( toAbstract( view ) );
	return selectedIndex.isValid() ? selectedIndex.row() : -1;
}

QList< int > getSelectedItemIndexes( QListView * view )
{
	QList< int > selectedRowIndexes;
	const QModelIndexList selectedModelIndexes = getSelectedItemIndexes( toAbstract( view ) );
	for (const QModelIndex & index : selectedModelIndexes)
		selectedRowIndexes.append( index.row() );
	return selectedRowIndexes;
}

void selectItemByIndex( QListView * view, int index )
{
	selectItemByIndex( toAbstract( view ), view->model()->index( index, 0 ) );
}

void deselectItemByIndex( QListView * view, int index )
{
	deselectItemByIndex( toAbstract( view ), view->model()->index( index, 0 ) );
}


// high-level control

void selectAndSetCurrentByIndex( QListView * view, int index )
{
	selectAndSetCurrentByIndex( toAbstract( view ), view->model()->index( index, 0 ) );
}

void chooseItemByIndex( QListView * view, int index )
{
	chooseItemByIndex( toAbstract( view ), view->model()->index( index, 0 ) );
}

void selectSetCurrentAndScrollTo( QListView * view, int index )
{
	selectSetCurrentAndScrollTo( toAbstract( view ), view->model()->index( index, 0 ) );
}


//----------------------------------------------------------------------------------------------------------------------
// row-oriented table view convenience wrappers


static QAbstractItemView * toAbstract( QTableView * view )
{
	return static_cast< QAbstractItemView * >( view );
}


// current item

int getCurrentRowIndex( QTableView * view )
{
	QModelIndex currentIndex = getCurrentItemIndex( toAbstract( view ) );
	return currentIndex.isValid() ? currentIndex.row() : -1;
}

void setCurrentRowByIndex( QTableView * view, int rowIndex )
{
	setCurrentItemByIndex( toAbstract( view ), view->model()->index( rowIndex, 0 ) );
}

void unsetCurrentRow( QTableView * view )
{
	deselectAllAndUnsetCurrent( toAbstract( view ) );
}


// selected items

bool isSelectedRow( QTableView * view, int rowIndex )
{
	return isSelectedIndex( toAbstract( view ), view->model()->index( rowIndex, 0 ) );
}

int getSelectedRowIndex( QTableView * view )
{
	// For each row there may be multiple columns selected, but we want to count them as a single row.
	int selectedRowIndex = -1;
	const QModelIndexList selectedModelIndexes = getSelectedItemIndexes( toAbstract( view ) );
	for (const QModelIndex & index : selectedModelIndexes)
	{
		if (selectedRowIndex == -1) {
			selectedRowIndex = index.row();
		} else if (selectedRowIndex != index.row()) {
			reportLogicError( view->parentWidget(), {}, "Multiple items selected", "Multiple items are selected." );
			break;
		}
	}
	return selectedRowIndex;
}

QList< int > getSelectedRowIndexes( QTableView * view )
{
	// For each row there may be multiple columns selected, but we want to count them as a single row.
	QSet< int > selectedRowIndexes;
	const QModelIndexList selectedModelIndexes = getSelectedItemIndexes( toAbstract( view ) );
	for (const QModelIndex & index : selectedModelIndexes)
		selectedRowIndexes.insert( index.row() );
	return { selectedRowIndexes.begin(), selectedRowIndexes.end() };
}

void selectRowByIndex( QTableView * view, int rowIndex )
{
	// select all items (columns) in the row
	QModelIndex firstModelIndex = view->model()->index( rowIndex, 0 );
	QModelIndex lastModelIndex = view->model()->index( rowIndex, view->model()->columnCount() - 1 );
	QItemSelection selection( firstModelIndex, lastModelIndex );
	view->selectionModel()->select( selection, QItemSelectionModel::Select );
}

void deselectRowByIndex( QTableView * view, int rowIndex )
{
	// deselect all items (columns) in the row
	QModelIndex firstModelIndex = view->model()->index( rowIndex, 0 );
	QModelIndex lastModelIndex = view->model()->index( rowIndex, view->model()->columnCount() - 1 );
	QItemSelection selection( firstModelIndex, lastModelIndex );
	view->selectionModel()->select( selection, QItemSelectionModel::Deselect );
}

void deselectSelectedRows( QTableView * view )
{
	deselectSelectedItems( toAbstract( view ) );
}


// high-level control

void selectAndSetCurrentRowByIndex( QTableView * view, int rowIndex )
{
	selectRowByIndex( view, rowIndex );
	setCurrentRowByIndex( view, rowIndex );
}

void chooseItemByIndex( QTableView * view, int index )
{
	deselectSelectedRows( view );
	selectRowByIndex( view, index );
	setCurrentRowByIndex( view, index );
}




//======================================================================================================================
// button actions


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

	deselectAllAndUnsetCurrent( widget );

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
// miscellaneous


void expandParentsOfNode( QTreeView * view, const QModelIndex & modelIindex )
{
	for (QModelIndex currentIndex = modelIindex.parent(); currentIndex.isValid(); currentIndex = currentIndex.parent())
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

void setButtonColor( QAbstractButton * button, QColor color )
{
	/*
	QPalette palette = button->palette();
	palette.setColor( QPalette::Button, color );
	//palette.setColor( QPalette::Button, color );
	button->setAutoFillBackground(true);
	button->setPalette( palette )
	*/
	button->setStyleSheet( QStringLiteral( "background-color: %1; border: none;" ).arg( color.name() ) );
}

void restoreButtonColor( QAbstractButton * button )
{
	//button->setPalette( qApp->palette() );
	button->setStyleSheet( QString() );
}


} // namespace wdg
