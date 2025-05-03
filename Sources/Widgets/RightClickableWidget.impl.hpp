//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Generic widget that pops up a context menu when right-clicked
//======================================================================================================================

#include "RightClickableWidget.hpp"

#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>


//======================================================================================================================

template< typename SuperClass >
RightClickableWidget< SuperClass >::RightClickableWidget( QWidget * parent )
:
	SuperClass( parent )
{
	contextMenu = new QMenu( this );  // will be deleted when this view (its parent) is deleted
}

/*
template< typename SuperClass >
void RightClickableWidget< SuperClass >::_mousePressEvent( QMouseEvent * e )
{
	SuperClass::mousePressEvent( e );

	if (e->button() == Qt::RightButton)
		emit rightClicked();
}
*/

template< typename SuperClass >
QAction * RightClickableWidget< SuperClass >::_addAction( const QString & text, const QKeySequence & shortcut )
{
	QAction * action = new QAction( text, this );  // will be deleted when this view (its parent) is deleted
	action->setShortcut( shortcut );
	action->setShortcutContext( Qt::WindowShortcut );  // only listen to this shortcut when the current window has focus
	this->parentWidget()->addAction( action );  // register it to this widget, so the shortcut is checked
	contextMenu->addAction( action );  // register it to the menu, so that it appears there when right-clicked
	return action;
}

template< typename SuperClass >
void RightClickableWidget< SuperClass >::_contextMenuEvent( QContextMenuEvent * event )
{
	contextMenu->popup( event->globalPos() );
}
