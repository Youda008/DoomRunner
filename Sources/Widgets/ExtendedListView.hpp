//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: list view that supports editing of item names and behaves correctly on both internal and external
//              drag&drop operations
//======================================================================================================================

#ifndef EXTENDED_LIST_VIEW_INCLUDED
#define EXTENDED_LIST_VIEW_INCLUDED


#include "Essential.hpp"

#include "DataModels/ModelCommon.hpp"  // AccessStyle
#include "Utils/EventFilters.hpp"      // ModifierHandler
#include "Utils/ErrorHandling.hpp"     // ErrorReportingComponent

#include <QListView>

#include <type_traits>  // underlying_type

class AListModel;
class DnDProgressGuard;

class QMenu;
class QAction;
class QKeySequence;
class QString;


//======================================================================================================================
// drag&drop properties

/// What kind of data will represent an item when it is dragged out of the widget.
namespace DnDOutputType { enum Values : uint
{
	None       = 0,
	FilePaths  = (1 << 0),   ///< the items will be exported as file-system paths
};}
using DnDOutputTypes = std::underlying_type_t< DnDOutputType::Values >;

/// Where the items have been dragged from.
namespace DnDSource { enum Values : uint
{
	None         = 0,
	ThisWidget   = (1 << 0),   ///< drag&drop within this widget for manual reordering of the items in the list
	OtherWidget  = (1 << 1),   ///< drag&drop from another widget for moving items between different widgets
	ExternalApp  = (1 << 2),   ///< drag&drop from a file explorer window for inserting file paths into the list
};}
using DnDSources = std::underlying_type_t< DnDSource::Values >;

struct DnDSourcesExp
{
	bool thisWidget;
	bool otherWidget;
	bool externalApp;
};
template<> inline DnDSourcesExp expandToBools< DnDSourcesExp, DnDSources >( DnDSources dndSources )
{
	return
	{
		areFlagsSet( dndSources, DnDSource::ThisWidget ),
		areFlagsSet( dndSources, DnDSource::OtherWidget ),
		areFlagsSet( dndSources, DnDSource::ExternalApp ),
	};
}


//======================================================================================================================
/** List view that supports editing of item names and behaves correctly on both internal and external drag&drop actions.
  * Should be used together with GenericListModel. */

class ExtendedListView : public QListView, protected ErrorReportingComponent {

	Q_OBJECT

	using thisClass = ExtendedListView;
	using QBaseView = QListView;  ///< The Qt's abstract view class we inherit from

 public:

	ExtendedListView( QWidget * parent );
	virtual ~ExtendedListView() override;

	virtual void setModel( QAbstractItemModel * model ) override;

	//-- icons ---------------------------------------------------------------------------------------------------------

	/// Enables/disables the item icons and updates the text of the context menu entry, default is disabled.
	void toggleIcons( bool enabled );

	bool areIconsEnabled() const;

	//-- editing item content ------------------------------------------------------------------------------------------

	/// Enables/disables checkboxes in front of items, default is disabled.
	void toggleCheckboxes( bool enabled );

	/// Enables/disables editing the item names by double-clicking on them, default is disabled.
	void toggleItemEditing( bool enabled );

	/// Returns whether any of the items is in edit mode (after double-click, F2, etc...).
	bool isItemBeingEdited() const;

	/// Opens edit mode for the current item.
	bool startEditingCurrentItem();

	/// Closes edit mode for the currently edited and commits the edit data into the model.
	void stopEditingAndCommit();

	//-- drag&drop -----------------------------------------------------------------------------------------------------

	/// Configures the view properties to export the items dragged out of this widget into selected types.
	/**
	  * \param outputTypes What kind of data will represent items when they are dragged out of the widget.
	  */
	void setDnDOutputTypes( DnDOutputTypes outputTypes );

	/// Configures the view properties to support the selected input drag&drop operations.
	/**
	  * \param dndSources From which drag&drop sources can items be dropped into this widget.
	  */
	void setAllowedDnDSources( DnDSources dndSources );

	void toggleAllowedDnDSources( DnDSources dndSourcesToSwitch, bool enabled );

	/// If this is true, it means some insert or remove operations are about to be performed.
	bool isDragAndDropInProgress() const  { return isBeingDraggedFrom || isBeingDroppedTo; }

	//-- context menu --------------------------------------------------------------------------------------------------

	/// Available actions for the right-click context menu. Each one is associated with a key shortcut.
	enum MenuAction : uint
	{
		None             = 0,

		AddAndDelete     = (1 << 0),   ///< add a new item and delete the selected items                            (available only in editable views)
		Clone            = (1 << 1),   ///< add a copy of the selected item with a new name                         (available only in editable views)
		Copy             = (1 << 2),   ///< copy selected items into the clipboard                                  (available also in read-only views)
		CutAndPaste      = (1 << 3),   ///< cut selected items into the clipboard or paste them from the clipboard  (available only in editable views)
		Move             = (1 << 4),   ///< move selected items up or down                                          (available only in editable views)
		InsertSeparator  = (1 << 5),   ///< insert named visual separator between items                             (available only in editable views)
		Find             = (1 << 6),   ///< open a search bar to find an existing item by name                      (available also in read-only views)
		OpenFileLocation = (1 << 7),   ///< open the directory of the last clicked file in a system file explorer   (available also in read-only views)
		ToggleIcons      = (1 << 8),   ///< show or hide the file or directory icons                                (available also in read-only views)

		All              = makeBitMask(9)
	};
	using MenuActions = std::underlying_type_t< MenuAction >;

	/// Enables the ability to open a context menu by clicking with the right mouse button.
	/**
	  * \param actions Specifies the entries the context menu will have. Each will create a corresponding QAction object.
	  */
	void enableContextMenu( MenuActions actions );

	/// Creates a custom action and adds it to the context menu.
	/** The resulting QAction object will emit triggered() signal that needs to be connected to the desired callback.
	  * The returned pointer is non-owning and must not be deleted, because it will be deleted by its parent view.
	  * The context menu must be enabled first by calling enableContextMenu(). */
	QAction * addAction( const QString & text, const QKeySequence & shortcut );

	/// Enables/disables those actions that modify the list (inserting, deleting, reordering)
	void toggleListModifications( bool enabled );

 signals:

	/// Emitted either when items are dropped to this view from another widget or just moved within this view itself.
	/** This is always preceeded by signal rowsInserted() (or itemsInserted() in case of GenericListModel)
	  * and only means that all the list and selection updates have finished and the view + model is in its final state.
	  *
	  * \param destRowIdx Index of the first row in the destination list where the items were dropped.
	  * \param itemCount Number of the items that were dropped.
	  * \param type Type of the drag&drop event (see enum DnDType).
	  */
	void dragAndDropFinished( int destRowIdx, int itemCount, DnDSources source );

 //actions:

	// The following actions will emit triggered() signal when the context menu entry is clicked or when
	// the corresponding key shortcut is pressed. The signal must be connected to a callback that does the job.
	//
	// The public actions must be connected and handled by the owner of this view and its corresponding model,
	// because in different use cases the operations are performed differently and the callbacks may be re-used
	// for multiple different signals.
	//
	// The private actions are connected and handled internally and work universally in all use cases.

	public:  QAction * addItemAction = nullptr;            ///< corresponds to MenuAction::AddAndDelete
	public:  QAction * deleteItemAction = nullptr;         ///< corresponds to MenuAction::AddAndDelete
	public:  QAction * cloneItemAction = nullptr;          ///< corresponds to MenuAction::Clone

	private: QAction * cutItemsAction = nullptr;           ///< corresponds to MenuAction::CutAndPaste
	private: QAction * copyItemsAction = nullptr;          ///< corresponds to MenuAction::Copy
	private: QAction * pasteItemsAction = nullptr;         ///< corresponds to MenuAction::CutAndPaste

	public:  QAction * moveItemUpAction = nullptr;         ///< corresponds to MenuAction::Move
	public:  QAction * moveItemDownAction = nullptr;       ///< corresponds to MenuAction::Move
	public:  QAction * moveItemToTopAction = nullptr;      ///< corresponds to MenuAction::Move
	public:  QAction * moveItemToBottomAction = nullptr;   ///< corresponds to MenuAction::Move

	public:  QAction * insertSeparatorAction = nullptr;    ///< corresponds to MenuAction::InsertSeparator
	public:  QAction * findItemAction = nullptr;           ///< corresponds to MenuAction::Find
	private: QAction * openFileLocationAction = nullptr;   ///< corresponds to MenuAction::OpenFileLocation
	public:  QAction * toggleIconsAction = nullptr;        ///< corresponds to MenuAction::ToggleIcons

 private: // overriden event callbacks

	virtual void contextMenuEvent( QContextMenuEvent * e ) override;

	virtual void dragEnterEvent( QDragEnterEvent * event ) override;
	virtual void dragMoveEvent( QDragMoveEvent * event ) override;
	virtual void dropEvent( QDropEvent * event ) override;
	virtual void startDrag( Qt::DropActions supportedActions ) override;

	virtual void keyPressEvent( QKeyEvent * event ) override;
	virtual void keyReleaseEvent( QKeyEvent * event ) override;

 protected slots:

	void cutSelectedItems();
	void copySelectedItems();
	void pasteAboveSelectedItem();

	void openCurrentFileLocation();
	void toggleIcons();

 private: // helpers

	bool isReadOnly() const   { return access == AccessStyle::ReadOnly; }

	/// Sets the properties of the Qt's list view so that it supports our drag&drop settings.
	void updateQtViewProperties();
	/// Sets up the data model to support the export and import formats we need for our drag&drop settings to work.
	void updateModelExportImportFormats();

	/// Which one of our drag&drop types this drop event represents.
	DnDSources getDnDSource( QDropEvent * event ) const;
	/// Which one of the Qt's drop actions do we want to perform based on our drag&drop settings.
	Qt::DropAction getPreferredDnDAction( DnDSources dndSource ) const;
	/// Does proposed drop operation comply with our drag&drop settings?
	bool isDropAcceptable( QDragMoveEvent * event ) const;

	/// Retrieves drop indexes, updates selection and emits a signal.
	void onDragAndDropFinished( DnDSources source, DnDProgressGuard & dndProgressGuard );

	bool assertCanAddEditAction( const char * actionDesc ) const;

	void toggleCheckStateOfSelectedItems();

 private: // internal members

	AListModel * ownModel = nullptr;  ///< quick access to our own specialized model
	QMenu * contextMenu = nullptr;
	ModifierHandler modifierHandler;
	bool isBeingDraggedFrom = false;
	bool isBeingDroppedTo = false;
	std::optional< DnDSources > droppedFrom;

 private: // configuration

	AccessStyle access = AccessStyle::ReadOnly;
	bool allowEditNames = false;
	MenuActions contextMenuActions = 0;
	bool allowModifyList = false;
	DnDOutputTypes enabledDnDOutputTypes = 0;
	DnDSources allowedDnDSources = 0;

};


#endif // EXTENDED_LIST_VIEW_INCLUDED
