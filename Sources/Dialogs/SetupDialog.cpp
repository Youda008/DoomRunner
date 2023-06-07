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
#include "DoomFileInfo.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/MiscUtils.hpp"  // makeFileFilter, highlightPathIfInvalid

#include <QString>
#include <QStringBuilder>
#include <QDir>
#include <QFileInfo>
#include <QAction>
#include <QTimer>


//======================================================================================================================
//  SetupDialog

SetupDialog::SetupDialog(
	QWidget * parent,
	const QDir & baseDir,
	const QList< Engine > & engineList,
	const QList< IWAD > & iwadList, const IwadSettings & iwadSettings,
	const MapSettings & mapSettings, const ModSettings & modSettings,
	const LauncherSettings & settings
)
:
	QDialog( parent ),
	DialogWithBrowseDir(
		this, PathContext( baseDir, settings.pathStyle )
	),
	engineModel( engineList,
		/*makeDisplayString*/ []( const Engine & engine ) -> QString { return engine.name % "   [" % engine.path % "]"; }
	),
	iwadModel( iwadList,
		/*makeDisplayString*/ []( const IWAD & iwad ) -> QString { return iwad.name % "   [" % iwad.path % "]"; }
	),
	iwadSettings( iwadSettings ),
	mapSettings( mapSettings ),
	modSettings( modSettings ),
	settings( settings )
{
	ui = new Ui::SetupDialog;
	ui->setupUi( this );

	lastUsedDir = iwadSettings.dir;

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
	ui->absolutePathsChkBox->setChecked( settings.pathStyle == PathStyle::Absolute );
	ui->showEngineOutputChkBox->setChecked( settings.showEngineOutput );
	ui->closeOnLaunchChkBox->setChecked( settings.closeOnLaunch );

	ui->styleCmbBox->addItem( "System default" );
	ui->styleCmbBox->addItems( themes::getAvailableAppStyles() );
	if (!settings.appStyle.isNull())
	{
		int idx = ui->styleCmbBox->findText( settings.appStyle );
		ui->styleCmbBox->setCurrentIndex( idx > 0 ? idx : 0 );
	}

	switch (settings.colorScheme)
	{
		case ColorScheme::Dark:  ui->schemeBtn_dark->click(); break;
		case ColorScheme::Light: ui->schemeBtn_light->click(); break;
		default:                 ui->schemeBtn_system->click(); break;
	}

	// mark invalid paths
	highlightDirPathIfInvalid( ui->iwadDirLine, iwadSettings.dir );
	highlightDirPathIfInvalid( ui->mapDirLine, mapSettings.dir );
	highlightDirPathIfInvalid( ui->modDirLine, modSettings.dir );

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

	connect( ui->styleCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectAppStyle );
	connect( ui->schemeBtn_system, &QRadioButton::clicked, this, &thisClass::setDefaultScheme );
	connect( ui->schemeBtn_dark, &QRadioButton::clicked, this, &thisClass::setDarkScheme );
	connect( ui->schemeBtn_light, &QRadioButton::clicked, this, &thisClass::setLightScheme );

	connect( ui->showEngineOutputChkBox, &QCheckBox::toggled, this, &thisClass::toggleShowEngineOutput );
	connect( ui->closeOnLaunchChkBox, &QCheckBox::toggled, this, &thisClass::toggleCloseOnLaunch );

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

	// setup editing
	engineModel.toggleEditing( false );
	ui->engineListView->toggleNameEditing( false );
	ui->engineListView->toggleListModifications( true );

	// set drag&drop behaviour
	ui->engineListView->toggleIntraWidgetDragAndDrop( true );
	ui->engineListView->toggleInterWidgetDragAndDrop( false );
	ui->engineListView->toggleExternalFileDragAndDrop( true );

	// set reaction to a double-click on an item
	connect( ui->engineListView, &QListView::doubleClicked, this, &thisClass::editEngine );

	// setup enter key detection and reaction
	ui->engineListView->installEventFilter( &engineConfirmationFilter );
	connect( &engineConfirmationFilter, &ConfirmationFilter::choiceConfirmed, this, &thisClass::editSelectedEngine );

	// setup reaction to key shortcuts and right click
	ui->engineListView->toggleContextMenu( true );
	ui->engineListView->enableOpenFileLocation();
	connect( ui->engineListView->addItemAction, &QAction::triggered, this, &thisClass::engineAdd );
	connect( ui->engineListView->deleteItemAction, &QAction::triggered, this, &thisClass::engineDelete );
	connect( ui->engineListView->moveItemUpAction, &QAction::triggered, this, &thisClass::engineMoveUp );
	connect( ui->engineListView->moveItemDownAction, &QAction::triggered, this, &thisClass::engineMoveDown );
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
	ui->iwadListView->toggleListModifications( !iwadSettings.updateFromDir );

	// set drag&drop behaviour
	ui->iwadListView->toggleIntraWidgetDragAndDrop( !iwadSettings.updateFromDir );
	ui->iwadListView->toggleInterWidgetDragAndDrop( false );
	ui->iwadListView->toggleExternalFileDragAndDrop( !iwadSettings.updateFromDir );

	// setup reaction to key shortcuts and right click
	ui->iwadListView->toggleContextMenu( true );
	ui->iwadListView->enableOpenFileLocation();
	connect( ui->iwadListView->addItemAction, &QAction::triggered, this, &thisClass::iwadAdd );
	connect( ui->iwadListView->deleteItemAction, &QAction::triggered, this, &thisClass::iwadDelete );
	connect( ui->iwadListView->moveItemUpAction, &QAction::triggered, this, &thisClass::iwadMoveUp );
	connect( ui->iwadListView->moveItemDownAction, &QAction::triggered, this, &thisClass::iwadMoveDown );
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
//  engines

void SetupDialog::engineAdd()
{
	EngineDialog dialog( this, pathContext, {} );

	int code = dialog.exec();

	if (code == QDialog::Accepted)
	{
		appendItem( ui->engineListView, engineModel, dialog.engine );
	}
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

void SetupDialog::editSelectedEngine()
{
	int selectedEngineIdx = getSelectedItemIndex( ui->engineListView );
	if (selectedEngineIdx >= 0)
	{
		editEngine( engineModel.makeIndex( selectedEngineIdx ) );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  IWADs

void SetupDialog::iwadAdd()
{
	QString path = OwnFileDialog::getOpenFileName( this, "Locate the IWAD", lastUsedDir,
		  makeFileFilter( "Doom data files", iwadSuffixes )
		+ makeFileFilter( "DukeNukem data files", dukeSuffixes )
		+ "All files (*)"
	);
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
		path = pathContext.getRelativePath( path );

	// next time use this dir as the starting dir of the file dialog for convenience
	lastUsedDir = getDirOfFile( path );

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

	ui->iwadListView->toggleListModifications( !enabled );

	// populate the list
	if (iwadSettings.updateFromDir && isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is empty
		updateIWADsFromDir();
}

void SetupDialog::manageIWADsManually()
{
	toggleAutoIWADUpdate( false );
}

void SetupDialog::manageIWADsAutomatically()
{
	toggleAutoIWADUpdate( true );
}

void SetupDialog::toggleIWADSubdirs( bool checked )
{
	iwadSettings.searchSubdirs = checked;

	if (iwadSettings.updateFromDir && isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is empty
		updateIWADsFromDir();
}


//----------------------------------------------------------------------------------------------------------------------
//  game file directories

void SetupDialog::browseIWADDir()
{
	browseDir( this, "with IWADs", ui->iwadDirLine );
}

void SetupDialog::browseMapDir()
{
	browseDir( this, "with maps", ui->mapDirLine );
}

void SetupDialog::browseModDir()
{
	browseDir( this, "with mods", ui->modDirLine );
}

void SetupDialog::changeIWADDir( const QString & dir )
{
	iwadSettings.dir = dir;

	highlightDirPathIfInvalid( ui->iwadDirLine, dir );

	if (iwadSettings.updateFromDir && isValidDir( iwadSettings.dir ))
		updateIWADsFromDir();
}

void SetupDialog::changeMapDir( const QString & dir )
{
	mapSettings.dir = dir;

	highlightDirPathIfInvalid( ui->mapDirLine, dir );
}

void SetupDialog::changeModDir( const QString & dir )
{
	modSettings.dir = dir;

	highlightDirPathIfInvalid( ui->modDirLine, dir );
}

void SetupDialog::updateIWADsFromDir()
{
	updateListFromDir( iwadModel, ui->iwadListView, iwadSettings.dir, iwadSettings.searchSubdirs, pathContext, isIWAD );
}


//----------------------------------------------------------------------------------------------------------------------
//  theme options

void SetupDialog::selectAppStyle( int index )
{
	if (index == 0)
		settings.appStyle.clear();
	else
		settings.appStyle = ui->styleCmbBox->itemText( index );

	themes::setAppStyle( settings.appStyle );
}

void SetupDialog::setDefaultScheme()
{
	settings.colorScheme = ColorScheme::SystemDefault;

	themes::setAppColorScheme( settings.colorScheme );
}

void SetupDialog::setDarkScheme()
{
	settings.colorScheme = ColorScheme::Dark;

	themes::setAppColorScheme( settings.colorScheme );

	// The default Windows 10 style doesn't respect the dark colors.
	if (settings.appStyle.isNull())
	{
		ui->styleCmbBox->setCurrentText("Fusion");
	}
}

void SetupDialog::setLightScheme()
{
	settings.colorScheme = ColorScheme::Light;

	themes::setAppColorScheme( settings.colorScheme );
}


//----------------------------------------------------------------------------------------------------------------------
//  other

void SetupDialog::toggleAbsolutePaths( bool checked )
{
	settings.pathStyle = checked ? PathStyle::Absolute : PathStyle::Relative;

	pathContext.setPathStyle( settings.pathStyle );

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

void SetupDialog::toggleShowEngineOutput( bool checked )
{
	settings.showEngineOutput = checked;

	if (checked && settings.closeOnLaunch)
	{
		// both options cannot be enabled, that would make no sense
		ui->closeOnLaunchChkBox->setChecked( false );
	}
}

void SetupDialog::toggleCloseOnLaunch( bool checked )
{
	settings.closeOnLaunch = checked;

	if (checked && settings.showEngineOutput)
	{
		// both options cannot be enabled, that would make no sense
		ui->showEngineOutputChkBox->setChecked( false );
	}
}
