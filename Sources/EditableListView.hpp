//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  2.7.2019
// Description: list view that supports editing of item names and behaves correctly on both internal and external
//              drag&drop operations
//======================================================================================================================

#ifndef DRAG_AND_DROP_LIST_VIEW_INCLUDED
#define DRAG_AND_DROP_LIST_VIEW_INCLUDED


#include "Common.hpp"

#include "EventFilters.hpp"  // ModifierHandler

#include <QListView>
class QMenu;


//======================================================================================================================
/** List view that supports editing of item names and behaves correctly on both internal and external drag&drop actions.
  * Should be used together with EditableListModel. */

class EditableListView : public QListView {

	Q_OBJECT

	using thisClass = EditableListView;
	using superClass = QListView;

 public:

	EditableListView( QWidget * parent );
	virtual ~EditableListView() override;

	// We support 3 kinds of drag&drop operations, that can be separately enabled/disabled
	// 1. intra-widget d&d - drag&drop from inside this widget for manual reordering of items on the list
	// 2. inter-widget d&d - drag&drop from another widget for moving items between different widgets
	// 3. external file d&d - drag&drop from directory window for inserting file paths into the list

	/** internal drag&drop for reordering items inside this widget, enabled by default */
	void toggleIntraWidgetDragAndDrop( bool enabled );

	/** internal drag&drop for moving items from other widgets, disabled by default */
	void toggleInterWidgetDragAndDrop( bool enabled );

	/** external drag&drop for moving files from directory window, disabled by default */
	void toggleExternalFileDragAndDrop( bool enabled );

	/** enables/disables editing the item names by double-clicking on them */
	void toggleNameEditing( bool enabled );

	/** setting this to false will grey-out the context menu items so that they can't be clicked */
	void toggleContextMenu( bool enabled );

	/** enables clone action in a right-click context menu and CTRL+C shortcut */
	void enableItemCloning();

	/** enables "Open file location" action in a right-click context menu */
	void enableOpenFileLocation();

	/** enables adding a named separator line between items of this list view */
	void enableInsertSeparator();

 public: // members

	// these actions will emit trigger signals when a menu item is clicked or a shortcut is pressed
	QAction * addAction = nullptr;
	QAction * deleteAction = nullptr;
	QAction * cloneAction = nullptr;
	QAction * moveUpAction = nullptr;
	QAction * moveDownAction = nullptr;
	QAction * openFileLocationAction = nullptr;
	QAction * insertSeparatorAction = nullptr;

 protected: // methods

	// drag&drop

	virtual void dragEnterEvent( QDragEnterEvent * event ) override;
	virtual void dragMoveEvent( QDragMoveEvent * event ) override;
	virtual void dropEvent( QDropEvent * event ) override;
	virtual void startDrag( Qt::DropActions supportedActions ) override;

	/** update QAbstractItemView's properties based on our new settings */
	void updateDragDropMode();

	/** does proposed drop operation comply with our settings? */
	bool isDropAcceptable( QDragMoveEvent * event );

	bool isIntraWidgetDnD( QDropEvent * event );
	bool isInterWidgetDnD( QDropEvent * event );
	bool isExternFileDnD( QDropEvent * event );

	void itemsDropped();

	// keyboard control

	virtual void keyPressEvent( QKeyEvent * event ) override;
	virtual void keyReleaseEvent( QKeyEvent * event ) override;

	// right-click menu

	virtual void contextMenuEvent( QContextMenuEvent * e ) override;

	// misc

	QAction * addOwnAction( const QString & text, const QKeySequence & shortcut );

 protected slots:

	void openFileLocation();

 signals:

	void itemsDropped( int row, int count );

 protected: // members

	bool allowIntraWidgetDnD;
	bool allowInterWidgetDnD;
	bool allowExternFileDnD;
	bool allowEditNames;

	ModifierHandler modifierHandler;

	bool contexMenuActive;

	QMenu * contextMenu;

};


#endif // DRAG_AND_DROP_LIST_VIEW_INCLUDED
