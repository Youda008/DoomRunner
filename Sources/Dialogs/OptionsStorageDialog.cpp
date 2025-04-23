//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of Options Storage dialog
//======================================================================================================================

#include "OptionsStorageDialog.hpp"
#include "ui_OptionsStorageDialog.h"


//======================================================================================================================
// OptionsStorageDialog

OptionsStorageDialog::OptionsStorageDialog( QWidget * parent, const StorageSettings & settings )
:
	QDialog( parent ),
	DialogCommon( this, u"OptionsStorageDialog" ),
	storageSettings( settings )
{
	ui = new Ui::OptionsStorageDialog;
	ui->setupUi( this );

	// restore options

	restoreStorage( storageSettings.launchOptsStorage, ui->launchBtn_none, ui->launchBtn_global, ui->launchBtn_preset );
	restoreStorage( storageSettings.gameOptsStorage, ui->gameplayBtn_none, ui->gameplayBtn_global, ui->gameplayBtn_preset );
	restoreStorage( storageSettings.compatOptsStorage, ui->compatBtn_none, ui->compatBtn_global, ui->compatBtn_preset );
	restoreStorage( storageSettings.videoOptsStorage, ui->videoBtn_none, ui->videoBtn_global, ui->videoBtn_preset );
	restoreStorage( storageSettings.audioOptsStorage, ui->audioBtn_none, ui->audioBtn_global, ui->audioBtn_preset );

	// setup buttons

	connect( ui->launchBtn_none, &QRadioButton::clicked, this, &thisClass::onLaunchStorageChosen_none );
	connect( ui->launchBtn_global, &QRadioButton::clicked, this, &thisClass::onLaunchStorageChosen_global );
	connect( ui->launchBtn_preset, &QRadioButton::clicked, this, &thisClass::onLaunchStorageChosen_preset );

	connect( ui->gameplayBtn_none, &QRadioButton::clicked, this, &thisClass::onGameplayStorageChosen_none );
	connect( ui->gameplayBtn_global, &QRadioButton::clicked, this, &thisClass::onGameplayStorageChosen_global );
	connect( ui->gameplayBtn_preset, &QRadioButton::clicked, this, &thisClass::onGameplayStorageChosen_preset );

	connect( ui->compatBtn_none, &QRadioButton::clicked, this, &thisClass::onCompatStorageChosen_none );
	connect( ui->compatBtn_global, &QRadioButton::clicked, this, &thisClass::onCompatStorageChosen_global );
	connect( ui->compatBtn_preset, &QRadioButton::clicked, this, &thisClass::onCompatStorageChosen_preset );

	connect( ui->videoBtn_none, &QRadioButton::clicked, this, &thisClass::onVideoStorageChosen_none );
	connect( ui->videoBtn_global, &QRadioButton::clicked, this, &thisClass::onVideoStorageChosen_global );
	connect( ui->videoBtn_preset, &QRadioButton::clicked, this, &thisClass::onVideoStorageChosen_preset );

	connect( ui->audioBtn_none, &QRadioButton::clicked, this, &thisClass::onAudioStorageChosen_none );
	connect( ui->audioBtn_global, &QRadioButton::clicked, this, &thisClass::onAudioStorageChosen_global );
	connect( ui->audioBtn_preset, &QRadioButton::clicked, this, &thisClass::onAudioStorageChosen_preset );
}

void OptionsStorageDialog::restoreStorage( OptionsStorage storage, QRadioButton * noneBtn, QRadioButton * globalBtn, QRadioButton * presetBtn )
{
	if (storage == DontStore)
		noneBtn->click();
	else if (storage == StoreGlobally)
		globalBtn->click();
	else if (storage == StoreToPreset)
		presetBtn->click();
}

OptionsStorageDialog::~OptionsStorageDialog()
{
	delete ui;
}


//----------------------------------------------------------------------------------------------------------------------
// slots

void OptionsStorageDialog::onLaunchStorageChosen_none()
{
	storageSettings.launchOptsStorage = DontStore;
}

void OptionsStorageDialog::onLaunchStorageChosen_global()
{
	storageSettings.launchOptsStorage = StoreGlobally;
}

void OptionsStorageDialog::onLaunchStorageChosen_preset()
{
	storageSettings.launchOptsStorage = StoreToPreset;
}

void OptionsStorageDialog::onGameplayStorageChosen_none()
{
	storageSettings.gameOptsStorage = DontStore;
}

void OptionsStorageDialog::onGameplayStorageChosen_global()
{
	storageSettings.gameOptsStorage = StoreGlobally;
}

void OptionsStorageDialog::onGameplayStorageChosen_preset()
{
	storageSettings.gameOptsStorage = StoreToPreset;
}

void OptionsStorageDialog::onCompatStorageChosen_none()
{
	storageSettings.compatOptsStorage = DontStore;
}

void OptionsStorageDialog::onCompatStorageChosen_global()
{
	storageSettings.compatOptsStorage = StoreGlobally;
}

void OptionsStorageDialog::onCompatStorageChosen_preset()
{
	storageSettings.compatOptsStorage = StoreToPreset;
}

void OptionsStorageDialog::onVideoStorageChosen_none()
{
	storageSettings.videoOptsStorage = DontStore;
}

void OptionsStorageDialog::onVideoStorageChosen_global()
{
	storageSettings.videoOptsStorage = StoreGlobally;
}

void OptionsStorageDialog::onVideoStorageChosen_preset()
{
	storageSettings.videoOptsStorage = StoreToPreset;
}

void OptionsStorageDialog::onAudioStorageChosen_none()
{
	storageSettings.audioOptsStorage = DontStore;
}

void OptionsStorageDialog::onAudioStorageChosen_global()
{
	storageSettings.audioOptsStorage = StoreGlobally;
}

void OptionsStorageDialog::onAudioStorageChosen_preset()
{
	storageSettings.audioOptsStorage = StoreToPreset;
}
