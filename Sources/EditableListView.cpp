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
#include <QFileInfo>
#include <QMimeData>
#include <QUrl>

#include <QDebug>


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

bool EditableListView::isIntraWidgetDnD( QDragMoveEvent * event )
{
	return event->source() == this;
}

bool EditableListView::isInterWidgetDnD( QDragMoveEvent * event )
{
	return event->source() != this && !event->mimeData()->hasUrls();
}

bool EditableListView::isExternFileDnD( QDragMoveEvent * event )
{
	return event->source() != this && event->mimeData()->hasUrls();
}

void EditableListView::dragEnterEvent( QDragEnterEvent * event )
{
	// QListView::dragEnterEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. accept if event contains at leats one mime type present in model->mimeTypes or model->canDropMimeData

	qDebug() << "EditableListView::dragEnterEvent";
	qDebug() << "  action: " << event->dropAction();
	//qDebug() << "  formats: ";
	//for (QString format : event->mimeData()->formats())
	//	qDebug() << "    " << format;

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

	//qDebug() << "EditableListView::dragMoveEvent";

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
}

void EditableListView::toggleNameEditing( bool enabled )
{
	allowEditNames = enabled;

	if (enabled)
		setEditTriggers( QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed );
	else
		setEditTriggers( QAbstractItemView::NoEditTriggers );
}
