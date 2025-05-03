//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: QTreeView extended by own general-purpose functionality
//======================================================================================================================

#include "ExtendedTreeView.hpp"

#include "ExtendedViewCommon.impl.hpp"
#include "Utils/OSUtils.hpp"  // openFileLocation

#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QClipboard>


//======================================================================================================================

ExtendedTreeView::ExtendedTreeView( QWidget * parent )
:
	QTreeView( parent ),
	// NOTE: We are passing a string view made from a temporary QString.
	//       But because Qt uses reference counting and copy-on-write, the internal string buffer will keep existing.
	ExtendedViewCommon( parent, u"ExtendedListView", this->objectName() )
{}

ExtendedTreeView::~ExtendedTreeView() = default;


//----------------------------------------------------------------------------------------------------------------------
// model setup

void ExtendedTreeView::setModel( QAbstractItemModel * model )
{
	QBaseView::setModel( model );

	fsModel = dynamic_cast< QFileSystemModel * >( model );

	updateColumnSize();  // adapt view to the current state of the new model
	connect( model, &QAbstractItemModel::dataChanged, this, &ThisClass::onDataChanged );  // prepare for future changes
	connect( model, &QAbstractItemModel::layoutChanged, this, &ThisClass::onLayoutChanged );  // prepare for future changes
}


//----------------------------------------------------------------------------------------------------------------------
// icons

struct EmptyIconProvider : public QFileIconProvider
{
	virtual QIcon icon( IconType ) const override { return QIcon(); }
	virtual QIcon icon( const QFileInfo & ) const override { return QIcon(); }
};

bool ExtendedTreeView::areIconsEnabled() const
{
	return fsModel && !dynamic_cast< EmptyIconProvider * >( fsModel->iconProvider() );
}

void ExtendedTreeView::toggleIcons( bool enabled )
{
	if (fsModel)
	{
		if (enabled)
		{
			fsModel->setIconProvider( new QFileIconProvider );
		}
		else
		{
			fsModel->setIconProvider( new EmptyIconProvider );
		}
		toggleIconsAction->setText( enabled ? "Hide icons" : "Show icons" );
	}
}

void ExtendedTreeView::toggleIcons()
{
	toggleIcons( !areIconsEnabled() );
}


//----------------------------------------------------------------------------------------------------------------------
// automatic column resizing

void ExtendedTreeView::onDataChanged( const QModelIndex &, const QModelIndex &, const QVector<int> & )
{
	updateColumnSize();
}

void ExtendedTreeView::onLayoutChanged( const QList< QPersistentModelIndex > &, QAbstractItemModel::LayoutChangeHint )
{
	updateColumnSize();
}

void ExtendedTreeView::updateColumnSize()
{
	// The tree view operates in columns and text that does not fit in the column's width is clipped.
	// This is the only way how to always keep the column wide enough for all the currently visible items to fit in
	// and rather display a horizontal scrollbar when they are wider than the widget.
	if (automaticallyResizeColumns)
	{
		for (int columnIdx = 0; columnIdx < model()->columnCount(); ++columnIdx)
			if (!this->isColumnHidden( columnIdx ))
				this->resizeColumnToContents( columnIdx );
	}
}


//----------------------------------------------------------------------------------------------------------------------
// context menu

void ExtendedTreeView::enableContextMenu( MenuActions actions )
{
	_enableContextMenu( actions );
}

QAction * ExtendedTreeView::addAction( const QString & text, const QKeySequence & shortcut )
{
	return _addAction( text, shortcut );
}

void ExtendedTreeView::toggleListModifications( bool enabled )
{
	_toggleListModifications( enabled );
}

void ExtendedTreeView::contextMenuEvent( QContextMenuEvent * event )
{
	return _contextMenuEvent( event );
}


//----------------------------------------------------------------------------------------------------------------------
// copy&paste

void ExtendedTreeView::cutSelectedItems()
{
	_cutSelectedItems();
}

void ExtendedTreeView::copySelectedItems()
{
	_copySelectedItems();
}

void ExtendedTreeView::pasteAboveSelectedItem()
{
	_pasteAboveSelectedItem();
}


//----------------------------------------------------------------------------------------------------------------------
// other actions

QString ExtendedTreeView::getCurrentFilePath() const
{
	QModelIndex currentIdx = this->selectionModel()->currentIndex();
	if (!currentIdx.isValid())
	{
		reportUserError( "No item chosen", "You did not click on any file." );
		return {};
	}


	if (!fsModel)
	{
		reportLogicError( u"getCurrentFilePath", "Unsupported model", "This action is only possible with QFileSystemModel." );
		return {};
	}

	return fsModel->filePath( currentIdx );
}

void ExtendedTreeView::openCurrentFile()
{
	QString filePath = getCurrentFilePath();
	if (filePath.isEmpty())
		return;

	os::openFileInDefaultApp( filePath );  // errors are handled inside
}

void ExtendedTreeView::openCurrentFileLocation()
{
	QString filePath = getCurrentFilePath();
	if (filePath.isEmpty())
		return;

	os::openFileLocation( filePath );  // errors are handled inside
}
