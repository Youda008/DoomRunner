//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description:
//======================================================================================================================

#include "EngineDialog.hpp"
#include "ui_EngineDialog.h"

#include <QFileDialog>


//======================================================================================================================

EngineDialog::EngineDialog( QWidget * parent, const PathHelper & pathHelper, const Engine & engine )

	: QDialog( parent )
	, pathHelper( pathHelper )
	, engine( engine )
{
	ui = new Ui::EngineDialog;
	ui->setupUi(this);

	ui->nameLine->setText( engine.name );
	ui->pathLine->setText( engine.path );
	ui->configDirLine->setText( engine.configDir );

	connect( ui->browseEngineBtn, &QPushButton::clicked, this, &thisClass::browseEngine );
	connect( ui->browseConfigsBtn, &QPushButton::clicked, this, &thisClass::browseConfigDir );

	connect( ui->nameLine, &QLineEdit::textChanged, this, &thisClass::updateName );
	connect( ui->pathLine, &QLineEdit::textChanged, this, &thisClass::updatePath );
	connect( ui->configDirLine, &QLineEdit::textChanged, this, &thisClass::updateConfigDir );

	connect( ui->buttonBox, &QDialogButtonBox::accepted, this, &thisClass::accept );
	connect( ui->buttonBox, &QDialogButtonBox::rejected, this, &thisClass::reject );
}

EngineDialog::~EngineDialog()
{
	delete ui;
}

void EngineDialog::showEvent( QShowEvent * event )
{
	QDialog::showEvent( event );

	if (engine.path.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())
		browseEngine();
}

void EngineDialog::browseEngine()
{
	QString path = QFileDialog::getOpenFileName( this, "Locate engine's executable", ui->pathLine->text(),
 #ifdef _WIN32
		"Executable files (*.exe);;"
 #endif
		"All files (*)"
	);
	if (path.isNull())  // user probably clicked cancel TODO
		return;

	// the path comming out of the file dialog is always absolute
	if (pathHelper.useRelativePaths())
		path = pathHelper.getRelativePath( path );

	ui->pathLine->setText( path );

	if (ui->nameLine->text().isEmpty())  // don't overwrite existing name
		ui->nameLine->setText( QFileInfo( path ).dir().dirName() );

	if (ui->configDirLine->text().isEmpty())  // don't overwrite existing config dir
		ui->configDirLine->setText( QFileInfo( path ).dir().path() );
}

void EngineDialog::browseConfigDir()
{
	QString dirPath = QFileDialog::getExistingDirectory( this, "Locate engine's config directory", ui->configDirLine->text() );
	if (dirPath.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathHelper.useRelativePaths())
		dirPath = pathHelper.getRelativePath( dirPath );

	ui->configDirLine->setText( dirPath );
}

void EngineDialog::updateName( const QString & text )
{
	engine.name = text;
}

void EngineDialog::updatePath( const QString & text )
{
	engine.path = text;
}

void EngineDialog::updateConfigDir( const QString & text )
{
	engine.configDir = text;
}
