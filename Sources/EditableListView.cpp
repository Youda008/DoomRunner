//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  2.7.2019
// Description: list view that supports editing of item names and behaves correctly on both internal and external
//              drag&drop operations
//======================================================================================================================

#include "EditableListView.hpp"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>


//======================================================================================================================
/* When attempting to make a drag&drop from a new source work properly, there are 3 things to remember:
 *  1. View must support the drop action type the source emits. Some use MoveAction, some CopyAction, ...
 *  2. Model::mimeTypes() must return the MIME type, that is used by the source widget.
 *  3. Model::canDropMimeData(...) must be correctly implemented to support both the MIME type and the drop action
 */

//======================================================================================================================

EditableListView::EditableListView( QWidget * parent ) : QListView ( parent )
{
	allowIntraWidgetDnD = true;
	allowInterWidgetDnD = false;
	allowExternFileDnD = false;
	updateDragDropMode();
	setDefaultDropAction( Qt::MoveAction );
	setDropIndicatorShown( true );

	allowEditNames = false;
	setEditTriggers( QAbstractItemView::NoEditTriggers );
}

void EditableListView::updateDragDropMode()
{
	const bool externalDrops = allowInterWidgetDnD || allowExternFileDnD;

	if (!allowIntraWidgetDnD && !externalDrops)
		setDragDropMode( NoDragDrop );
	else if (allowIntraWidgetDnD && !externalDrops)
		setDragDropMode( InternalMove );
	else
		setDragDropMode( DragDrop );
}

void EditableListView::toggleIntraWidgetDragAndDrop( bool enabled )
{
	allowIntraWidgetDnD = enabled;

	updateDragDropMode();
}

void EditableListView::toggleInterWidgetDragAndDrop( bool enabled )
{
	allowInterWidgetDnD = enabled;

	updateDragDropMode();
}

void EditableListView::toggleExternalFileDragAndDrop( bool enabled )
{
	allowExternFileDnD = enabled;

	updateDragDropMode();
}

bool EditableListView::isDropAcceptable( QDragMoveEvent * event )
{
	if (isIntraWidgetDnD( event ) && allowIntraWidgetDnD && event->possibleActions() & Qt::MoveAction)
		return true;
	else if (isInterWidgetDnD( event ) && allowInterWidgetDnD && event->possibleActions() & Qt::MoveAction)
		return true;
	else if (isExternFileDnD( event ) && allowExternFileDnD)
		return true;
	else
		return false;
}

bool EditableListView::isIntraWidgetDnD( QDropEvent * event )
{
	return event->source() == this;
}

bool EditableListView::isInterWidgetDnD( QDropEvent * event )
{
	return event->source() != this && !event->mimeData()->hasUrls();
}

bool EditableListView::isExternFileDnD( QDropEvent * event )
{
	return event->source() != this && event->mimeData()->hasUrls();
}

void EditableListView::dragEnterEvent( QDragEnterEvent * event )
{
	// QListView::dragEnterEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. accept if event contains at leats one mime type present in model->mimeTypes or model->canDropMimeData
	// We override it, so that we apply our own rules and restrictions for the drag&drop operation.

	if (isDropAcceptable( event )) {  // does proposed drop operation comply with our settings?
		superClass::dragEnterEvent( event );  // let it calc the index and query the model if the drop is ok there
	} else {
		event->ignore();
	}
}

void EditableListView::dragMoveEvent( QDragMoveEvent * event )
{
	// QListView::dragMoveEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. accept if event contains at leats one mime type present in model->mimeTypes or model->canDropMimeData
	// 3. draw drop indicator according to position
	// We override it, so that we apply our own rules and restrictions for the drag&drop operation.

	if (isDropAcceptable( event )) {  // does proposed drop operation comply with our settings?
		superClass::dragMoveEvent( event );  // let it query the model if the drop is ok there and draw the indicator
	} else {
		event->ignore();
	}
}

void EditableListView::dropEvent( QDropEvent * event )
{
	// QListView::dropEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. get drop index from cursor position
	// 3. if model->dropMimeData then accept drop event

	superClass::dropEvent( event );

	// announce dropped files now only if it's an external drag&drop, otherwise postpone it because of the issue decribed below
	if (isExternFileDnD( event ))
		emit itemsDropped();
}

void EditableListView::startDrag( Qt::DropActions supportedActions )
{
	// idiotic workaround because Qt is fucking retarded
	//
	// When an internal reordering drag&drop is performed, Qt doesn't update the selection and leaves the selection
	// on the old indexes, where are now some completely different items.
	// You can't manually update the indexes in dropEvent, because at some point after it Qt calls removeRows on items
	// that are CURRENTLY SELECTED, instead of on items that were selected at the beginning of this drag&drop operation.
	// So we must update the selection at some point AFTER the drag&drop operation is finished and the rows removed.
	//
	// QAbstractItemView::startDrag, despite its confusing name, is the common parent function for
	// Model::dropMimeData and Model::removeRows both of which happen when items are dropped.
	// So this is the right place to update the selection.
	//
	// But outside an item model, there is no information abouth the target drop index. So the model must write down
	// the index and then let other classes retrieve it at the right time.
	//
	// And like it wasn't enough, we can't retrieve the drop index here, because we cannot cast our abstract model
	// into the correct model, because it's a template class whose template parameter is not known here.
	// So the only way is to emit a signal to the owner of this ListView, which then catches it, queries the model
	// for a drop index and then performs the update.
	//
	// And even that isn't enough, because this is called only when the source of the drag is this application.
	// When you drag files from a directory window, then dropEvent is called from somewhere else, so in that case
	// we send the signal from dropEvent, there the deletion of the selected items doesn't happen.

	superClass::startDrag( supportedActions );
	// at this point the drag&drop should be finished and source rows removed, so we can safely update the selection
	emit itemsDropped();
}

void EditableListView::toggleNameEditing( bool enabled )
{
	allowEditNames = enabled;

	if (enabled)
		setEditTriggers( QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed );
	else
		setEditTriggers( QAbstractItemView::NoEditTriggers );
}
