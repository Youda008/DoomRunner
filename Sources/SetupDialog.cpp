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
#include "SharedData.hpp"
#include "WidgetUtils.hpp"

#include <QString>
#include <QStringBuilder>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFileDialog>


//======================================================================================================================
//  SetupDialog

SetupDialog::SetupDialog( QWidget * parent, bool useAbsolutePaths, const QDir & baseDir, const QList< Engine > & engines,
                          const QList< IWAD > & iwads, bool iwadListFromDir, const QString & iwadDir, bool iwadSubdirs,
                          const QString & mapDir, const QString & modDir )
	: QDialog( parent )
	, pathHelper( useAbsolutePaths, baseDir )
	, engineModel( engines,
		/*makeDisplayString*/[]( const Engine & engine ) -> QString { return engine.name % "  [" % engine.path % "]"; }
	  )
	, iwadModel( iwads,
		/*makeDisplayString*/[]( const IWAD & iwad ) -> QString { return iwad.name % "  [" % iwad.path % "]"; }
	  )
	, iwadListFromDir( iwadListFromDir )
	, iwadDir( iwadDir )
	, iwadSubdirs( iwadSubdirs )
	, mapDir( mapDir )
	, modDir( modDir )
{
	ui = new Ui::SetupDialog;
	ui->setupUi(this);

	// setup engine view
	ui->engineListView->setModel( &engineModel );
	engineModel.setPathHelper( &pathHelper );
	ui->engineListView->toggleNameEditing( false );
	ui->engineListView->toggleIntraWidgetDragAndDrop( true );
	ui->engineListView->toggleInterWidgetDragAndDrop( false );
	ui->engineListView->toggleExternalFileDragAndDrop( true );
	connect( ui->engineListView, &EditableListView::itemsDropped, this, &thisClass::enginesDropped );

	// setup iwad view
	ui->iwadListView->setModel( &iwadModel );
	iwadModel.setPathHelper( &pathHelper );
	ui->iwadListView->toggleNameEditing( false );
	ui->iwadListView->toggleIntraWidgetDragAndDrop( !iwadListFromDir );
	ui->iwadListView->toggleInterWidgetDragAndDrop( false );
	ui->iwadListView->toggleExternalFileDragAndDrop( !iwadListFromDir );
	connect( ui->iwadListView, &EditableListView::itemsDropped, this, &thisClass::iwadsDropped );

	// initialize widget data
	if (iwadListFromDir) {
		ui->manageIWADs_auto->click();
		manageIWADsAutomatically();
	}
	ui->iwadDirLine->setText( iwadDir );
	ui->iwadSubdirs->setChecked( iwadSubdirs );
	ui->mapDirLine->setText( mapDir );
	ui->modDirLine->setText( modDir );
	ui->absolutePathsChkBox->setChecked( pathHelper.useAbsolutePaths() );

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

	connect( ui->engineListView, &QListView::doubleClicked, this, &thisClass::editEngine );

	connect( ui->absolutePathsChkBox, &QCheckBox::toggled, this, &thisClass::toggleAbsolutePaths );

	connect( ui->doneBtn, &QPushButton::clicked, this, &thisClass::accept );

	// setup an update timer
	//startTimer( 1000 );
}

void SetupDialog::timerEvent( QTimerEvent * event )  // called once per second
{
	QDialog::timerEvent( event );

	tickCount++;
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
	iwadListFromDir = enabled;

	ui->iwadDirLine->setEnabled( enabled );
	ui->iwadDirBtn->setEnabled( enabled );
	ui->iwadSubdirs->setEnabled( enabled );
	ui->iwadBtnAdd->setEnabled( !enabled );
	ui->iwadBtnDel->setEnabled( !enabled );
	ui->iwadBtnUp->setEnabled( !enabled );
	ui->iwadBtnDown->setEnabled( !enabled );

	ui->iwadListView->toggleIntraWidgetDragAndDrop( !enabled );
	ui->iwadListView->toggleExternalFileDragAndDrop( !enabled );

	if (enabled && QDir( iwadDir ).exists())
		updateIWADsFromDir();
}

void SetupDialog::toggleIWADSubdirs( bool checked )
{
	iwadSubdirs = checked;

	if (iwadListFromDir && QDir( iwadDir ).exists())
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
	if (pathHelper.useRelativePaths())
		path = pathHelper.getRelativePath( path );

	targetLine->setText( path );
	// the rest of the actions will be performed in the line edit callback,
	// because we want to do the same things when user edits the path manually
}

void SetupDialog::changeIWADDir( const QString & text )
{
	iwadDir = text;

	if (iwadListFromDir && QDir( iwadDir ).exists())
		updateIWADsFromDir();
}

void SetupDialog::changeMapDir( const QString & text )
{
	mapDir = text;
}

void SetupDialog::changeModDir( const QString & text )
{
	modDir = text;
}

void SetupDialog::iwadAdd()
{
	QString path = QFileDialog::getOpenFileName( this, "Locate the IWAD", QString(),
	                                             "Doom mod files (*.wad *.WAD *.iwad *.IWAD *.pk3 *.PK3 *.ipk3 *.IPK3 *.pk7 *.PK7 *.ipk7 *.IPK7);;"
	                                             "All files (*)" );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathHelper.useRelativePaths())
		path = pathHelper.getRelativePath( path );

	appendItem( iwadModel, { QFileInfo( path ) } );
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
	EngineDialog dialog( this, pathHelper, {} );

	int code = dialog.exec();

	if (code == QDialog::Accepted)
		appendItem( engineModel, dialog.engine );
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

	EngineDialog dialog( this, pathHelper, selectedEngine );

	int code = dialog.exec();

	if (code == QDialog::Accepted) {
		selectedEngine = dialog.engine;
	}
}

void SetupDialog::enginesDropped()
{

}

void SetupDialog::iwadsDropped()
{

}

void SetupDialog::updateIWADsFromDir()
{
	updateListFromDir< IWAD >( iwadModel, ui->iwadListView, iwadDir, iwadSubdirs, pathHelper, isIWAD );
}

void SetupDialog::toggleAbsolutePaths( bool checked )
{
	pathHelper.toggleAbsolutePaths( checked );

	for (Engine & engine : engineModel)
		engine.path = pathHelper.convertPath( engine.path );
	engineModel.contentChanged( 0 );

	if (iwadListFromDir && !iwadDir.isEmpty())
		iwadDir = pathHelper.convertPath( iwadDir );
	ui->iwadDirLine->setText( iwadDir );
	for (IWAD & iwad : iwadModel)
		iwad.path = pathHelper.convertPath( iwad.path );
	iwadModel.contentChanged( 0 );

	mapDir = pathHelper.convertPath( mapDir );
	ui->mapDirLine->setText( mapDir );

	modDir = pathHelper.convertPath( modDir );
	ui->modDirLine->setText( modDir );
}

void SetupDialog::closeDialog()
{
	reject();
}
