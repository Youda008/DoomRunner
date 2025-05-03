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
class QKeySequence;
class QMenu;
class QContextMenuEvent;


//======================================================================================================================
/// Generic widget that pops up a context menu when right-clicked

template< typename SuperClass >
class RightClickableWidget : public SuperClass {

	using ThisClass = RightClickableWidget;

 protected: // code deduplication helpers

	RightClickableWidget( QWidget * parent );

	QAction * _addAction( const QString & text, const QKeySequence & shortcut );

	//void _mousePressEvent( QMouseEvent * e );
	void _contextMenuEvent( QContextMenuEvent * e );

 private:

	QMenu * contextMenu = nullptr;

};


#endif // RIGHTCLICKABLE_WIDGET_INCLUDED
