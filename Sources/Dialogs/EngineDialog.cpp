//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Engine Properties dialog that appears when you try to add of modify an engine
//======================================================================================================================

#include "EngineDialog.hpp"
#include "ui_EngineDialog.h"

#include "Utils/MiscUtils.hpp"  // highlightInvalidPath
#include "Utils/ErrorHandling.hpp"

#include <QDir>


//======================================================================================================================

EngineDialog::EngineDialog( QWidget * parent, const PathConvertor & pathConv, const EngineInfo & engine, QString lastUsedDir_ )
:
	QDialog( parent ),
	DialogWithPaths( this, pathConv ),
	engine( engine )
{
	ui = new Ui::EngineDialog;
	ui->setupUi(this);

	DialogWithPaths::lastUsedDir = lastUsedDir_;

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

	connect( ui->autoDetectBtn, &QPushButton::clicked, this, &thisClass::autofillEngineFields );

	//connect( ui->nameLine, &QLineEdit::textChanged, this, &thisClass::onNameChanged );
	connect( ui->executableLine, &QLineEdit::textChanged, this, &thisClass::onExecutableChanged );
	connect( ui->configDirLine, &QLineEdit::textChanged, this, &thisClass::onConfigDirChanged );
	connect( ui->dataDirLine, &QLineEdit::textChanged, this, &thisClass::onDataDirChanged );

	//connect( ui->familyCmbBox, QOverload<int>::of( &QComboBox::currentIndexChanged ), this, &thisClass::onFamilySelected );
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

// This is called when the window layout is initialized and widget sizes calculated,
// but before the window is physically shown (drawn for the first time).
void EngineDialog::showEvent( QShowEvent * event )
{
	// This can't be called in the constructor, because the widgets still don't have their final sizes there.
	// Calling it in the onWindowShown() on the other hand, causes the window to briefly appear with the original layout
	// and then switch to the adjusted layout in the next frame.
	adjustUi();

	superClass::showEvent( event );

	// This will be called after the window is fully initialized and physically shown (drawn for the first time).
	QMetaObject::invokeMethod( this, &thisClass::onWindowShown, Qt::ConnectionType::QueuedConnection );
}

// This is called after the window is fully initialized and physically shown (drawn for the first time).
void EngineDialog::onWindowShown()
{
	// This needs to be called when the window is fully initialized and shown,
	// otherwise calling done() will bug the window in a half-shown state instead of closing it properly.
	// Even showEvent() is too early for this.

	if (engine.executablePath.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())
		browseExecutable();

	if (engine.executablePath.isEmpty() && engine.name.isEmpty() && engine.configDir.isEmpty())  // user closed the browseEngine dialog
		done( QDialog::Rejected );
}

static void suggestUserEngineInfo( Engine & userEngineInfo, const EngineTraits & autoEngineInfo )
{
	if (userEngineInfo.name.isEmpty())  // if the user already gave it a name, let him have it
	{
		userEngineInfo.name  = autoEngineInfo.displayName();
	}

	userEngineInfo.family    = autoEngineInfo.currentEngineFamily();
	userEngineInfo.configDir = autoEngineInfo.getDefaultConfigDir();
	userEngineInfo.dataDir   = autoEngineInfo.getDefaultDataDir();
}

void EngineDialog::autofillEngineInfo( EngineInfo & engine, const QString & executablePath )
{
	// load the info that can be determined from the executable path
	engine.executablePath = executablePath;
	engine.autoDetectTraits( executablePath );  // read executable version info and auto-detects its properties

	// automatically suggest the most common user-defined paths and options based on the derived engine info
	suggestUserEngineInfo( engine, engine );
	// keep the suggested paths in the original form, some may be better stored as relative, some as absolute
}

void EngineDialog::autofillEngineFields()
{
	// fill the initial values with some auto-detected suggestions
	autofillEngineInfo( engine, ui->executableLine->text() );  // the path in executableLine is already converted by DialogWithPaths

	// store the automatically suggested directories for path highlighting later
	suggestedConfigDir = engine.configDir;
	suggestedDataDir = engine.dataDir;

	ui->nameLine->setText( engine.name );
	ui->configDirLine->setText( engine.configDir );
	ui->dataDirLine->setText( engine.dataDir );
	ui->familyCmbBox->setCurrentIndex( int( engine.family ) );
}

void EngineDialog::browseExecutable()
{
	bool confirmed = DialogWithPaths::browseFile( this, "engine's executable", ui->executableLine,
 #if IS_WINDOWS
		"Executable files (*.exe);;"
 #endif
		"All files (*)"
	);

	if (confirmed)
	{
		// auto-fill the other fields based on the current value of ui->executableLine
		autofillEngineFields();
	}
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
	// We don't have to store the UI data on every change, doing it once after confirmation is enough.
}

void EngineDialog::onExecutableChanged( const QString & text )
{
	// We don't have to store the UI data on every change, doing it once after confirmation is enough.

	highlightFilePathIfInvalid( ui->executableLine, text );
}

void EngineDialog::onConfigDirChanged( const QString & text )
{
	// We don't have to store the UI data on every change, doing it once after confirmation is enough.

	if (pathConvertor.convertPath( text ) == pathConvertor.convertPath( suggestedConfigDir )
	 && QFileInfo( suggestedDataDir ).dir().exists())  // don't highlight with green if our suggestion is nonsense
		highlightDirPathIfFileOrCanBeCreated( ui->configDirLine, text );
	else
		highlightDirPathIfInvalid( ui->configDirLine, text );
}

void EngineDialog::onDataDirChanged( const QString & text )
{
	// We don't have to store the UI data on every change, doing it once after confirmation is enough.

	if (pathConvertor.convertPath( text ) == pathConvertor.convertPath( suggestedDataDir )
	 && QFileInfo( suggestedDataDir ).dir().exists())  // don't highlight with green if our suggestion is nonsense
		highlightDirPathIfFileOrCanBeCreated( ui->dataDirLine, text );
	else
		highlightDirPathIfInvalid( ui->dataDirLine, text );
}

void EngineDialog::onFamilySelected( int /*familyIdx*/ )
{
	// We don't have to store the UI data on every change, doing it once after confirmation is enough.
}

void EngineDialog::accept()
{
	// verify requirements

	QString nameLineText = ui->nameLine->text();
	if (nameLineText.isEmpty())
	{
		reportUserError( this, "Engine name cannot be empty", "Please give the engine some name." );
		return;  // refuse the user's confirmation
	}

	QString executableLineText = ui->executableLine->text();
	if (executableLineText.isEmpty())
	{
		reportUserError( this, "Executable path cannot be empty",
			"Please specify the engine's executable path."
		);
		return;  // refuse the user's confirmation
	}
	else if (fs::isInvalidFile( executableLineText ))
	{
		reportUserError( this, "Executable doesn't exist",
			"Please fix the engine's executable path, such file doesn't exist."
		);
		return;  // refuse the user's confirmation
	}

	QString configDirLineText = ui->configDirLine->text();
	if (configDirLineText.isEmpty())
	{
		reportUserError( this, "Config dir cannot be empty",
			"Please specify the engine's config directory, this launcher cannot operate without it."
		);
		return;  // refuse the user's confirmation
	}
	else if (configDirLineText != suggestedConfigDir && fs::isInvalidDir( configDirLineText ))
	{
		reportUserError( this, "Config dir doesn't exist",
			"Please fix the engine's config directory, such directory doesn't exist."
		);
		return;  // refuse the user's confirmation
	}

	QString dataDirLineText = ui->dataDirLine->text();
	if (dataDirLineText.isEmpty())
	{
		reportUserError( this, "Data dir cannot be empty",
			"Please specify the engine's data directory, this launcher cannot operate without it."
		);
		return;  // refuse the user's confirmation
	}
	else if (dataDirLineText != suggestedDataDir && fs::isInvalidDir( dataDirLineText ))
	{
		reportUserError( this, "Data dir doesn't exist",
			"Please fix the engine's data directory, such directory doesn't exist."
		);
		return;  // refuse the user's confirmation
	}

	// all problems fixed -> remove highlighting if it was there
	unhighlightListItem( engine );

	// apply the UI changes
	// We don't have to save the UI data to our struct on every change, doing it once after confirmation is enough.
	// Some operations like reading executable version info would be too expensive to do in every edit callback call.

	engine.name = std::move( nameLineText );

	// If the executableLine was edited manually without the browse button where all the auto-detection happens,
	// the engine's application info must be updated.
	if (engine.executablePath != executableLineText)  // the app info was constructed from executable that is no longer used
	{
		engine.executablePath = pathConvertor.convertPath( executableLineText );
		engine.autoDetectTraits( executableLineText );
	}

	engine.configDir = std::move( configDirLineText );
	engine.dataDir = std::move( dataDirLineText );

	int familyIdx = ui->familyCmbBox->currentIndex();
	if (familyIdx < 0 || familyIdx >= int( EngineFamily::_EnumEnd ))
	{
		reportLogicError( this, "Invalid engine family index", "Family combo-box index is out of bounds." );
		return;
	}
	engine.family = EngineFamily( familyIdx );
	engine.setFamilyTraits( engine.family );

	assert( engine.isCorrectlyInitialized() );

	// accept the user's confirmation
	superClass::accept();
}
