//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: list view that supports editing of item names and behaves correctly on both internal and external
//              drag&drop operations
//======================================================================================================================

#include "ExtendedListView.hpp"

#include "ExtendedViewCommon.impl.hpp"
#include "DataModels/GenericListModel.hpp"
#include "Utils/EventFilters.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/OSUtils.hpp"  // openFileLocation
#include "Utils/MiscUtils.hpp"  // getType
#include "Utils/ErrorHandling.hpp"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QApplication>


//======================================================================================================================
// When attempting to make a drag&drop from a new source work properly, there are 3 things to remember:
//  1. View must support the drop action type the source emits. Some use MoveAction, some CopyAction, ...
//  2. Model::mimeTypes() must return the MIME type, that is used by the source widget.
//  3. Model::canDropMimeData(...) must be correctly implemented to support both the MIME type and the drop action


//======================================================================================================================
// idiotic workaround because Qt is fucking retarded
//
// When an internal drag&drop for item reordering is performed, Qt doesn't update the selection and leaves selected
// those items sitting at the old indexes where the drag&drop started and where are now some completely different items.
//
// We can't manually update the indexes in dropEvent, because after dropEvent Qt calls model.removeRows() on items
// that are CURRENTLY SELECTED, instead of on items that were selected at the beginning of the drag&drop operation.
// So we must update the selection at some point AFTER the drag&drop operation is finished and the rows removed.
//
// The correct place seems to be (despite its confusing name) QAbstractItemView::startDrag. It is a common
// parent function for Model::dropMimeData() and Model::removeRows() both of which happen when items are dropped.
// However this is called only when the source of the drag is this application.
// When you drag files from a file explorer window, then dropEvent is called from somewhere else. In that case
// we update the selection in dropEvent, because there the deletion of the selected items doesn't happen.


//----------------------------------------------------------------------------------------------------------------------
// construction

ExtendedListView::ExtendedListView( QWidget * parent )
:
	QListView( parent ),
	// NOTE: We are passing a string view made from a temporary QString.
	//       But because Qt uses reference counting and copy-on-write, the internal string buffer will keep existing.
	ExtendedViewCommon( parent, u"ExtendedListView", this->objectName() )
{
	// defaults
	toggleItemEditing( false );
	toggleListModifications( false );
	setAllowedDnDSources( DnDSource::None );
	setDnDOutputTypes( false );
}

ExtendedListView::~ExtendedListView() = default;


//----------------------------------------------------------------------------------------------------------------------
// model setup

void ExtendedListView::setModel( QAbstractItemModel * model )
{
	QBaseView::setModel( model );

	ownModel = dynamic_cast< AListModel * >( model );
	if (ownModel)
	{
		access = ownModel->accessStyle();
	}
	else
	{
		// ExtendedListView should be used together with GenericListModel, otherwise it won't work properly.
		logLogicError() << "assigned model is not GenericListModel, some functions are disabled";
	}
}


//----------------------------------------------------------------------------------------------------------------------
// icons

bool ExtendedListView::areIconsEnabled() const
{
	return ownModel ? ownModel->areIconsEnabled() : false;
}

void ExtendedListView::toggleIcons( bool enabled )
{
	if (ownModel)
	{
		ownModel->toggleIcons( enabled );
		toggleIconsAction->setText( enabled ? "Hide icons" : "Show icons" );
	}
}

void ExtendedListView::toggleIcons()
{
	toggleIcons( !areIconsEnabled() );
}


//----------------------------------------------------------------------------------------------------------------------
// editing item content

void ExtendedListView::toggleCheckboxes( bool enabled )
{
	if (ownModel)
	{
		ownModel->toggleCheckboxes( enabled );
	}
}

void ExtendedListView::toggleItemEditing( bool enabled )
{
	if (enabled && isReadOnly())
	{
		logLogicError() << "attempted to enable editing items in a read-only list view";
		return;
	}

	allowEditNames = enabled;

	QBaseView::setEditTriggers(
		enabled
		? (QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed)
		: QAbstractItemView::NoEditTriggers
	);

	if (ownModel)
	{
		ownModel->toggleItemEditing( enabled );
	}
}

bool ExtendedListView::isItemBeingEdited() const
{
	return this->state() == QAbstractItemView::EditingState;
}

bool ExtendedListView::startEditingCurrentItem()
{
	this->edit( this->currentIndex() );
	return isItemBeingEdited();
}

void ExtendedListView::stopEditingAndCommit()
{
	// yet another idiotic workaround because Qt is fucking dumb
	//
	// Qt does not give us access to the editor and does not allow us to manually close it or commit the data of it.
	// But when the current index is changed, it is done automatically. So we change the current index to some nonsense
	// and then restore it back, and Qt will do it for us for a bit of extra overhead.

	QModelIndex currentIndex = this->currentIndex();
	this->selectionModel()->setCurrentIndex( QModelIndex(), QItemSelectionModel::NoUpdate );
	this->selectionModel()->setCurrentIndex( currentIndex, QItemSelectionModel::NoUpdate );
}


//----------------------------------------------------------------------------------------------------------------------
// drag&drop

void ExtendedListView::setDnDOutputTypes( DnDOutputTypes outputTypes )
{
	enabledDnDOutputTypes = outputTypes;

	// set the most suitable QAbstractItemView's drag&drop properties
	updateQtViewProperties();

	// set the required model export/import properties
	updateModelExportImportFormats();
}

void ExtendedListView::setAllowedDnDSources( DnDSources dndSources )
{
	if (dndSources != DnDSource::None && isReadOnly())
	{
		logLogicError() << "attempted to enable drag&drop in a read-only list view.";
		return;
	}

	allowedDnDSources = dndSources;

	// set the most suitable QAbstractItemView's drag&drop properties
	updateQtViewProperties();

	// set the required model export/import properties
	updateModelExportImportFormats();
}

void ExtendedListView::toggleAllowedDnDSources( DnDSources dndSourcesToSwitch, bool enabled )
{
	auto newDnDSources = withToggledFlags( allowedDnDSources, dndSourcesToSwitch, enabled );
	setAllowedDnDSources( newDnDSources );
}

void ExtendedListView::updateQtViewProperties()
{
	auto allowedDndSources_ = expandToBools< DnDSourcesExp >( allowedDnDSources );
	bool canDragItems = enabledDnDOutputTypes != DnDOutputType::None || allowedDndSources_.thisWidget;
	bool canDropItems = allowedDnDSources != DnDSource::None;

	// defaultDropAction - This property is a bit of a mystery,
	// because it's documentation is very brief and the source code surrounding it is very complicated.
	// We're just gonna set it according to the access type of this view and then override the action in dragEnterEvent.
	Qt::DropAction defaultDnDAction = Qt::IgnoreAction;
	if (canDragItems)
	{
		if (this->isReadOnly())
			defaultDnDAction = Qt::CopyAction;
		else
			defaultDnDAction = Qt::MoveAction;
	}
	QBaseView::setDefaultDropAction( defaultDnDAction );

	// dragDropMode
	QAbstractItemView::DragDropMode dndMode = DragDropMode::NoDragDrop;
	if (canDragItems && !canDropItems) {
		dndMode = DragDropMode::DragOnly;
	} else if (!canDragItems && canDropItems) {
		dndMode = DragDropMode::DropOnly;
	} else if (canDragItems && canDropItems) {
		if (allowedDnDSources == DnDSource::ThisWidget)  // can only drop from this widget and not from anywhere else
			dndMode = DragDropMode::InternalMove;
		else
			dndMode = DragDropMode::DragDrop;
	}
	QBaseView::setDragDropMode( dndMode );

	QBaseView::setDropIndicatorShown( canDropItems );
}

void ExtendedListView::updateModelExportImportFormats()
{
	if (!ownModel)
	{
		return;
	}

	bool dragItemsAsFiles = areFlagsSet( enabledDnDOutputTypes, DnDOutputType::FilePaths );
	auto allowedDndSources_ = expandToBools< DnDSourcesExp >( allowedDnDSources );

	ExportFormats exportFormats = 0;
	ExportFormats importFormats = 0;
	if (dragItemsAsFiles)
	{
		exportFormats |= ExportFormat::FileUrls;
	}
	if (allowedDndSources_.thisWidget)
	{
		exportFormats |= ExportFormat::Indexes;
		importFormats |= ExportFormat::Indexes;
	}
	if (allowedDndSources_.otherWidget || allowedDndSources_.externalApp)
	{
		importFormats |= ExportFormat::FileUrls;
	}
	if (cutItemsAction || copyItemsAction || pasteItemsAction)
	{
		exportFormats |= ExportFormat::Json;
		importFormats |= ExportFormat::Json;
	}

	ownModel->setEnabledExportFormats( exportFormats );
	ownModel->setEnabledImportFormats( importFormats );
}

DnDSources ExtendedListView::getDnDSource( QDropEvent * event ) const
{
	if (event->source() == this)
		return DnDSource::ThisWidget;
	else if (event->source() != nullptr)
		return DnDSource::OtherWidget;
	else
		return DnDSource::ExternalApp;
}

Qt::DropAction ExtendedListView::getPreferredDnDAction( DnDSources dndSource ) const
{
	auto allowedDndSources_ = expandToBools< DnDSourcesExp >( allowedDnDSources );

	if (dndSource == DnDSource::ThisWidget && allowedDndSources_.thisWidget)
		return Qt::MoveAction;
	else if (dndSource == DnDSource::OtherWidget && allowedDndSources_.otherWidget)
		return Qt::MoveAction;
	else if (dndSource == DnDSource::ExternalApp && allowedDndSources_.externalApp)
		return Qt::CopyAction;
	else
		return Qt::IgnoreAction;
}

bool ExtendedListView::isDropAcceptable( QDragMoveEvent * event ) const
{
	return getPreferredDnDAction( getDnDSource( event ) ) != Qt::IgnoreAction;
}

/** Sets a selected phase of drag&drop operation to "in progress"
  * and then resets it back to "idle" at the end of the current scope. */
class DnDProgressGuard
{
	bool & isDnDInProgress;
 public:
	DnDProgressGuard( bool & isInProgress ) : isDnDInProgress( isInProgress ) { isDnDInProgress = true; }
	void setNoLongerInProgress() { isDnDInProgress = false; }
	~DnDProgressGuard() { isDnDInProgress = false; }  // setting it to false twice is not a problem, better than a condition
};

// called when the user moves the cursor holding an item into a new drop zone (widget)
void ExtendedListView::dragEnterEvent( QDragEnterEvent * event )
{
	// QListView::dragEnterEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. accept if the event contains at least one mime type from model->mimeTypes()
	// We override it, so that we apply our own rules and restrictions for the drag&drop operation.

	auto dndSource = getDnDSource( event );

	auto preferredAction = getPreferredDnDAction( dndSource );
	if (preferredAction == Qt::IgnoreAction)  // proposed drop event doesn't comply with our drag&drop settings
	{
		event->ignore();
		return;
	}
	else if (areFlagsSet( event->possibleActions(), preferredAction ))
	{
		// The drop action proposed by Qt is often not suitable, we'll rather choose it on our own.
		event->setDropAction( preferredAction );
	}

	QBaseView::dragEnterEvent( event );  // let it calc the index and query the model if the drop is ok there
}

// called when the user moves the cursor holding an item within the current drop zone (widget)
void ExtendedListView::dragMoveEvent( QDragMoveEvent * event )
{
	// QListView::dragMoveEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. accept if model->canDropMimeData( mime, action, index )
	// 3. draw drop indicator according to position
	// We override it, so that we apply our own rules and restrictions for the drag&drop operation.

	if (!isDropAcceptable( event ))  // does proposed drop operation comply with our settings?
	{
		event->ignore();
		return;
	}

	QBaseView::dragMoveEvent( event );  // let it query the model if the drop is ok there and draw the indicator
}

void ExtendedListView::dropEvent( QDropEvent * event )
{
	// QListView::dropEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. get drop index from cursor position
	// 3. if model->dropMimeData() then accept drop event

	auto dndSource = getDnDSource( event );

	auto preferredAction = getPreferredDnDAction( dndSource );
	if (preferredAction == Qt::IgnoreAction)  // proposed drop event doesn't comply with our drag&drop settings
	{
		event->ignore();
		return;
	}
	else if (areFlagsSet( event->possibleActions(), preferredAction ))
	{
		// The drop action proposed by Qt is often not suitable, we'll rather choose it on our own.
		event->setDropAction( preferredAction );
	}

	DnDProgressGuard droppedToGuard( isBeingDroppedTo );

	QBaseView::dropEvent( event );

	// If the dropped items come from somewhere else, then from this widget's point of view
	// the drag&drop is already finished (this list won't change anymore).
	// If it comes from this widget (items are being reordered), then it will be finished
	// when the items are removed from their original position.
	if (dndSource != DnDSource::ThisWidget)
	{
		onDragAndDropFinished( dndSource, droppedToGuard );
	}
	else
	{
		droppedFrom = dndSource;
	}
}

void ExtendedListView::startDrag( Qt::DropActions supportedActions )
{
	DnDProgressGuard draggedFromGuard( isBeingDraggedFrom );

	QBaseView::startDrag( supportedActions );

	if (droppedFrom)
	{
		// Now the reordering drag&drop is finished and source rows removed.
		onDragAndDropFinished( *droppedFrom, draggedFromGuard );
		droppedFrom.reset();
	}
}

void ExtendedListView::onDragAndDropFinished( DnDSources source, DnDProgressGuard & dndProgressGuard )
{
	// idiotic workaround because Qt is fucking retarded   (read the comment at the top)
	//
	// retrieve the destination drop indexes from the model and update the selection accordingly

	DropTarget * targetModel = dynamic_cast< DropTarget * >( this->model() );
	if (!targetModel)
	{
		// ExtendedListView should be used only together with GenericListModel, otherwise drag&drop won't work properly
		logLogicError() << "assigned model is not a DropTarget, drag&drop won't work properly";
		return;
	}

	if (targetModel->wasDroppedInto())
	{
		int row = targetModel->droppedRow();
		int count = targetModel->droppedCount();

		// When an item is in edit mode and current index changes, the content of the line editor is dumped
		// into old current item and the edit mode closed. Therefore we must change the current index in advance,
		// otherwise the edit content gets saved into a wrong item.
		wdg::unsetCurrentItem( this );
		wdg::deselectSelectedItems( this );
		for (int i = 0; i < count; i++)
			wdg::selectItemByIndex( this, row + i );
		wdg::setCurrentItemByIndex( this, row + count - 1 );

		// we want this to be already false inside the registered callbacks for the dragAndDropFinished signal
		dndProgressGuard.setNoLongerInProgress();

		emit dragAndDropFinished( row, count, source );

		targetModel->resetDropState();
	}
}


//----------------------------------------------------------------------------------------------------------------------
// context menu

void ExtendedListView::enableContextMenu( MenuActions actions )
{
	_enableContextMenu( actions );

	updateModelExportImportFormats();
}

QAction * ExtendedListView::addAction( const QString & text, const QKeySequence & shortcut )
{
	return _addAction( text, shortcut );
}

void ExtendedListView::toggleListModifications( bool enabled )
{
	_toggleListModifications( enabled );
}

void ExtendedListView::contextMenuEvent( QContextMenuEvent * event )
{
	return _contextMenuEvent( event );
}


//----------------------------------------------------------------------------------------------------------------------
// copy&paste

void ExtendedListView::cutSelectedItems()
{
	_cutSelectedItems();
}

void ExtendedListView::copySelectedItems()
{
	_copySelectedItems();
}

void ExtendedListView::pasteAboveSelectedItem()
{
	_pasteAboveSelectedItem();
}


//----------------------------------------------------------------------------------------------------------------------
// other actions

QString ExtendedListView::getCurrentFilePath() const
{
	QModelIndex currentIdx = this->selectionModel()->currentIndex();
	if (!currentIdx.isValid())
	{
		reportUserError( "No item chosen", "You did not click on any file." );
		return {};
	}

	auto userData = this->model()->data( currentIdx, Qt::UserRole );
	if (getType( userData ) != QMetaType::Type::QString)  // NOLINT
	{
		reportLogicError( u"getCurrentFilePath", "Unexpected model behaviour", "The model did not return QString for UserRole" );
		return {};
	}

	return userData.toString();
}

void ExtendedListView::openCurrentFile()
{
	QString filePath = getCurrentFilePath();
	if (filePath.isEmpty())
		return;

	os::openFileInDefaultApp( filePath );  // errors are handled inside
}

void ExtendedListView::openCurrentFileLocation()
{
	QString filePath = getCurrentFilePath();
	if (filePath.isEmpty())
		return;

	os::openFileLocation( filePath );  // errors are handled inside
}


//----------------------------------------------------------------------------------------------------------------------
// keyboard control

static inline bool isArrowKey( int key )
{
	return key >= Qt::Key_Left && key <= Qt::Key_Down;
}

void ExtendedListView::keyPressEvent( QKeyEvent * event )
{
	int key = event->key();

	bool isModifier = modifierHandler.updateModifiers_pressed( key );

	if (!isModifier)
	{
		if (key == Qt::Key_Space)
		{
			// When user has multiple items selected and presses space, default implementation only checks/unchecks
			// the current item, not all the selected ones. Therefore we have to do it manually here.
			toggleCheckStateOfSelectedItems();
			return;  // supress the original handling of spacebar
		}
	}

	QBaseView::keyPressEvent( event );
}

void ExtendedListView::toggleCheckStateOfSelectedItems()
{
	auto * model = this->model();
	const auto selectedIndexes = this->selectionModel()->selectedIndexes();
	for (const QModelIndex & selectedIdx : selectedIndexes)
	{
		Qt::ItemFlags flags = model->flags( selectedIdx );
		if (flags & Qt::ItemIsUserCheckable)
		{
			Qt::CheckState state = Qt::CheckState( model->data( selectedIdx, Qt::CheckStateRole ).toInt() );
			model->setData( selectedIdx, state == Qt::Checked ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole );
		}
	}
}

void ExtendedListView::keyReleaseEvent( QKeyEvent * event )
{
	int key = event->key();

	modifierHandler.updateModifiers_released( key );

	// supress arrow navigation when CTRL is pressed, otherwise the selection would get messed up
	if (isArrowKey( key ) && modifierHandler.pressedModifiers() != 0)
	{
		return;
	}

	QBaseView::keyReleaseEvent( event );
}
