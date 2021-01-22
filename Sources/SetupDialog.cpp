//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#include "SetupDialog.hpp"
#include "ui_SetupDialog.h"

#include "EngineDialog.hpp"

#include "EventFilters.hpp"  // ConfirmationFilter
#include "WidgetUtils.hpp"
#include "FileSystemUtils.hpp"  // PathContext
#include "DoomUtils.hpp"

#include <QString>
#include <QStringBuilder>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QAction>


//======================================================================================================================
//  local helpers

static bool isValidDir( const QString & dirPath )
{
	return !dirPath.isEmpty() && QDir( dirPath ).exists();
}


//======================================================================================================================
//  SetupDialog

SetupDialog::SetupDialog( QWidget * parent, bool useAbsolutePaths, const QDir & baseDir,
                          const QList< Engine > & engineList,
                          const QList< IWAD > & iwadList, const IwadSettings & iwadSettings,
                          const MapSettings & mapSettings, const ModSettings & modSettings,
                          OptionsStorage optsStorage,
                          bool closeOnLaunch )
:
	QDialog( parent ),
	pathContext( useAbsolutePaths, baseDir ),
	engineModel( engineList,
		/*makeDisplayString*/ []( const Engine & engine ) -> QString { return engine.name % "  [" % engine.path % "]"; }
	),
	iwadModel( iwadList,
		/*makeDisplayString*/ []( const IWAD & iwad ) -> QString { return iwad.name % "  [" % iwad.path % "]"; }
	),
	iwadSettings( iwadSettings ),
	mapSettings( mapSettings ),
	modSettings( modSettings ),
	optsStorage( optsStorage ),
	closeOnLaunch( closeOnLaunch )
{
	ui = new Ui::SetupDialog;
	ui->setupUi( this );

	// setup list view

	setupEngineView();
	setupIWADView();

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
	ui->absolutePathsChkBox->setChecked( pathContext.useAbsolutePaths() );
	if (optsStorage == DONT_STORE)
		ui->optsStorage_none->click();
	else if (optsStorage == STORE_GLOBALLY)
		ui->optsStorage_global->click();
	else if (optsStorage == STORE_TO_PRESET)
		ui->optsStorage_preset->click();
	ui->closeOnLaunchChkBox->setChecked( closeOnLaunch );

	// setup signals

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

	connect( ui->doneBtn, &QPushButton::clicked, this, &thisClass::accept );

	// setup an update timer
	//startTimer( 1000 );
}

void SetupDialog::setupEngineView()
{
	// give the model our path convertor, it will need it for converting paths dropped from directory
	engineModel.setPathContext( &pathContext );

	// set data source for the view
	ui->engineListView->setModel( &engineModel );

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
	connect( ui->engineListView->addAction, &QAction::triggered, this, &thisClass::engineAdd );
	connect( ui->engineListView->deleteAction, &QAction::triggered, this, &thisClass::engineDelete );
	connect( ui->engineListView->moveUpAction, &QAction::triggered, this, &thisClass::engineMoveUp );
	connect( ui->engineListView->moveDownAction, &QAction::triggered, this, &thisClass::engineMoveDown );
}

void SetupDialog::setupIWADView()
{
	// give the model our path convertor, it will need it for converting paths dropped from directory
	iwadModel.setPathContext( &pathContext );

	// specify where to get the string for edit mode
	iwadModel.setEditStringFunc( []( IWAD & iwad ) -> QString & { return iwad.name; } );

	// set data source for the view
	ui->iwadListView->setModel( &iwadModel );

	// set drag&drop behaviour
	ui->iwadListView->toggleNameEditing( !iwadSettings.updateFromDir );
	ui->iwadListView->toggleIntraWidgetDragAndDrop( !iwadSettings.updateFromDir );
	ui->iwadListView->toggleInterWidgetDragAndDrop( false );
	ui->iwadListView->toggleExternalFileDragAndDrop( !iwadSettings.updateFromDir );

	// setup reaction to key shortcuts and right click
	ui->iwadListView->toggleContextMenu( iwadSettings.updateFromDir );
	connect( ui->iwadListView->addAction, &QAction::triggered, this, &thisClass::engineAdd );
	connect( ui->iwadListView->deleteAction, &QAction::triggered, this, &thisClass::engineDelete );
	connect( ui->iwadListView->moveUpAction, &QAction::triggered, this, &thisClass::engineMoveUp );
	connect( ui->iwadListView->moveDownAction, &QAction::triggered, this, &thisClass::engineMoveDown );
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
		if (iwadSettings.updateFromDir && isValidDir( iwadSettings.dir ))
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
	QString path = QFileDialog::getExistingDirectory( this, "Locate the directory with "+dirPurpose, targetLine->text() );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.useRelativePaths())
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
	QString path = QFileDialog::getOpenFileName( this, "Locate the IWAD", QString(),
	                                             "Doom data files (*.wad *.WAD *.iwad *.IWAD *.pk3 *.PK3 *.ipk3 *.IPK3 *.pk7 *.PK7 *.ipk7 *.IPK7);;"
	                                             "DukeNukem data files (*.grp *.rff);;"
	                                             "All files (*)" );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.useRelativePaths())
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
	int selectedEngineIdx = getSelectedItemIdx( ui->engineListView );
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
	pathContext.toggleAbsolutePaths( checked );

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
	optsStorage = DONT_STORE;
}

void SetupDialog::optsStorage_global()
{
	optsStorage = STORE_GLOBALLY;
}

void SetupDialog::optsStorage_preset()
{
	optsStorage = STORE_TO_PRESET;
}

void SetupDialog::toggleCloseOnLaunch( bool checked )
{
	closeOnLaunch = checked;
}

void SetupDialog::closeDialog()
{
	reject();
}
