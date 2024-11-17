//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: list view that supports editing of item names and behaves correctly on both internal and external
//              drag&drop operations
//======================================================================================================================

#ifndef EDITABLE_LIST_VIEW_INCLUDED
#define EDITABLE_LIST_VIEW_INCLUDED


#include "Essential.hpp"

#include "Utils/EventFilters.hpp"  // ModifierHandler
#include "Utils/ErrorHandling.hpp"  // LoggingComponent

#include <QListView>
#include <optional>

class QMenu;
class QAction;
class QKeySequence;
class QString;

//======================================================================================================================

// We support 3 kinds of drag&drop operations, that can be separately enabled/disabled:
enum class DnDType
{
	IntraWidget,   // drag&drop from inside this widget for manual reordering of items on the list
	InterWidget,   // drag&drop from another widget for moving items between different widgets
	ExternalFile,  // drag&drop from a file explorer window for inserting file paths into the list
};


//======================================================================================================================
/** List view that supports editing of item names and behaves correctly on both internal and external drag&drop actions.
  * Should be used together with EditableListModel. */

class EditableListView : public QListView, protected LoggingComponent {

	Q_OBJECT

	using thisClass = EditableListView;
	using superClass = QListView;

 public:

	EditableListView( QWidget * parent );
	virtual ~EditableListView() override;

	// editing

	/// Enables/disables editing the item names by double-clicking on them, default is disabled.
	void toggleNameEditing( bool enabled );

	/// Returns whether any of the items is in edit mode (after double-click, F2, etc...).
	bool isEditModeOpen() const;

	/// Opens edit mode for the current item.
	bool startEditingCurrentItem();

	/// Closes edit mode for the currently edited and commits the edit data into the model.
	void stopEditingAndCommit();

	/// Enables/disables actions (context menu entries, key presses) that modify the list (inserting, deleting, reordering)
	void toggleListModifications( bool enabled );

	// context menu

	/// Enables/disables the ability to open a context menu by clicking with right mouse button.
	void toggleContextMenu( bool enabled );

	/// Enables clone action in a right-click context menu and CTRL+C shortcut.
	void enableItemCloning();

	/// Enables adding a named separator line between items of this list view.
	void enableInsertSeparator();

	/// Enables opening a search bar via a context menu and a key shortcut.
	void enableFinding();

	/// Enables "Open file location" action in a right-click context menu.
	void enableOpenFileLocation();

	/// Allows the user to show or hide item icons via context menu.
	void enableTogglingIcons();

	/// Creates a custom action and adds it to the context menu.
	/** The resulting QAction object will emit triggered() signal that needs to be connected to the desired callback. */
	QAction * addAction( const QString & text, const QKeySequence & shortcut );

	// drag&drop

	/// internal drag&drop for reordering items inside this widget, enabled by default
	void toggleIntraWidgetDragAndDrop( bool enabled );

	/// internal drag&drop for moving items from other widgets, disabled by default
	void toggleInterWidgetDragAndDrop( bool enabled );

	/// external drag&drop for moving files from a file explorer window, disabled by default
	void toggleExternalFileDragAndDrop( bool enabled );

 public slots:

	/// Attempts to open a directory of the last clicked item in a new File Explorer window.
	void openCurrentFileLocation();

	/// Enables/disables the item icons and updates the text of the context menu entry, default is disabled.
	void toggleIcons( bool enabled );

	bool areIconsEnabled() const;

 signals:

	/// emitted either when items are dropped to this view from another widget or just moved within this view itself
	void itemsDropped( int row, int count, DnDType type );

 public: // actions (context menu entries with shortcuts)

	// These actions will emit triggered() signal that needs to be connected to the desired callback.
	QAction * addItemAction = nullptr;
	QAction * deleteItemAction = nullptr;
	QAction * cloneItemAction = nullptr;
	QAction * moveItemUpAction = nullptr;
	QAction * moveItemDownAction = nullptr;
	QAction * insertSeparatorAction = nullptr;
	QAction * findItemAction = nullptr;
	QAction * openFileLocationAction = nullptr;
	QAction * toggleIconsAction = nullptr;

 protected: // methods

	// right-click menu

	virtual void contextMenuEvent( QContextMenuEvent * e ) override;

	// drag&drop

	virtual void dragEnterEvent( QDragEnterEvent * event ) override;
	virtual void dragMoveEvent( QDragMoveEvent * event ) override;
	virtual void dropEvent( QDropEvent * event ) override;
	virtual void startDrag( Qt::DropActions supportedActions ) override;

	/// Updates QAbstractItemView's properties based on our new settings
	void updateDragDropMode();

	/// Does proposed drop operation comply with our settings?
	bool isDropAcceptable( QDragMoveEvent * event );

	DnDType getDnDType( QDropEvent * event );

	/// Retrieves drop indexes, updates selection and emits a signal.
	void onItemsDropped( DnDType type );

	// keyboard control

	virtual void keyPressEvent( QKeyEvent * event ) override;
	virtual void keyReleaseEvent( QKeyEvent * event ) override;

 protected slots:

	void toggleIcons();

 protected: // internal members

	QMenu * contextMenu;
	ModifierHandler modifierHandler;
	std::optional< DnDType > postponedDnDType;

 protected: // configuration

	bool allowEditNames;
	bool allowModifyList;

	bool contextMenuEnabled;

	bool allowIntraWidgetDnD;
	bool allowInterWidgetDnD;
	bool allowExternFileDnD;

};


#endif // EDITABLE_LIST_VIEW_INCLUDED
