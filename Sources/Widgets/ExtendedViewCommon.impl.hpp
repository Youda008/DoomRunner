//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: functionality common for all our views
//======================================================================================================================

#include "ExtendedViewCommon.hpp"

#include "Utils/ErrorHandling.hpp"
#include "Utils/WidgetUtils.hpp"  // getRowIndexToInsertTo

#include <QMenu>
#include <QAction>
#include <QWidget>
#include <QContextMenuEvent>
#include <QModelIndex>
#include <QItemSelection>
#include <QClipboard>
#include <QGuiApplication>


//======================================================================================================================

template< typename SubClass >
void ExtendedViewCommon< SubClass >::_enableContextMenu()
{
	contextMenu = new QMenu( thisAsSubClass() );  // will be deleted when this view (its parent) is deleted
}

template< typename SubClass >
bool ExtendedViewCommon< SubClass >::_assertCanAddEditAction( const char * actionDesc ) const
{
	bool isReadOnly = thisAsSubClass()->isReadOnly();
	if (isReadOnly)
	{
		logLogicError() << "attempted to add \""<<actionDesc<<"\" context menu actions to a read-only list view";
	}
	return !isReadOnly;
}

template< typename SubClass >
QAction * ExtendedViewCommon< SubClass >::_addCustomMenuAction( QMenu * menu, const QString & text, const QKeySequence & shortcut )
{
	QAction * action = new QAction( text, thisAsSubClass() );  // will be deleted when this view (its parent) is deleted
	action->setShortcut( shortcut );
	action->setShortcutContext( Qt::WidgetShortcut );  // only listen to this shortcut when this widget has focus
	static_cast< QWidget * >( static_cast< SubClass * >( this ) )->addAction( action );  // register it to this widget, so the shortcut is checked
	menu->addAction( action );  // register it to the menu, so that it appears there when right-clicked
	return action;
}

template< typename SubClass >
void ExtendedViewCommon< SubClass >::_addStandardMenuActions( MenuActions actions )
{
	auto * thisAsSubclass = thisAsSubClass();

	if (isFlagSet( actions, MenuAction::OpenFile ))
	{
		openFileAction = _addCustomMenuAction( contextMenu, "Open file", {} );
		thisAsSubclass->connect( openFileAction, &QAction::triggered, thisAsSubclass, &SubClass::openCurrentFile );
	}
	if (isFlagSet( actions, MenuAction::OpenFileLocation ))
	{
		openFileLocationAction = _addCustomMenuAction( contextMenu, "Open file location", {} );
		thisAsSubclass->connect( openFileLocationAction, &QAction::triggered, thisAsSubclass, &SubClass::openCurrentFileLocation );
	}

	if (isFlagSet( actions, MenuAction::AddAndDelete ) && _assertCanAddEditAction( "And and Delete" ))
	{
		addItemAction = _addCustomMenuAction( contextMenu, "Add", { Qt::Key_Insert } );
		deleteItemAction = _addCustomMenuAction( contextMenu, "Delete", { Qt::Key_Delete } );
	}
	if (isFlagSet( actions, MenuAction::Clone ) && _assertCanAddEditAction( "Clone" ))
	{
		cloneItemAction = _addCustomMenuAction( contextMenu, "Clone", { Qt::CTRL | Qt::ALT | Qt::Key_C } );
	}

	if (isFlagSet( actions, MenuAction::InsertSeparator ) && _assertCanAddEditAction( "Insert separator" ))
	{
		insertSeparatorAction = _addCustomMenuAction( contextMenu, "Insert separator", { Qt::CTRL | Qt::Key_Slash } );
	}

	if (isFlagSet( actions, MenuAction::CutAndPaste ) && _assertCanAddEditAction( "Cut" ))
	{
		cutItemsAction = _addCustomMenuAction( contextMenu, "Cut",   { Qt::CTRL | Qt::Key_X } );
		thisAsSubclass->connect( cutItemsAction, &QAction::triggered, thisAsSubclass, &SubClass::cutSelectedItems );
	}
	if (isFlagSet( actions, MenuAction::Copy ))
	{
		copyItemsAction = _addCustomMenuAction( contextMenu, "Copy",  { Qt::CTRL | Qt::Key_C } );
		thisAsSubclass->connect( copyItemsAction, &QAction::triggered, thisAsSubclass, &SubClass::copySelectedItems );
	}
	if (isFlagSet( actions, MenuAction::CutAndPaste ) && _assertCanAddEditAction( "Paste" ))
	{
		pasteItemsAction = _addCustomMenuAction( contextMenu, "Paste", { Qt::CTRL | Qt::Key_V } );
		thisAsSubclass->connect( pasteItemsAction, &QAction::triggered, thisAsSubclass, &SubClass::pasteAboveSelectedItem );
	}

	if (isFlagSet( actions, MenuAction::Move ) && _assertCanAddEditAction( "Move up and down" ))
	{
		moveItemUpAction = _addCustomMenuAction( contextMenu, "Move up", { Qt::CTRL | Qt::Key_Up } );
		moveItemDownAction = _addCustomMenuAction( contextMenu, "Move down", { Qt::CTRL | Qt::Key_Down } );
		moveItemToTopAction = _addCustomMenuAction( contextMenu, "Move to top", { Qt::CTRL | Qt::ALT | Qt::Key_Up } );
		moveItemToBottomAction = _addCustomMenuAction( contextMenu, "Move to bottom", { Qt::CTRL | Qt::ALT | Qt::Key_Down } );
	}

	if (isAnyOfFlagsSet( actions, MenuAction::SortByName | MenuAction::SortByType | MenuAction::SortBySize | MenuAction::SortByDate ))
	{
		sortItemsSubmenu = contextMenu->addMenu( "Sort by" );
		if (isFlagSet( actions, MenuAction::SortByName ))
		{
			sortByNameAscAction = _addCustomMenuAction( sortItemsSubmenu, "Name (ascending)", {} );
			sortByNameDscAction = _addCustomMenuAction( sortItemsSubmenu, "Name (descending)", {} );
			thisAsSubclass->connect( sortByNameAscAction, &QAction::triggered, thisAsSubclass, &SubClass::onSortByNameAscTriggered );
			thisAsSubclass->connect( sortByNameDscAction, &QAction::triggered, thisAsSubclass, &SubClass::onSortByNameDscTriggered );
		}
		if (isFlagSet( actions, MenuAction::SortByType ))
		{
			sortByTypeAscAction = _addCustomMenuAction( sortItemsSubmenu, "Type (ascending)", {} );
			sortByTypeDscAction = _addCustomMenuAction( sortItemsSubmenu, "Type (descending)", {} );
			thisAsSubclass->connect( sortByTypeAscAction, &QAction::triggered, thisAsSubclass, &SubClass::onSortByTypeAscTriggered );
			thisAsSubclass->connect( sortByTypeDscAction, &QAction::triggered, thisAsSubclass, &SubClass::onSortByTypeDscTriggered );
		}
		if (isFlagSet( actions, MenuAction::SortBySize ))
		{
			sortBySizeAscAction = _addCustomMenuAction( sortItemsSubmenu, "Size (ascending)", {} );
			sortBySizeDscAction = _addCustomMenuAction( sortItemsSubmenu, "Size (descending)", {} );
			thisAsSubclass->connect( sortBySizeAscAction, &QAction::triggered, thisAsSubclass, &SubClass::onSortBySizeAscTriggered );
			thisAsSubclass->connect( sortBySizeDscAction, &QAction::triggered, thisAsSubclass, &SubClass::onSortBySizeDscTriggered );
		}
		if (isFlagSet( actions, MenuAction::SortByDate ))
		{
			sortByDateAscAction = _addCustomMenuAction( sortItemsSubmenu, "Date modified (ascending)", {} );
			sortByDateDscAction = _addCustomMenuAction( sortItemsSubmenu, "Date modified (descending)", {} );
			thisAsSubclass->connect( sortByDateAscAction, &QAction::triggered, thisAsSubclass, &SubClass::onSortByDateAscTriggered );
			thisAsSubclass->connect( sortByDateDscAction, &QAction::triggered, thisAsSubclass, &SubClass::onSortByDateDscTriggered );
		}
	}

	if (isFlagSet( actions, MenuAction::Find ))
	{
		findItemAction = _addCustomMenuAction( contextMenu, "Find", QKeySequence::Find );
	}

	if (isFlagSet( actions, MenuAction::ToggleIcons ))
	{
		toggleIconsAction = _addCustomMenuAction( contextMenu, "Show icons", {} );
		thisAsSubclass->connect( toggleIconsAction, &QAction::triggered, thisAsSubclass, QOverload<>::of( &SubClass::toggleIcons ) );
	}
}

template< typename SubClass >
void ExtendedViewCommon< SubClass >::_addMenuSeparator()
{
	contextMenu->addSeparator();
}

template< typename SubClass >
void ExtendedViewCommon< SubClass >::_contextMenuEvent( QContextMenuEvent * event )
{
	QModelIndex clickedItemIndex = thisAsSubClass()->indexAt( event->pos() );

	if (!contextMenu)  // not enabled
		return;

	if (openFileAction)
		openFileAction->setEnabled( clickedItemIndex.isValid() );  // read-only
	if (openFileLocationAction)
		openFileLocationAction->setEnabled( clickedItemIndex.isValid() );  // read-only

	if (addItemAction)
		addItemAction->setEnabled( allowModifyList );
	if (deleteItemAction)
		deleteItemAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (cloneItemAction)
		cloneItemAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );

	if (insertSeparatorAction)
		insertSeparatorAction->setEnabled( allowModifyList );

	if (cutItemsAction)
		cutItemsAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (copyItemsAction)
		copyItemsAction->setEnabled( clickedItemIndex.isValid() );  // read-only
	if (pasteItemsAction)
		pasteItemsAction->setEnabled( allowModifyList );

	if (moveItemUpAction)
		moveItemUpAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (moveItemDownAction)
		moveItemDownAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (moveItemToTopAction)
		moveItemToTopAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (moveItemToBottomAction)
		moveItemToBottomAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );

	if (findItemAction)
		findItemAction->setEnabled( true );  // read-only

	if (toggleIconsAction)
		toggleIconsAction->setEnabled( true );  // read-only

	contextMenu->popup( event->globalPos() );
}

template< typename SubClass >
void ExtendedViewCommon< SubClass >::_toggleListModifications( bool enabled )
{
	if (enabled && thisAsSubClass()->isReadOnly())
	{
		logLogicError() << "attempted to enable list modifications in a read-only list view.";
		return;
	}

	allowModifyList = enabled;
}


//----------------------------------------------------------------------------------------------------------------------
// copy&paste

template< typename SubClass >
void ExtendedViewCommon< SubClass >::_cutSelectedItems()
{
	_copySelectedItems();

	// remove the selected items
	const QItemSelection selection = thisAsSubClass()->selectionModel()->selection();
	for (const QItemSelectionRange & range : selection)
		thisAsSubClass()->model()->removeRows( range.top(), range.height() );
}

template< typename SubClass >
void ExtendedViewCommon< SubClass >::_copySelectedItems()
{
	QModelIndexList indexes = thisAsSubClass()->selectionModel()->selectedIndexes();
	if (indexes.isEmpty())
	{
		return;
	}
	//std::sort( indexes.begin(), indexes.end(), []( QModelIndex & i1, QModelIndex & i2 ) { return i1.row() < i2.row(); } );

	// serialize the selected items into MIME data
	QMimeData * mimeData = thisAsSubClass()->model()->mimeData( indexes );

	// save the serialized data to the system clipboard
	qApp->clipboard()->setMimeData( mimeData );  // ownership is transferred to the clipboard
}

template< typename SubClass >
void ExtendedViewCommon< SubClass >::_pasteAboveSelectedItem()
{
	// get the serialized data from the system clipboard
	const QMimeData * mimeData = qApp->clipboard()->mimeData();  // ownership remains in the clipboard
	if (!mimeData)
	{
		reportUserError( "Clipboard empty", "There is nothing to paste. Copy something first." );
		return;
	}

	// deserialize and insert the data above the last selected item
	auto rowToDrop = wdg::getRowIndexToInsertTo( thisAsSubClass() );
	// Although some people might call the cut&paste combo a "move action", for our model it's a "copy action".
	thisAsSubClass()->model()->dropMimeData( mimeData, Qt::CopyAction, rowToDrop, 0, QModelIndex() );
}
