//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Initial Setup dialog
//======================================================================================================================

#include "SetupDialog.hpp"
#include "ui_SetupDialog.h"

#include "EngineDialog.hpp"

#include "DoomFiles.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/PathCheckUtils.hpp"  // highlightPathIfInvalid
#include "Utils/MiscUtils.hpp"  // makeFileFilter
#include "Utils/ErrorHandling.hpp"

#include <QString>
#include <QStringBuilder>
#include <QDir>
#include <QFileInfo>
#include <QAction>


//======================================================================================================================
// SetupDialog

SetupDialog::SetupDialog(
	QWidget * parent,
	const PathConvertor & pathConv,
	const EngineSettings & engineSettings, const PtrList< EngineInfo > & engineList,
	const IwadSettings & iwadSettings, const PtrList< IWAD > & iwadList,
	const MapSettings & mapSettings, const ModSettings & modSettings,
	const LauncherSettings & settings,
	const AppearanceSettings & appearance
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
	settings( settings ),
	appearance( appearance )
{
	ui = new Ui::SetupDialog;
	ui->setupUi( this );

	lastUsedDir = iwadSettings.dir;

	// setup input path validators

	setPathValidator( ui->iwadDirLine );
	setPathValidator( ui->mapDirLine );

	// setup list views

	setupEngineList();
	setupIWADList();

	// initialize widget data

	if (iwadSettings.updateFromDir)
	{
		ui->manageIWADs_auto->click();
		onManageIWADsAutomaticallySelected();
	}
	ui->iwadDirLine->setText( iwadSettings.dir );
	ui->iwadSubdirs->setChecked( iwadSettings.searchSubdirs );
	ui->mapDirLine->setText( mapSettings.dir );
	ui->absolutePathsChkBox->setChecked( settings.pathStyle.isAbsolute() );
	ui->showEngineOutputChkBox->setChecked( settings.showEngineOutput );
	ui->closeOnLaunchChkBox->setChecked( settings.closeOnLaunch );

	ui->styleCmbBox->addItem( "System default" );
	ui->styleCmbBox->addItems( themes::getAvailableAppStyles() );
	if (!appearance.appStyle.isNull())
	{
		int idx = ui->styleCmbBox->findText( appearance.appStyle );
		ui->styleCmbBox->setCurrentIndex( idx > 0 ? idx : 0 );
	}

	switch (appearance.colorScheme)
	{
		case ColorScheme::Dark:  ui->schemeBtn_dark->click(); break;
		case ColorScheme::Light: ui->schemeBtn_light->click(); break;
		default:                 ui->schemeBtn_system->click(); break;
	}

	// mark invalid paths
	highlightDirPathIfInvalid( ui->iwadDirLine, iwadSettings.dir );
	highlightDirPathIfInvalid( ui->mapDirLine, mapSettings.dir );

	// setup buttons

	connect( ui->manageIWADs_manual, &QRadioButton::clicked, this, &thisClass::onManageIWADsManuallySelected );
	connect( ui->manageIWADs_auto, &QRadioButton::clicked, this, &thisClass::onManageIWADsAutomaticallySelected );

	connect( ui->iwadDirBtn, &QPushButton::clicked, this, &thisClass::browseIWADDir );
	connect( ui->mapDirBtn, &QPushButton::clicked, this, &thisClass::browseMapDir );

	connect( ui->iwadDirLine, &QLineEdit::textChanged, this, &thisClass::onIWADDirChanged );
	connect( ui->mapDirLine, &QLineEdit::textChanged, this, &thisClass::onMapDirChanged );

	connect( ui->iwadSubdirs, &QCheckBox::toggled, this, &thisClass::onIWADSubdirsToggled );

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
	engineModel.setPathConvertor( &pathConvertor );

	// setup editing
	engineModel.toggleEditing( false );
	ui->engineListView->toggleNameEditing( false );
	ui->engineListView->toggleListModifications( true );

	// set drag&drop behaviour
	ui->engineListView->toggleIntraWidgetDragAndDrop( true );
	ui->engineListView->toggleInterWidgetDragAndDrop( false );
	ui->engineListView->toggleExternalFileDragAndDrop( true );
	connect( ui->engineListView, &EditableListView::itemsDropped, this, &thisClass::onEnginesDropped );

	// set reaction to clicks inside the view
	connect( ui->engineListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &thisClass::onEngineSelectionChanged );
	connect( ui->engineListView, &QListView::doubleClicked, this, &thisClass::onEngineDoubleClicked );

	// setup enter key detection and reaction
	ui->engineListView->installEventFilter( &engineConfirmationFilter );
	connect( &engineConfirmationFilter, &ConfirmationFilter::choiceConfirmed, this, &thisClass::onEngineConfirmed );

	// setup reaction to key shortcuts and right click
	ui->engineListView->toggleContextMenu( true );
	setDefaultEngineAction = ui->engineListView->addAction( "Set as default", {} );
	ui->engineListView->enableOpenFileLocation();
	connect( ui->engineListView->addItemAction, &QAction::triggered, this, &thisClass::engineAdd );
	connect( ui->engineListView->deleteItemAction, &QAction::triggered, this, &thisClass::engineDelete );
	connect( ui->engineListView->moveItemUpAction, &QAction::triggered, this, &thisClass::engineMoveUp );
	connect( ui->engineListView->moveItemDownAction, &QAction::triggered, this, &thisClass::engineMoveDown );
	connect( ui->engineListView->moveItemToTopAction, &QAction::triggered, this, &thisClass::engineMoveToTop );
	connect( ui->engineListView->moveItemToBottomAction, &QAction::triggered, this, &thisClass::engineMoveToBottom );
	connect( setDefaultEngineAction, &QAction::triggered, this, &thisClass::setEngineAsDefault );

	// setup buttons
	connect( ui->engineBtnAdd, &QPushButton::clicked, this, &thisClass::engineAdd );
	connect( ui->engineBtnDel, &QPushButton::clicked, this, &thisClass::engineDelete );
	connect( ui->engineBtnUp, &QPushButton::clicked, this, &thisClass::engineMoveUp );
	connect( ui->engineBtnDown, &QPushButton::clicked, this, &thisClass::engineMoveDown );
}

void SetupDialog::setupIWADList()
{
	// connect the view with model
	ui->iwadListView->setModel( &iwadModel );

	// set selection rules
	ui->iwadListView->setSelectionMode( QAbstractItemView::SingleSelection );

	// give the model our path convertor, it will need it for converting paths dropped from directory
	iwadModel.setPathConvertor( &pathConvertor );

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
	connect( ui->iwadListView->moveItemToTopAction, &QAction::triggered, this, &thisClass::iwadMoveToTop );
	connect( ui->iwadListView->moveItemToBottomAction, &QAction::triggered, this, &thisClass::iwadMoveToBottom );
	connect( setDefaultIWADAction, &QAction::triggered, this, &thisClass::setIWADAsDefault );

	// setup buttons
	connect( ui->iwadBtnAdd, &QPushButton::clicked, this, &thisClass::iwadAdd );
	connect( ui->iwadBtnDel, &QPushButton::clicked, this, &thisClass::iwadDelete );
	connect( ui->iwadBtnUp, &QPushButton::clicked, this, &thisClass::iwadMoveUp );
	connect( ui->iwadBtnDown, &QPushButton::clicked, this, &thisClass::iwadMoveDown );
}

void SetupDialog::timerEvent( QTimerEvent * event )  // called once per second
{
	QDialog::timerEvent( event );

	tickCount++;

 #if IS_DEBUG_BUILD
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
// local utils

template< typename ListModel >
void setSelectedItemAsDefault( QListView * view, ListModel & model, QAction * setDefaultAction, QString & defaultItemID )
{
	auto * selectedItem = wdg::getSelectedItem( view, model );
	if (!selectedItem)
	{
		reportUserError( view->parentWidget(), "No item selected", "No item is selected." );
		return;
	}

	QString prevDefaultItemID = std::move( defaultItemID );
	defaultItemID = selectedItem->getID();

	// unmark the previous default entry
	int prevIdx = findSuch( model, [&]( const auto & item ){ return item.getID() == prevDefaultItemID; } );
	if (prevIdx >= 0)
		unmarkItemAsDefault( model[ prevIdx ] );

	if (defaultItemID != prevDefaultItemID)
	{
		// mark the new default entry
		markItemAsDefault( *selectedItem );
		setDefaultAction->setText( "Unset as default" );
	}
	else  // already marked, clear the default status
	{
		defaultItemID.clear();
		setDefaultAction->setText( "Set as default" );
	}
}


//----------------------------------------------------------------------------------------------------------------------
// engines

void SetupDialog::engineAdd()
{
	EngineDialog dialog( this, pathConvertor, {}, lastUsedDir );

	int code = dialog.exec();

	lastUsedDir = dialog.takeLastUsedDir();

	if (code == QDialog::Accepted)
	{
		wdg::appendItem( ui->engineListView, engineModel, dialog.engine );
	}
}

void SetupDialog::engineDelete()
{
	int defaultIndex = findSuch( engineModel, [&]( const Engine & e ){ return e.getID() == engineSettings.defaultEngine; } );

	const auto deletedIndexes = wdg::deleteSelectedItems( ui->engineListView, engineModel );

	if (!deletedIndexes.isEmpty() && deletedIndexes[0] == defaultIndex)
		engineSettings.defaultEngine.clear();
}

void SetupDialog::engineMoveUp()
{
	wdg::moveSelectedItemsUp( ui->engineListView, engineModel );
}

void SetupDialog::engineMoveDown()
{
	wdg::moveSelectedItemsDown( ui->engineListView, engineModel );
}

void SetupDialog::engineMoveToTop()
{
	wdg::moveSelectedItemsToTop( ui->engineListView, engineModel );
}

void SetupDialog::engineMoveToBottom()
{
	wdg::moveSelectedItemsToBottom( ui->engineListView, engineModel );
}

void SetupDialog::onEnginesDropped( int row, int count, DnDType type )
{
	if (type == DnDType::IntraWidget)  // engines were just moved within the list
		return;

	// Engine (or more of them) got dragged&dropped from other place,
	// in that case we only have the executable path, other things must be filled automatically.
	for (int engineIdx = row; engineIdx < row + count; ++engineIdx)
	{
		EngineInfo & engine = engineModel[ engineIdx ];
		// the executablePath is already converted by the ListModel
		EngineDialog::autofillEngineInfo( engine, engine.executablePath );
	}
}

void SetupDialog::onEngineDoubleClicked( const QModelIndex & index )
{
	editEngine( engineModel[ index.row() ] );
}

void SetupDialog::onEngineConfirmed()
{
	if (EngineInfo * selectedEngine = wdg::getSelectedItem( ui->engineListView, engineModel ))
	{
		editEngine( *selectedEngine );
	}
}

void SetupDialog::editEngine( EngineInfo & selectedEngine )
{
	EngineDialog dialog( this, pathConvertor, selectedEngine, lastUsedDir );

	int code = dialog.exec();

	lastUsedDir = dialog.takeLastUsedDir();

	if (code == QDialog::Accepted)
	{
		selectedEngine = dialog.engine;
	}
}

void SetupDialog::onEngineSelectionChanged( const QItemSelection &, const QItemSelection & )
{
	// For some reason i don't understand Qt sometimes (not always) calls this method
	// when we're shifting items in the ListModel and when there are null pointers in the original item positions.
	// And for some other reason i also don't understand, getSelectedItemIndex() queries all items for flags.
	// Therefore without this check we crash in derefencing null in ListModel::flags().
	if (engineModel.isMovingInProgress())
		return;

	const EngineInfo * selectedEngine = wdg::getSelectedItem( ui->engineListView, engineModel );
	setDefaultEngineAction->setEnabled( selectedEngine != nullptr );  // only allow this action if something is selected
	if (selectedEngine)
	{
		// allow unsetting as default
		bool isDefaultItem = selectedEngine->getID() == engineSettings.defaultEngine;
		setDefaultEngineAction->setText( !isDefaultItem ? "Set as default" : "Unset as default" );
	}
}

void SetupDialog::setEngineAsDefault()
{
	setSelectedItemAsDefault( ui->engineListView, engineModel, setDefaultEngineAction, engineSettings.defaultEngine );
}


//----------------------------------------------------------------------------------------------------------------------
// IWADs

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
	int defaultIndex = findSuch( iwadModel, [&]( const IWAD & i ){ return i.getID() == iwadSettings.defaultIWAD; } );

	const auto deletedIndexes = wdg::deleteSelectedItems( ui->iwadListView, iwadModel );

	if (!deletedIndexes.isEmpty() && deletedIndexes[0] == defaultIndex)
		iwadSettings.defaultIWAD.clear();
}

void SetupDialog::iwadMoveUp()
{
	wdg::moveSelectedItemsUp( ui->iwadListView, iwadModel );
}

void SetupDialog::iwadMoveDown()
{
	wdg::moveSelectedItemsDown( ui->iwadListView, iwadModel );
}

void SetupDialog::iwadMoveToTop()
{
	wdg::moveSelectedItemsToTop( ui->iwadListView, iwadModel );
}

void SetupDialog::iwadMoveToBottom()
{
	wdg::moveSelectedItemsToBottom( ui->iwadListView, iwadModel );
}

void SetupDialog::onIWADSelectionChanged( const QItemSelection &, const QItemSelection & )
{
	// For some reason i don't understand Qt sometimes (not always) calls this method
	// when we're shifting items in the ListModel and when there are null pointers in the original item positions.
	// And for some other reason i also don't understand, getSelectedItemIndex() queries all items for flags.
	// Therefore without this check we crash in derefencing null in ListModel::flags().
	if (iwadModel.isMovingInProgress())
		return;

	const IWAD * selectedIWAD = wdg::getSelectedItem( ui->iwadListView, iwadModel );
	setDefaultIWADAction->setEnabled( selectedIWAD != nullptr );  // only allow this action if something is selected
	if (selectedIWAD)
	{
		// allow unsetting as default
		bool isDefaultItem = selectedIWAD->getID() == iwadSettings.defaultIWAD;
		setDefaultIWADAction->setText( !isDefaultItem ? "Set as default" : "Unset as default" );
	}
}

void SetupDialog::setIWADAsDefault()
{
	setSelectedItemAsDefault( ui->iwadListView, iwadModel, setDefaultIWADAction, iwadSettings.defaultIWAD );
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

	iwadModel.toggleEditing( !enabled );
	ui->iwadListView->toggleNameEditing( !enabled );
	ui->iwadListView->toggleListModifications( !enabled );

	// populate the list
	if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is empty
		updateIWADsFromDir();
}

void SetupDialog::onManageIWADsManuallySelected()
{
	toggleAutoIWADUpdate( false );
}

void SetupDialog::onManageIWADsAutomaticallySelected()
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
// game file directories

void SetupDialog::browseIWADDir()
{
	DialogWithPaths::browseDir( this, "with IWADs", ui->iwadDirLine );
}

void SetupDialog::browseMapDir()
{
	DialogWithPaths::browseDir( this, "with maps", ui->mapDirLine );
}

void SetupDialog::onIWADDirChanged( const QString & dir )
{
	iwadSettings.dir = sanitizeInputPath( dir );

	highlightDirPathIfInvalid( ui->iwadDirLine, iwadSettings.dir );

	if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))
		updateIWADsFromDir();
}

void SetupDialog::onMapDirChanged( const QString & dir )
{
	mapSettings.dir = sanitizeInputPath( dir );

	highlightDirPathIfInvalid( ui->mapDirLine, mapSettings.dir );
}

void SetupDialog::updateIWADsFromDir()
{
	wdg::updateListFromDir( iwadModel, ui->iwadListView, iwadSettings.dir, iwadSettings.searchSubdirs, pathConvertor, doom::canBeIWAD );

	if (!iwadSettings.defaultIWAD.isEmpty())
	{
		// the default item marking was lost during the update, mark it again
		int defaultIdx = findSuch( iwadModel, [&]( const IWAD & i ){ return i.getID() == iwadSettings.defaultIWAD; } );
		if (defaultIdx >= 0)
			markItemAsDefault( iwadModel[ defaultIdx ] );
	}
}


//----------------------------------------------------------------------------------------------------------------------
// theme options

void SetupDialog::onAppStyleSelected( int index )
{
	if (index == 0)
		appearance.appStyle.clear();
	else
		appearance.appStyle = ui->styleCmbBox->itemText( index );

	themes::setAppStyle( appearance.appStyle );
}

void SetupDialog::onDefaultSchemeChosen()
{
	appearance.colorScheme = ColorScheme::SystemDefault;

	themes::setAppColorScheme( appearance.colorScheme );
}

void SetupDialog::onDarkSchemeChosen()
{
	appearance.colorScheme = ColorScheme::Dark;

	themes::setAppColorScheme( appearance.colorScheme );

 #if IS_WINDOWS
	// The default Windows style doesn't work well with dark colors. "Fusion" is the only style where it looks good.
	// So if someone selects default style while dark mode is enabled in the OS settings, redirect to "Fusion".
	if (appearance.appStyle.isNull() || appearance.appStyle == themes::getDefaultAppStyle())
	{
		ui->styleCmbBox->setCurrentIndex( ui->styleCmbBox->findText("Fusion") );
	}
 #endif
}

void SetupDialog::onLightSchemeChosen()
{
	appearance.colorScheme = ColorScheme::Light;

	themes::setAppColorScheme( appearance.colorScheme );
}


//----------------------------------------------------------------------------------------------------------------------
// other

void SetupDialog::onAbsolutePathsToggled( bool checked )
{
	settings.pathStyle.toggleAbsolute( checked );
	pathConvertor.setPathStyle( settings.pathStyle );

	for (Engine & engine : engineModel)
	{
		engine.executablePath = pathConvertor.convertPath( engine.executablePath );
		// don't convert the config/data dirs, some of them may be better stored as relative, some as absolute
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
