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
#include "DoomFiles.hpp"
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
	const PathConvertor & pathConv,
	const EngineSettings & engineSettings, const QList< EngineInfo > & engineList,
	const IwadSettings & iwadSettings, const QList< IWAD > & iwadList,
	const MapSettings & mapSettings, const ModSettings & modSettings,
	const LauncherSettings & settings
)
:
	QDialog( parent ),
	DialogWithPaths( this, pathConv ),
	engineSettings( engineSettings ),
	engineModel( engineList,
		/*makeDisplayString*/ []( const Engine & engine ) -> QString { return engine.name % "   [" % engine.executablePath % "]"; }
	),
	iwadSettings( iwadSettings ),
	iwadModel( iwadList,
		/*makeDisplayString*/ []( const IWAD & iwad ) -> QString { return iwad.name % "   [" % iwad.path % "]"; }
	),
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

	connect( ui->iwadDirLine, &QLineEdit::textChanged, this, &thisClass::onIWADDirChanged );
	connect( ui->mapDirLine, &QLineEdit::textChanged, this, &thisClass::onMapDirChanged );
	connect( ui->modDirLine, &QLineEdit::textChanged, this, &thisClass::onModDirChanged );

	connect( ui->iwadSubdirs, &QCheckBox::toggled, this, &thisClass::onIWADSubdirsToggled );

	connect( ui->iwadBtnAdd, &QPushButton::clicked, this, &thisClass::iwadAdd );
	connect( ui->iwadBtnDel, &QPushButton::clicked, this, &thisClass::iwadDelete );
	connect( ui->iwadBtnUp, &QPushButton::clicked, this, &thisClass::iwadMoveUp );
	connect( ui->iwadBtnDown, &QPushButton::clicked, this, &thisClass::iwadMoveDown );

	connect( ui->engineBtnAdd, &QPushButton::clicked, this, &thisClass::engineAdd );
	connect( ui->engineBtnDel, &QPushButton::clicked, this, &thisClass::engineDelete );
	connect( ui->engineBtnUp, &QPushButton::clicked, this, &thisClass::engineMoveUp );
	connect( ui->engineBtnDown, &QPushButton::clicked, this, &thisClass::engineMoveDown );

	connect( ui->absolutePathsChkBox, &QCheckBox::toggled, this, &thisClass::onAbsolutePathsToggled );

	connect( ui->styleCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onAppStyleSelected );
	connect( ui->schemeBtn_system, &QRadioButton::clicked, this, &thisClass::onDefaultSchemeChosen );
	connect( ui->schemeBtn_dark, &QRadioButton::clicked, this, &thisClass::onDarkSchemeChosen );
	connect( ui->schemeBtn_light, &QRadioButton::clicked, this, &thisClass::onLightSchemeChosen );

	connect( ui->showEngineOutputChkBox, &QCheckBox::toggled, this, &thisClass::onShowEngineOutputToggled );
	connect( ui->closeOnLaunchChkBox, &QCheckBox::toggled, this, &thisClass::onCloseOnLaunchToggled );

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
	engineModel.setPathContext( &pathConvertor );

	// setup editing
	engineModel.toggleEditing( false );
	ui->engineListView->toggleNameEditing( false );
	ui->engineListView->toggleListModifications( true );

	// set drag&drop behaviour
	ui->engineListView->toggleIntraWidgetDragAndDrop( true );
	ui->engineListView->toggleInterWidgetDragAndDrop( false );
	ui->engineListView->toggleExternalFileDragAndDrop( true );

	// set reaction to clicks inside the view
	connect( ui->engineListView, &QListView::doubleClicked, this, &thisClass::editEngine );
	connect( ui->engineListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::onEngineSelectionChanged );

	// setup enter key detection and reaction
	ui->engineListView->installEventFilter( &engineConfirmationFilter );
	connect( &engineConfirmationFilter, &ConfirmationFilter::choiceConfirmed, this, &thisClass::editSelectedEngine );

	// setup reaction to key shortcuts and right click
	ui->engineListView->toggleContextMenu( true );
	setDefaultEngineAction = ui->engineListView->addAction( "Set as default", {} );
	ui->engineListView->enableOpenFileLocation();
	connect( ui->engineListView->addItemAction, &QAction::triggered, this, &thisClass::engineAdd );
	connect( ui->engineListView->deleteItemAction, &QAction::triggered, this, &thisClass::engineDelete );
	connect( ui->engineListView->moveItemUpAction, &QAction::triggered, this, &thisClass::engineMoveUp );
	connect( ui->engineListView->moveItemDownAction, &QAction::triggered, this, &thisClass::engineMoveDown );
	connect( setDefaultEngineAction, &QAction::triggered, this, &thisClass::setEngineAsDefault );
}

void SetupDialog::setupIWADList()
{
	// connect the view with model
	ui->iwadListView->setModel( &iwadModel );

	// set selection rules
	ui->iwadListView->setSelectionMode( QAbstractItemView::SingleSelection );

	// give the model our path convertor, it will need it for converting paths dropped from directory
	iwadModel.setPathContext( &pathConvertor );

	// setup editing
	iwadModel.toggleEditing( !iwadSettings.updateFromDir );
	ui->iwadListView->toggleNameEditing( !iwadSettings.updateFromDir );
	ui->iwadListView->toggleListModifications( !iwadSettings.updateFromDir );

	// set drag&drop behaviour
	ui->iwadListView->toggleIntraWidgetDragAndDrop( !iwadSettings.updateFromDir );
	ui->iwadListView->toggleInterWidgetDragAndDrop( false );
	ui->iwadListView->toggleExternalFileDragAndDrop( !iwadSettings.updateFromDir );

	// set reaction to clicks inside the view
	connect( ui->iwadListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::onIWADSelectionChanged );

	// setup reaction to key shortcuts and right click
	ui->iwadListView->toggleContextMenu( true );
	setDefaultIWADAction = ui->iwadListView->addAction( "Set as default", {} );
	ui->iwadListView->enableOpenFileLocation();
	connect( ui->iwadListView->addItemAction, &QAction::triggered, this, &thisClass::iwadAdd );
	connect( ui->iwadListView->deleteItemAction, &QAction::triggered, this, &thisClass::iwadDelete );
	connect( ui->iwadListView->moveItemUpAction, &QAction::triggered, this, &thisClass::iwadMoveUp );
	connect( ui->iwadListView->moveItemDownAction, &QAction::triggered, this, &thisClass::iwadMoveDown );
	connect( setDefaultIWADAction, &QAction::triggered, this, &thisClass::setIWADAsDefault );
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
		if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))  // the second prevents clearing the list when the path is invalid
			updateIWADsFromDir();
	}
}

SetupDialog::~SetupDialog()
{
	delete ui;
}


//----------------------------------------------------------------------------------------------------------------------
//  local utils

template< typename ListModel >
void setItemAsDefault( QListView * view, ListModel & model, QAction * setDefaultAction, QString & defaultItemID )
{
	int selectedIdx = wdg::getSelectedItemIndex( view );
	if (selectedIdx < 0)
	{
		QMessageBox::warning( view->parentWidget(), "No item selected", "No item is selected." );
		return;
	}

	auto & selectedItem = model[ selectedIdx ];

	QString prevDefaultItemID = std::move( defaultItemID );
	defaultItemID = selectedItem.getID();

	// unmark the previous default entry
	int prevIdx = findSuch( model, [&]( const auto & item ){ return item.getID() == prevDefaultItemID; } );
	if (prevIdx >= 0)
		model[ prevIdx ].textColor = themes::getCurrentPalette().color( QPalette::Text );

	if (defaultItemID != prevDefaultItemID)
	{
		// mark the new default entry
		selectedItem.textColor = themes::getCurrentPalette().defaultEntryText;
		setDefaultAction->setText( "Unset as default" );
	}
	else  // already marked, clear the default status
	{
		defaultItemID.clear();
		setDefaultAction->setText( "Set as default" );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  engines

void SetupDialog::engineAdd()
{
	EngineDialog dialog( this, pathConvertor, {} );

	int code = dialog.exec();

	if (code == QDialog::Accepted)
	{
		wdg::appendItem( ui->engineListView, engineModel, dialog.engine );
	}
}

void SetupDialog::engineDelete()
{
	int defaultIdx = findSuch( engineModel, [&]( const Engine & e ){ return e.getID() == engineSettings.defaultEngine; } );

	int deletedIdx = wdg::deleteSelectedItem( ui->engineListView, engineModel );

	if (deletedIdx == defaultIdx)
		engineSettings.defaultEngine.clear();
}

void SetupDialog::engineMoveUp()
{
	wdg::moveUpSelectedItem( ui->engineListView, engineModel );
}

void SetupDialog::engineMoveDown()
{
	wdg::moveDownSelectedItem( ui->engineListView, engineModel );
}

void SetupDialog::onEngineSelectionChanged( const QItemSelection &, const QItemSelection & )
{
	int selectedIdx = wdg::getSelectedItemIndex( ui->engineListView );
	setDefaultEngineAction->setEnabled( selectedIdx >= 0 );  // only allow this action if something is selected
	if (selectedIdx >= 0)
	{
		// allow unsetting as default
		bool isDefaultItem = engineModel[ selectedIdx ].getID() == engineSettings.defaultEngine;
		setDefaultEngineAction->setText( !isDefaultItem ? "Set as default" : "Unset as default" );
	}
}

void SetupDialog::setEngineAsDefault()
{
	setItemAsDefault( ui->engineListView, engineModel, setDefaultEngineAction, engineSettings.defaultEngine );
}

void SetupDialog::editEngine( const QModelIndex & index )
{
	EngineInfo & selectedEngine = engineModel[ index.row() ];

	EngineDialog dialog( this, pathConvertor, selectedEngine );

	int code = dialog.exec();

	if (code == QDialog::Accepted)
	{
		selectedEngine = dialog.engine;
	}
}

void SetupDialog::editSelectedEngine()
{
	int selectedEngineIdx = wdg::getSelectedItemIndex( ui->engineListView );
	if (selectedEngineIdx >= 0)
	{
		editEngine( engineModel.makeIndex( selectedEngineIdx ) );
	}
}


//----------------------------------------------------------------------------------------------------------------------
//  IWADs

void SetupDialog::iwadAdd()
{
	QString path = DialogWithPaths::browseFile( this, "IWAD", lastUsedDir,
		  makeFileFilter( "Doom data files", doom::iwadSuffixes )
		+ makeFileFilter( "DukeNukem data files", doom::dukeSuffixes )
		+ "All files (*)"
	);
	if (path.isEmpty())  // user probably clicked cancel
		return;

	wdg::appendItem( ui->iwadListView, iwadModel, { QFileInfo( path ) } );
}

void SetupDialog::iwadDelete()
{
	int defaultIdx = findSuch( iwadModel, [&]( const IWAD & i ){ return i.getID() == iwadSettings.defaultIWAD; } );

	int deletedIdx = wdg::deleteSelectedItem( ui->iwadListView, iwadModel );

	if (deletedIdx == defaultIdx)
		iwadSettings.defaultIWAD.clear();
}

void SetupDialog::iwadMoveUp()
{
	wdg::moveUpSelectedItem( ui->iwadListView, iwadModel );
}

void SetupDialog::iwadMoveDown()
{
	wdg::moveDownSelectedItem( ui->iwadListView, iwadModel );
}

void SetupDialog::onIWADSelectionChanged( const QItemSelection &, const QItemSelection & )
{
	int selectedIdx = wdg::getSelectedItemIndex( ui->iwadListView );
	setDefaultIWADAction->setEnabled( selectedIdx >= 0 );  // only allow this action if something is selected
	if (selectedIdx >= 0)
	{
		// allow unsetting as default
		bool isDefaultItem = iwadModel[ selectedIdx ].getID() == iwadSettings.defaultIWAD;
		setDefaultIWADAction->setText( !isDefaultItem ? "Set as default" : "Unset as default" );
	}
}

void SetupDialog::setIWADAsDefault()
{
	setItemAsDefault( ui->iwadListView, iwadModel, setDefaultIWADAction, iwadSettings.defaultIWAD );
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
	if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is empty
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

void SetupDialog::onIWADSubdirsToggled( bool checked )
{
	iwadSettings.searchSubdirs = checked;

	if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is empty
		updateIWADsFromDir();
}


//----------------------------------------------------------------------------------------------------------------------
//  game file directories

void SetupDialog::browseIWADDir()
{
	DialogWithPaths::browseDir( this, "with IWADs", ui->iwadDirLine );
}

void SetupDialog::browseMapDir()
{
	DialogWithPaths::browseDir( this, "with maps", ui->mapDirLine );
}

void SetupDialog::browseModDir()
{
	DialogWithPaths::browseDir( this, "with mods", ui->modDirLine );
}

void SetupDialog::onIWADDirChanged( const QString & dir )
{
	iwadSettings.dir = dir;

	highlightDirPathIfInvalid( ui->iwadDirLine, dir );

	if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))
		updateIWADsFromDir();
}

void SetupDialog::onMapDirChanged( const QString & dir )
{
	mapSettings.dir = dir;

	highlightDirPathIfInvalid( ui->mapDirLine, dir );
}

void SetupDialog::onModDirChanged( const QString & dir )
{
	modSettings.dir = dir;

	highlightDirPathIfInvalid( ui->modDirLine, dir );
}

void SetupDialog::updateIWADsFromDir()
{
	wdg::updateListFromDir( iwadModel, ui->iwadListView, iwadSettings.dir, iwadSettings.searchSubdirs, pathConvertor, doom::isIWAD );
}


//----------------------------------------------------------------------------------------------------------------------
//  theme options

void SetupDialog::onAppStyleSelected( int index )
{
	if (index == 0)
		settings.appStyle.clear();
	else
		settings.appStyle = ui->styleCmbBox->itemText( index );

	themes::setAppStyle( settings.appStyle );
}

void SetupDialog::onDefaultSchemeChosen()
{
	settings.colorScheme = ColorScheme::SystemDefault;

	themes::setAppColorScheme( settings.colorScheme );
}

void SetupDialog::onDarkSchemeChosen()
{
	settings.colorScheme = ColorScheme::Dark;

	themes::setAppColorScheme( settings.colorScheme );

	// The default Windows 10 style doesn't respect the dark colors.
	if (settings.appStyle.isNull())
	{
		ui->styleCmbBox->setCurrentText("Fusion");
	}
}

void SetupDialog::onLightSchemeChosen()
{
	settings.colorScheme = ColorScheme::Light;

	themes::setAppColorScheme( settings.colorScheme );
}


//----------------------------------------------------------------------------------------------------------------------
//  other

void SetupDialog::onAbsolutePathsToggled( bool checked )
{
	settings.pathStyle = checked ? PathStyle::Absolute : PathStyle::Relative;
	pathConvertor.setPathStyle( settings.pathStyle );

	for (Engine & engine : engineModel)
	{
		engine.executablePath = pathConvertor.convertPath( engine.executablePath );
		engine.configDir = pathConvertor.convertPath( engine.configDir );
	}
	engineModel.contentChanged( 0 );

	iwadSettings.dir = pathConvertor.convertPath( iwadSettings.dir );
	ui->iwadDirLine->setText( iwadSettings.dir );
	for (IWAD & iwad : iwadModel)
	{
		iwad.path = pathConvertor.convertPath( iwad.path );
	}
	iwadModel.contentChanged( 0 );

	mapSettings.dir = pathConvertor.convertPath( mapSettings.dir );
	ui->mapDirLine->setText( mapSettings.dir );

	modSettings.dir = pathConvertor.convertPath( modSettings.dir );
	ui->modDirLine->setText( modSettings.dir );
}

void SetupDialog::onShowEngineOutputToggled( bool checked )
{
	settings.showEngineOutput = checked;

	if (checked && settings.closeOnLaunch)
	{
		// both options cannot be enabled, that would make no sense
		ui->closeOnLaunchChkBox->setChecked( false );
	}
}

void SetupDialog::onCloseOnLaunchToggled( bool checked )
{
	settings.closeOnLaunch = checked;

	if (checked && settings.showEngineOutput)
	{
		// both options cannot be enabled, that would make no sense
		ui->showEngineOutputChkBox->setChecked( false );
	}
}
