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

EngineDialog::EngineDialog( QWidget * parent, const PathConvertor & pathConv, const EngineInfo & engine )
:
	QDialog( parent ),
	DialogWithPaths( this, pathConv ),
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
	ui->executableLine->setText( engine.executablePath );
	ui->configDirLine->setText( engine.configDir );
	ui->dataDirLine->setText( engine.dataDir );
	ui->familyCmbBox->setCurrentIndex( int(engine.family) );

	// mark invalid paths
	highlightFilePathIfInvalid( ui->executableLine, engine.executablePath  );
	highlightDirPathIfInvalid( ui->configDirLine, engine.configDir );
	highlightDirPathIfInvalid( ui->dataDirLine, engine.dataDir );

	connect( ui->browseExecutableBtn, &QPushButton::clicked, this, &thisClass::browseExecutable );
	connect( ui->browseConfigDirBtn, &QPushButton::clicked, this, &thisClass::browseConfigDir );
	connect( ui->browseDataDirBtn, &QPushButton::clicked, this, &thisClass::browseDataDir );

	//connect( ui->nameLine, &QLineEdit::textChanged, this, &thisClass::onNameChanged );
	connect( ui->executableLine, &QLineEdit::textChanged, this, &thisClass::onExecutableChanged );
	connect( ui->configDirLine, &QLineEdit::textChanged, this, &thisClass::onConfigDirChanged );
	connect( ui->dataDirLine, &QLineEdit::textChanged, this, &thisClass::onDataDirChanged );

	//connect( ui->familyCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onFamilySelected );

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

	if (engine.executablePath.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())
		browseExecutable();

	if (engine.executablePath.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())  // user closed the browseEngine dialog
		done( QDialog::Rejected );
}

static QString suggestEngineName( const EngineInfo & engine )
{
 #if IS_WINDOWS

	// On Windows we can use the metadata built into the executable, or the name of its directory.
	if (!engine.exeAppName().isEmpty())
		return engine.exeAppName();  // exe metadata should be most reliable source
	else
		return fs::getDirnameOfFile( engine.executablePath );

 #else

	// On Linux we have to fallback to the binary name (or use the Flatpak name if there is one).
	if (engine.sandboxEnvType() != os::Sandbox::None)
		return engine.sandboxAppName();
	else
		return engine.exeBaseName();

 #endif
}

static QString suggestEngineConfigDir( const EngineInfo & engine )
{
 #if IS_WINDOWS

	// On Windows, engines usually store their config in the directory of its binaries,
	// with the exception of latest GZDoom (thanks Graph) that started storing it to Documents\My Games\GZDoom
	if (engine.exeAppName() == "GZDoom" && engine.exeVersion() >= Version(4,9,0))
		return os::getDocumentsDir()%"/My Games/GZDoom";
	else
		return fs::getDirOfFile( engine.executablePath );

 #else

	// On Linux they store them in standard user's app config dir (usually something like /home/youda/.config/).
	if (engine.sandboxEnvType() == os::Sandbox::Snap)
		return os::getHomeDir()%"/snap/"%engine.exeBaseName()%"/current/.config/"%engine.exeBaseName();
	else if (engine.sandboxEnvType() == os::Sandbox::Flatpak)  // the engine is a Flatpak installation
		return os::getHomeDir()%"/.var/app/"%engine.sandboxAppName()%"/.config/"%engine.exeBaseName();
	else
		return os::getConfigDirForApp( engine.executablePath );  // -> /home/youda/.config/zdoom

 #endif
}

static QString suggestEngineDataDir( const EngineInfo & engine )
{
 #if IS_WINDOWS

	// On Windows, engines usually store their data in the directory of its binaries,
	// with the exception of latest GZDoom (thanks Graph) that started storing it to Saved Games\GZDoom
	if (engine.exeAppName() == "GZDoom" && engine.exeVersion() >= Version(4,9,0))
		return os::getSavedGamesDir()%"/GZDoom";
	else
		return fs::getDirOfFile( engine.executablePath );

 #else

	// On Linux it is generally the same as config dir.
	return suggestEngineConfigDir( engine );

 #endif
}

void EngineDialog::browseExecutable()
{
	QString executablePath = DialogWithPaths::browseFile( this, "engine's executable", QString(),
 #if IS_WINDOWS
		"Executable files (*.exe);;"
 #endif
		"All files (*)"
	);
	if (executablePath.isNull())  // user probably clicked cancel
		return;

	engine.executablePath = executablePath;
	engine.initSandboxInfo( executablePath );  // find out whether the engine is installed in a sandbox environment
	engine.loadAppInfo( executablePath );  // read executable version info and infer application name
	// the Engine fields will be initialized in the line-edit callbacks

	suggestedName = suggestEngineName( engine );
	suggestedConfigDir = suggestEngineConfigDir( engine );
	suggestedDataDir = suggestEngineDataDir( engine );
	EngineFamily guessedFamily = guessEngineFamily( engine.appNameNormalized() );

	// the suggested paths are always absolute
	if (pathConvertor.usingRelativePaths())
	{
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
	DialogWithPaths::browseDir( this, "where engine stores configs", ui->configDirLine );
}

void EngineDialog::browseDataDir()
{
	DialogWithPaths::browseDir( this, "where engine stores data files", ui->dataDirLine );
}

void EngineDialog::onNameChanged( const QString & /*text*/ )
{

}

void EngineDialog::onExecutableChanged( const QString & text )
{
	highlightFilePathIfInvalid( ui->executableLine, text );
}

void EngineDialog::onConfigDirChanged( const QString & text )
{
	if (text == suggestedConfigDir
	 && QFileInfo( suggestedDataDir ).dir().exists())  // don't highlight with green if our suggestion is nonsense
		highlightDirPathIfFileOrCanBeCreated( ui->configDirLine, text );
	else
		highlightDirPathIfInvalid( ui->configDirLine, text );
}

void EngineDialog::onDataDirChanged( const QString & text )
{
	if (text == suggestedDataDir
	 && QFileInfo( suggestedDataDir ).dir().exists())  // don't highlight with green if our suggestion is nonsense
		highlightDirPathIfFileOrCanBeCreated( ui->dataDirLine, text );
	else
		highlightDirPathIfInvalid( ui->dataDirLine, text );
}

void EngineDialog::onFamilySelected( int /*familyIdx*/ )
{

}

void EngineDialog::accept()
{
	// verify requirements

	QString nameLineText = ui->nameLine->text();
	if (nameLineText.isEmpty())
	{
		QMessageBox::warning( this, "Engine name cannot be empty", "Please give the engine some name." );
		return;  // refuse the user's confirmation
	}

	QString executableLineText = ui->executableLine->text();
	if (executableLineText.isEmpty())
	{
		QMessageBox::warning( this, "Executable path cannot be empty",
			"Please specify the engine's executable path."
		);
		return;  // refuse the user's confirmation
	}
	else if (fs::isInvalidFile( executableLineText ))
	{
		QMessageBox::warning( this, "Executable doesn't exist",
			"Please fix the engine's executable path, such file doesn't exist."
		);
		return;  // refuse the user's confirmation
	}

	QString configDirLineText = ui->configDirLine->text();
	if (configDirLineText.isEmpty())
	{
		QMessageBox::warning( this, "Config dir cannot be empty",
			"Please specify the engine's config directory, this launcher cannot operate without it."
		);
		return;  // refuse the user's confirmation
	}
	else if (configDirLineText != suggestedConfigDir && fs::isInvalidDir( configDirLineText ))
	{
		QMessageBox::warning( this, "Config dir doesn't exist",
			"Please fix the engine's config directory, such directory doesn't exist."
		);
		return;  // refuse the user's confirmation
	}

	QString dataDirLineText = ui->dataDirLine->text();
	if (dataDirLineText.isEmpty())
	{
		QMessageBox::warning( this, "Data dir cannot be empty",
			"Please specify the engine's data directory, this launcher cannot operate without it."
		);
		return;  // refuse the user's confirmation
	}
	else if (dataDirLineText != suggestedDataDir && fs::isInvalidDir( dataDirLineText ))
	{
		QMessageBox::warning( this, "Data dir doesn't exist",
			"Please fix the engine's data directory, such directory doesn't exist."
		);
		return;  // refuse the user's confirmation
	}

	// all problems fixed -> remove highlighting if it was there
	unhighlightListItem( engine );

	// apply the UI changes
	// We don't have to save the UI data to our struct on every change, doing it once after confirmation is enough.
	// Some operations like reading executable version info would be too expensive to do in every edit callback call.

	engine.name = nameLineText;

	engine.executablePath = executableLineText;
	// If the executableLine was edited manually without the browse button where all the auto-detection happens,
	// the engine's application info must be updated.
	if (engine.appInfoSrcExePath() != executableLineText)  // the app info was constructed from executable that is no longer used
	{
		engine.loadAppInfo( executableLineText );
	}

	engine.configDir = configDirLineText;
	engine.dataDir = dataDirLineText;

	int familyIdx = ui->familyCmbBox->currentIndex();
	if (familyIdx < 0 || familyIdx >= int(EngineFamily::_EnumEnd))
	{
		reportBugToUser( this, "Invalid engine family index", "Family combo-box index is out of bounds." );
	}
	engine.family = EngineFamily( familyIdx );
	engine.assignFamilyTraits( engine.family );

	// accept the user's confirmation
	superClass::accept();
}
