//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: list view that supports editing of item names and behaves correctly on both internal and external
//              drag&drop operations
//======================================================================================================================

#include "EditableListView.hpp"

#include "ListModel.hpp"
#include "Utils/EventFilters.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/OSUtils.hpp"  // openFileLocation
#include "Utils/ErrorHandling.hpp"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QKeyEvent>
#include <QMenu>


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
// We can't manually update the indexes in dropEvent, because after dropEvent Qt calls model.removeRows on items
// that are CURRENTLY SELECTED, instead of on items that were selected at the beginning of the drag&drop operation.
// So we must update the selection at some point AFTER the drag&drop operation is finished and the rows removed.
//
// The correct place seems to be (despite its confusing name) QAbstractItemView::startDrag. It is a common
// parent function for Model::dropMimeData and Model::removeRows both of which happen when items are dropped.
// However this is called only when the source of the drag is this application.
// When you drag files from a directory window, then dropEvent is called from somewhere else. In that case
// we update the selection in dropEvent, because there the deletion of the selected items doesn't happen.


//======================================================================================================================

EditableListView::EditableListView( QWidget * parent )
:
	QListView( parent ),
	LoggingComponent("EditableListView")
{
	allowEditNames = false;
	this->setEditTriggers( QAbstractItemView::NoEditTriggers );

	allowModifyList = false;

	contextMenu = new QMenu( this );  // will be deleted when this QListView (their parent) is deleted
	addItemAction = addAction( "Add", { Qt::Key_Insert } );
	deleteItemAction = addAction( "Delete", { Qt::Key_Delete } );
	moveItemUpAction = addAction( "Move up", { Qt::CTRL + Qt::Key_Up } );  // shut-up clang, this is the official way to do it by Qt doc
	moveItemDownAction = addAction( "Move down", { Qt::CTRL + Qt::Key_Down } );
	contextMenuEnabled = false;

	allowIntraWidgetDnD = true;
	allowInterWidgetDnD = false;
	allowExternFileDnD = false;
	updateDragDropMode();
	this->setDefaultDropAction( Qt::MoveAction );
	this->setDropIndicatorShown( true );
}

QAction * EditableListView::addAction( const QString & text, const QKeySequence & shortcut )
{
	QAction * action = new QAction( text, this );
	action->setShortcut( shortcut );
	action->setShortcutContext( Qt::WidgetShortcut );  // only listen to this shortcut when this widget has focus
	QWidget::addAction( action );  // register it to this widget, so the shortcut is checked
	contextMenu->addAction( action );  // register it to the menu, so that it appears there when right-clicked
	return action;
}

EditableListView::~EditableListView()
{
	// QAction members get deleted as children of this QListView (their parent)
}


//----------------------------------------------------------------------------------------------------------------------
//  editing

void EditableListView::toggleNameEditing( bool enabled )
{
	allowEditNames = enabled;

	if (enabled)
		this->setEditTriggers( QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed );
	else
		this->setEditTriggers( QAbstractItemView::NoEditTriggers );
}

bool EditableListView::isEditModeOpen() const
{
	return this->state() == QAbstractItemView::EditingState;
}

bool EditableListView::startEditingCurrentItem()
{
	this->edit( this->currentIndex() );
	return isEditModeOpen();
}

void EditableListView::stopEditingAndCommit()
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

void EditableListView::toggleListModifications( bool enabled )
{
	allowModifyList = enabled;
}


//----------------------------------------------------------------------------------------------------------------------
//  right-click menu

void EditableListView::toggleContextMenu( bool enabled )
{
	contextMenuEnabled = enabled;
}

void EditableListView::enableItemCloning()
{
	cloneItemAction = addAction( "Clone", { Qt::CTRL + Qt::Key_C } );
}

void EditableListView::enableInsertSeparator()
{
	insertSeparatorAction = addAction( "Insert separator", { Qt::CTRL + Qt::Key_Slash } );
}

void EditableListView::enableFinding()
{
	findItemAction = addAction( "Find", QKeySequence::Find );
}

void EditableListView::enableOpenFileLocation()
{
	openFileLocationAction = addAction( "Open file location", {} );
	connect( openFileLocationAction, &QAction::triggered, this, &thisClass::openCurrentFileLocation );
}

void EditableListView::enableTogglingIcons()
{
	toggleIconsAction = addAction( "Show icons", {} );
	connect( toggleIconsAction, &QAction::triggered, this, QOverload<>::of( &thisClass::toggleIcons ) );
}

void EditableListView::contextMenuEvent( QContextMenuEvent * event )
{
	QModelIndex clickedItemIndex = this->indexAt( event->pos() );

	if (!contextMenuEnabled)
		return;

	if (addItemAction)
		addItemAction->setEnabled( allowModifyList );
	if (deleteItemAction)
		deleteItemAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (cloneItemAction)
		cloneItemAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (moveItemUpAction)
		moveItemUpAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (moveItemDownAction)
		moveItemDownAction->setEnabled( allowModifyList && clickedItemIndex.isValid() );
	if (insertSeparatorAction)
		insertSeparatorAction->setEnabled( allowModifyList );
	if (findItemAction)
		findItemAction->setEnabled( true );
	if (openFileLocationAction)
		openFileLocationAction->setEnabled( clickedItemIndex.isValid() );
	if (toggleIconsAction)
		toggleIconsAction->setEnabled( true );

	contextMenu->popup( event->globalPos() );
}

void EditableListView::openCurrentFileLocation()
{
	QModelIndex currentIdx = selectionModel()->currentIndex();
	if (!currentIdx.isValid())
	{
		reportUserError( this->parentWidget(), "No item chosen", "You did not click on any file." );
		return;
	}

	auto userData = model()->data( currentIdx, Qt::UserRole );
	if (userData.type() != QVariant::Type::String)
	{
		reportLogicError( this->parentWidget(), "Unsupported model",
			"EditableListView should be used only together with ReadOnlyListModel or EditableListModel, "
			"otherwise openFileLocation will not work."
		);
		return;
	}

	QString filePath = userData.toString();

	os::openFileLocation( filePath );  // errors are handled inside
}

bool EditableListView::areIconsEnabled() const
{
	if (ListModelCommon * model = dynamic_cast< ListModelCommon * >( this->model() ))
	{
		return model->areIconsEnabled();
	}
	else
	{
		return false;
	}
}

void EditableListView::toggleIcons()
{
	ListModelCommon * model = dynamic_cast< ListModelCommon * >( this->model() );
	if (!model)
	{
		reportLogicError( this->parentWidget(), "Unsupported model",
			"EditableListView should be used only together with ReadOnlyListModel or EditableListModel, "
			"otherwise icons cannot be toggled."
		);
		return;
	}

	bool newIconState = !model->areIconsEnabled();
	model->toggleIcons( newIconState );
	toggleIconsAction->setText( newIconState ? "Hide icons" : "Show icons" );
}

void EditableListView::toggleIcons( bool enabled )
{
	if (areIconsEnabled() != enabled)
	{
		toggleIcons();
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  drag&drop

void EditableListView::updateDragDropMode()
{
	const bool externalDrops = allowInterWidgetDnD || allowExternFileDnD;

	if (!allowIntraWidgetDnD && !externalDrops)
		this->setDragDropMode( NoDragDrop );
	else if (allowIntraWidgetDnD && !externalDrops)
		this->setDragDropMode( InternalMove );
	else
		this->setDragDropMode( DragDrop );
}

void EditableListView::toggleIntraWidgetDragAndDrop( bool enabled )
{
	allowIntraWidgetDnD = enabled;

	updateDragDropMode();
}

void EditableListView::toggleInterWidgetDragAndDrop( bool enabled )
{
	allowInterWidgetDnD = enabled;

	updateDragDropMode();
}

void EditableListView::toggleExternalFileDragAndDrop( bool enabled )
{
	allowExternFileDnD = enabled;

	updateDragDropMode();
}

bool EditableListView::isDropAcceptable( QDragMoveEvent * event )
{
	if (isIntraWidgetDnD( event ) && allowIntraWidgetDnD && event->possibleActions() & Qt::MoveAction)
		return true;
	else if (isInterWidgetDnD( event ) && allowInterWidgetDnD && event->possibleActions() & Qt::MoveAction)
		return true;
	else if (isExternFileDnD( event ) && allowExternFileDnD)
		return true;
	else
		return false;
}

bool EditableListView::isIntraWidgetDnD( QDropEvent * event )
{
	return event->source() == this;
}

bool EditableListView::isInterWidgetDnD( QDropEvent * event )
{
	return event->source() != this && !event->mimeData()->hasUrls();
}

bool EditableListView::isExternFileDnD( QDropEvent * event )
{
	return event->source() != this && event->mimeData()->hasUrls();
}

void EditableListView::dragEnterEvent( QDragEnterEvent * event )
{
	// QListView::dragEnterEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. accept if event contains at leats one mime type present in model->mimeTypes or model->canDropMimeData
	// We override it, so that we apply our own rules and restrictions for the drag&drop operation.

	if (isDropAcceptable( event ))  // does proposed drop operation comply with our settings?
	{
		superClass::dragEnterEvent( event );  // let it calc the index and query the model if the drop is ok there
	}
	else
	{
		event->ignore();
	}
}

void EditableListView::dragMoveEvent( QDragMoveEvent * event )
{
	// QListView::dragMoveEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. accept if event contains at leats one mime type present in model->mimeTypes or model->canDropMimeData
	// 3. draw drop indicator according to position
	// We override it, so that we apply our own rules and restrictions for the drag&drop operation.

	if (isDropAcceptable( event ))  // does proposed drop operation comply with our settings?
	{
		superClass::dragMoveEvent( event );  // let it query the model if the drop is ok there and draw the indicator
	}
	else
	{
		event->ignore();
	}
}

void EditableListView::dropEvent( QDropEvent * event )
{
	// QListView::dropEvent in short:
	// 1. if mode is InternalMove then discard events from external sources and copy actions
	// 2. get drop index from cursor position
	// 3. if model->dropMimeData then accept drop event
	superClass::dropEvent( event );

	// announce dropped files now only if it's an external drag&drop
	// otherwise postpone it because of the issue decribed at the top
	if (isExternFileDnD( event ))
		itemsDropped();
}

void EditableListView::startDrag( Qt::DropActions supportedActions )
{
	superClass::startDrag( supportedActions );

	// at this point the drag&drop should be finished and source rows removed, so we can safely update the selection
	itemsDropped();
}

void EditableListView::itemsDropped()
{
	// idiotic workaround because Qt is fucking retarded   (read the comment at the top)
	//
	// retrieve the destination drop indexes from the model and update the selection accordingly

	DropTarget * model = dynamic_cast< DropTarget * >( this->model() );
	if (!model)
	{
		// EditableListView should be used only together with EditableListModel, otherwise drag&drop won't work properly
		logLogicError() << "assigned model is not a DropTarget, drag&drop won't work properly";
		return;
	}

	if (model->wasDroppedInto())
	{
		int row = model->droppedRow();
		int count = model->droppedCount();

		// When an item is in edit mode and current index changes, the content of the line editor is dumped
		// into old current item and the edit mode closed. Therefore we must change the current index in advance,
		// otherwise the edit content gets saved into a wrong item.
		wdg::unsetCurrentItem( this );
		wdg::deselectSelectedItems( this );
		for (int i = 0; i < count; i++)
			wdg::selectItemByIndex( this, row + i );
		wdg::setCurrentItemByIndex( this, row + count - 1 );

		emit itemsDropped( row, count );

		model->resetDropState();
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  keyboard control

static inline bool isArrowKey( int key )
{
	return key >= Qt::Key_Left && key <= Qt::Key_Down;
}

void EditableListView::keyPressEvent( QKeyEvent * event )
{
	int key = event->key();

	bool isModifier = modifierHandler.updateModifiers_pressed( key );

	if (!isModifier)
	{
		if (key == Qt::Key_Space)
		{
			// When user has multiple items selected and presses space, default implementation only checks/unchecks
			// the current item, not all the selected ones. Therefore we have to do it manually here.
			const auto selectedIndexes = this->selectionModel()->selectedIndexes();
			for (const QModelIndex & selectedIdx : selectedIndexes)
			{
				Qt::ItemFlags flags = model()->flags( selectedIdx );
				if (flags & Qt::ItemIsUserCheckable)
				{
					Qt::CheckState state = Qt::CheckState( model()->data( selectedIdx, Qt::CheckStateRole ).toInt() );
					model()->setData( selectedIdx, state == Qt::Checked ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole );
				}
			}
			return;  // supress the original handling of spacebar
		}
	}

	superClass::keyPressEvent( event );
}

void EditableListView::keyReleaseEvent( QKeyEvent * event )
{
	int key = event->key();

	modifierHandler.updateModifiers_released( key );

	// supress arrow navigation when CTRL is pressed, otherwise the selection would get messed up
	if (isArrowKey( key ) && modifierHandler.pressedModifiers() != 0)
	{
		return;
	}

	superClass::keyReleaseEvent( event );
}
