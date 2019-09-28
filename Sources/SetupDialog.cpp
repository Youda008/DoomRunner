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
#include "Utils.hpp"

#include <QFileDialog>
#include <QDir>
#include <QDirIterator>


//======================================================================================================================
//  SetupDialog

SetupDialog::SetupDialog( QWidget * parent, PathHelper & pathHelper, QList< Engine > & engines,
	                      QList< IWAD > & iwads, bool & iwadListFromDir, QString & iwadDir, bool & iwadSubdirs,
	                      QString & mapDir, bool & mapSubdirs, QString & modDir )
	: QDialog( parent )
	, pathHelper( pathHelper )
	, engines( engines )
	, engineModel( engines, /*makeDisplayString*/[]( const Engine & engine ) -> QString
	                                             { return engine.name % "  [" % engine.path % "]"; } )
	, iwads( iwads )
	, iwadModel( iwads, /*makeDisplayString*/[]( const IWAD & iwad ) -> QString
	                                         { return iwad.name % "  [" % iwad.path % "]"; } )
	, iwadListFromDir( iwadListFromDir )
	, iwadDir( iwadDir )
	, iwadSubdirs( iwadSubdirs )
	, mapDir( mapDir )
	, mapSubdirs( mapSubdirs )
	, modDir( modDir )
{
	ui = new Ui::SetupDialog;
	ui->setupUi(this);

	// setup view models
	ui->engineListView->setModel( &engineModel );
	ui->iwadListView->setModel( &iwadModel );

	// initialize widget data
	if (iwadListFromDir) {
		ui->manageIWADs_auto->click();
		manageIWADsAutomatically();
	}
	ui->iwadDirLine->setText( iwadDir );
	ui->iwadSubdirs->setChecked( iwadSubdirs );
	ui->mapDirLine->setText( mapDir );
	ui->mapSubdirs->setChecked( mapSubdirs );
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
	connect( ui->mapSubdirs, &QCheckBox::toggled, this, &thisClass::toggleMapSubdirs );

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

	connect( ui->doneBtn, &QPushButton::clicked, this, &thisClass::closeDialog );

	// setup an update timer
	//startTimer( 1000 );
}

void SetupDialog::timerEvent( QTimerEvent * )  // called once per second
{
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

	// In order to not duplicate functionality, we delegate the request to load IWADs from directory to MainWindow,
	// because it has to be able to do it on its own anyway. And we pass there a ref to our list view, because we expect
	// it to refresh the view when the data change is finished, and it might also change the list view's selection.
	emit iwadListNeedsUpdate( ui->iwadListView );
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

void SetupDialog::browseDir( QString dirPurpose, QLineEdit * targetLine )
{
	QString path = QFileDialog::getExistingDirectory( this, "Locate the directory with "+dirPurpose );
	if (path.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathHelper.useRelativePaths())
		path = pathHelper.getRelativePath( path );

	targetLine->setText( path );
	// the rest of the actions will be performed in the line edit callback,
	// because we want to do the same things when user edits the path manually
}

void SetupDialog::changeIWADDir( QString text )
{
	iwadDir = text;

	// In order to not duplicate functionality, we delegate the request to load IWADs from directory to MainWindow,
	// because it has to be able to do it on its own anyway. And we pass there a ref to our list view, because we expect
	// it to refresh the view when the data change is finished, and it might also change the list view's selection.
	emit iwadListNeedsUpdate( ui->iwadListView );
}

void SetupDialog::changeMapDir( QString text )
{
	mapDir = text;
}

void SetupDialog::changeModDir( QString text )
{
	modDir = text;
}

void SetupDialog::toggleIWADSubdirs( bool checked )
{
	iwadSubdirs = checked;

	emit iwadListNeedsUpdate( ui->iwadListView );
}

void SetupDialog::toggleMapSubdirs( bool checked )
{
	mapSubdirs = checked;
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

	iwads.append({ QFileInfo( path ).fileName(), path });
	iwadModel.updateView( iwads.size() - 1 );
}

void SetupDialog::iwadDelete()
{
	int deletedIdx = deleteSelectedItem( ui->iwadListView, iwadModel );

	// let the parent window update its elements so that they don't point to a non-existing item
	emit iwadDeleted( deletedIdx );
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
	QString name, path;
	EngineDialog dialog( this, pathHelper, name, path );
	int code = dialog.exec();
	if (code == QDialog::Accepted)
		appendItem( ui->engineListView, engineModel, { name, path } );
}

void SetupDialog::engineDelete()
{
	int deletedIdx = deleteSelectedItem( ui->engineListView, engineModel );

	// let the parent window update its elements so that they don't point to a non-existing item
	emit engineDeleted( deletedIdx );
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
	Engine & selectedEngine = engines[ index.row() ];
	QString name = selectedEngine.name;
	QString path = selectedEngine.path;
	EngineDialog dialog( this, pathHelper, name, path );
	int code = dialog.exec();
	if (code == QDialog::Accepted) {
		selectedEngine.name = name;
		selectedEngine.path = path;
	}
}

void SetupDialog::toggleAbsolutePaths( bool checked )
{
	// because MainWindow is the owner of the paths displayed here (and many more),
	// we let it update all the paths in all data models at once
	emit absolutePathsToggled( checked );
	// and then just update our widgets
	engineModel.updateView(0);
	iwadModel.updateView(0);
	ui->iwadDirLine->setText( iwadDir );
	ui->mapDirLine->setText( mapDir );
	ui->modDirLine->setText( modDir );
}

void SetupDialog::closeDialog()
{
	accept();
}
