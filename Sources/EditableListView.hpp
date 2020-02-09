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

#include <QListView>


//======================================================================================================================
/** List view that supports editing of item names and behaves correctly on both internal and external drag&drop actions.
  * Should be used together with EditableListModel. */

class EditableListView : public QListView {

	Q_OBJECT

	using superClass = QListView;

 public:

	EditableListView( QWidget * parent );

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

 protected: // methods

	virtual void dragEnterEvent( QDragEnterEvent * event ) override;
	virtual void dragMoveEvent( QDragMoveEvent * event ) override;
	virtual void dropEvent( QDropEvent * event ) override;
	virtual void startDrag(Qt::DropActions supportedActions) override;

	/** update QAbstractItemView's properties based on our new settings */
	void updateDragDropMode();

	/** does proposed drop operation comply with our settings? */
	bool isDropAcceptable( QDragMoveEvent * event );

	bool isIntraWidgetDnD( QDropEvent * event );
	bool isInterWidgetDnD( QDropEvent * event );
	bool isExternFileDnD( QDropEvent * event );

 signals:

	void itemsDropped();

 protected: // members

	bool allowIntraWidgetDnD;
	bool allowInterWidgetDnD;
	bool allowExternFileDnD;
	bool allowEditNames;

};


#endif // DRAG_AND_DROP_LIST_VIEW_INCLUDED
