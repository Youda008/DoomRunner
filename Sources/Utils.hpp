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

// all these functions assume lists in single-selection mode

int getSelectedItemIdx( QListView * view );
void selectItemByIdx( QListView * view, int index );
void deselectItemByIdx( QListView * view, int index );
void deselectSelectedItem( QListView * view );
void changeSelectionTo( QListView * view, int index );

template< typename Object >
void appendItem( QListView * view, AObjectListModel< Object > & model, const Object & item )
{
	model.list().append( item );

	// change the selection to the new item
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx >= 0)   // deselect currently selected item, if any
		deselectItemByIdx( view, selectedIdx );
	selectItemByIdx( view, model.list().size() - 1 );

	model.updateUI( 0 );
}

template< typename Object >
void deleteSelectedItem( QListView * view, AObjectListModel< Object > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		if (!model.list().isEmpty())
			QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return;
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
}

template< typename Object >
void moveUpSelectedItem( QListView * view, AObjectListModel< Object > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return;
	}
	if (selectedIdx == 0) {  // if the selected item is the first one, do nothing
		return;
	}

	model.list().move( selectedIdx, selectedIdx - 1 );

	// update selection
	deselectItemByIdx( view, selectedIdx );
	selectItemByIdx( view, selectedIdx - 1 );

	model.updateUI( selectedIdx - 1 );
}

template< typename Object >
void moveDownSelectedItem( QListView * view, AObjectListModel< Object > & model )
{
	int selectedIdx = getSelectedItemIdx( view );
	if (selectedIdx < 0) {  // if no item is selected
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return;
	}
	if (selectedIdx == model.list().size() - 1) {  // if the selected item is the last one, do nothing
		return;
	}

	model.list().move( selectedIdx, selectedIdx + 1 );

	// update selection
	deselectItemByIdx( view, selectedIdx );
	selectItemByIdx( view, selectedIdx + 1 );

	model.updateUI( selectedIdx );
}


//======================================================================================================================
//  list widget helpers

// QListWidgets inherits from QListView, so for the rest we can use the functions above

void appendItem( QListWidget * widget, const QString & text, bool checkable, Qt::CheckState initialState );
void deleteSelectedItem( QListWidget * widget );
void moveUpSelectedItem( QListWidget * widget );
void moveDownSelectedItem( QListWidget * widget );


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
