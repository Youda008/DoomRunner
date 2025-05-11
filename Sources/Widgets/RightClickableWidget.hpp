//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Generic widget that pops up a context menu when right-clicked
//======================================================================================================================

#ifndef RIGHTCLICKABLE_WIDGET_INCLUDED
#define RIGHTCLICKABLE_WIDGET_INCLUDED


#include "Essential.hpp"

class QString;
class QWidget;
class QAction;
class QMenu;
class QContextMenuEvent;
class QKeySequence;
class QMouseEvent;


//======================================================================================================================
/// Generic widget that pops up a context menu when right-clicked

template< typename SuperClass >
class RightClickableWidget : public SuperClass {

	using ThisClass = RightClickableWidget;

 protected: // code deduplication helpers

	RightClickableWidget( QWidget * parent );

	QAction * _addMenuAction( const QString & text, const QKeySequence & shortcut );

	//void _mousePressEvent( QMouseEvent * event );
	void _contextMenuEvent( QContextMenuEvent * event );

 private:

	QMenu * contextMenu = nullptr;

};


//======================================================================================================================


#endif // RIGHTCLICKABLE_WIDGET_INCLUDED
