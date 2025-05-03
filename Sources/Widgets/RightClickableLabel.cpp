//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QLabel that emits a signal when right-clicked
//======================================================================================================================

#include "RightClickableLabel.hpp"

#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>


//======================================================================================================================

RightClickableLabel::RightClickableLabel( QWidget * parent )
:
	QLabel( parent )
{
	contextMenu = new QMenu( this );  // will be deleted when this view (its parent) is deleted
}

RightClickableLabel::~RightClickableLabel() {}

/*
void RightClickableLabel::mousePressEvent( QMouseEvent * e )
{
	superClass::mousePressEvent( e );

	if (e->button() == Qt::RightButton)
		emit rightClicked();
}
*/

QAction * RightClickableLabel::addAction( const QString & text, const QKeySequence & shortcut )
{
	QAction * action = new QAction( text, this );  // will be deleted when this view (its parent) is deleted
	action->setShortcut( shortcut );
	action->setShortcutContext( Qt::WindowShortcut );  // only listen to this shortcut when the current window has focus
	this->parentWidget()->addAction( action );  // register it to this widget, so the shortcut is checked
	contextMenu->addAction( action );  // register it to the menu, so that it appears there when right-clicked
	return action;
}

void RightClickableLabel::contextMenuEvent( QContextMenuEvent * event )
{
	contextMenu->popup( event->globalPos() );
}
