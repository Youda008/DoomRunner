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

#include "LangUtils.hpp"
#include "ListModel.hpp"
#include "DirTreeModel.hpp"

#include <QListView>
class QTreeView;
#include <QComboBox>
#include <QScrollBar>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>
#include <functional>


//======================================================================================================================
//  list view helpers - all of these function assume a 1-dimensional non-recursive list view/widget

//----------------------------------------------------------------------------------------------------------------------
//  selection manipulation

/// assumes a single-selection mode, will throw a message box error otherwise
int getSelectedItemIdx( QListView * view );
bool isSelectedIdx( QListView * view, int index );
bool isSomethingSelected( QListView * view );

void selectItemByIdx( QListView * view, int index );
void deselectItemByIdx( QListView * view, int index );
void deselectSelectedItems( QListView * view );
void changeSelectionTo( QListView * view, int index );

//----------------------------------------------------------------------------------------------------------------------
//  button actions

template< typename Item >
void appendItem( AListModel< Item > & model, const Item & item )
{
	model.startAppending( 1 );

	model.append( item );

	model.finishAppending();
}

template< typename Item >
int deleteSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		if (!model.isEmpty())
			QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	deselectItemByIdx( view, selectedIdx );

	model.startCompleteUpdate();

	model.removeAt( selectedIdx );

	model.finishCompleteUpdate();

	// restore selection
	if (selectedIdx < model.size()) {                  // try to select the item that follows after the deleted one
		selectItemByIdx( view, selectedIdx );
	} else {                                           // if the deleted item was the last one
		if (selectedIdx > 0) {                         // and not the only one
			selectItemByIdx( view, selectedIdx - 1 );  // select the previous
		}
	}

	return selectedIdx;
}

template< typename Item >
QVector<int> deleteSelectedItems( QListView * view, AListModel< Item > & model )
{
	QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
	if (selectedIndexes.isEmpty()) {
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
	for (int selectedIdx : selectedIndexesAsc) {
		deselectItemByIdx( view, selectedIdx );
		model.removeAt( selectedIdx - deletedCnt );
		deletedCnt++;
	}

	model.finishCompleteUpdate();

	// try to select some nearest item, so that user can click 'delete' repeatedly to delete all of them
	if (firstSelectedIdx < model.size()) {  // if the first deleted item index is still within range of existing ones
		selectItemByIdx( view, firstSelectedIdx ); // select that one
	} else if (!model.isEmpty()) {          // otherwise select the previous, if there is any
		selectItemByIdx( view, firstSelectedIdx - 1 );
	}

	return selectedIndexesAsc;
}

template< typename Item >
int cloneSelectedItem( QListView * view, AListModel< Item > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	model.startAppending();

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
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}
	if (selectedIdx == 0) {  // if the selected item is the first one, do nothing
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
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}
	if (selectedIdx == model.size() - 1) {  // if the selected item is the last one, do nothing
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
	if (selectedIndexes.isEmpty()) {
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
		return {};

	// do the move
	for (int selectedIdx : selectedIndexesAsc)
		model.move( selectedIdx, selectedIdx - 1 );

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
	if (selectedIndexes.isEmpty()) {
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
		return {};

	// do the move
	for (int selectedIdx : selectedIndexesDesc)
		model.move( selectedIdx, selectedIdx + 1 );

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

/** attempts to select a previously selected item defined by persistant itemID */
template< typename Item >  // Item must have getID() method that returns some kind of persistant unique identifier
bool selectItemByID( QListView * view, const AListModel< Item > & model, const decltype( model[0].getID() ) & itemID )
{
	if (!itemID.isEmpty()) {
		int newItemIdx = findSuch( model.list(), [ &itemID ]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0) {
			selectItemByIdx( view, newItemIdx );
			return true;
		}
	}
	return false;
}

template< typename Item >
void fillListFromDir( AListModel< Item > & model, const QString & dir, bool recursively,
                      std::function< bool ( const QFileInfo & file ) > isDesiredFile,
                      std::function< Item ( const QFileInfo & file ) > makeItemFromFile )
{
	QDir dir_( dir );
	if (!dir_.exists())
		return;

	QDirIterator dirIt( dir_ );
	while (dirIt.hasNext()) {
		QFileInfo entry( dirIt.next() );
		if (entry.isDir()) {
			QString dirName = entry.fileName();
			if (recursively && dirName != "." && dirName != "..") {
				fillListFromDir( model, entry.filePath(), recursively, isDesiredFile, makeItemFromFile );
			}
		} else {
			if (isDesiredFile( entry )) {
				// Because Item is generic template param, we don't know to construct it from file.
				// So the caller needs to describe it by a function.
				model.append( makeItemFromFile( entry ) );
			}
		}
	}
}

template< typename Item >
void updateListFromDir( AListModel< Item > & model, QListView * view, const QString & dir, bool recursively,
                        std::function< bool ( const QFileInfo & file ) > isDesiredFile,
                        std::function< Item ( const QFileInfo & file ) > makeItemFromFile )
{
	if (dir.isEmpty())
		return;

	// Doing a differential update (deleting only things that were deleted and adding only things that were added)
	// is not worth here. It's too complicated and prone to bugs and its advantages are too small.
	// Instead we just clear everything and then load it from scratch according to the current state of the directory
	// and update selection and scroll bar.

	// note down the current scroll bar position
	auto scrollPos = view->verticalScrollBar()->value();

	// note down the currently selected item
	auto selectedItemID = getSelectedItemID( view, model );  // empty string when nothing is selected

	deselectSelectedItems( view );

	//model.startCompleteUpdate();

	model.clear();

	fillListFromDir( model, dir, recursively, isDesiredFile, makeItemFromFile );

	//model.finishCompleteUpdate();  // this resets the highlighted item pointed to by a mouse cursor
	model.contentChanged(0);         // and this is an acceptable workaround, instead of differential update

	// restore the selection so that the same file remains selected
	selectItemByID( view, model, selectedItemID );

	// restore the scroll bar position, so that it doesn't move when an item is selected
	view->verticalScrollBar()->setValue( scrollPos );
}
/*
template< typename Item >
void updateListFromDirNew( AListModel< Item > & model, const QString & dir, bool recursively,
                           std::function< bool ( const QFileInfo & file ) > isDesiredFile,
                           std::function< Item ( const QFileInfo & file ) > makeItemFromFile )
{
	if (dir.isEmpty())
		return;

	modelIt = model.begin();
	dirIt = dir.begin();

	while (modelIt != modelEnd && dirIt != dirEnd)
	{
		if (modelIt->name == dirIt->name) {
			dirIt++;
			modelEnd++;
		} else (modelIt->name < dirIt->name) { // file has been deleted from directory
			begin remove rows
			remove row
			end remove rows
			modelIt++
		} else (modelIt->name > dirIt->name) { // file has been added to directory
			begin insert rows
			insert row
			end insert rows
			dirIt++
		}
	}

	while (modelIt != modelEnd)
	{
		remove row
	}

	while (dirIt != dirEnd)
	{
		append row
	}
}
*/

//======================================================================================================================
//  tree view helpers


//----------------------------------------------------------------------------------------------------------------------
//  selection manipulation

QModelIndex getSelectedItemIdx( QTreeView * view );
bool isSomethingSelected( QTreeView * view );

void selectItemByIdx( QTreeView * view, const QModelIndex & index );
void deselectSelectedItems( QTreeView * view );
void changeSelectionTo( QTreeView * view, const QModelIndex & index );

//----------------------------------------------------------------------------------------------------------------------
//  complete tree update helpers

/** gets a persistent item ID that survives node shifting, adding or removal */
TreePath getSelectedItemID( QTreeView * view, const DirTreeModel & model );

/** attempts to select a previously selected item defined by persistant itemID */
bool selectItemByID( QTreeView * view, const DirTreeModel & model, const TreePath & itemID );

void fillTreeFromDir( DirTreeModel & model, const QModelIndex & parent, const QString & dir, std::function< bool ( const QFileInfo & file ) > isDesiredFile );
void updateTreeFromDir( DirTreeModel & model, QTreeView * view, const QString & dir, std::function< bool ( const QFileInfo & file ) > isDesiredFile );


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
	if (!itemID.isEmpty()) {
		int newItemIdx = findSuch( model.list(), [ &itemID ]( const Item & item ) { return item.getID() == itemID; } );
		if (newItemIdx >= 0) {
			view->setCurrentIndex( newItemIdx );
			return true;
		}
	}
	return false;
}


#endif // WIDGET_UTILS_INCLUDED
