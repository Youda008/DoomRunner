//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Qt widget helpers
//======================================================================================================================

#ifndef WIDGET_UTILS_INCLUDED
#define WIDGET_UTILS_INCLUDED


#include "Essential.hpp"

#include "DataModels/GenericListModel.hpp"
#include "ContainerUtils.hpp"    // findSuch
#include "FileSystemUtils.hpp"   // traverseDirectory
#include "ErrorHandling.hpp"

#include <QAbstractItemView>
#include <QListView>
#include <QTreeView>
#include <QTableView>
#include <QComboBox>
#include <QScrollBar>
#include <QColor>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
class QTableWidget;
class QAbstractButton;

#include <functional>


//======================================================================================================================
// implementation notes
//
// When an item is in edit mode and current index changes, the content of the line editor is dumped into the old
// current item and the edit mode closed. So if you make any changes to the order of the items and then change
// the current item, the editor content gets saved into a wrong item.
// So before any re-ordering, the current item is unset (set to invalid QModelIndex) to force the content dump before
// the reordering, and then set the current item to the new one, after the reordering is done.


namespace wdg {


//======================================================================================================================
// selection manipulation


template< typename ListModel >
auto * getItemByRowIndex( ListModel && model, int rowIdx )
{
	return (rowIdx >= 0 && rowIdx < model.size()) ? &model[ rowIdx ] : nullptr;
}


//----------------------------------------------------------------------------------------------------------------------
// generic view

// current item
QModelIndex getCurrentItemIndex( QAbstractItemView * view );
void setCurrentItemByIndex( QAbstractItemView * view, const QModelIndex & index );
void unsetCurrentItem( QAbstractItemView * view );

// selected items
bool isSelectedIndex( QAbstractItemView * view, const QModelIndex & index );
bool isSomethingSelected( QAbstractItemView * view );
QModelIndex getSelectedItemIndex( QAbstractItemView * view );
QModelIndexList getSelectedItemIndexes( QAbstractItemView * view );
QModelIndexList getSelectedRows( QAbstractItemView * view );
void selectItemByIndex( QAbstractItemView * view, const QModelIndex & index );
void deselectItemByIndex( QAbstractItemView * view, const QModelIndex & index );
void deselectSelectedItems( QAbstractItemView * view );

// high-level control
void selectAndSetCurrentByIndex( QAbstractItemView * view, const QModelIndex & index );
void deselectAllAndUnsetCurrent( QAbstractItemView * view );
/// Deselects currently selected items, selects new one and makes it the current item.
/** Basically equivalent to left-clicking on an item. */
void chooseItemByIndex( QAbstractItemView * view, const QModelIndex & index );
/// Selects an item, sets it as current, and moves the scrollbar so that the item is visible.
void selectSetCurrentAndScrollTo( QAbstractItemView * view, const QModelIndex & index );


//----------------------------------------------------------------------------------------------------------------------
// 1D list view convenience wrappers converting QModelIndex to int or vice versa

// current item
int getCurrentItemIndex( QListView * view );
void setCurrentItemByIndex( QListView * view, int index );

template< typename ListModel >
auto * getCurrentItem( QListView * view, ListModel && model )
{
	return getItemByRowIndex( model, getCurrentItemIndex( view ) );
}

// selected items
bool isSelectedIndex( QListView * view, int index );
int getSelectedItemIndex( QListView * view );  // assumes a single-selection mode, will throw a message box error otherwise
QList<int> getSelectedItemIndexes( QListView * view );
void selectItemByIndex( QListView * view, int index );
void deselectItemByIndex( QListView * view, int index );

template< typename ListModel >
auto * getSelectedItem( QListView * view, ListModel && model )  // assumes a single-selection mode, will throw a message box error otherwise
{
	return getItemByRowIndex( model, getSelectedItemIndex( view ) );
}

// high-level control
void selectAndSetCurrentByIndex( QListView * view, int index );
/// Deselects currently selected items, selects new one and makes it the current item.
/** Basically equivalent to left-clicking on an item. */
void chooseItemByIndex( QListView * view, int index );
/// Selects an item, sets it as current, and moves the scrollbar so that the item is visible.
void selectSetCurrentAndScrollTo( QListView * view, int index );


//----------------------------------------------------------------------------------------------------------------------
// row-oriented table view convenience wrappers

// current item
int getCurrentRowIndex( QTableView * view );
void setCurrentRowByIndex( QTableView * view, int rowIndex );
void unsetCurrentRow( QTableView * view );

template< typename ListModel >
auto * getCurrentItem( QTableView * view, ListModel && model )
{
	return getItemByRowIndex( model, getCurrentRowIndex( view ) );
}

// selected items
bool isSelectedRow( QTableView * view, int rowIndex );
int getSelectedRowIndex( QTableView * view );  // assumes a single-selection mode, will throw a message box error otherwise
QList<int> getSelectedRowIndexes( QTableView * view );
void selectRowByIndex( QTableView * view, int rowIndex );
void deselectRowByIndex( QTableView * view, int rowIndex );
void deselectSelectedRows( QTableView * view );

template< typename ListModel >
auto * getSelectedItem( QTableView * view, ListModel && model )  // assumes a single-selection mode, will throw a message box error otherwise
{
	return getItemByRowIndex( model, getSelectedRowIndex( view ) );
}

// high-level control
void selectAndSetCurrentRowByIndex( QTableView * view, int rowIndex );
/// Deselects currently selected rows, selects new one and makes it the current row.
/** Basically equivalent to left-clicking on an item. */
void chooseRowByIndex( QTableView * view, int rowIndex );


//----------------------------------------------------------------------------------------------------------------------
// combo box

inline int getCurrentItemIndex( QComboBox * comboBox )                 { return comboBox->currentIndex(); }
inline void setCurrentItemByIndex( QComboBox * comboBox, int index )   { comboBox->setCurrentIndex( index ); }
inline void unsetCurrentItem( QComboBox * comboBox )                   { comboBox->setCurrentIndex( -1 ); }

template< typename ListModel >
auto * getCurrentItem( QComboBox * comboBox, ListModel && model )
{
	return getItemByRowIndex( model, getCurrentItemIndex( comboBox ) );
}




//======================================================================================================================
// button actions - all of these function assume a 1-dimensional non-recursive list view/widget


namespace impl {

template< typename IsLessThan >
QList<int> getSortedRows( const QModelIndexList & selectedIndexes, const IsLessThan & isLessThan )
{
	QList<int> selectedRowsAsc;
	selectedRowsAsc.reserve( selectedIndexes.size() );
	for (const QModelIndex & index : selectedIndexes)
		selectedRowsAsc.append( index.row() );
	std::sort( selectedRowsAsc.begin(), selectedRowsAsc.end(), isLessThan );
	return selectedRowsAsc;
}

} // namespace impl


/// Adds an item to the end of the list and selects it.
template< typename ListModel >
int appendItem( QListView * view, ListModel & model, const typename ListModel::Item & item )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::appendItem", "Model cannot be modified",
			"Cannot insert item because the model is locked for changes."
		);
		return -1;
	}

	deselectAllAndUnsetCurrent( view );

	model.startAppendingItems( 1 );
	model.append( item );
	model.finishAppendingItems();
	// we're modifying the model ourselves so we don't need to be notified about it

	selectAndSetCurrentByIndex( view, model.size() - 1 );  // select the appended item

	return model.size() - 1;
}

/// Adds an item to the begining of the list and selects it.
template< typename ListModel >
void prependItem( QListView * view, ListModel & model, const typename ListModel::Item & item )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::prependItem", "Model cannot be modified",
			"Cannot insert item because the model is locked for changes."
		);
		return;
	}

	deselectAllAndUnsetCurrent( view );

	model.startInsertingItems( 0 );
	model.prepend( item );
	model.finishInsertingItems();
	// we're modifying the model ourselves so we don't need to be notified about it

	selectAndSetCurrentByIndex( view, 0 );  // select the prepended item
}

/// Adds an item to the middle of the list and selects it.
template< typename ListModel >
void insertItem( QListView * view, ListModel & model, const typename ListModel::Item & item, int index )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::insertItem", "Model cannot be modified",
			"Cannot insert item because the model is locked for changes."
		);
		return;
	}

	deselectAllAndUnsetCurrent( view );

	model.startInsertingItems( index );
	model.insert( index, item );
	model.finishInsertingItems();
	// we're modifying the model ourselves so we don't need to be notified about it

	selectAndSetCurrentByIndex( view, index );  // select the inserted item
}

/// Removes all selected items and attempts to select the item following the removed ones.
/** Returns sorted indexes of the removed items. Pops up a warning box if nothing is selected. */
template< typename ListModel >
QList<int> removeSelectedItems( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::deleteSelectedItems", "Model cannot be modified",
			"Cannot remove selected items because the model is locked for changes."
		);
		return {};
	}

	const QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		if (!model.isEmpty())
			reportUserError( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the removal, we need them sorted in ascending order
	QList<int> selectedRowsAsc = impl::getSortedRows( selectedIndexes, []( int i1, int i2 ) { return i1 < i2; } );

	int topMostSelectedIdx = selectedRowsAsc.first();

	deselectAllAndUnsetCurrent( view );

	// remove all the selected items
	uint removedCnt = 0;
	for (int selectedIdx : as_const( selectedRowsAsc ))
	{
		model.startRemovingItems( selectedIdx - removedCnt );
		model.removeAt( selectedIdx - removedCnt );  // every removed item shifts the indexes of the following items
		model.finishRemovingItems();
		// we're modifying the model ourselves so we don't need to be notified about it
		removedCnt++;
	}

	// try to select some nearest item, so that user can click 'delete' repeatedly to delete all of them
	if (topMostSelectedIdx < model.size())                       // if the first removed item index is still within range of existing ones,
	{
		selectAndSetCurrentByIndex( view, topMostSelectedIdx );  // select that one,
	}
	else if (!model.isEmpty())                                   // otherwise select the previous, if there is any
	{
		selectAndSetCurrentByIndex( view, topMostSelectedIdx - 1 );
	}

	return selectedRowsAsc;
}

/// Creates a copy of a selected item and selects the newly created one.
/** Returns the index of the originally selected item to be cloned. Pops up a warning box if nothing is selected. */
template< typename ListModel >
int cloneSelectedItem( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::cloneSelectedItem", "Model cannot be modified",
			"Cannot clone selected item because the model is locked for changes."
		);
		return -1;
	}

	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		reportUserError( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}
	auto newItemIdx = model.size();

	deselectAllAndUnsetCurrent( view );

	model.startAppendingItems( 1 );
	model.append( model[ selectedIdx ] );
	model.finishAppendingItems();
	// we're modifying the model ourselves so we don't need to be notified about it

	auto & newItem = model[ newItemIdx ];

	// append some postfix to the item name to distinguish it from the original
	model.startEditingItemData();
	newItem.setEditString( newItem.getEditString() + " - clone" );
	model.finishEditingItemData( newItemIdx, 1, AListModel::onlyEditRole );

	selectAndSetCurrentByIndex( view, newItemIdx );  // select the new item

	return selectedIdx;
}

/// Moves all selected items up and updates the selection to point to the new positions.
/** Returns the original indexes of the selected items before moving. Pops up a warning box if nothing is selected. */
template< typename ListModel >
QList<int> moveSelectedItemsUp( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::moveSelectedItemsUp", "Model cannot be modified",
			"Cannot move selected items because the model is locked for changes."
		);
		return {};
	}

	const QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		reportUserError( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the move, we need them sorted in ascending order
	QList<int> selectedRowsAsc = impl::getSortedRows( selectedIndexes, []( int i1, int i2 ) { return i1 < i2; } );

	// if the selected items are at the top, do nothing
	if (selectedRowsAsc.first() == 0)
	{
		return {};
	}

	int currentIdx = getCurrentItemIndex( view );

	deselectAllAndUnsetCurrent( view );

	model.startReorderingItems();

	// do the move and select the new positions
	for (int selectedIdx : as_const( selectedRowsAsc ))
	{
		model.move( selectedIdx, selectedIdx - 1 );
		selectItemByIndex( view, selectedIdx - 1 );
	}

	model.finishReorderingItems();
	// we're modifying the model ourselves so we don't need to be notified about it

	if (currentIdx >= 1)                                // if the current item was not the first one,
	{
		setCurrentItemByIndex( view, currentIdx - 1 );  // set the previous one as current,
	}
	else                                                // otherwise
	{
		setCurrentItemByIndex( view, 0 );               // set the first one as current
	}

	return selectedRowsAsc;
}

/// Moves all selected items down and updates the selection to point to the new positions.
/** Returns the original indexes of the selected items before moving. Pops up a warning box if nothing is selected. */
template< typename ListModel >
QList<int> moveSelectedItemsDown( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::moveSelectedItemsDown", "Model cannot be modified",
			"Cannot move selected items because the model is locked for changes."
		);
		return {};
	}

	const QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		reportUserError( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the move, we need them sorted in descending order
	QList<int> selectedRowsDesc = impl::getSortedRows( selectedIndexes, []( int i1, int i2 ) { return i1 > i2; } );

	// if the selected items are at the bottom, do nothing
	if (selectedRowsDesc.first() == model.size() - 1)
	{
		return {};
	}

	int currentIdx = getCurrentItemIndex( view );

	deselectAllAndUnsetCurrent( view );

	model.startReorderingItems();

	// do the move and select the new positions
	for (int selectedIdx : as_const( selectedRowsDesc ))
	{
		model.move( selectedIdx, selectedIdx + 1 );
		selectItemByIndex( view, selectedIdx + 1 );
	}

	model.finishReorderingItems();
	// we're modifying the model ourselves so we don't need to be notified about it

	if (currentIdx < model.size() - 1)                    // if the current item was not the last one,
	{
		setCurrentItemByIndex( view, currentIdx + 1 );    // set the next one as current,
	}
	else                                                  // otherwise
	{
		setCurrentItemByIndex( view, model.size() - 1 );  // set the last one as current
	}

	return selectedRowsDesc;
}

/// Moves all selected items to the top of the list and updates the selection to point to the new positions.
/** Returns the original indexes of the selected items before moving. Pops up a warning box if nothing is selected. */
template< typename ListModel >
QList<int> moveSelectedItemsToTop( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::moveSelectedItemsToTop", "Model cannot be modified",
			"Cannot move selected items because the model is locked for changes."
		);
		return {};
	}

	const QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		reportUserError( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the move, we need them sorted in ascending order
	QList<int> selectedRowsAsc = impl::getSortedRows( selectedIndexes, []( int i1, int i2 ) { return i1 < i2; } );

	int currentIdx = getCurrentItemIndex( view );

	deselectAllAndUnsetCurrent( view );

	model.startReorderingItems();

	// do the move and select the new positions
	// move the items from top to bottom so that the remaining indexes remain valid
	int movedCnt = 0;
	for (int selectedIdx : as_const( selectedRowsAsc ))
	{
		int destIdx = movedCnt;  // move below the already moved items
		if (selectedIdx != destIdx)
			model.move( selectedIdx, destIdx );
		selectItemByIndex( view, destIdx );
		if (selectedIdx == currentIdx)  // if we just moved the current item, set the new location as current too
			setCurrentItemByIndex( view, destIdx );
		movedCnt++;
	}

	model.finishReorderingItems();
	// we're modifying the model ourselves so we don't need to be notified about it

	return selectedRowsAsc;
}

/// Moves all selected items to the bottom of the list and updates the selection to point to the new positions.
/** Returns the original indexes of the selected items before moving. Pops up a warning box if nothing is selected. */
template< typename ListModel >
QList<int> moveSelectedItemsToBottom( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportLogicError( view->parentWidget(), u"wdg::moveSelectedItemsToBottom", "Model cannot be modified",
			"Cannot move selected items because the model is locked for changes."
		);
		return {};
	}

	const QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		reportUserError( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the move, we need them sorted in descending order
	QList<int> selectedRowsDesc = impl::getSortedRows( selectedIndexes, []( int i1, int i2 ) { return i1 > i2; } );

	int currentIdx = getCurrentItemIndex( view );

	deselectAllAndUnsetCurrent( view );

	model.startReorderingItems();

	// do the move and select the new positions
	// move the items from bottom to top so that the remaining indexes remain valid
	int movedCnt = 0;
	for (int selectedIdx : as_const( selectedRowsDesc ))
	{
		int destIdx = model.size() - 1 - movedCnt;  // move above the already moved items
		if (selectedIdx != destIdx)
			model.move( selectedIdx, destIdx );
		selectItemByIndex( view, destIdx );
		if (selectedIdx == currentIdx)  // if we just moved the current item, set the new location as current too
			setCurrentItemByIndex( view, destIdx );
		movedCnt++;
	}

	model.finishReorderingItems();
	// we're modifying the model ourselves so we don't need to be notified about it

	return selectedRowsDesc;
}

bool editItemAtIndex( QListView * view, int index );




//======================================================================================================================
// button actions for table widget


/// Adds a row to the end of the table and selects it.
int appendRow( QTableWidget * widget );

/// Removes a selected row and attempts to select the row following the removed one.
/** Returns the index of the selected and removed row. Pops up a warning box if nothing is selected. */
int removeSelectedRow( QTableWidget * widget );

void swapTableRows( QTableWidget * widget, int row1, int row2 );


bool editCellAtIndex( QTableView * view, int row, int column );




//======================================================================================================================
// common complete update helpers


/// Fills a model with entries found in a directory.
template< typename ListModel >
void updateModelFromDir(
	ListModel & model, const QString & dir, bool recursively, bool includeEmptyItem,
	const PathConvertor & pathConvertor, std::function< bool ( const QFileInfo & file ) > isDesiredFile
){
	using Item = typename ListModel::Item;

	// Doing a differential update (removing only things that were deleted and adding only things that were added)
	// is not worth here. It's too complicated and prone to bugs and its advantages are too small.
	// Instead we just clear everything and then load it from scratch according to the current state of the directory
	// and update selection and scroll bar.

	model.startCompleteUpdate();  // this resets the highlighted item pointed to by a mouse cursor,
	                              // but that's an acceptable drawback, instead of making differential update
	clearButKeepAllocated( model );

	// in combo-box item cannot be deselected, so we provide an empty item to express "no selection"
	if (includeEmptyItem)
		model.append( Item() );

	traverseDirectory( dir, recursively, fs::EntryType::FILE, pathConvertor, [&]( const QFileInfo & file )
	{
		if (isDesiredFile( file ))
		{
			model.append( Item( file ) );
		}
	});

	// some operating systems don't traverse the directory entries in alphabetical order, so we need to sort them on our own
	if constexpr (!IS_WINDOWS && !IS_MACOS)
	{
		model.sortByID();  // for most item types, their ID is either their file name or file path
	}

	model.finishCompleteUpdate();
}




//======================================================================================================================
// complete update helpers for list-view


/// Gets a persistent item ID of the current item that survives node shifting, adding or removal.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
QString getCurrentItemID( QListView * view, const ListModel & model )
{
	int selectedItemIdx = getCurrentItemIndex( view );
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/// Gets a persistent item ID of a selected item that survives node shifting, adding or removal.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
QString getSelectedItemID( QListView * view, const ListModel & model )
{
	int selectedItemIdx = getSelectedItemIndex( view );
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/// Attempts to set a previous current item defined by its persistant itemID.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
bool setCurrentItemByID( QListView * view, const ListModel & model, const QString & itemID )
{
	using Item = typename ListModel::Item;

	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			setCurrentItemByIndex( view, newItemIdx );
			return true;
		}
	}
	return false;
}

/// Attempts to select a previously selected item defined by its persistant itemID.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
bool selectItemByID( QListView * view, const ListModel & model, const QString & itemID )
{
	using Item = typename ListModel::Item;

	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			selectItemByIndex( view, newItemIdx );
			return true;
		}
	}
	return false;
}

/// Gets persistent item IDs that survive node shifting, adding or removal.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
auto getSelectedItemIDs( QListView * view, const ListModel & model ) -> QStringList
{
	QStringList itemIDs;
	const auto selectedIndexes = getSelectedItemIndexes( view );
	itemIDs.reserve( selectedIndexes.size() );
	for (int selectedItemIdx : selectedIndexes)
		itemIDs.append( model[ selectedItemIdx ].getID() );
	return itemIDs;
}

/// Attempts to select previously selected items defined by their persistant itemIDs.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
void selectItemsByIDs( QListView * view, const ListModel & model, const QStringList & itemIDs )
{
	using Item = typename ListModel::Item;

	for (const auto & itemID : itemIDs)
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
			selectItemByIndex( view, newItemIdx );
	}
}

/// Compares two selections of persistent item IDs.
template< typename ItemID >
bool areSelectionsEqual( const QList< ItemID > & selection1, const QList< ItemID > & selection2 )
{
	// The selected indexes are normally ordered in the order in which the user selected them.
	// So to be able to compare them we need to normalize the order first.

	QList< ItemID > orderedSelection1;
	orderedSelection1.reserve( selection1.size() );
	for (const ItemID & id : selection1)
		orderedSelection1.append( id );
	std::sort( orderedSelection1.begin(), orderedSelection1.end() );

	QList< ItemID > orderedSelection2;
	orderedSelection2.reserve( selection2.size() );
	for (const ItemID & id : selection2)
		orderedSelection2.append( id );
	std::sort( orderedSelection2.begin(), orderedSelection2.end() );

	return orderedSelection1 == orderedSelection2;
}

/// Fills a list with entries found in a directory.
template< typename ListModel >
void updateListFromDir(
	ListModel & model, QListView * view, const QString & dir, bool recursively,
	const PathConvertor & pathConvertor, std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	// note down the current scroll bar position
	auto scrollPos = view->verticalScrollBar()->value();

	// note down the current item
	auto currentItemID = getCurrentItemID( view, model );

	// note down the selected items
	auto selectedItemIDs = getSelectedItemIDs( view, model );  // empty string when nothing is selected

	deselectAllAndUnsetCurrent( view );

	updateModelFromDir( model, dir, recursively, /*includeEmptyItem*/false, pathConvertor, isDesiredFile );

	// restore the selection so that the same file remains selected
	selectItemsByIDs( view, model, selectedItemIDs );

	// restore the current item so that the same file remains current
	setCurrentItemByID( view, model, currentItemID );

	// restore the scroll bar position, so that it doesn't move when an item is selected
	view->verticalScrollBar()->setValue( scrollPos );
}




//======================================================================================================================
// complete update helpers for combo-box


/// Gets a persistent item ID that survives node shifting, adding or removal.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
QString getCurrentItemID( QComboBox * comboBox, const ListModel & model )
{
	int selectedItemIdx = getCurrentItemIndex( comboBox );
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/// Attempts to select a previously selected item defined by persistant itemID.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
bool setCurrentItemByID( QComboBox * comboBox, const ListModel & model, const QString & itemID )
{
	using Item = typename ListModel::Item;

	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			setCurrentItemByIndex( comboBox, newItemIdx );
			return true;
		}
	}
	return false;
}

/// Fills a combo-box with entries found in a directory.
template< typename ListModel >
void updateComboBoxFromDir(
	ListModel & model, QComboBox * comboBox, const QString & dir, bool recursively, bool includeEmptyItem,
	const PathConvertor & pathConvertor, std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	// note down the currently selected item
	QString lastText = comboBox->currentText();

	unsetCurrentItem( comboBox );

	updateModelFromDir( model, dir, recursively, includeEmptyItem, pathConvertor, isDesiredFile );

	// restore the originally selected item, the selection will be reset if the item does not exist in the new content
	// because findText returns -1 which is valid value for setCurrentIndex
	setCurrentItemByIndex( comboBox, comboBox->findText( lastText ) );
}




//======================================================================================================================
// miscellaneous


/// Determines where to insert new items based on the currently selected items.
int getRowIndexToInsertTo( QAbstractItemView * view );

/// Expands all parent nodes from the selected node up to the root node, so that the selected node is immediately visible.
void expandParentsOfNode( QTreeView * view, const QModelIndex & index );

/// Sets the scrollbar so that the item at index is in the middle of the widget, if possible.
void scrollToItemAtIndex( QAbstractItemView * view, const QModelIndex & modelIndex );

/// Sets the scrollbar so that the item at index is in the middle of the widget, if possible.
void scrollToItemAtIndex( QListView * view, int index );

/// Sets the scrollbar so that the current item is in the middle of the widget, if possible.
void scrollToCurrentItem( QAbstractItemView * view );

/// Changes text color of this widget.
void setTextColor( QWidget * widget, QColor color );

/// Restores all colors of this widget to default.
void restoreColors( QWidget * widget );

/// Changes background color of a button.
void setButtonColor( QAbstractButton * button, QColor color );

/// Restores the background color of a button.
void restoreButtonColor( QAbstractButton * button );

/// makes a hyperlink for a widget's text
#define HYPERLINK( text, url ) \
	"<a href=\""%url%"\"><span style=\"\">"%text%"</span></a>"




//======================================================================================================================


} // namespace wdg


#endif // WIDGET_UTILS_INCLUDED
