//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Qt widget helpers
//======================================================================================================================

#ifndef WIDGET_UTILS_INCLUDED
#define WIDGET_UTILS_INCLUDED


#include "Essential.hpp"

#include "CommonTypes.hpp"
#include "ContainerUtils.hpp"    // findSuch
#include "FileSystemUtils.hpp"   // traverseDirectory
#include "Widgets/ListModel.hpp"
#include "ErrorHandling.hpp"

#include <QAbstractItemView>
#include <QListView>
#include <QTreeView>
#include <QTableView>
#include <QComboBox>
#include <QScrollBar>
#include <QColor>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

class QTableWidget;

#include <functional>
#include <type_traits>


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
//  selection manipulation


//----------------------------------------------------------------------------------------------------------------------
//  list view helpers

// current item
int getCurrentItemIndex( QListView * view );
void setCurrentItemByIndex( QListView * view, int index );
void unsetCurrentItem( QListView * view );

// selected items
bool isSelectedIndex( QListView * view, int index );
bool isSomethingSelected( QListView * view );
int getSelectedItemIndex( QListView * view );  // assumes a single-selection mode, will throw a message box error otherwise
QVector<int> getSelectedItemIndexes( QListView * view );
void selectItemByIndex( QListView * view, int index );
void deselectItemByIndex( QListView * view, int index );
void deselectSelectedItems( QListView * view );

// high-level control
void selectAndSetCurrentByIndex( QListView * view, int index );
void deselectAllAndUnsetCurrent( QListView * view );
/// Deselects currently selected items, selects new one and makes it the current item.
/** Basically equivalent to left-clicking on an item. */
void chooseItemByIndex( QListView * view, int index );


//----------------------------------------------------------------------------------------------------------------------
//  tree view helpers

// current item
QModelIndex getCurrentItemIndex( QTreeView * view );
void setCurrentItemByIndex( QTreeView * view, const QModelIndex & index );
void unsetCurrentItem( QTreeView * view );

// selected items
bool isSelectedIndex( QTreeView * view, const QModelIndex & index );
bool isSomethingSelected( QTreeView * view );
QModelIndex getSelectedItemIndex( QTreeView * view );
QModelIndexList getSelectedItemIndexes( QTreeView * view );
QModelIndexList getSelectedRows( QTreeView * view );
void selectItemByIndex( QTreeView * view, const QModelIndex & index );
void deselectItemByIndex( QTreeView * view, const QModelIndex & index );
void deselectSelectedItems( QTreeView * view );

// high-level control
void selectAndSetCurrentByIndex( QTreeView * view, const QModelIndex & index );
void deselectAllAndUnsetCurrent( QTreeView * view );
/// Deselects currently selected items, selects new one and makes it the current item.
/** Basically equivalent to left-clicking on an item. */
void chooseItemByIndex( QTreeView * view, const QModelIndex & index );


//----------------------------------------------------------------------------------------------------------------------
//  row-oriented table view helpers

// current item
int getCurrentRowIndex( QTableView * view );
void setCurrentRowByIndex( QTableView * view, int rowIndex );
void unsetCurrentRow( QTableView * view );

// selected items
bool isSelectedRow( QTableView * view, int rowIndex );
bool isSomethingSelected( QTableView * view );
int getSelectedRowIndex( QTableView * view );  // assumes a single-selection mode, will throw a message box error otherwise
QVector<int> getSelectedRowIndexes( QTableView * view );
void selectRowByIndex( QTableView * view, int rowIndex );
void deselectRowByIndex( QTableView * view, int rowIndex );
void deselectSelectedRows( QTableView * view );

// high-level control
void selectAndSetCurrentRowByIndex( QTableView * view, int rowIndex );
void deselectAllAndUnsetCurrentRow( QTableView * view );
/// Deselects currently selected rows, selects new one and makes it the current row.
/** Basically equivalent to left-clicking on an item. */
void chooseRowByIndex( QTableView * view, int rowIndex );




//======================================================================================================================
//  button actions - all of these function assume a 1-dimensional non-recursive list view/widget


/// Adds an item to the end of the list and selects it.
template< typename ListModel >
int appendItem( QListView * view, ListModel & model, const typename ListModel::Item & item )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot append item because the model is locked for changes."
		);
		return -1;
	}

	deselectAllAndUnsetCurrent( view );

	model.startAppending( 1 );

	model.append( item );

	model.finishAppending();

	selectAndSetCurrentByIndex( view, model.size() - 1 );  // select the appended item

	return model.size() - 1;
}

/// Adds an item to the begining of the list and selects it.
template< typename ListModel >
void prependItem( QListView * view, ListModel & model, const typename ListModel::Item & item )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot prepend item because the model is locked for changes."
		);
		return;
	}

	deselectAllAndUnsetCurrent( view );

	model.startInserting( 0 );

	model.prepend( item );

	model.finishInserting();

	selectAndSetCurrentByIndex( view, 0 );  // select the prepended item
}

/// Adds an item to the middle of the list and selects it.
template< typename ListModel >
void insertItem( QListView * view, ListModel & model, const typename ListModel::Item & item, int index )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot insert item because the model is locked for changes."
		);
		return;
	}

	deselectAllAndUnsetCurrent( view );

	model.startInserting( index );

	model.insert( index, item );

	model.finishInserting();

	selectAndSetCurrentByIndex( view, index );  // select the inserted item
}

/// Deletes a selected item and attempts to select an item following the deleted one.
/** Returns the index of the selected and deleted item. Pops up a warning box if nothing is selected. */
template< typename ListModel >
int deleteSelectedItem( QListView * view, ListModel & model )
{
	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		if (!model.isEmpty())
			QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	deselectAllAndUnsetCurrent( view );

	model.startDeleting( selectedIdx );

	model.removeAt( selectedIdx );

	model.finishDeleting();

	// try to select some nearest item, so that user can click 'delete' repeatedly to delete all of them
	if (selectedIdx < model.size())
	{
		selectAndSetCurrentByIndex( view, selectedIdx );
	}
	else  // ........................................................... if the deleted item was the last one,
	{
		if (selectedIdx > 0)  // ....................................... and not the only one,
		{
			selectAndSetCurrentByIndex( view, selectedIdx - 1 );  // ... select the previous
		}
	}

	return selectedIdx;
}

/// Deletes all selected items and attempts to select the item following the deleted ones.
/** Pops up a warning box if nothing is selected. */
template< typename ListModel >
QVector<int> deleteSelectedItems( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot delete selected items because the model is locked for changes."
		);
		return {};
	}

	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		if (!model.isEmpty())
			QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the delete, we need them sorted in ascending order
	QVector<int> selectedIndexesAsc;
	for (const QModelIndex & index : selectedIndexes)
		selectedIndexesAsc.push_back( index.row() );
	std::sort( selectedIndexesAsc.begin(), selectedIndexesAsc.end(), []( int idx1, int idx2 ) { return idx1 < idx2; } );

	int firstSelectedIdx = selectedIndexesAsc[0];

	deselectAllAndUnsetCurrent( view );

	model.startCompleteUpdate();

	// delete all the selected items
	uint deletedCnt = 0;
	for (int selectedIdx : selectedIndexesAsc)
	{
		model.removeAt( selectedIdx - deletedCnt );
		deletedCnt++;
	}

	model.finishCompleteUpdate();

	// try to select some nearest item, so that user can click 'delete' repeatedly to delete all of them
	if (firstSelectedIdx < model.size())                       // if the first deleted item index is still within range of existing ones,
	{
		selectAndSetCurrentByIndex( view, firstSelectedIdx );  // select that one,
	}
	else if (!model.isEmpty())                                 // otherwise select the previous, if there is any
	{
		selectAndSetCurrentByIndex( view, firstSelectedIdx - 1 );
	}

	return selectedIndexesAsc;
}

/// Creates a copy of a selected item and selects the newly created one.
/** Returns the index of the originally selected item to be cloned. Pops up a warning box if nothing is selected. */
template< typename ListModel >
int cloneSelectedItem( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot clone selected item because the model is locked for changes."
		);
		return -1;
	}

	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	deselectAllAndUnsetCurrent( view );

	model.startAppending( 1 );

	model.append( model[ selectedIdx ] );

	model.finishAppending();

	// append some postfix to the item name to distinguish it from the original
	QModelIndex newItemIdx = model.index( model.size() - 1, 0 );
	QString origName = model.data( newItemIdx, Qt::EditRole ).toString();
	model.setData( newItemIdx, origName+" - clone", Qt::EditRole );

	model.contentChanged( newItemIdx.row() );

	selectAndSetCurrentByIndex( view, model.size() - 1 );  // select the new item

	return selectedIdx;
}

/// Moves a selected item up and updates the selection to point to the new position.
/** Returns index of the originally selected item before moving. Pops up a warning box if nothing is selected. */
template< typename ListModel >
int moveUpSelectedItem( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot move up selected item because the model is locked for changes."
		);
		return -1;
	}

	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	// if the selected item is the first one, do nothing
	if (selectedIdx == 0)
	{
		return selectedIdx;
	}

	deselectAllAndUnsetCurrent( view );

	model.orderAboutToChange();

	model.move( selectedIdx, selectedIdx - 1 );

	model.orderChanged();

	selectAndSetCurrentByIndex( view, selectedIdx - 1 );  // select the new position of the moved item

	return selectedIdx;
}

/// Moves a selected item down and updates the selection to point to the new position.
/** Returns index of the originally selected item before moving. Pops up a warning box if nothing is selected. */
template< typename ListModel >
int moveDownSelectedItem( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot move down selected item because the model is locked for changes."
		);
		return -1;
	}

	int selectedIdx = getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	// if the selected item is the last one, do nothing
	if (selectedIdx == model.size() - 1)
	{
		return selectedIdx;
	}

	deselectAllAndUnsetCurrent( view );

	model.orderAboutToChange();

	model.move( selectedIdx, selectedIdx + 1 );

	model.orderChanged();

	selectAndSetCurrentByIndex( view, selectedIdx + 1 );  // select the new position of the moved item

	return selectedIdx;
}

/// Moves all selected items up and updates the selection to point to the new position.
/** Pops up a warning box if nothing is selected. */
template< typename ListModel >
QVector<int> moveUpSelectedItems( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot move up selected items because the model is locked for changes."
		);
		return {};
	}

	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the move, we need them sorted in ascending order
	QVector<int> selectedIndexesAsc;
	for (const QModelIndex & index : selectedIndexes)
		selectedIndexesAsc.push_back( index.row() );
	std::sort( selectedIndexesAsc.begin(), selectedIndexesAsc.end(), []( int idx1, int idx2 ) { return idx1 < idx2; } );

	// if the selected items are at the bottom, do nothing
	if (selectedIndexesAsc.first() == 0)
	{
		return {};
	}

	int currentIdx = getCurrentItemIndex( view );

	deselectAllAndUnsetCurrent( view );

	model.orderAboutToChange();

	// do the move and select the new positions
	for (int selectedIdx : selectedIndexesAsc)
	{
		model.move( selectedIdx, selectedIdx - 1 );
		selectItemByIndex( view, selectedIdx - 1 );
	}

	model.orderChanged();

	if (currentIdx >= 1)                                // if the current item was not the first one,
	{
		setCurrentItemByIndex( view, currentIdx - 1 );  // set the previous one as current,
	}
	else                                                // otherwise
	{
		setCurrentItemByIndex( view, 0 );               // set the first one as current
	}

	return selectedIndexesAsc;
}

/// Moves all selected items down and updates the selection to point to the new position.
/** Pops up a warning box if nothing is selected. */
template< typename ListModel >
QVector<int> moveDownSelectedItems( QListView * view, ListModel & model )
{
	if (!model.canBeModified())
	{
		reportBugToUser( view->parentWidget(), "Model cannot be modified",
			"Cannot move down selected items because the model is locked for changes."
		);
		return {};
	}

	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty())
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return {};
	}

	// the list of indexes is not sorted, they are in the order in which user selected them
	// but for the move, we need them sorted in descending order
	QVector<int> selectedIndexesDesc;
	for (const QModelIndex & index : selectedIndexes)
		selectedIndexesDesc.push_back( index.row() );
	std::sort( selectedIndexesDesc.begin(), selectedIndexesDesc.end(), []( int idx1, int idx2 ) { return idx1 > idx2; } );

	// if the selected items are at the top, do nothing
	if (selectedIndexesDesc.first() == model.size() - 1)
	{
		return {};
	}

	int currentIdx = getCurrentItemIndex( view );

	deselectAllAndUnsetCurrent( view );

	model.orderAboutToChange();

	// do the move and select the new positions
	for (int selectedIdx : selectedIndexesDesc)
	{
		model.move( selectedIdx, selectedIdx + 1 );
		selectItemByIndex( view, selectedIdx + 1 );
	}

	model.orderChanged();

	if (currentIdx < model.size() - 1)                    // if the current item was not the last one,
	{
		setCurrentItemByIndex( view, currentIdx + 1 );    // set the next one as current,
	}
	else                                                  // otherwise
	{
		setCurrentItemByIndex( view, model.size() - 1 );  // set the last one as current
	}

	return selectedIndexesDesc;
}

bool editItemAtIndex( QListView * view, int index );




//======================================================================================================================
//  button actions for table widget


/// Adds a row to the end of the table and selects it.
int appendRow( QTableWidget * widget );

/// Deletes a selected row and attempts to select the row following the deleted one.
/** Returns the index of the selected and deleted row. Pops up a warning box if nothing is selected. */
int deleteSelectedRow( QTableWidget * widget );

void swapTableRows( QTableWidget * widget, int row1, int row2 );


bool editCellAtIndex( QTableView * view, int row, int column );




//======================================================================================================================
//  complete update helpers for list-view


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
auto getSelectedItemIDs( QListView * view, const ListModel & model ) -> QStringVec
{
	QStringVec itemIDs;
	for (int selectedItemIdx : getSelectedItemIndexes( view ))
		itemIDs.append( model[ selectedItemIdx ].getID() );
	return itemIDs;
}

/// Attempts to select previously selected items defined by their persistant itemIDs.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
void selectItemsByIDs( QListView * view, const ListModel & model, const QStringVec & itemIDs )
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
bool areSelectionsEqual( const QVector< ItemID > & selection1, const QVector< ItemID > & selection2 )
{
	// The selected indexes are normally ordered in the order in which the user selected them.
	// So to be able to compare them we need to normalize the order first.

	QVector< ItemID > orderedSelection1;
	for (const ItemID & id : selection1)
		orderedSelection1.push_back( id );
	std::sort( orderedSelection1.begin(), orderedSelection1.end() );

	QVector< ItemID > orderedSelection2;
	for (const ItemID & id : selection2)
		orderedSelection2.push_back( id );
	std::sort( orderedSelection2.begin(), orderedSelection2.end() );

	return orderedSelection1 == orderedSelection2;
}

/// Fills a list with entries found in a directory.
template< typename ListModel >
void updateListFromDir( ListModel & model, QListView * view, const QString & dir, bool recursively,
                        const PathConvertor & pathConvertor, std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	using Item = typename ListModel::Item;

	// Doing a differential update (deleting only things that were deleted and adding only things that were added)
	// is not worth here. It's too complicated and prone to bugs and its advantages are too small.
	// Instead we just clear everything and then load it from scratch according to the current state of the directory
	// and update selection and scroll bar.

	// note down the current scroll bar position
	auto scrollPos = view->verticalScrollBar()->value();

	// note down the current item
	auto currentItemID = getCurrentItemID( view, model );

	// note down the selected items
	auto selectedItemIDs = getSelectedItemIDs( view, model );  // empty string when nothing is selected

	deselectAllAndUnsetCurrent( view );

	model.startCompleteUpdate();  // this resets the highlighted item pointed to by a mouse cursor,
	                              // but that's an acceptable drawback, instead of making differential update
	model.clear();

	traverseDirectory( dir, recursively, fs::EntryType::FILE, pathConvertor, [&]( const QFileInfo & file )
	{
		if (isDesiredFile( file ))
		{
			model.append( Item( file ) );
		}
	});

	model.finishCompleteUpdate();

	// restore the selection so that the same file remains selected
	selectItemsByIDs( view, model, selectedItemIDs );

	// restore the current item so that the same file remains current
	setCurrentItemByID( view, model, currentItemID );

	// restore the scroll bar position, so that it doesn't move when an item is selected
	view->verticalScrollBar()->setValue( scrollPos );
}




//======================================================================================================================
//  complete update helpers for combo-box


/// Gets a persistent item ID that survives node shifting, adding or removal.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
QString getCurrentItemID( QComboBox * view, const ListModel & model )
{
	int selectedItemIdx = view->currentIndex();
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/// Attempts to select a previously selected item defined by persistant itemID.
template< typename ListModel >  // Item must have getID() method that returns some kind of persistant unique identifier
bool setCurrentItemByID( QComboBox * view, const ListModel & model, const QString & itemID )
{
	using Item = typename ListModel::Item;

	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model, [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			view->setCurrentIndex( newItemIdx );
			return true;
		}
	}
	return false;
}

/// Fills a combo-box with entries found in a directory.
template< typename ListModel >
void updateComboBoxFromDir( ListModel & model, QComboBox * view, const QString & dir, bool recursively,
                            bool includeEmptyItem, const PathConvertor & pathConvertor,
                            std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	using Item = typename ListModel::Item;

	// note down the currently selected item
	QString lastText = view->currentText();

	view->setCurrentIndex( -1 );

	model.startCompleteUpdate();

	model.clear();

	// in combo-box item cannot be deselected, so we provide an empty item to express "no selection"
	if (includeEmptyItem)
		model.append( QString() );

	traverseDirectory( dir, recursively, fs::EntryType::FILE, pathConvertor, [&]( const QFileInfo & file )
	{
		if (isDesiredFile( file ))
		{
			model.append( Item( file ) );
		}
	});

	model.finishCompleteUpdate();

	// restore the originally selected item, the selection will be reset if the item does not exist in the new content
	// because findText returns -1 which is valid value for setCurrentIndex
	view->setCurrentIndex( view->findText( lastText ) );
}




//======================================================================================================================
//  miscellaneous


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

/// makes a hyperlink for a widget's text
#define HYPERLINK( text, url ) \
	"<a href=\""%url%"\"><span style=\"\">"%text%"</span></a>"


} // namespace wdg


#endif // WIDGET_UTILS_INCLUDED
