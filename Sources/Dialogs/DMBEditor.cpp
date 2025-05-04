//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: internal editor of Doom Mod Bundles
//======================================================================================================================

#include "DMBEditor.hpp"
#include "ui_DMBEditor.h"

#include "DoomFiles.hpp"
#include "Dialogs/WADDescViewer.hpp"
#include "Utils/DoomModBundles.hpp"
#include "Utils/FileSystemUtils.hpp"
#include "Utils/PathCheckUtils.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/MiscUtils.hpp"  // makeFileFilter

#include <QAction>
#include <QFileInfo>
#include <QGuiApplication>


//======================================================================================================================

DMBEditor::DMBEditor(
	QWidget * parentWidget, const PathConvertor & pathConv, QString lastUsedDir_, bool showIcons, QString filePath
) :
	QDialog( parentWidget ),
	DialogWithPaths( this, u"EngineDialog", pathConv ),
	modModel( u"modModel",
		/*makeDisplayString*/ []( const Mod & mod ) { return mod.name; }
	)
{
	ui = new Ui::DMBEditor;
	ui->setupUi( this );

	DialogWithPaths::lastUsedDir = std::move( lastUsedDir_ );

	origFilePath = std::move( filePath );

	// set window title
	if (!origFilePath.isEmpty())
		this->setWindowTitle( fs::getFileNameFromPath( origFilePath ) );
	else
		this->setWindowTitle( "new Mod Bundle" );

	ui->saveBtn->setEnabled( !origFilePath.isEmpty() );
	ui->deleteBtn->setEnabled( !origFilePath.isEmpty() );

	// setup and populate mod list
	setupModList( showIcons );
	if (!origFilePath.isEmpty())
		loadModsFromDMB( origFilePath );

	// show this editor above the parent widget, slightly to the right
	QPoint globalPos = parentWidget->mapToGlobal( QPoint( 40, 0 ) );
	this->move( globalPos.x(), globalPos.y() );

	connect( ui->saveBtn, &QPushButton::clicked, this, &ThisClass::onSaveBtnClicked );
	connect( ui->saveAsBtn, &QPushButton::clicked, this, &ThisClass::onSaveAsBtnClicked );
	connect( ui->deleteBtn, &QPushButton::clicked, this, &ThisClass::onDeleteBtnClicked );
}

DMBEditor::~DMBEditor()
{
	delete ui;
}

void DMBEditor::setupModList( bool showIcons )
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
	createNewDMBAction = ui->modListView->addAction( "Create new Mod Bundle", {} );
	addExistingDMBAction = ui->modListView->addAction( "Add existing Mod Bundle", {} );
	ui->modListView->toggleListModifications( true );
	connect( ui->modListView->addItemAction, &QAction::triggered, this, &ThisClass::modAdd );
	connect( ui->modListView->deleteItemAction, &QAction::triggered, this, &ThisClass::modDelete );
	connect( ui->modListView->moveItemUpAction, &QAction::triggered, this, &ThisClass::modMoveUp );
	connect( ui->modListView->moveItemDownAction, &QAction::triggered, this, &ThisClass::modMoveDown );
	connect( ui->modListView->moveItemToTopAction, &QAction::triggered, this, &ThisClass::modMoveToTop );
	connect( ui->modListView->moveItemToBottomAction, &QAction::triggered, this, &ThisClass::modMoveToBottom );
	connect( createNewDMBAction, &QAction::triggered, this, &ThisClass::modCreateNewDMB );
	connect( addExistingDMBAction, &QAction::triggered, this, &ThisClass::modAddExistingDMB );

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

void DMBEditor::loadModsFromDMB( const QString & filePath )
{
	const QStringList entries = dmb::getEntries( filePath );

	wdg::deselectAllAndUnsetCurrent( ui->modListView );
	modModel.startCompleteUpdate();
	modModel.clear();
	for (const QString & filePath : entries)
	{
		modModel.append( Mod( filePath ) );
	}
	modModel.finishCompleteUpdate();
}

bool DMBEditor::saveModsToDMB( const QString & filePath )
{
	QStringList entries;
	for (const Mod & mod : modModel)
	{
		entries.append( mod.path );
	}

	bool saved = dmb::saveEntries( filePath, std::move(entries) );
	if (saved)
	{
		savedFilePath = filePath;
	}

	return saved;
}


//----------------------------------------------------------------------------------------------------------------------
// mod list manipulation

void DMBEditor::modAdd()
{
	const QStringList paths = DialogWithPaths::selectFiles( this, "mod file", {},
		  makeFileFilter( "Doom mod files", doom::pwadSuffixes )
		+ makeFileFilter( "DukeNukem data files", doom::dukeSuffixes )
		+ makeFileFilter( "Doom Mod Bundles", { dmb::fileSuffix } )
		+ "All files (*)"
	);
	if (paths.isEmpty())  // user probably clicked cancel
		return;

	for (const QString & path : paths)
	{
		Mod mod( path );

		wdg::appendItem( ui->modListView, modModel, mod );
	}
}

void DMBEditor::modAddDir()
{
	QString path = DialogWithPaths::selectDir( this, "of the mod" );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	Mod mod( path );

	wdg::appendItem( ui->modListView, modModel, mod );
}

void DMBEditor::modDelete()
{
	wdg::removeSelectedItems( ui->modListView, modModel );
}

void DMBEditor::modMoveUp()
{
	wdg::moveSelectedItemsUp( ui->modListView, modModel );
}

void DMBEditor::modMoveDown()
{
	wdg::moveSelectedItemsDown( ui->modListView, modModel );
}

void DMBEditor::modMoveToTop()
{
	wdg::moveSelectedItemsToTop( ui->modListView, modModel );
}

void DMBEditor::modMoveToBottom()
{
	wdg::moveSelectedItemsToBottom( ui->modListView, modModel );
}

void DMBEditor::modCreateNewDMB()
{
	QString newFilePath = createNewDMB();
	if (newFilePath.isEmpty())
		return;  // update the data only if user clicked Ok and the save was successful

	Mod mod( newFilePath );

	wdg::appendItem( ui->modListView, modModel, mod );
}

void DMBEditor::modAddExistingDMB()
{
	const QStringList filePaths = addExistingDMB();
	if (filePaths.isEmpty())  // user probably clicked cancel
		return;

	for (const QString & path : filePaths)
	{
		Mod mod( path );

		wdg::appendItem( ui->modListView, modModel, mod );
	}
}

void DMBEditor::onModDoubleClicked( const QModelIndex & index )
{
	QFileInfo fileInfo( modModel[ index.row() ].path );

	if (fileInfo.isDir())
	{
		return;
	}

	if (fileInfo.suffix() == dmb::fileSuffix)
	{
		if (!PathChecker::checkItemFilePath( modModel[ index.row() ], true, "selected Mod Bundle", "" ))
		{
			return;  // do not open the dialog for non-existing file
		}

		auto result = editDMB( fileInfo.filePath() );

		// update the mod list
		if (result.outcome == DMBEditor::Outcome::SavedAsNew)
		{
			Mod newDMB( result.savedFilePath, /*checked*/true );

			modModel.startAppendingItems( 1 );
			modModel.append( newDMB );
			modModel.finishAppendingItems();
		}
		else if (result.outcome == DMBEditor::Outcome::Deleted)
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

QString DMBEditor::createNewDMB()
{
	DMBEditor editor( ui->modListView, pathConvertor, lastUsedDir, ui->modListView->areIconsEnabled(), {} );

	int code = editor.exec();

	if (code != QDialog::Accepted || editor.savedFilePath.isEmpty())  // dialog cancelled or saving the file failed
	{
		return {};
	}

	lastUsedDir = editor.takeLastUsedDir();

	return editor.savedFilePath;
}

QStringList DMBEditor::addExistingDMB()
{
	const QStringList filePaths = DialogWithPaths::selectFiles( this, "Mod Bundle", {},
		makeFileFilter( "Doom Mod Bundles", { dmb::fileSuffix } )
		+ "All files (*)"
	);

	if (filePaths.isEmpty())  // user probably clicked cancel
	{
		return {};
	}

	return filePaths;
}

DMBEditor::Result DMBEditor::editDMB( const QString & filePath )
{
	DMBEditor editor( this, pathConvertor, lastUsedDir, ui->modListView->areIconsEnabled(), filePath );

	int code = editor.exec();

	if (code != QDialog::Accepted)
	{
		return { DMBEditor::Outcome::Cancelled, {} };
	}

	lastUsedDir = editor.takeLastUsedDir();

	return { editor.outcome, std::move( editor.savedFilePath ) };
}


//----------------------------------------------------------------------------------------------------------------------
// dialog finalization

void DMBEditor::onSaveBtnClicked()
{
	bool saved = saveModsToDMB( origFilePath );
	outcome = saved ? Outcome::SavedToExisting : Outcome::Failed;

	SuperClass::accept();  // regardless if the save was successful, close the dialog
}

void DMBEditor::onSaveAsBtnClicked()
{
	QString destFilePath = DialogWithPaths::selectDestFile( this, "Save the Mod Bundle", lastUsedDir,
		makeFileFilter( "Doom Mod Bundles", { dmb::fileSuffix } )
		+ "All files (*)"
	);
	if (destFilePath.isEmpty())
	{
		return;  // user clicked cancel, return back to the dialog
	}

	bool saved = saveModsToDMB( destFilePath );
	outcome = saved ? Outcome::SavedAsNew : Outcome::Failed;

	SuperClass::accept();  // regardless if the save was successful, close the dialog
}

void DMBEditor::onDeleteBtnClicked()
{
	bool fileDeleted = fs::deleteFile( origFilePath );
	if (fileDeleted)
	{
		outcome = Outcome::Deleted;
	}
	else
	{
		reportRuntimeError( "Cannot delete Mod Bundle",
			"Failed to delete the current Mod Bundle \""%origFilePath%"\""
		);
		outcome = Outcome::Failed;
	}

	SuperClass::accept();  // regardless if the save was successful, close the dialog
}
