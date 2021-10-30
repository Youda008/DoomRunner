//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  6.2.2020
// Description: Qt widget helpers
//======================================================================================================================

#ifndef WIDGET_UTILS_INCLUDED
#define WIDGET_UTILS_INCLUDED


#include "Common.hpp"

#include "LangUtils.hpp"  // findSuch
#include "ListModel.hpp"
#include "FileSystemUtils.hpp"  // fillListFromDir

#include <QListView>
class QTreeView;
#include <QComboBox>
#include <QScrollBar>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <functional>


//======================================================================================================================
//  1D list view helpers - all of these function assume a 1-dimensional non-recursive list view/widget

//----------------------------------------------------------------------------------------------------------------------
//  selection manipulation

int getSelectedItemIdx( QListView * view );  // assumes a single-selection mode, will throw a message box error otherwise
QVector<int> getSelectedItemIdxs( QListView * view );
bool isSelectedIdx( QListView * view, int index );
bool isSomethingSelected( QListView * view );

void selectItemByIdx( QListView * view, int index );
void deselectItemByIdx( QListView * view, int index );
void deselectSelectedItems( QListView * view );
void changeSelectionTo( QListView * view, int index );

//----------------------------------------------------------------------------------------------------------------------
//  button actions

template< typename Item >
void appendItem( QListView * view, AListModel< Item > & model, const Item & item )
{
	model.startAppending( 1 );

	model.append( item );

	model.finishAppending();

	changeSelectionTo( view, model.size() - 1 );
}

template< typename Item >
void prependItem( QListView * view, AListModel< Item > & model, const Item & item )
{
	model.startInserting( 0 );

	model.prepend( item );

	model.finishInserting();

	changeSelectionTo( view, 0 );
}

template< typename Item >
int deleteSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0)
	{
		if (!model.isEmpty())
			QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	deselectItemByIdx( view, selectedIdx );

	model.startDeleting( selectedIdx );

	model.removeAt( selectedIdx );

	model.finishDeleting();

	// try to select the item that follows after the deleted one
	if (selectedIdx < model.size())
	{
		selectItemByIdx( view, selectedIdx );
	}
	else                                               // if the deleted item was the last one
	{
		if (selectedIdx > 0)                           // and not the only one
		{
			selectItemByIdx( view, selectedIdx - 1 );  // select the previous
		}
	}

	return selectedIdx;
}

template< typename Item >
QVector<int> deleteSelectedItems( QListView * view, AListModel< Item > & model )
{
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

	model.startCompleteUpdate();

	// delete all the selected items
	uint deletedCnt = 0;
	for (int selectedIdx : selectedIndexesAsc)
	{
		deselectItemByIdx( view, selectedIdx );
		model.removeAt( selectedIdx - deletedCnt );
		deletedCnt++;
	}

	model.finishCompleteUpdate();

	// try to select some nearest item, so that user can click 'delete' repeatedly to delete all of them
	if (firstSelectedIdx < model.size())           // if the first deleted item index is still within range of existing ones
	{
		selectItemByIdx( view, firstSelectedIdx ); // select that one
	}
	else if (!model.isEmpty())                     // otherwise select the previous, if there is any
	{
		selectItemByIdx( view, firstSelectedIdx - 1 );
	}

	return selectedIndexesAsc;
}

template< typename Item >
int cloneSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	model.startAppending( 1 );

	model.append( model[ selectedIdx ] );

	model.finishAppending();

	// append some postfix to the item name to distinguish it from the original
	QModelIndex newItemIdx = model.index( model.size() - 1, 0 );
	QString origName = model.data( newItemIdx, Qt::EditRole ).toString();
	model.setData( newItemIdx, origName+" - clone", Qt::EditRole );

	model.contentChanged( newItemIdx.row() );

	changeSelectionTo( view, model.size() - 1 );

	return selectedIdx;
}

template< typename Item >
int moveUpSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
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

	model.move( selectedIdx, selectedIdx - 1 );

	// update selection
	deselectItemByIdx( view, selectedIdx );
	selectItemByIdx( view, selectedIdx - 1 );

	model.contentChanged( selectedIdx - 1 );  // this is basically just swapping content of 2 items

	return selectedIdx;
}

template< typename Item >
int moveDownSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
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

	model.move( selectedIdx, selectedIdx + 1 );

	// update selection
	deselectItemByIdx( view, selectedIdx );
	selectItemByIdx( view, selectedIdx + 1 );

	model.contentChanged( selectedIdx );  // this is basically just swapping content of 2 items

	return selectedIdx;
}

template< typename Item >
QVector<int> moveUpSelectedItems( QListView * view, AListModel< Item > & model )
{
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

	// do the move
	for (int selectedIdx : selectedIndexesAsc)
	{
		model.move( selectedIdx, selectedIdx - 1 );
	}

	// update selection
	deselectItemByIdx( view, selectedIndexesAsc.last() );
	selectItemByIdx( view, selectedIndexesAsc.first() - 1 );

	model.contentChanged( selectedIndexesAsc.first() - 1 );  // this is basically just swapping content of few items

	return selectedIndexesAsc;
}

template< typename Item >
QVector<int> moveDownSelectedItems( QListView * view, AListModel< Item > & model )
{
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

	// do the move
	for (int selectedIdx : selectedIndexesDesc)
	{
		model.move( selectedIdx, selectedIdx + 1 );
	}

	// update selection
	deselectItemByIdx( view, selectedIndexesDesc.last() );
	selectItemByIdx( view, selectedIndexesDesc.first() + 1 );

	model.contentChanged( selectedIndexesDesc.last() );  // this is basically just swapping content of few items

	return selectedIndexesDesc;
}


//----------------------------------------------------------------------------------------------------------------------
//  complete list update helpers

/** gets a persistent item ID that survives node shifting, adding or removal */
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
auto getSelectedItemID( QListView * view, const AListModel< Item > & model ) -> decltype( model[0].getID() )
{
	int selectedItemIdx = getSelectedItemIdx( view );
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/** attempts to select a previously selected item defined by its persistant itemID */
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
bool selectItemByID( QListView * view, const AListModel< Item > & model, const decltype( model[0].getID() ) & itemID )
{
	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model.list(), [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			selectItemByIdx( view, newItemIdx );
			return true;
		}
	}
	return false;
}

/** gets persistent item IDs that survive node shifting, adding or removal */
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
auto getSelectedItemIDs( QListView * view, const AListModel< Item > & model ) -> QVector< decltype( model[0].getID() ) >
{
	QVector< decltype( model[0].getID() ) > itemIDs;
	for (int selectedItemIdx : getSelectedItemIdxs( view ))
		itemIDs.append( model[ selectedItemIdx ].getID() );
	return itemIDs;
}

/** attempts to select previously selected items defined by their persistant itemIDs */
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
void selectItemsByIDs( QListView * view, const AListModel< Item > & model, const QVector< decltype( model[0].getID() ) > & itemIDs )
{
	for (const auto & itemID : itemIDs)
	{
		int newItemIdx = findSuch( model.list(), [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
			selectItemByIdx( view, newItemIdx );
	}
}


template< typename Item >
void updateListFromDir( AListModel< Item > & model, QListView * view, const QString & dir, bool recursively,
                        const PathContext & pathContext, std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	// Doing a differential update (deleting only things that were deleted and adding only things that were added)
	// is not worth here. It's too complicated and prone to bugs and its advantages are too small.
	// Instead we just clear everything and then load it from scratch according to the current state of the directory
	// and update selection and scroll bar.

	// note down the current scroll bar position
	auto scrollPos = view->verticalScrollBar()->value();

	// note down the currently selected item
	auto selectedItemID = getSelectedItemID( view, model );  // empty string when nothing is selected

	deselectSelectedItems( view );

	model.startCompleteUpdate();  // this resets the highlighted item pointed to by a mouse cursor,
	                              // but that's an acceptable drawback, instead of making differential update
	model.clear();

	fillListFromDir( model.list(), dir, recursively, pathContext, isDesiredFile );

	model.finishCompleteUpdate();

	// restore the selection so that the same file remains selected
	selectItemByID( view, model, selectedItemID );

	// restore the scroll bar position, so that it doesn't move when an item is selected
	view->verticalScrollBar()->setValue( scrollPos );
}


//======================================================================================================================
//  combo box helpers

//----------------------------------------------------------------------------------------------------------------------
//  complete box update helpers

/** gets a persistent item ID that survives node shifting, adding or removal */
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
auto getSelectedItemID( QComboBox * view, const AListModel< Item > & model ) -> decltype( model[0].getID() )
{
	int selectedItemIdx = view->currentIndex();
	if (selectedItemIdx >= 0)
		return model[ selectedItemIdx ].getID();
	else
		return {};
}

/** attempts to select a previously selected item defined by persistant itemID */
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
bool selectItemByID( QComboBox * view, const AListModel< Item > & model, const decltype( model[0].getID() ) & itemID )
{
	if (!itemID.isEmpty())
	{
		int newItemIdx = findSuch( model.list(), [&]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0)
		{
			view->setCurrentIndex( newItemIdx );
			return true;
		}
	}
	return false;
}

template< typename Item >
void updateComboBoxFromDir( AListModel< Item > & model, QComboBox * view, const QString & dir, bool recursively,
                            bool includeEmptyItem, const PathContext & pathContext,
                            std::function< bool ( const QFileInfo & file ) > isDesiredFile )
{
	// note down the currently selected item
	QString lastText = view->currentText();

	view->setCurrentIndex( -1 );

	model.startCompleteUpdate();

	model.clear();

	// in combo-box item cannot be deselected, so we provide an empty item to express "no selection"
	if (includeEmptyItem)
		model.append( QString() );

	fillListFromDir( model.list(), dir, recursively, pathContext, isDesiredFile );

	model.finishCompleteUpdate();

	// restore the originally selected item, the selection will be reset if the item does not exist in the new content
	// because findText returns -1 which is valid value for setCurrentIndex
	view->setCurrentIndex( view->findText( lastText ) );
}


//======================================================================================================================
//  tree view helpers


//----------------------------------------------------------------------------------------------------------------------
//  selection manipulation

QModelIndex getSelectedItemIdx( QTreeView * view );
QModelIndexList getSelectedItemIdxs( QTreeView * view );
QModelIndexList getSelectedRows( QTreeView * view );
bool isSelectedIdx( QTreeView * view, const QModelIndex & index );
bool isSomethingSelected( QTreeView * view );

void selectItemByIdx( QTreeView * view, const QModelIndex & index );
void deselectSelectedItems( QTreeView * view );
void changeSelectionTo( QTreeView * view, const QModelIndex & index );


#endif // WIDGET_UTILS_INCLUDED
