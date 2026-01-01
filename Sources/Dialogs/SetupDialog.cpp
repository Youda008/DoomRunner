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
#include "Utils/StringUtils.hpp"  // emptyString
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
	DialogWithPaths( this, u"SetupDialog", pathConv, iwadSettings.dir ),
	engineSettings( engineSettings ),
	engineModel( u"engineModel", engineList,
		/*makeDisplayString*/ []( const Engine & engine ) -> QString { return engine.name % "   [" % engine.executablePath % "]"; }
	),
	iwadSettings( iwadSettings ),
	iwadModel( u"iwadModel", iwadList,
		/*makeDisplayString*/ []( const IWAD & iwad ) -> QString { return iwad.name % "   [" % iwad.path % "]"; }
	),
	mapSettings( mapSettings ),
	modSettings( modSettings ),
	settings( settings ),
	appearance( appearance )
{
	ui = new Ui::SetupDialog;
	ui->setupUi( this );

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

	connect( ui->manageIWADs_manual, &QRadioButton::clicked, this, &ThisClass::onManageIWADsManuallySelected );
	connect( ui->manageIWADs_auto, &QRadioButton::clicked, this, &ThisClass::onManageIWADsAutomaticallySelected );

	connect( ui->iwadDirBtn, &QPushButton::clicked, this, &ThisClass::selectIWADDir );
	connect( ui->mapDirBtn, &QPushButton::clicked, this, &ThisClass::selectMapDir );

	connect( ui->iwadDirLine, &QLineEdit::textChanged, this, &ThisClass::onIWADDirChanged );
	connect( ui->mapDirLine, &QLineEdit::textChanged, this, &ThisClass::onMapDirChanged );

	connect( ui->iwadSubdirs, &QCheckBox::toggled, this, &ThisClass::onIWADSubdirsToggled );

	connect( ui->absolutePathsChkBox, &QCheckBox::toggled, this, &ThisClass::onAbsolutePathsToggled );

	connect( ui->styleCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &ThisClass::onAppStyleSelected );
	connect( ui->schemeBtn_system, &QRadioButton::clicked, this, &ThisClass::onDefaultSchemeChosen );
	connect( ui->schemeBtn_dark, &QRadioButton::clicked, this, &ThisClass::onDarkSchemeChosen );
	connect( ui->schemeBtn_light, &QRadioButton::clicked, this, &ThisClass::onLightSchemeChosen );

	connect( ui->showEngineOutputChkBox, &QCheckBox::toggled, this, &ThisClass::onShowEngineOutputToggled );
	connect( ui->closeOnLaunchChkBox, &QCheckBox::toggled, this, &ThisClass::onCloseOnLaunchToggled );

	connect( ui->doneBtn, &QPushButton::clicked, this, &ThisClass::accept );

	// setup an update timer
	startTimer( 1000 );
}

void SetupDialog::setupEngineList()
{
	// connect the view with the model
	ui->engineListView->setModel( &engineModel );

	// set selection rules
	ui->engineListView->setSelectionMode( QAbstractItemView::SingleSelection );

	// set drag&drop behaviour
	engineModel.setPathConvertor( pathConvertor );  // the model needs our path convertor for converting paths dropped from a file explorer
	ui->engineListView->setAllowedDnDSources( DnDSource::ThisWidget | DnDSource::ExternalApp );
	connect( &engineModel, &AListModel::itemsInserted, this, &ThisClass::onEnginesInserted );

	// set reaction to clicks inside the view
	connect( ui->engineListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ThisClass::onEngineSelectionChanged );
	connect( ui->engineListView, &QListView::doubleClicked, this, &ThisClass::onEngineDoubleClicked );

	// setup enter key detection and reaction
	ui->engineListView->installEventFilter( &engineConfirmationFilter );
	connect( &engineConfirmationFilter, &ConfirmationFilter::choiceConfirmed, this, &ThisClass::onEngineConfirmed );

	// setup reaction to key shortcuts and right click
	ui->engineListView->enableContextMenu();
	ui->engineListView->addStandardMenuActions( ExtendedListView::MenuAction::OpenFileLocation );
	ui->engineListView->addMenuSeparator();
	ui->engineListView->addStandardMenuActions( ExtendedListView::MenuAction::AddAndDelete );
	ui->engineListView->addMenuSeparator();
	ui->engineListView->addStandardMenuActions( ExtendedListView::MenuAction::CutCopyPaste );
	ui->engineListView->addMenuSeparator();
	ui->engineListView->addStandardMenuActions( ExtendedListView::MenuAction::Move );
	ui->engineListView->addMenuSeparator();
	setDefaultEngineAction = ui->engineListView->addCustomMenuAction( "Set as default", {} );

	ui->engineListView->toggleListModifications( true );

	connect( ui->engineListView->addItemAction, &QAction::triggered, this, &ThisClass::engineAdd );
	connect( ui->engineListView->deleteItemAction, &QAction::triggered, this, &ThisClass::engineDelete );
	connect( ui->engineListView->moveItemUpAction, &QAction::triggered, this, &ThisClass::engineMoveUp );
	connect( ui->engineListView->moveItemDownAction, &QAction::triggered, this, &ThisClass::engineMoveDown );
	connect( ui->engineListView->moveItemToTopAction, &QAction::triggered, this, &ThisClass::engineMoveToTop );
	connect( ui->engineListView->moveItemToBottomAction, &QAction::triggered, this, &ThisClass::engineMoveToBottom );
	connect( setDefaultEngineAction, &QAction::triggered, this, &ThisClass::setEngineAsDefault );

	// setup buttons
	connect( ui->engineBtnAdd, &QPushButton::clicked, this, &ThisClass::engineAdd );
	connect( ui->engineBtnDel, &QPushButton::clicked, this, &ThisClass::engineDelete );
	connect( ui->engineBtnUp, &QPushButton::clicked, this, &ThisClass::engineMoveUp );
	connect( ui->engineBtnDown, &QPushButton::clicked, this, &ThisClass::engineMoveDown );
}

void SetupDialog::setupIWADList()
{
	// connect the view with the model
	ui->iwadListView->setModel( &iwadModel );

	// set selection rules
	ui->iwadListView->setSelectionMode( QAbstractItemView::ExtendedSelection );

	// setup editing
	ui->iwadListView->toggleItemEditing( !iwadSettings.updateFromDir );

	// set drag&drop behaviour
	iwadModel.setPathConvertor( pathConvertor );  // the model needs our path convertor for converting paths dropped from a file explorer
	if (!iwadSettings.updateFromDir)
		ui->iwadListView->setAllowedDnDSources( DnDSource::ThisWidget | DnDSource::ExternalApp );

	// set reaction to clicks inside the view
	connect( ui->iwadListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ThisClass::onIWADSelectionChanged );

	// setup reaction to key shortcuts and right click
	ui->iwadListView->enableContextMenu();
	ui->iwadListView->addStandardMenuActions( ExtendedListView::MenuAction::OpenFileLocation );
	ui->iwadListView->addMenuSeparator();
	ui->iwadListView->addStandardMenuActions( ExtendedListView::MenuAction::AddAndDelete );
	ui->iwadListView->addMenuSeparator();
	ui->iwadListView->addStandardMenuActions( ExtendedListView::MenuAction::Move );
	ui->iwadListView->addMenuSeparator();
	setDefaultIWADAction = ui->iwadListView->addCustomMenuAction( "Set as default", {} );

	ui->iwadListView->toggleListModifications( !iwadSettings.updateFromDir );

	connect( ui->iwadListView->addItemAction, &QAction::triggered, this, &ThisClass::iwadAdd );
	connect( ui->iwadListView->deleteItemAction, &QAction::triggered, this, &ThisClass::iwadDelete );
	connect( ui->iwadListView->moveItemUpAction, &QAction::triggered, this, &ThisClass::iwadMoveUp );
	connect( ui->iwadListView->moveItemDownAction, &QAction::triggered, this, &ThisClass::iwadMoveDown );
	connect( ui->iwadListView->moveItemToTopAction, &QAction::triggered, this, &ThisClass::iwadMoveToTop );
	connect( ui->iwadListView->moveItemToBottomAction, &QAction::triggered, this, &ThisClass::iwadMoveToBottom );
	connect( setDefaultIWADAction, &QAction::triggered, this, &ThisClass::setIWADAsDefault );

	// setup buttons
	connect( ui->iwadBtnAdd, &QPushButton::clicked, this, &ThisClass::iwadAdd );
	connect( ui->iwadBtnDel, &QPushButton::clicked, this, &ThisClass::iwadDelete );
	connect( ui->iwadBtnUp, &QPushButton::clicked, this, &ThisClass::iwadMoveUp );
	connect( ui->iwadBtnDown, &QPushButton::clicked, this, &ThisClass::iwadMoveDown );
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
		if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is invalid
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
	EngineDialog dialog( this, pathConvertor, {}, std::move(lastUsedDir) );

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

	const auto removedIndexes = wdg::removeSelectedItems( ui->engineListView, engineModel );

	if (removedIndexes.contains( defaultIndex ))
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

void SetupDialog::onEnginesInserted( int row, int count )
{
	// Engine (or more of them) got copy&pasted or dragged&dropped into this list.
	for (int engineIdx = row; engineIdx < row + count; ++engineIdx)
	{
		EngineInfo & engine = engineModel[ engineIdx ];

		// If it was copy&pasted or dragged&dropped from this list, it already contains everything.
		// But if it was dragged&dropped from a file explorer, we have to deduce everything automatically.
		if (!engine.isInitialized())
		{
			// the executablePath is already converted to the right path style by the ListModel
			EngineDialog::autofillEngineInfo( engine, engine.executablePath );
		}
	}
}

void SetupDialog::onEngineDoubleClicked( const QModelIndex & index )
{
	editEngine( index.row() );
}

void SetupDialog::onEngineConfirmed()
{
	if (int selectedIdx = wdg::getSelectedItemIndex( ui->engineListView ); selectedIdx >= 0)
	{
		editEngine( selectedIdx );
	}
}

void SetupDialog::editEngine( int engineIdx )
{
	EngineInfo & engine = engineModel[ engineIdx ];

	EngineDialog dialog( this, pathConvertor, engine, std::move(lastUsedDir) );

	int code = dialog.exec();

	lastUsedDir = dialog.takeLastUsedDir();

	if (code == QDialog::Accepted)
	{
		engineModel.startEditingItemData();
		engine = dialog.engine;
		engineModel.finishEditingItemData( engineIdx, 1, AListModel::allDataRoles );
	}
}

void SetupDialog::onEngineSelectionChanged( const QItemSelection &, const QItemSelection & )
{
	// Optimization: Don't update when the list is not in its final state and is going to change right away.
	if (ui->engineListView->isDragAndDropInProgress())
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
	QString path = DialogWithPaths::selectFile( this, "IWAD", emptyString,
		  makeFileDialogFilter( "Doom data files", doom::getIWADSuffixes() )
		+ "All files (*)"
	);
	if (path.isEmpty())  // user probably clicked cancel
		return;

	wdg::appendItem( ui->iwadListView, iwadModel, IWAD( path ) );
}

void SetupDialog::iwadDelete()
{
	int defaultIndex = findSuch( iwadModel, [&]( const IWAD & i ){ return i.getID() == iwadSettings.defaultIWAD; } );

	const auto removedIndexes = wdg::removeSelectedItems( ui->iwadListView, iwadModel );

	if (removedIndexes.contains( defaultIndex ))
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
	// Optimization: Don't update when the list is not in its final state and is going to change right away.
	if (ui->engineListView->isDragAndDropInProgress())
		return;

	const IWAD * currentIWAD = wdg::getCurrentItem( ui->iwadListView, iwadModel );
	setDefaultIWADAction->setEnabled( currentIWAD != nullptr );  // only allow this action if something is selected
	if (currentIWAD)
	{
		// allow unsetting as default
		bool isDefaultItem = currentIWAD->getID() == iwadSettings.defaultIWAD;
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

	ui->iwadListView->toggleItemEditing( !enabled );
	ui->iwadListView->toggleListModifications( !enabled );

	ui->iwadListView->setAllowedDnDSources(
		!enabled ? (DnDSource::ThisWidget | DnDSource::ExternalApp) : DnDSource::None
	);

	// populate the list
	if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is invalid
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

	if (iwadSettings.updateFromDir && fs::isValidDir( iwadSettings.dir ))  // don't clear the current items when the dir line is invalid
		updateIWADsFromDir();
}


//----------------------------------------------------------------------------------------------------------------------
// game file directories

void SetupDialog::selectIWADDir()
{
	DialogWithPaths::selectDir( this, "with IWADs", ui->iwadDirLine );
}

void SetupDialog::selectMapDir()
{
	DialogWithPaths::selectDir( this, "with maps", ui->mapDirLine );
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

	engineModel.startEditingItemData();
	for (Engine & engine : engineModel)
	{
		engine.executablePath = pathConvertor.convertPath( engine.executablePath );
		// don't convert the config/data dirs, some of them may be better stored as relative, some as absolute
	}
	engineModel.finishEditingItemData( 0, -1, AListModel::onlyDisplayRole );

	iwadSettings.dir = pathConvertor.convertPath( iwadSettings.dir );
	ui->iwadDirLine->setText( iwadSettings.dir );
	engineModel.startEditingItemData();
	for (IWAD & iwad : iwadModel)
	{
		iwad.path = pathConvertor.convertPath( iwad.path );
	}
	engineModel.finishEditingItemData( 0, -1, AListModel::onlyDisplayRole );

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
