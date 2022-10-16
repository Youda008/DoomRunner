//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Initial Setup dialog
//======================================================================================================================

#include "SetupDialog.hpp"
#include "ui_SetupDialog.h"

#include "EngineDialog.hpp"

#include "OwnFileDialog.hpp"
#include "EventFilters.hpp"  // ConfirmationFilter
#include "WidgetUtils.hpp"
#include "FileSystemUtils.hpp"  // PathContext
#include "DoomUtils.hpp"

#include <QString>
#include <QStringBuilder>
#include <QDir>
#include <QFileInfo>
#include <QAction>


//======================================================================================================================
//  SetupDialog

SetupDialog::SetupDialog(
	QWidget * parent,
	const QDir & baseDir,
	const QList< Engine > & engineList,
	const QList< IWAD > & iwadList, const IwadSettings & iwadSettings,
	const MapSettings & mapSettings, const ModSettings & modSettings,
	const LauncherOptions & opts
)
:
	QDialog( parent ),
	pathContext( baseDir, opts.useAbsolutePaths ),
	engineModel( engineList,
		/*makeDisplayString*/ []( const Engine & engine ) -> QString { return engine.name % "  [" % engine.path % "]"; }
	),
	iwadModel( iwadList,
		/*makeDisplayString*/ []( const IWAD & iwad ) -> QString { return iwad.name % "  [" % iwad.path % "]"; }
	),
	iwadSettings( iwadSettings ),
	mapSettings( mapSettings ),
	modSettings( modSettings ),
	opts( opts )
{
	ui = new Ui::SetupDialog;
	ui->setupUi( this );

	// setup list views

	setupEngineList();
	setupIWADList();

	// initialize widget data

	if (iwadSettings.updateFromDir)
	{
		ui->manageIWADs_auto->click();
		manageIWADsAutomatically();
	}
	ui->iwadDirLine->setText( iwadSettings.dir );
	ui->iwadSubdirs->setChecked( iwadSettings.searchSubdirs );
	ui->mapDirLine->setText( mapSettings.dir );
	ui->modDirLine->setText( modSettings.dir );
	ui->absolutePathsChkBox->setChecked( pathContext.usingAbsolutePaths() );
	if (opts.launchOptsStorage == DontStore)
		ui->optsStorage_none->click();
	else if (opts.launchOptsStorage == StoreGlobally)
		ui->optsStorage_global->click();
	else if (opts.launchOptsStorage == StoreToPreset)
		ui->optsStorage_preset->click();
	ui->closeOnLaunchChkBox->setChecked( opts.closeOnLaunch );
	ui->showEngineOutputChkBox->setChecked( opts.showEngineOutput );

	// setup buttons

	connect( ui->manageIWADs_manual, &QRadioButton::clicked, this, &thisClass::manageIWADsManually );
	connect( ui->manageIWADs_auto, &QRadioButton::clicked, this, &thisClass::manageIWADsAutomatically );

	connect( ui->iwadDirBtn, &QPushButton::clicked, this, &thisClass::browseIWADDir );
	connect( ui->mapDirBtn, &QPushButton::clicked, this, &thisClass::browseMapDir );
	connect( ui->modDirBtn, &QPushButton::clicked, this, &thisClass::browseModDir );

	connect( ui->iwadDirLine, &QLineEdit::textChanged, this, &thisClass::changeIWADDir );
	connect( ui->mapDirLine, &QLineEdit::textChanged, this, &thisClass::changeMapDir );
	connect( ui->modDirLine, &QLineEdit::textChanged, this, &thisClass::changeModDir );

	connect( ui->iwadSubdirs, &QCheckBox::toggled, this, &thisClass::toggleIWADSubdirs );

	connect( ui->iwadBtnAdd, &QPushButton::clicked, this, &thisClass::iwadAdd );
	connect( ui->iwadBtnDel, &QPushButton::clicked, this, &thisClass::iwadDelete );
	connect( ui->iwadBtnUp, &QPushButton::clicked, this, &thisClass::iwadMoveUp );
	connect( ui->iwadBtnDown, &QPushButton::clicked, this, &thisClass::iwadMoveDown );

	connect( ui->engineBtnAdd, &QPushButton::clicked, this, &thisClass::engineAdd );
	connect( ui->engineBtnDel, &QPushButton::clicked, this, &thisClass::engineDelete );
	connect( ui->engineBtnUp, &QPushButton::clicked, this, &thisClass::engineMoveUp );
	connect( ui->engineBtnDown, &QPushButton::clicked, this, &thisClass::engineMoveDown );

	connect( ui->absolutePathsChkBox, &QCheckBox::toggled, this, &thisClass::toggleAbsolutePaths );

	connect( ui->optsStorage_none, &QRadioButton::clicked, this, &thisClass::optsStorage_none );
	connect( ui->optsStorage_global, &QRadioButton::clicked, this, &thisClass::optsStorage_global );
	connect( ui->optsStorage_preset, &QRadioButton::clicked, this, &thisClass::optsStorage_preset );

	connect( ui->closeOnLaunchChkBox, &QCheckBox::toggled, this, &thisClass::toggleCloseOnLaunch );
	connect( ui->showEngineOutputChkBox, &QCheckBox::toggled, this, &thisClass::toggleShowEngineOutput );

	connect( ui->doneBtn, &QPushButton::clicked, this, &thisClass::accept );

	// setup an update timer
	startTimer( 1000 );
}

void SetupDialog::setupEngineList()
{
	// connect the view with model
	ui->engineListView->setModel( &engineModel );

	// set selection rules
	ui->engineListView->setSelectionMode( QAbstractItemView::SingleSelection );

	// give the model our path convertor, it will need it for converting paths dropped from directory
	engineModel.setPathContext( &pathContext );

	// set drag&drop behaviour
	ui->engineListView->toggleNameEditing( false );
	ui->engineListView->toggleIntraWidgetDragAndDrop( true );
	ui->engineListView->toggleInterWidgetDragAndDrop( false );
	ui->engineListView->toggleExternalFileDragAndDrop( true );

	// set reaction to a double-click on an item
	connect( ui->engineListView, &QListView::doubleClicked, this, &thisClass::editEngine );

	// setup enter key detection and reaction
	ui->engineListView->installEventFilter( &engineConfirmationFilter );
	connect( &engineConfirmationFilter, &ConfirmationFilter::choiceConfirmed, this, &thisClass::editCurrentEngine );

	// setup reaction to key shortcuts and right click
	ui->engineListView->toggleContextMenu( true );
	ui->engineListView->enableOpenFileLocation();
	connect( ui->engineListView->addAction, &QAction::triggered, this, &thisClass::engineAdd );
	connect( ui->engineListView->deleteAction, &QAction::triggered, this, &thisClass::engineDelete );
	connect( ui->engineListView->moveUpAction, &QAction::triggered, this, &thisClass::engineMoveUp );
	connect( ui->engineListView->moveDownAction, &QAction::triggered, this, &thisClass::engineMoveDown );
}

void SetupDialog::setupIWADList()
{
	// connect the view with model
	ui->iwadListView->setModel( &iwadModel );

	// set selection rules
	ui->iwadListView->setSelectionMode( QAbstractItemView::SingleSelection );

	// give the model our path convertor, it will need it for converting paths dropped from directory
	iwadModel.setPathContext( &pathContext );

	// setup editing
	iwadModel.toggleEditing( !iwadSettings.updateFromDir );
	ui->iwadListView->toggleNameEditing( !iwadSettings.updateFromDir );

	// set drag&drop behaviour
	ui->iwadListView->toggleIntraWidgetDragAndDrop( !iwadSettings.updateFromDir );
	ui->iwadListView->toggleInterWidgetDragAndDrop( false );
	ui->iwadListView->toggleExternalFileDragAndDrop( !iwadSettings.updateFromDir );

	// setup reaction to key shortcuts and right click
	ui->iwadListView->toggleContextMenu( iwadSettings.updateFromDir );
	ui->iwadListView->enableOpenFileLocation();
	connect( ui->iwadListView->addAction, &QAction::triggered, this, &thisClass::iwadAdd );
	connect( ui->iwadListView->deleteAction, &QAction::triggered, this, &thisClass::iwadDelete );
	connect( ui->iwadListView->moveUpAction, &QAction::triggered, this, &thisClass::iwadMoveUp );
	connect( ui->iwadListView->moveDownAction, &QAction::triggered, this, &thisClass::iwadMoveDown );
}

void SetupDialog::timerEvent( QTimerEvent * event )  // called once per second
{
	QDialog::timerEvent( event );

	tickCount++;

 #ifdef QT_DEBUG
	constexpr uint dirUpdateDelay = 8;
 #else
	constexpr uint dirUpdateDelay = 2;
 #endif

	if (tickCount % dirUpdateDelay == 0)
	{
		if (iwadSettings.updateFromDir && isValidDir( iwadSettings.dir ))  // the second prevents clearing the list when the path is invalid
			updateIWADsFromDir();
	}
}

SetupDialog::~SetupDialog()
{
	delete ui;
}


//----------------------------------------------------------------------------------------------------------------------
//  slots

void SetupDialog::manageIWADsManually()
{
	toggleAutoIWADUpdate( false );
}

void SetupDialog::manageIWADsAutomatically()
{
	toggleAutoIWADUpdate( true );
}

void SetupDialog::toggleAutoIWADUpdate( bool enabled )
{
	iwadSettings.updateFromDir = enabled;

	// activate/deactivate the corresponding widgets

	ui->iwadDirLine->setEnabled( enabled );
	ui->iwadDirBtn->setEnabled( enabled );
	ui->iwadSubdirs->setEnabled( enabled );
	ui->iwadBtnAdd->setEnabled( !enabled );
	ui->iwadBtnDel->setEnabled( !enabled );
	ui->iwadBtnUp->setEnabled( !enabled );
	ui->iwadBtnDown->setEnabled( !enabled );

	ui->iwadListView->toggleIntraWidgetDragAndDrop( !enabled );
	ui->iwadListView->toggleExternalFileDragAndDrop( !enabled );

	ui->iwadListView->toggleContextMenu( !enabled );

	// populate the list
	if (iwadSettings.updateFromDir && isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is empty
		updateIWADsFromDir();
}

void SetupDialog::toggleIWADSubdirs( bool checked )
{
	iwadSettings.searchSubdirs = checked;

	if (iwadSettings.updateFromDir && isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is empty
		updateIWADsFromDir();
}


void SetupDialog::browseIWADDir()
{
	browseDir( "IWADs", ui->iwadDirLine );
}

void SetupDialog::browseMapDir()
{
	browseDir( "maps", ui->mapDirLine );
}

void SetupDialog::browseModDir()
{
	browseDir( "mods", ui->modDirLine );
}

void SetupDialog::browseDir( const QString & dirPurpose, QLineEdit * targetLine )
{
	QString path = OwnFileDialog::getExistingDirectory( this, "Locate the directory with "+dirPurpose, targetLine->text() );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
		path = pathContext.getRelativePath( path );

	targetLine->setText( path );
	// the rest of the actions will be performed in the line edit callback,
	// because we want to do the same things when user edits the path manually
}

void SetupDialog::changeIWADDir( const QString & text )
{
	iwadSettings.dir = text;

	if (iwadSettings.updateFromDir && isValidDir( iwadSettings.dir ))
		updateIWADsFromDir();
}

void SetupDialog::changeMapDir( const QString & text )
{
	mapSettings.dir = text;
}

void SetupDialog::changeModDir( const QString & text )
{
	modSettings.dir = text;
}

void SetupDialog::iwadAdd()
{
	QString path = OwnFileDialog::getOpenFileName( this, "Locate the IWAD", QString(),
		  makeFileFilter( "Doom data files", iwadSuffixes )
		+ makeFileFilter( "DukeNukem data files", dukeSuffixes )
		+ "All files (*)"
	);
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
		path = pathContext.getRelativePath( path );

	appendItem( ui->iwadListView, iwadModel, { QFileInfo( path ) } );
}

void SetupDialog::iwadDelete()
{
	deleteSelectedItem( ui->iwadListView, iwadModel );
}

void SetupDialog::iwadMoveUp()
{
	moveUpSelectedItem( ui->iwadListView, iwadModel );
}

void SetupDialog::iwadMoveDown()
{
	moveDownSelectedItem( ui->iwadListView, iwadModel );
}

void SetupDialog::engineAdd()
{
	EngineDialog dialog( this, pathContext, {} );

	int code = dialog.exec();

	if (code == QDialog::Accepted)
		appendItem( ui->iwadListView, engineModel, dialog.engine );
}

void SetupDialog::engineDelete()
{
	deleteSelectedItem( ui->engineListView, engineModel );
}

void SetupDialog::engineMoveUp()
{
	moveUpSelectedItem( ui->engineListView, engineModel );
}

void SetupDialog::engineMoveDown()
{
	moveDownSelectedItem( ui->engineListView, engineModel );
}

void SetupDialog::editEngine( const QModelIndex & index )
{
	Engine & selectedEngine = engineModel[ index.row() ];

	EngineDialog dialog( this, pathContext, selectedEngine );

	int code = dialog.exec();

	if (code == QDialog::Accepted)
	{
		selectedEngine = dialog.engine;
	}
}

void SetupDialog::editCurrentEngine()
{
	int selectedEngineIdx = getSelectedItemIndex( ui->engineListView );
	if (selectedEngineIdx >= 0)
	{
		editEngine( engineModel.makeIndex( selectedEngineIdx ) );
	}
}

void SetupDialog::updateIWADsFromDir()
{
	updateListFromDir< IWAD >( iwadModel, ui->iwadListView, iwadSettings.dir, iwadSettings.searchSubdirs, pathContext, isIWAD );
}

void SetupDialog::toggleAbsolutePaths( bool checked )
{
	opts.useAbsolutePaths = checked;

	pathContext.toggleAbsolutePaths( opts.useAbsolutePaths );

	for (Engine & engine : engineModel)
	{
		engine.path = pathContext.convertPath( engine.path );
		engine.configDir = pathContext.convertPath( engine.configDir );
	}
	engineModel.contentChanged( 0 );

	iwadSettings.dir = pathContext.convertPath( iwadSettings.dir );
	ui->iwadDirLine->setText( iwadSettings.dir );
	for (IWAD & iwad : iwadModel)
	{
		iwad.path = pathContext.convertPath( iwad.path );
	}
	iwadModel.contentChanged( 0 );

	mapSettings.dir = pathContext.convertPath( mapSettings.dir );
	ui->mapDirLine->setText( mapSettings.dir );

	modSettings.dir = pathContext.convertPath( modSettings.dir );
	ui->modDirLine->setText( modSettings.dir );
}

void SetupDialog::optsStorage_none()
{
	opts.launchOptsStorage = DontStore;
}

void SetupDialog::optsStorage_global()
{
	opts.launchOptsStorage = StoreGlobally;
}

void SetupDialog::optsStorage_preset()
{
	opts.launchOptsStorage = StoreToPreset;
}

void SetupDialog::toggleCloseOnLaunch( bool checked )
{
	opts.closeOnLaunch = checked;

	if (checked && opts.showEngineOutput)
	{
		// both options cannot be enabled, that would make no sense
		ui->showEngineOutputChkBox->setChecked( false );
	}
}

void SetupDialog::toggleShowEngineOutput( bool checked )
{
	opts.showEngineOutput = checked;

	if (checked && opts.closeOnLaunch)
	{
		// both options cannot be enabled, that would make no sense
		ui->closeOnLaunchChkBox->setChecked( false );
	}
}
