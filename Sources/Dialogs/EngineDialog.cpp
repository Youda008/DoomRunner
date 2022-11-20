//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Engine Properties dialog that appears when you try to add of modify an engine
//======================================================================================================================

#include "EngineDialog.hpp"
#include "ui_EngineDialog.h"

#include "ColorThemes.hpp"  // updateWindowBorder
#include "OwnFileDialog.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/OSUtils.hpp"
#include "Utils/MiscUtils.hpp"  // highlightInvalidPath

#include <QDir>
#include <QTimer>
#include <QMessageBox>


//======================================================================================================================

EngineDialog::EngineDialog( QWidget * parent, const PathContext & pathContext, const Engine & engine )
:
	QDialog( parent ),
	pathContext( pathContext ),
	engine( engine )
{
	ui = new Ui::EngineDialog;
	ui->setupUi(this);

	updateWindowBorder( this );  // on Windows we need to manually make title bar of every new window dark, if dark theme is used

	// automatically initialize family combox fox from existing engine families
	for (size_t familyIdx = 0; familyIdx < size_t(EngineFamily::_EnumEnd); ++familyIdx)
	{
		ui->familyCmbBox->addItem( familyToStr( EngineFamily(familyIdx) ) );
	}
	ui->familyCmbBox->setCurrentIndex( 0 );  // set this right at the start so that index is never -1

	// fill existing engine properties
	ui->nameLine->setText( engine.name );
	ui->pathLine->setText( engine.path );
	ui->configDirLine->setText( engine.configDir );
	ui->familyCmbBox->setCurrentIndex( int(engine.family) );

	// mark invalid paths
	highlightInvalidFile( ui->pathLine, engine.path  );
	highlightInvalidDir( ui->configDirLine, engine.configDir );

	connect( ui->browseEngineBtn, &QPushButton::clicked, this, &thisClass::browseEngine );
	connect( ui->browseConfigsBtn, &QPushButton::clicked, this, &thisClass::browseConfigDir );

	connect( ui->nameLine, &QLineEdit::textChanged, this, &thisClass::updateName );
	connect( ui->pathLine, &QLineEdit::textChanged, this, &thisClass::updatePath );
	connect( ui->configDirLine, &QLineEdit::textChanged, this, &thisClass::updateConfigDir );

	connect( ui->familyCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::selectFamily );

	connect( ui->buttonBox, &QDialogButtonBox::accepted, this, &thisClass::accept );
	connect( ui->buttonBox, &QDialogButtonBox::rejected, this, &thisClass::reject );

	// this will call the function when the window is fully initialized and displayed
	QTimer::singleShot( 0, this, &thisClass::onWindowShown );
}

EngineDialog::~EngineDialog()
{
	delete ui;
}

void EngineDialog::onWindowShown()
{
	// This needs to be called when the window is fully initialized and shown, otherwise it will bug itself in a
	// half-shown state and not close properly.

	if (engine.path.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())
		browseEngine();

	if (engine.path.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())  // user closed the browseEngine dialog
		done( QDialog::Rejected );
}

static QString getEngineName( const QString & enginePath )
{
	// In Windows we can use the directory name, which can tell slightly more than just the binary
	// but in Linux we have to fallback to the binary name, because all binaries are in same dir.
	if (isWindows())
		return getDirnameOfFile( enginePath );
	else
		return getFileNameFromPath( enginePath );
}

void EngineDialog::browseEngine()
{
	QString enginePath = OwnFileDialog::getOpenFileName( this, "Locate engine's executable", ui->pathLine->text(),
 #if IS_WINDOWS
		"Executable files (*.exe);;"
 #endif
		"All files (*)"
	);
	if (enginePath.isNull())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
		enginePath = pathContext.getRelativePath( enginePath );

	ui->pathLine->setText( enginePath );

	if (ui->nameLine->text().isEmpty())  // don't overwrite existing name
		ui->nameLine->setText( getEngineName( enginePath ) );

	if (ui->configDirLine->text().isEmpty())  // don't overwrite existing config dir
		ui->configDirLine->setText( getAppDataDir( enginePath ) );

	// guess the engine family based on executable's name
	QString executableName = getFileBasenameFromPath( enginePath );
	EngineFamily guessedFamily = guessEngineFamily( executableName );
	ui->familyCmbBox->setCurrentIndex( int(guessedFamily) );
}

void EngineDialog::browseConfigDir()
{
	QString dirPath = OwnFileDialog::getExistingDirectory( this, "Locate engine's config directory", ui->configDirLine->text() );
	if (dirPath.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathContext.usingRelativePaths())
		dirPath = pathContext.getRelativePath( dirPath );

	ui->configDirLine->setText( dirPath );
}

void EngineDialog::updateName( const QString & text )
{
	engine.name = text;
}

void EngineDialog::updatePath( const QString & text )
{
	engine.path = text;

	highlightInvalidFile( ui->pathLine, text );
}

void EngineDialog::updateConfigDir( const QString & text )
{
	engine.configDir = text;

	highlightInvalidDir( ui->configDirLine, text );
}

void EngineDialog::selectFamily( int familyIdx )
{
	if (familyIdx < 0 || familyIdx >= int(EngineFamily::_EnumEnd))
	{
		QMessageBox::critical( this, "Invalid engine family index",
			"Family combo-box index is out of bounds. This shouldn't be possible, please create an issue on Github page." );
	}

	engine.family = EngineFamily( familyIdx );
}
