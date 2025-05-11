//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: functionality common for all our views
//======================================================================================================================

#ifndef EXTENDED_VIEW_COMMON_INCLUDED
#define EXTENDED_VIEW_COMMON_INCLUDED


#include "Essential.hpp"

#include "Utils/LangUtils.hpp"      // makeBitMask
#include "Utils/ErrorHandling.hpp"  // ErrorReportingComponent

#include "qnamespace.h"  // SortOrder

class QMenu;
class QAction;
class QContextMenuEvent;
class QString;
class QKeySequence;


//======================================================================================================================

template< typename SubClass >
class ExtendedViewCommon : protected ErrorReportingComponent {

 public:

	ExtendedViewCommon( QWidget * self, QStringView componentType, QStringView componentName )
		: ErrorReportingComponent( self, componentType, componentName ) {}

	//-- context menu --------------------------------------------------------------------------------------------------

	/// Available actions for the right-click context menu. Each one is associated with a key shortcut.
	enum MenuAction : uint
	{
		None             = 0,

		OpenFile         = (1 <<  1),   ///< open the last clicked file in the default application assigned for it   (available also in read-only views)
		OpenFileLocation = (1 <<  2),   ///< open the directory of the last clicked file in a system file explorer   (available also in read-only views)
		//----------------------------------------------------------------------------------------------------------------------------------------------
		AddAndDelete     = (1 <<  3),   ///< add a new item and delete the selected items                            (available only in editable views)
		Clone            = (1 <<  4),   ///< add a copy of the selected item with a new name                         (available only in editable views)
		//----------------------------------------------------------------------------------------------------------------------------------------------
		InsertSeparator  = (1 <<  5),   ///< insert named visual separator between items                             (available only in editable views)
		//----------------------------------------------------------------------------------------------------------------------------------------------
		Copy             = (1 <<  6),   ///< copy selected items into the clipboard                                  (available also in read-only views)
		CutAndPaste      = (1 <<  7),   ///< cut selected items into the clipboard or paste them from the clipboard  (available only in editable views)
		CutCopyPaste     = MenuAction::Copy | MenuAction::CutAndPaste,
		//----------------------------------------------------------------------------------------------------------------------------------------------
		Move             = (1 <<  8),   ///< move selected items up or down                                          (available only in editable views)
		//----------------------------------------------------------------------------------------------------------------------------------------------
		SortByName       = (1 <<  9),   ///< sort the items by their name                                            (available only in editable views)
		SortByType       = (1 << 10),   ///< sort the files by their type                                            (available only in editable views)
		SortBySize       = (1 << 11),   ///< sort the files by their size                                            (available only in editable views)
		SortByDate       = (1 << 12),   ///< sort the files by their date of the last modification                   (available only in editable views)
		SortFilesBy      = MenuAction::SortByName | MenuAction::SortByType | MenuAction::SortBySize | MenuAction::SortByDate,
		//----------------------------------------------------------------------------------------------------------------------------------------------
		Find             = (1 << 13),   ///< open a search bar to find an existing item by name                      (available also in read-only views)
		//----------------------------------------------------------------------------------------------------------------------------------------------
		ToggleIcons      = (1 << 14),   ///< show or hide the file or directory icons                                (available also in read-only views)

		All              = makeBitMask(14)
	};
	using MenuActions = std::underlying_type_t< MenuAction >;

	enum class SortKey
	{
		// These numbers correspond to the column indexes in the default QFileSystemModel, so don't change them!
		Name = 0,
		Type = 2,
		Size = 1,
		Date = 3,
	};

 protected:  // code de-duplication helpers that cannot directly be public

	// context menu

	/// Enables the ability to open a context menu by clicking with the right mouse button.
	void _enableContextMenu();

	/**
	  * \param actions Specifies the entries the context menu will have.
	  */

	bool _assertCanAddEditAction( const char * actionDesc ) const;

	/// Creates a custom action and adds it to the context menu.
	/** The resulting QAction object will emit triggered() signal that needs to be connected to the desired callback.
	  * The returned pointer is non-owning and must not be deleted, because it will be deleted by its parent view.
	  * The context menu must be enabled first by calling enableContextMenu(). */
	QAction * _addCustomMenuAction( QMenu * menu, const QString & text, const QKeySequence & shortcut );

	/// Adds one or more of the pre-defined actions to the context menu.
	/** Each enum value has it's corresponding QAction object in this class, whose signal may need to be handled
	  * based on whether it's public or not (read more details below).
	  * The returned pointer is non-owning and must not be deleted, because it will be deleted by its parent view.
	  * The context menu must be enabled first by calling enableContextMenu(). */
	void _addStandardMenuActions( MenuActions actions );

	/// Appends a visual separator to the context menu.
	void _addMenuSeparator();

	/// Enables/disables those actions that modify the list (inserting, deleting, reordering).
	void _toggleListModifications( bool enabled );

	void _contextMenuEvent( QContextMenuEvent * event );

	// copy&paste

	void _cutSelectedItems();
	void _copySelectedItems();
	void _pasteAboveSelectedItem();

 //actions:

	// The following actions will emit triggered() signal when the context menu entry is clicked or when
	// the corresponding key shortcut is pressed. The signal must be connected to a callback that does the job.
	//
	// The public actions must be connected and handled by the owner of this view and its corresponding model,
	// because in different use cases the operations are performed differently and the callbacks may be re-used
	// for multiple different signals.
	//
	// The private actions are connected and handled internally and work universally in all use cases.

	protected: QAction * openFileAction = nullptr;           ///< corresponds to MenuAction::OpenFile
	protected: QAction * openFileLocationAction = nullptr;   ///< corresponds to MenuAction::OpenFileLocation

	public:    QAction * addItemAction = nullptr;            ///< corresponds to MenuAction::AddAndDelete
	public:    QAction * deleteItemAction = nullptr;         ///< corresponds to MenuAction::AddAndDelete
	public:    QAction * cloneItemAction = nullptr;          ///< corresponds to MenuAction::Clone

	public:    QAction * insertSeparatorAction = nullptr;    ///< corresponds to MenuAction::InsertSeparator

	protected: QAction * cutItemsAction = nullptr;           ///< corresponds to MenuAction::CutAndPaste
	protected: QAction * copyItemsAction = nullptr;          ///< corresponds to MenuAction::Copy
	protected: QAction * pasteItemsAction = nullptr;         ///< corresponds to MenuAction::CutAndPaste

	public:    QAction * moveItemUpAction = nullptr;         ///< corresponds to MenuAction::Move
	public:    QAction * moveItemDownAction = nullptr;       ///< corresponds to MenuAction::Move
	public:    QAction * moveItemToTopAction = nullptr;      ///< corresponds to MenuAction::Move
	public:    QAction * moveItemToBottomAction = nullptr;   ///< corresponds to MenuAction::Move

	protected: QMenu   * sortItemsSubmenu = nullptr;
	// For simplicity, the following actions are collapsed
	// into a single signal sortActionTriggered(...).
	protected: QAction * sortByNameAscAction = nullptr;      ///< corresponds to MenuAction::SortByName
	protected: QAction * sortByNameDscAction = nullptr;      ///< corresponds to MenuAction::SortByName
	protected: QAction * sortByTypeAscAction = nullptr;      ///< corresponds to MenuAction::SortByType
	protected: QAction * sortByTypeDscAction = nullptr;      ///< corresponds to MenuAction::SortByType
	protected: QAction * sortBySizeAscAction = nullptr;      ///< corresponds to MenuAction::SortBySize
	protected: QAction * sortBySizeDscAction = nullptr;      ///< corresponds to MenuAction::SortBySize
	protected: QAction * sortByDateAscAction = nullptr;      ///< corresponds to MenuAction::SortByDate
	protected: QAction * sortByDateDscAction = nullptr;      ///< corresponds to MenuAction::SortByDate

	public:    QAction * findItemAction = nullptr;           ///< corresponds to MenuAction::Find

	public:    QAction * toggleIconsAction = nullptr;        ///< corresponds to MenuAction::ToggleIcons

 private: // helpers

	auto * thisAsSubClass()        { return static_cast<       SubClass * >( this ); }
	auto * thisAsSubClass() const  { return static_cast< const SubClass * >( this ); }

 protected:

	QMenu * contextMenu = nullptr;
	MenuActions contextMenuActions = 0;
	bool allowModifyList = false;

};


//======================================================================================================================


#endif // EXTENDED_VIEW_COMMON_INCLUDED
