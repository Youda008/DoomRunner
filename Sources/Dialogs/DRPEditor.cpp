//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: internal editor of DoomRunner Packs
//======================================================================================================================

#include "DRPEditor.hpp"
#include "ui_DRPEditor.h"

#include "DoomFiles.hpp"
#include "Dialogs/WADDescViewer.hpp"
#include "Utils/DoomRunnerPacks.hpp"
#include "Utils/FileSystemUtils.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/MiscUtils.hpp"  // makeFileFilter

#include <QFileInfo>
#include <QGuiApplication>


//======================================================================================================================

DRPEditor::DRPEditor(
	QWidget * parentWidget, const PathConvertor & pathConv, QString lastUsedDir_, bool showIcons, QString filePath
) :
	QDialog( parentWidget ),
	DialogWithPaths( this, u"EngineDialog", pathConv ),
	modModel( u"modModel",
		/*makeDisplayString*/ []( const Mod & mod ) { return mod.name; }
	)
{
	ui = new Ui::DRPEditor;
	ui->setupUi( this );

	DialogWithPaths::lastUsedDir = std::move( lastUsedDir_ );

	origFilePath = std::move( filePath );

	// set window title
	if (!origFilePath.isEmpty())
		this->setWindowTitle( fs::getFileNameFromPath( origFilePath ) );
	else
		this->setWindowTitle( "New DoomRunner Pack" );

	ui->saveBtn->setEnabled( !origFilePath.isEmpty() );
	ui->deleteBtn->setEnabled( !origFilePath.isEmpty() );

	// setup and populate mod list
	setupModList( showIcons );
	if (!origFilePath.isEmpty())
		loadModsFromDRP( origFilePath );

	// show this editor above the parent widget, slightly to the right
	QPoint globalPos = parentWidget->mapToGlobal( QPoint( 40, 0 ) );
	this->move( globalPos.x(), globalPos.y() );

	connect( ui->saveBtn, &QPushButton::clicked, this, &ThisClass::onSaveBtnClicked );
	connect( ui->saveAsBtn, &QPushButton::clicked, this, &ThisClass::onSaveAsBtnClicked );
	connect( ui->deleteBtn, &QPushButton::clicked, this, &ThisClass::onDeleteBtnClicked );
}

DRPEditor::~DRPEditor()
{
	delete ui;
}

void DRPEditor::setupModList( bool showIcons )
{
	// connect the view with the model
	ui->modListView->setModel( &modModel );

	// set selection rules
	ui->modListView->setSelectionMode( QAbstractItemView::ExtendedSelection );

	// set drag&drop behaviour
	modModel.setPathConvertor( pathConvertor );  // the model needs our path convertor for converting paths dropped from a file explorer
	ui->modListView->setDnDOutputTypes( DnDOutputType::FilePaths );
	ui->modListView->setAllowedDnDSources( DnDSource::ThisWidget | DnDSource::OtherWidget | DnDSource::ExternalApp );

	// set reaction when an item is clicked
	connect( ui->modListView, &QListView::doubleClicked, this, &ThisClass::onModDoubleClicked );

	// setup reaction to key shortcuts and right click
	ui->modListView->enableContextMenu( 0
		| ExtendedListView::MenuAction::OpenFile
		| ExtendedListView::MenuAction::OpenFileLocation
		| ExtendedListView::MenuAction::AddAndDelete
		| ExtendedListView::MenuAction::Copy
		| ExtendedListView::MenuAction::CutAndPaste
		| ExtendedListView::MenuAction::Move
	);
	createNewDRPAction = ui->modListView->addAction( "Create new DR pack", {} );
	addExistingDRPAction = ui->modListView->addAction( "Add existing DR pack", {} );
	ui->modListView->toggleListModifications( true );
	connect( ui->modListView->addItemAction, &QAction::triggered, this, &ThisClass::modAdd );
	connect( ui->modListView->deleteItemAction, &QAction::triggered, this, &ThisClass::modDelete );
	connect( ui->modListView->moveItemUpAction, &QAction::triggered, this, &ThisClass::modMoveUp );
	connect( ui->modListView->moveItemDownAction, &QAction::triggered, this, &ThisClass::modMoveDown );
	connect( ui->modListView->moveItemToTopAction, &QAction::triggered, this, &ThisClass::modMoveToTop );
	connect( ui->modListView->moveItemToBottomAction, &QAction::triggered, this, &ThisClass::modMoveToBottom );
	connect( createNewDRPAction, &QAction::triggered, this, &ThisClass::modCreateNewDRP );
	connect( addExistingDRPAction, &QAction::triggered, this, &ThisClass::modAddExistingDRP );

	// setup icons (must be set called after enableContextMenu, because it requires toggleIconsAction)
	ui->modListView->toggleIcons( showIcons );  // we need to do this instead of model.toggleIcons() in order to update the action text

	// setup buttons
	connect( ui->modBtnAdd, &QToolButton::clicked, this, &ThisClass::modAdd );
	connect( ui->modBtnAddDir, &QToolButton::clicked, this, &ThisClass::modAddDir );
	connect( ui->modBtnDel, &QToolButton::clicked, this, &ThisClass::modDelete );
	connect( ui->modBtnUp, &QToolButton::clicked, this, &ThisClass::modMoveUp );
	connect( ui->modBtnDown, &QToolButton::clicked, this, &ThisClass::modMoveDown );
}


//----------------------------------------------------------------------------------------------------------------------
// mod list loading and saving

void DRPEditor::loadModsFromDRP( const QString & filePath )
{
	const QStringList entries = drp::getEntries( filePath );

	wdg::deselectAllAndUnsetCurrent( ui->modListView );
	modModel.startCompleteUpdate();
	modModel.clear();
	for (const QString & filePath : entries)
	{
		modModel.append( Mod( filePath ) );
	}
	modModel.finishCompleteUpdate();
}

bool DRPEditor::saveModsToDRP( const QString & filePath )
{
	QStringList entries;
	for (const Mod & mod : modModel)
	{
		entries.append( mod.path );
	}

	bool saved = drp::saveEntries( filePath, std::move(entries) );
	if (saved)
	{
		savedFilePath = filePath;
	}

	return saved;
}


//----------------------------------------------------------------------------------------------------------------------
// mod list manipulation

void DRPEditor::modAdd()
{
	const QStringList paths = DialogWithPaths::selectFiles( this, "mod file", {},
		  makeFileFilter( "Doom mod files", doom::pwadSuffixes )
		+ makeFileFilter( "DukeNukem data files", doom::dukeSuffixes )
		+ makeFileFilter( "DoomRunner Pack files", { drp::fileSuffix } )
		+ "All files (*)"
	);
	if (paths.isEmpty())  // user probably clicked cancel
		return;

	for (const QString & path : paths)
	{
		Mod mod{ QFileInfo( path ) };

		wdg::appendItem( ui->modListView, modModel, mod );
	}
}

void DRPEditor::modAddDir()
{
	QString path = DialogWithPaths::selectDir( this, "of the mod" );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	Mod mod{ QFileInfo( path ) };

	wdg::appendItem( ui->modListView, modModel, mod );
}

void DRPEditor::modDelete()
{
	wdg::removeSelectedItems( ui->modListView, modModel );
}

void DRPEditor::modMoveUp()
{
	wdg::moveSelectedItemsUp( ui->modListView, modModel );
}

void DRPEditor::modMoveDown()
{
	wdg::moveSelectedItemsDown( ui->modListView, modModel );
}

void DRPEditor::modMoveToTop()
{
	wdg::moveSelectedItemsToTop( ui->modListView, modModel );
}

void DRPEditor::modMoveToBottom()
{
	wdg::moveSelectedItemsToBottom( ui->modListView, modModel );
}

void DRPEditor::modCreateNewDRP()
{
	QString newFilePath = createNewDRP();
	if (newFilePath.isEmpty())
		return;  // update the data only if user clicked Ok and the save was successful

	Mod mod( newFilePath );

	wdg::appendItem( ui->modListView, modModel, mod );
}

void DRPEditor::modAddExistingDRP()
{
	const QStringList filePaths = addExistingDRP();
	if (filePaths.isEmpty())  // user probably clicked cancel
		return;

	for (const QString & path : filePaths)
	{
		Mod mod( path );

		wdg::appendItem( ui->modListView, modModel, mod );
	}
}

void DRPEditor::onModDoubleClicked( const QModelIndex & index )
{
	QFileInfo fileInfo( modModel[ index.row() ].path );

	if (fileInfo.isDir())
	{
		return;
	}

	if (fileInfo.suffix() == drp::fileSuffix)
	{
		auto result = editDRP( fileInfo.filePath() );

		// update the mod list
		if (result.outcome == DRPEditor::Outcome::SavedAsNew)
		{
			Mod newDRP( result.savedFilePath, /*checked*/true );

			modModel.startAppendingItems( 1 );
			modModel.append( newDRP );
			modModel.finishAppendingItems();
		}
		else if (result.outcome == DRPEditor::Outcome::Deleted)
		{
			modModel.startRemovingItems( index.row(), 1 );
			modModel.removeAt( index.row() );
			modModel.finishRemovingItems();
		}
	}
	else
	{
		showTxtDescriptionFor( this, fileInfo.filePath(), "mod description" );
	}
}

QString DRPEditor::createNewDRP()
{
	DRPEditor editor( ui->modListView, pathConvertor, lastUsedDir, ui->modListView->areIconsEnabled(), {} );

	int code = editor.exec();

	if (code != QDialog::Accepted || editor.savedFilePath.isEmpty())  // dialog cancelled or saving the file failed
	{
		return {};
	}

	lastUsedDir = editor.takeLastUsedDir();

	return editor.savedFilePath;
}

QStringList DRPEditor::addExistingDRP()
{
	const QStringList filePaths = DialogWithPaths::selectFiles( this, "DoomRunner Pack", {},
		makeFileFilter( "DoomRunner Pack files", { drp::fileSuffix } )
		+ "All files (*)"
	);

	if (filePaths.isEmpty())  // user probably clicked cancel
	{
		return {};
	}

	return filePaths;
}

DRPEditor::Result DRPEditor::editDRP( const QString & filePath )
{
	DRPEditor editor( this, pathConvertor, lastUsedDir, ui->modListView->areIconsEnabled(), filePath );

	int code = editor.exec();

	if (code != QDialog::Accepted)
	{
		return { DRPEditor::Outcome::Cancelled, {} };
	}

	lastUsedDir = editor.takeLastUsedDir();

	return { editor.outcome, std::move( editor.savedFilePath ) };
}


//----------------------------------------------------------------------------------------------------------------------
// dialog finalization

void DRPEditor::onSaveBtnClicked()
{
	bool saved = saveModsToDRP( origFilePath );
	outcome = saved ? Outcome::SavedToExisting : Outcome::Failed;

	SuperClass::accept();  // regardless if the save was successful, close the dialog
}

void DRPEditor::onSaveAsBtnClicked()
{
	QString destFilePath = DialogWithPaths::selectDestFile( this, "Save DoomRunner Pack", lastUsedDir,
		makeFileFilter( "DoomRunner Pack files", { drp::fileSuffix } )
		+ "All files (*)"
	);
	if (destFilePath.isEmpty())
	{
		return;  // user clicked cancel, return back to the dialog
	}

	bool saved = saveModsToDRP( destFilePath );
	outcome = saved ? Outcome::SavedAsNew : Outcome::Failed;

	SuperClass::accept();  // regardless if the save was successful, close the dialog
}

void DRPEditor::onDeleteBtnClicked()
{
	bool fileDeleted = fs::deleteFile( origFilePath );
	if (fileDeleted)
	{
		outcome = Outcome::Deleted;
	}
	else
	{
		reportRuntimeError( "Cannot delete DoomRunner Pack",
			"Failed to delete the current DoomRunner Pack \""%origFilePath%"\""
		);
		outcome = Outcome::Failed;
	}

	SuperClass::accept();  // regardless if the save was successful, close the dialog
}
