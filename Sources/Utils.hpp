//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: miscellaneous utilities
//======================================================================================================================

#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED


#include "Common.hpp"

#include "ItemModels.hpp"

#include <QListView>
#include <QListWidget>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>


//======================================================================================================================
//  container helpers

/** checks whether the list contains such an element that satisfies condition */
template< typename Type >
bool containsSuch( const QList< Type > & list, std::function< bool ( const Type & elem ) > condition )
{
	for (const Type & elem : list)
		if (condition( elem ))
			return true;
	return false;
}

/** finds such element in the list that satisfies condition */
template< typename Type >
int findSuch( const QList< Type > & list, std::function< bool ( const Type & elem ) > condition )
{
	int i = 0;
	for (const Type & elem : list) {
		if (condition( elem ))
			return i;
		i++;
	}
	return -1;
}


//======================================================================================================================
//  list view helpers

// all of these function assume a 1-dimensional non-recursive list view/widget

/// assumes a single-selection mode, will throw a message box error otherwise
int getSelectedItemIdx( QListView * view );

void selectItemByIdx( QListView * view, int index );
void deselectItemByIdx( QListView * view, int index );
void deselectSelectedItems( QListView * view );
void changeSelectionTo( QListView * view, int index );

template< typename Object >
void appendItem( QListView * view, AObjectListModel< Object > & model, const Object & item )
{
	model.list().append( item );

	changeSelectionTo( view, model.list().size() - 1 );

	model.updateUI( model.list().size() - 1 );
}

template< typename Object >
int deleteSelectedItem( QListView * view, AObjectListModel< Object > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		if (!model.list().isEmpty())
			QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	// update selection
	if (selectedIdx == model.list().size() - 1) {      // if item is the last one
		deselectItemByIdx( view, selectedIdx );        // deselect it
		if (selectedIdx > 0) {                         // and if it's not the only one
			selectItemByIdx( view, selectedIdx - 1 );  // select the previous
		}
	}

	model.list().removeAt( selectedIdx );

	model.updateUI( selectedIdx );

	return selectedIdx;
}

template< typename Object >
int cloneSelectedItem( QListView * view, AObjectListModel< Object > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}

	model.list().append( model.list()[ selectedIdx ] );

	// append some postfix to the item name to distinguish it from the original
	QModelIndex newItemIdx = model.index( model.list().size() - 1, 0 );
	QString origName = model.data( newItemIdx, Qt::DisplayRole ).toString();
	model.setData( newItemIdx, origName+" - clone", Qt::DisplayRole );

	changeSelectionTo( view, model.list().size() - 1 );

	model.updateUI( newItemIdx.row() );

	return selectedIdx;
}

template< typename Object >
int moveUpSelectedItem( QListView * view, AObjectListModel< Object > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}
	if (selectedIdx == 0) {  // if the selected item is the first one, do nothing
		return selectedIdx;
	}

	model.list().move( selectedIdx, selectedIdx - 1 );

	// update selection
	deselectItemByIdx( view, selectedIdx );
	selectItemByIdx( view, selectedIdx - 1 );

	model.updateUI( selectedIdx - 1 );

	return selectedIdx;
}

template< typename Object >
int moveDownSelectedItem( QListView * view, AObjectListModel< Object > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return -1;
	}
	if (selectedIdx == model.list().size() - 1) {  // if the selected item is the last one, do nothing
		return selectedIdx;
	}

	model.list().move( selectedIdx, selectedIdx + 1 );

	// update selection
	deselectItemByIdx( view, selectedIdx );
	selectItemByIdx( view, selectedIdx + 1 );

	model.updateUI( selectedIdx );

	return selectedIdx;
}


//======================================================================================================================
/** helper for calculating relative and absolute paths according to current directory and settings */

class PathHelper {

	QDir _currentDir;  ///< directory to derive relative paths from
	bool _useAbsolutePaths;  ///< whether to store paths to engines, IWADs, maps and mods in absolute or relative form

 public:

	PathHelper( bool useAbsolutePaths ) : _currentDir( QDir::current() ), _useAbsolutePaths( useAbsolutePaths ) {}

	const QDir & currentDir() const                    { return _currentDir; }
	bool useAbsolutePaths() const                      { return _useAbsolutePaths; }
	bool useRelativePaths() const                      { return !_useAbsolutePaths; }
	void toggleAbsolutePaths( bool useAbsolutePaths )  { _useAbsolutePaths = useAbsolutePaths; }

	QString getAbsolutePath( QString path )
	{
		return QFileInfo( path ).absoluteFilePath();
	}
	QString getRelativePath( QString path )
	{
		return _currentDir.relativeFilePath( path );
	}
	QString convertPath( QString path )
	{
		return _useAbsolutePaths ? getAbsolutePath( path ) : getRelativePath( path );
	}

	void makeAbsolute( QDir & dir )
	{
		dir.makeAbsolute();
	}
	void makeRelative( QDir & dir )
	{
		dir = _currentDir.relativeFilePath( dir.path() );
	}
	void convertDir( QDir & dir )
	{
		_useAbsolutePaths ? makeAbsolute( dir ) : makeRelative( dir );
	}

};


//======================================================================================================================
//  misc

QString getMapNumber( QString mapName );


#endif // UTILS_INCLUDED
