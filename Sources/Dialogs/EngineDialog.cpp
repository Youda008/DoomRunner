//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Engine Properties dialog that appears when you try to add of modify an engine
//======================================================================================================================

#include "EngineDialog.hpp"
#include "ui_EngineDialog.h"

#include "OwnFileDialog.hpp"
#include "Utils/WidgetUtils.hpp"
#include "Utils/OSUtils.hpp"
#include "Utils/MiscUtils.hpp"  // highlightInvalidPath
#include "Utils/ErrorHandling.hpp"

#include <QDir>
#include <QTimer>
#include <QMessageBox>


//======================================================================================================================

EngineDialog::EngineDialog( QWidget * parent, const PathConvertor & pathConvertor, const Engine & engine )
:
	QDialog( parent ),
	DialogCommon( this ),
	pathConvertor( pathConvertor ),
	engine( engine )
{
	ui = new Ui::EngineDialog;
	ui->setupUi(this);

	// automatically initialize family combox fox from existing engine families
	for (size_t familyIdx = 0; familyIdx < size_t(EngineFamily::_EnumEnd); ++familyIdx)
	{
		ui->familyCmbBox->addItem( familyToStr( EngineFamily(familyIdx) ) );
	}
	ui->familyCmbBox->setCurrentIndex( 0 );  // set this right at the start so that index is never -1

	// fill existing engine properties
	ui->nameLine->setText( engine.name );
	ui->executableLine->setText( engine.path );
	ui->configDirLine->setText( engine.configDir );
	ui->dataDirLine->setText( engine.dataDir );
	ui->familyCmbBox->setCurrentIndex( int(engine.family) );

	// mark invalid paths
	highlightFilePathIfInvalid( ui->executableLine, engine.path  );
	highlightDirPathIfInvalid( ui->configDirLine, engine.configDir );
	highlightDirPathIfInvalid( ui->dataDirLine, engine.dataDir );

	connect( ui->browseExecutableBtn, &QPushButton::clicked, this, &thisClass::browseExecutable );
	connect( ui->browseConfigDirBtn, &QPushButton::clicked, this, &thisClass::browseConfigDir );
	connect( ui->browseDataDirBtn, &QPushButton::clicked, this, &thisClass::browseDataDir );

	connect( ui->nameLine, &QLineEdit::textChanged, this, &thisClass::onNameChanged );
	connect( ui->executableLine, &QLineEdit::textChanged, this, &thisClass::onExecutableChanged );
	connect( ui->configDirLine, &QLineEdit::textChanged, this, &thisClass::onConfigDirChanged );
	connect( ui->dataDirLine, &QLineEdit::textChanged, this, &thisClass::onDataDirChanged );

	connect( ui->familyCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onFamilySelected );

	// this will call the function when the window is fully initialized and displayed
	QTimer::singleShot( 0, this, &thisClass::onWindowShown );
}

void EngineDialog::adjustUi()
{
	// align the start of the line edits
	int maxLabelWidth = 0;
	if (int width = ui->nameLabel->width(); width > maxLabelWidth)
		maxLabelWidth = width;
	if (int width = ui->executableLabel->width(); width > maxLabelWidth)
		maxLabelWidth = width;
	if (int width = ui->configDirLabel->width(); width > maxLabelWidth)
		maxLabelWidth = width;
	if (int width = ui->dataDirLabel->width(); width > maxLabelWidth)
		maxLabelWidth = width;
	if (int width = ui->familyLabel->width(); width > maxLabelWidth)
		maxLabelWidth = width;
	ui->nameLabel->setMinimumWidth( maxLabelWidth );
	ui->executableLabel->setMinimumWidth( maxLabelWidth );
	ui->configDirLabel->setMinimumWidth( maxLabelWidth );
	ui->dataDirLabel->setMinimumWidth( maxLabelWidth );
	ui->familyLabel->setMinimumWidth( maxLabelWidth );
}

EngineDialog::~EngineDialog()
{
	delete ui;
}

void EngineDialog::showEvent( QShowEvent * event )
{
	// This can't be called in the constructor, because the widgets still don't have their final sizes there.
	adjustUi();

	superClass::showEvent( event );
}

void EngineDialog::onWindowShown()
{
	// This needs to be called when the window is fully initialized and shown, otherwise it will bug itself in a
	// half-shown state and not close properly.

	if (engine.path.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())
		browseExecutable();

	if (engine.path.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())  // user closed the browseEngine dialog
		done( QDialog::Rejected );
}

static QString suggestEngineName(
	[[maybe_unused]] const QString & executablePath, [[maybe_unused]] const os::ExecutableTraits & traits,
	[[maybe_unused]] const std::optional< ExeVersionInfo > & versionInfo
){
 #if IS_WINDOWS

	// On Windows we can use the metadata built into the executable, or the name of its directory.
	if (versionInfo)
		return versionInfo->appName;  // exe metadata should be most reliable source
	else
		return fs::getDirnameOfFile( executablePath );

 #else

	// On Linux we have to fallback to the binary name (or use the Flatpak name if there is one).
	if (traits.sandboxEnv != os::Sandbox::None)
		return traits.sandboxAppName;
	else
		return traits.executableBaseName;

 #endif
}

static QString suggestEngineConfigDir(
	[[maybe_unused]] const QString & executablePath, [[maybe_unused]] const os::ExecutableTraits & traits,
	[[maybe_unused]] const std::optional< ExeVersionInfo > & versionInfo
){
 #if IS_WINDOWS

	// On Windows, engines usually store their config in the directory of its binaries,
	// with the exception of latest GZDoom (thanks Graph) that started storing it to Documents\My Games\GZDoom
	if (versionInfo && versionInfo->appName == "GZDoom" && versionInfo->v >= Version(4,9,0))
		return os::getDocumentsDir()%"/My Games/GZDoom";
	else
		return fs::getDirOfFile( executablePath );

 #else

	// On Linux they store them in standard user's app config dir (usually something like /home/youda/.config/).
	if (traits.sandboxEnv == os::Sandbox::Snap)
		return os::getHomeDir()%"/snap/"%traits.executableBaseName%"/current/.config/"%traits.executableBaseName;
	else if (traits.sandboxEnv == os::Sandbox::Flatpak)  // the engine is a Flatpak installation
		return os::getHomeDir()%"/.var/app/"%traits.sandboxAppName%"/.config/"%traits.executableBaseName;
	else
		return os::getConfigDirForApp( executablePath );  // -> /home/youda/.config/zdoom

 #endif
}

static QString suggestEngineDataDir(
	[[maybe_unused]] const QString & executablePath, [[maybe_unused]] const os::ExecutableTraits & traits,
	[[maybe_unused]] const std::optional< ExeVersionInfo > & versionInfo
){
 #if IS_WINDOWS

	// On Windows, engines usually store their data in the directory of its binaries,
	// with the exception of latest GZDoom (thanks Graph) that started storing it to Saved Games\GZDoom
	if (versionInfo && versionInfo->appName == "GZDoom" && versionInfo->v >= Version(4,9,0))
		return os::getSavedGamesDir()%"/GZDoom";
	else
		return fs::getDirOfFile( executablePath );

 #else

	// On Linux it is generally the same as config dir.
	return suggestEngineConfigDir( executablePath, traits, versionInfo );

 #endif
}

void EngineDialog::browseExecutable()
{
	QString executablePath = OwnFileDialog::getOpenFileName( this, "Locate engine's executable", ui->executableLine->text(),
 #if IS_WINDOWS
		"Executable files (*.exe);;"
 #endif
		"All files (*)"
	);
	if (executablePath.isNull())  // user probably clicked cancel
		return;

	auto executableTraits = os::getExecutableTraits( executablePath );

 #if IS_WINDOWS
	// I hate you Graph!
	engineVersionInfo = readExeVersionInfo( executablePath );
 #endif

	suggestedName = suggestEngineName( executablePath, executableTraits, engineVersionInfo );
	suggestedConfigDir = suggestEngineConfigDir( executablePath, executableTraits, engineVersionInfo );
	suggestedDataDir = suggestEngineDataDir( executablePath, executableTraits, engineVersionInfo );
	EngineFamily guessedFamily = guessEngineFamily( executableTraits.executableBaseName );

	// the paths comming out of the file dialog and suggestions are always absolute
	if (pathConvertor.usingRelativePaths())
	{
		executablePath = pathConvertor.getRelativePath( executablePath );
		suggestedConfigDir = pathConvertor.getRelativePath( suggestedConfigDir );
		suggestedDataDir = pathConvertor.getRelativePath( suggestedDataDir );
	}

	ui->executableLine->setText( executablePath );
	ui->nameLine->setText( suggestedName );
	ui->configDirLine->setText( suggestedConfigDir );
	ui->dataDirLine->setText( suggestedDataDir );
	ui->familyCmbBox->setCurrentIndex( int(guessedFamily) );
}

void EngineDialog::browseConfigDir()
{
	QString dirPath = OwnFileDialog::getExistingDirectory( this, "Locate engine's config directory", ui->configDirLine->text() );
	if (dirPath.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathConvertor.usingRelativePaths())
		dirPath = pathConvertor.getRelativePath( dirPath );

	ui->configDirLine->setText( dirPath );
}

void EngineDialog::browseDataDir()
{
	QString dirPath = OwnFileDialog::getExistingDirectory( this, "Locate engine's data directory", ui->dataDirLine->text() );
	if (dirPath.isEmpty())  // user probably clicked cancel
		return;

	// the path comming out of the file dialog is always absolute
	if (pathConvertor.usingRelativePaths())
		dirPath = pathConvertor.getRelativePath( dirPath );

	ui->dataDirLine->setText( dirPath );
}

void EngineDialog::onNameChanged( const QString & text )
{
	engine.name = text;
}

void EngineDialog::onExecutableChanged( const QString & text )
{
	engine.path = text;

	highlightFilePathIfInvalid( ui->executableLine, text );
}

void EngineDialog::onConfigDirChanged( const QString & text )
{
	engine.configDir = text;

	if (text == suggestedConfigDir
	 && QFileInfo( suggestedDataDir ).dir().exists())  // don't highlight with green if our suggestion is nonsense
		highlightDirPathIfFileOrCanBeCreated( ui->configDirLine, text );
	else
		highlightDirPathIfInvalid( ui->configDirLine, text );
}

void EngineDialog::onDataDirChanged( const QString & text )
{
	engine.dataDir = text;

	if (text == suggestedDataDir
	 && QFileInfo( suggestedDataDir ).dir().exists())  // don't highlight with green if our suggestion is nonsense
		highlightDirPathIfFileOrCanBeCreated( ui->dataDirLine, text );
	else
		highlightDirPathIfInvalid( ui->dataDirLine, text );
}

void EngineDialog::onFamilySelected( int familyIdx )
{
	if (familyIdx < 0 || familyIdx >= int(EngineFamily::_EnumEnd))
	{
		reportBugToUser( this, "Invalid engine family index", "Family combo-box index is out of bounds." );
	}

	engine.family = EngineFamily( familyIdx );
}

void EngineDialog::accept()
{
	if (engine.name.isEmpty())
	{
		QMessageBox::warning( this, "Engine name cannot be empty", "Please give the engine some name." );
		return;
	}

	if (engine.path.isEmpty())
	{
		QMessageBox::warning( this, "Executable path cannot be empty",
			"Please specify the engine's executable path."
		);
		return;
	}
	else if (fs::isInvalidFile( engine.path ))
	{
		QMessageBox::warning( this, "Executable doesn't exist",
			"Please fix the engine's executable path, such file doesn't exist."
		);
		return;
	}

	if (engine.configDir.isEmpty())
	{
		QMessageBox::warning( this, "Config dir cannot be empty",
			"Please specify the engine's config directory, this launcher cannot operate without it."
		);
		return;
	}
	else if (engine.configDir != suggestedConfigDir && fs::isInvalidDir( engine.configDir ))
	{
		QMessageBox::warning( this, "Config dir doesn't exist",
			"Please fix the engine's config directory, such directory doesn't exist."
		);
		return;
	}

	if (engine.dataDir.isEmpty())
	{
		QMessageBox::warning( this, "Data dir cannot be empty",
			"Please specify the engine's data directory, this launcher cannot operate without it."
		);
		return;
	}
	else if (engine.dataDir != suggestedDataDir && fs::isInvalidDir( engine.dataDir ))
	{
		QMessageBox::warning( this, "Data dir doesn't exist",
			"Please fix the engine's data directory, such directory doesn't exist."
		);
		return;
	}

	// all problems fixed -> remove highlighting if it was there
	unhighlightListItem( engine );

	superClass::accept();
}
