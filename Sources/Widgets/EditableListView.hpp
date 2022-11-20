//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: list view that supports editing of item names and behaves correctly on both internal and external
//              drag&drop operations
//======================================================================================================================

#ifndef EDITABLE_LIST_VIEW_INCLUDED
#define EDITABLE_LIST_VIEW_INCLUDED


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

	// drag&drop
	// We support 3 kinds of drag&drop operations, that can be separately enabled/disabled:
	// 1. intra-widget d&d - drag&drop from inside this widget for manual reordering of items on the list
	// 2. inter-widget d&d - drag&drop from another widget for moving items between different widgets
	// 3. external file d&d - drag&drop from directory window for inserting file paths into the list

	/// internal drag&drop for reordering items inside this widget, enabled by default
	void toggleIntraWidgetDragAndDrop( bool enabled );

	/// internal drag&drop for moving items from other widgets, disabled by default
	void toggleInterWidgetDragAndDrop( bool enabled );

	/// external drag&drop for moving files from directory window, disabled by default
	void toggleExternalFileDragAndDrop( bool enabled );

	// editing

	/// Enables/disables editing the item names by double-clicking on them, default is disabled.
	void toggleNameEditing( bool enabled );

	/// Returns whether any of the items is in edit mode (after double-click, F2, etc...).
	bool isEditModeOpen() const;

	/// Opens edit mode for the current item.
	bool startEditingCurrentItem();

	/// Closes edit mode for the currently edited and commits the edit data into the model.
	void stopEditingAndCommit();

	// right-click menu

	/// Setting this to false will grey-out the context menu items so that they can't be clicked, false is default.
	void toggleContextMenu( bool enabled );

	/// Enables clone action in a right-click context menu and CTRL+C shortcut.
	void enableItemCloning();

	/// Enables "Open file location" action in a right-click context menu.
	void enableOpenFileLocation();

	/// Enables adding a named separator line between items of this list view.
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

	/// Updates QAbstractItemView's properties based on our new settings
	void updateDragDropMode();

	/// Does proposed drop operation comply with our settings?
	bool isDropAcceptable( QDragMoveEvent * event );

	bool isIntraWidgetDnD( QDropEvent * event );
	bool isInterWidgetDnD( QDropEvent * event );
	bool isExternFileDnD( QDropEvent * event );

	/// Retrieves drop indexes, updates selection and emits a signal.
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

	/// emitted either when items are dropped to this view from another widget or just moved within this view itself
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


#endif // EDITABLE_LIST_VIEW_INCLUDED
