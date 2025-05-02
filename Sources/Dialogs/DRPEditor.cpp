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

	// setup and populate path line
	setPathValidator( ui->destFileLine );
	if (!origFilePath.isEmpty())
		ui->destFileLine->setText( origFilePath );
	else
		ui->destFileLine->setText( fs::getPathFromFileName( lastUsedDir, "NewPack.drp" ) );

	// setup and populate mod list
	setupModList( showIcons );
	if (!origFilePath.isEmpty())
		loadModsFromDRP( origFilePath );

	// show this editor above the parent widget, slightly to the right
	QPoint globalPos = parentWidget->mapToGlobal( QPoint( 40, 0 ) );
	this->move( globalPos.x(), globalPos.y() );

	connect( ui->saveBtn, &QPushButton::clicked, this, &ThisClass::accept );
}

DRPEditor::~DRPEditor()
{
	delete ui;
}


//----------------------------------------------------------------------------------------------------------------------
// mod list manipulation

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
		| ExtendedListView::MenuAction::ToggleIcons
	);
	addExistingDRP = ui->modListView->addAction( "Add existing DR pack", {} );
	createNewDRP = ui->modListView->addAction( "Create new DR pack", {} );
	ui->modListView->toggleListModifications( true );
	connect( ui->modListView->addItemAction, &QAction::triggered, this, &ThisClass::modAdd );
	connect( ui->modListView->deleteItemAction, &QAction::triggered, this, &ThisClass::modDelete );
	connect( ui->modListView->moveItemUpAction, &QAction::triggered, this, &ThisClass::modMoveUp );
	connect( ui->modListView->moveItemDownAction, &QAction::triggered, this, &ThisClass::modMoveDown );
	connect( ui->modListView->moveItemToTopAction, &QAction::triggered, this, &ThisClass::modMoveToTop );
	connect( ui->modListView->moveItemToBottomAction, &QAction::triggered, this, &ThisClass::modMoveToBottom );
	connect( addExistingDRP, &QAction::triggered, this, &ThisClass::modAddExistingDRP );
	connect( createNewDRP, &QAction::triggered, this, &ThisClass::modCreateNewDRP );

	// setup icons (must be set called after enableContextMenu, because it requires toggleIconsAction)
	ui->modListView->toggleIcons( showIcons );  // we need to do this instead of model.toggleIcons() in order to update the action text

	// setup buttons
	connect( ui->destFileBtn, &QPushButton::clicked, this, &ThisClass::selectDestinationFile );
	connect( ui->modBtnAdd, &QToolButton::clicked, this, &ThisClass::modAdd );
	connect( ui->modBtnAddDir, &QToolButton::clicked, this, &ThisClass::modAddDir );
	connect( ui->modBtnDel, &QToolButton::clicked, this, &ThisClass::modDelete );
	connect( ui->modBtnUp, &QToolButton::clicked, this, &ThisClass::modMoveUp );
	connect( ui->modBtnDown, &QToolButton::clicked, this, &ThisClass::modMoveDown );
}

void DRPEditor::selectDestinationFile()
{
	DialogWithPaths::selectDestFile( this, "Save DoomRunner Pack", ui->destFileLine, drp::fileSuffix );
}

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

void DRPEditor::modAddExistingDRP()
{
	const QStringList paths = DialogWithPaths::selectFiles( this, "DoomRunner Pack", {},
		makeFileFilter( "DoomRunner Pack files", { drp::fileSuffix } )
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
	DRPEditor editor( ui->modListView, pathConvertor, lastUsedDir, ui->modListView->areIconsEnabled(), {} );

	int code = editor.exec();

	// update the data only if user clicked Ok and the save was successful
	if (code != QDialog::Accepted || editor.savedFilePath.isEmpty())
	{
		return;
	}

	lastUsedDir = editor.takeLastUsedDir();
	ui->modListView->toggleIcons( editor.areIconsEnabled() );

	Mod mod( QFileInfo( editor.savedFilePath ) );

	wdg::appendItem( ui->modListView, modModel, mod );
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
		DRPEditor editor( this, pathConvertor, lastUsedDir, ui->modListView->areIconsEnabled(), fileInfo.filePath() );

		int code = editor.exec();

		// update the data only if user clicked Ok
		if (code == QDialog::Accepted)
		{
			lastUsedDir = editor.takeLastUsedDir();
			ui->modListView->toggleIcons( editor.areIconsEnabled() );
		}
	}
	else
	{
		showTxtDescriptionFor( this, fileInfo.filePath(), "mod description" );
	}
}


//----------------------------------------------------------------------------------------------------------------------
// mod list loading and saving

void DRPEditor::loadModsFromDRP( const QString & filePath )
{
	QFileInfo fileInfo( filePath );

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

	return drp::saveEntries( filePath, std::move(entries) );
}

void DRPEditor::accept()
{
	// verify requirements
	QString destFilePath = sanitizeInputPath( ui->destFileLine->text() );
	if (destFilePath.isEmpty())
	{
		reportUserError( "Destination file path is empty",
			"Please specify where to save this DoomRunner Pack."
		);
		return;  // refuse the user's confirmation
	}
	else if (fs::isValidDir( destFilePath ))
	{
		reportUserError( "Destination file is invalid",
			"The destination path is a directory. Please choose another name."
		);
		return;  // refuse the user's confirmation
	}

	// save the mod list content to the file
	if (!origFilePath.isEmpty() && destFilePath != origFilePath)  // the DRP already exists, but new path has been requested
	{
		bool written = saveModsToDRP( origFilePath );
		if (written)
		{
			savedFilePath = origFilePath;

			bool renamedOrMoved = fs::renameOrMoveFile( origFilePath, destFilePath );
			if (renamedOrMoved)
			{
				savedFilePath = destFilePath;
			}
			else
			{
				reportRuntimeError( "Failed to relocate file",
					"Couldn't rename of move the file \""%origFilePath%"\" to \""%destFilePath%"\"."
				);
			}
		}
	}
	else  // this is a new DRP
	{
		bool written = saveModsToDRP( destFilePath );
		if (written)
		{
			savedFilePath = destFilePath;
		}
	}

	// accept the user's confirmation
	SuperClass::accept();
}

bool DRPEditor::areIconsEnabled() const
{
	return ui->modListView->areIconsEnabled();
}
