//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  13.5.2019
// Description: logic of the Game Options dialog
//======================================================================================================================

#include "GameOptsDialog.hpp"
#include "ui_GameOptsDialog.h"

#include <QString>
#include <QLineEdit>
#include <QIntValidator>


//======================================================================================================================

typedef uint flagsIdx;
constexpr flagsIdx DM_FLAGS_1 = 0;
constexpr flagsIdx DM_FLAGS_2 = 1;

struct DMFlag {
	flagsIdx flags;
	int32_t bit;
	bool defaultVal;
};

static const DMFlag FALLING_DAMAGE               = { DM_FLAGS_1, 8, false };
static const DMFlag DROP_WEAPON                  = { DM_FLAGS_2, 2, false };
static const DMFlag DOUBLE_AMMO                  = { DM_FLAGS_2, 64, false };
static const DMFlag INF_AMMO                     = { DM_FLAGS_1, 2048, false };
static const DMFlag INF_INVENTORY                = { DM_FLAGS_2, 65536, false };
static const DMFlag NO_MONSTERS                  = { DM_FLAGS_1, 4096, false };
static const DMFlag NO_MONSTERS_TO_EXIT          = { DM_FLAGS_2, 131072, false };
static const DMFlag MONSTERS_RESPAWN             = { DM_FLAGS_1, 8192, false };
static const DMFlag NO_RESPAWN                   = { DM_FLAGS_2, 16384, false };
static const DMFlag ITEMS_RESPAWN                = { DM_FLAGS_1, 16384, false };
static const DMFlag BIG_POWERUPS_RESPAWN         = { DM_FLAGS_2, 134217728, false };
static const DMFlag FAST_MONSTERS                = { DM_FLAGS_1, 32768, false };
static const DMFlag DEGENERATION                 = { DM_FLAGS_2, 128, false };
static const DMFlag ALLOW_AUTO_AIM               = { DM_FLAGS_2, 8388608, true };
static const DMFlag ALLOW_SUICIDE                = { DM_FLAGS_2, 4194304, true };
static const DMFlag ALLOW_JUMP_ALWAYS_OFF        = { DM_FLAGS_1, 65536, false };
static const DMFlag ALLOW_JUMP_ALWAYS_ON         = { DM_FLAGS_1, 131072, false };
static const DMFlag ALLOW_CROUCH_ALWAYS_OFF      = { DM_FLAGS_1, 4194304, false };
static const DMFlag ALLOW_CROUCH_ALWAYS_ON       = { DM_FLAGS_1, 8388608, false };
static const DMFlag ALLOW_FREELOOK_ALWAYS_OFF    = { DM_FLAGS_1, 262144, false };
static const DMFlag ALLOW_FREELOOK_ALWAYS_ON     = { DM_FLAGS_1, 524288, false };
static const DMFlag ALLOW_FOV                    = { DM_FLAGS_1, 1048576, true };
static const DMFlag ALLOW_BFG_AIMING             = { DM_FLAGS_2, 256, true };
static const DMFlag ALLOW_AUTOMAP                = { DM_FLAGS_2, 262144, true };
static const DMFlag AUTOMAP_ALLIES               = { DM_FLAGS_2, 524288, true };
static const DMFlag ALLOW_SPYING                 = { DM_FLAGS_2, 1048576, true };
static const DMFlag CHASECAM_CHEAT               = { DM_FLAGS_2, 2097152, false };
static const DMFlag CHECK_AMMO_FOR_WEAPON_SWITCH = { DM_FLAGS_2, 16777216, true };
static const DMFlag ICONS_DEATH_KILLS_ITS_SPAWNS = { DM_FLAGS_2, 33554432, false };
static const DMFlag END_SECTOR_COUNTS_FOR_KILL   = { DM_FLAGS_2, 67108864, true };

static const DMFlag WEAPONS_STAY                 = { DM_FLAGS_1, 4, false };
static const DMFlag ALLOW_POWERUPS               = { DM_FLAGS_1, 2, true };
static const DMFlag ALLOW_HEALTH                 = { DM_FLAGS_1, 1, true };
static const DMFlag ALLOW_ARMOR                  = { DM_FLAGS_1, 512, true };
static const DMFlag SPAWN_FARTHEST               = { DM_FLAGS_1, 128, false };
static const DMFlag SAME_MAP                     = { DM_FLAGS_1, 64, false };
static const DMFlag FORCE_RESPAWN                = { DM_FLAGS_1, 256, false };
static const DMFlag ALLOW_EXIT                   = { DM_FLAGS_1, 1024, true };
static const DMFlag BARRELS_RESPAWN              = { DM_FLAGS_2, 512, false };
static const DMFlag RESPAWN_PROTECTION           = { DM_FLAGS_2, 1024, false };
static const DMFlag LOSE_FRAG_IF_FRAGGED         = { DM_FLAGS_2, 32768, false };
static const DMFlag KEEP_FRAGS_GAINED            = { DM_FLAGS_2, 8192, false };
static const DMFlag NO_TEAM_SWITCHING            = { DM_FLAGS_2, 16, false };

static const DMFlag SPAWN_MULTI_WEAPONS          = { DM_FLAGS_1, 2097152, true };
static const DMFlag LOSE_ENTIRE_INVENTORY        = { DM_FLAGS_1, 16777216, false };
static const DMFlag KEEP_KEYS                    = { DM_FLAGS_1, 33554432, true };
static const DMFlag KEEP_WEAPONS                 = { DM_FLAGS_1, 67108864, true };
static const DMFlag KEEP_ARMOR                   = { DM_FLAGS_1, 134217728, true };
static const DMFlag KEEP_POWERUPS                = { DM_FLAGS_1, 268435456, true };
static const DMFlag KEEP_AMMO                    = { DM_FLAGS_1, 536870912, true };
static const DMFlag LOSE_HALF_AMMO               = { DM_FLAGS_1, 1073741824, false };
static const DMFlag SPAWN_WHERE_DIED             = { DM_FLAGS_2, 4096, false };


//======================================================================================================================

GameOptsDialog::GameOptsDialog( QWidget * parent, const GameplayOptions & gameOpts )
:
	QDialog( parent ),
	gameOpts( gameOpts )
{
	ui = new Ui::GameOptsDialog;
	ui->setupUi( this );

	ui->dmflags1_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );
	ui->dmflags2_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );

	ui->dmflags1_line->setText( QString::number( gameOpts.flags1 ) );
	ui->dmflags2_line->setText( QString::number( gameOpts.flags2 ) );

	updateCheckboxes();

	connect( ui->buttonBox, &QDialogButtonBox::accepted, this, &thisClass::accept );
	connect( ui->buttonBox, &QDialogButtonBox::rejected, this, &thisClass::reject );
}

GameOptsDialog::~GameOptsDialog()
{
	delete ui;
}

void GameOptsDialog::setFlag( const DMFlag & flag, bool enabled )
{
	int32_t * flags;
	QLineEdit * line;

	if (flag.flags == DM_FLAGS_1)
	{
		flags = &gameOpts.flags1;
		line = ui->dmflags1_line;
	}
	else
	{
		flags = &gameOpts.flags2;
		line = ui->dmflags2_line;
	}

	if (enabled != flag.defaultVal)
		*flags |= flag.bit;
	else
		*flags &= ~flag.bit;

	line->setText( QString::number( *flags ) );
}

bool GameOptsDialog::isEnabled( const DMFlag & flag ) const
{
	const int32_t * flags;

	if (flag.flags == DM_FLAGS_1)
		flags = &gameOpts.flags1;
	else
		flags = &gameOpts.flags2;

	if (flag.defaultVal == 0)
		return (*flags & flag.bit) != 0;
	else
		return (*flags & flag.bit) == 0;
}


//----------------------------------------------------------------------------------------------------------------------

void GameOptsDialog::on_fallingDamage_toggled( bool checked )
{
	setFlag( FALLING_DAMAGE, checked );
}

void GameOptsDialog::on_dropWeapon_toggled( bool checked )
{
	setFlag( DROP_WEAPON, checked );
}

void GameOptsDialog::on_doubleAmmo_toggled( bool checked )
{
	setFlag( DOUBLE_AMMO, checked );
}

void GameOptsDialog::on_infAmmo_toggled( bool checked )
{
	setFlag( INF_AMMO, checked );
}

void GameOptsDialog::on_infInventory_toggled( bool checked )
{
	setFlag( INF_INVENTORY, checked );
}

void GameOptsDialog::on_noMonsters_toggled( bool checked )
{
	setFlag( NO_MONSTERS, checked );
}

void GameOptsDialog::on_noMonstersToExit_toggled( bool checked )
{
	setFlag( NO_MONSTERS_TO_EXIT, checked );
}

void GameOptsDialog::on_monstersRespawn_toggled( bool checked )
{
	setFlag( MONSTERS_RESPAWN, checked );
}

void GameOptsDialog::on_noRespawn_toggled( bool checked )
{
	setFlag( NO_RESPAWN, checked );
}

void GameOptsDialog::on_itemsRespawn_toggled( bool checked )
{
	setFlag( ITEMS_RESPAWN, checked );
}

void GameOptsDialog::on_bigPowerupsRespawn_toggled( bool checked )
{
	setFlag( BIG_POWERUPS_RESPAWN, checked );
}

void GameOptsDialog::on_fastMonsters_toggled( bool checked )
{
	setFlag( FAST_MONSTERS, checked );
}

void GameOptsDialog::on_degeneration_toggled( bool checked )
{
	setFlag( DEGENERATION, checked );
}

void GameOptsDialog::on_allowAutoAim_toggled( bool checked )
{
	setFlag( ALLOW_AUTO_AIM, checked );
}

void GameOptsDialog::on_allowSuicide_toggled( bool checked )
{
	setFlag( ALLOW_SUICIDE, checked );
}

void GameOptsDialog::on_allowJump_stateChanged( int state )
{
	if (state == Qt::Unchecked)
	{
		setFlag( ALLOW_JUMP_ALWAYS_ON, false );
		setFlag( ALLOW_JUMP_ALWAYS_OFF, true );
	}
	else if (state == Qt::PartiallyChecked)
	{
		setFlag( ALLOW_JUMP_ALWAYS_ON, false );
		setFlag( ALLOW_JUMP_ALWAYS_OFF, false );
	}
	else if (state == Qt::Checked)
	{
		setFlag( ALLOW_JUMP_ALWAYS_ON, true );
		setFlag( ALLOW_JUMP_ALWAYS_OFF, false );
	}
}

void GameOptsDialog::on_allowCrouch_stateChanged( int state )
{
	if (state == Qt::Unchecked)
	{
		setFlag( ALLOW_CROUCH_ALWAYS_ON, false );
		setFlag( ALLOW_CROUCH_ALWAYS_OFF, true );
	}
	else if (state == Qt::PartiallyChecked)
	{
		setFlag( ALLOW_CROUCH_ALWAYS_ON, false );
		setFlag( ALLOW_CROUCH_ALWAYS_OFF, false );
	}
	else if (state == Qt::Checked)
	{
		setFlag( ALLOW_CROUCH_ALWAYS_ON, true );
		setFlag( ALLOW_CROUCH_ALWAYS_OFF, false );
	}
}

void GameOptsDialog::on_allowFreelook_stateChanged( int state )
{
	if (state == Qt::Unchecked)
	{
		setFlag( ALLOW_FREELOOK_ALWAYS_ON, false );
		setFlag( ALLOW_FREELOOK_ALWAYS_OFF, true );
	}
	else if (state == Qt::PartiallyChecked)
	{
		setFlag( ALLOW_FREELOOK_ALWAYS_ON, false );
		setFlag( ALLOW_FREELOOK_ALWAYS_OFF, false );
	}
	else if (state == Qt::Checked)
	{
		setFlag( ALLOW_FREELOOK_ALWAYS_ON, true );
		setFlag( ALLOW_FREELOOK_ALWAYS_OFF, false );
	}
}

void GameOptsDialog::on_allowFOV_toggled( bool checked )
{
	setFlag( ALLOW_FOV, checked );
}

void GameOptsDialog::on_allowBFGAiming_toggled( bool checked )
{
	setFlag( ALLOW_BFG_AIMING, checked );
}

void GameOptsDialog::on_allowAutomap_toggled( bool checked )
{
	setFlag( ALLOW_AUTOMAP, checked );
}

void GameOptsDialog::on_automapAllies_toggled( bool checked )
{
	setFlag( AUTOMAP_ALLIES, checked );
}

void GameOptsDialog::on_allowSpying_toggled( bool checked )
{
	setFlag( ALLOW_SPYING, checked );
}

void GameOptsDialog::on_chasecamCheat_toggled( bool checked )
{
	setFlag( CHASECAM_CHEAT, checked );
}

void GameOptsDialog::on_checkAmmoForWeaponSwitch_toggled( bool checked )
{
	setFlag( CHECK_AMMO_FOR_WEAPON_SWITCH, checked );
}

void GameOptsDialog::on_iconsDeathKillsItsSpawns_toggled( bool checked )
{
	setFlag( ICONS_DEATH_KILLS_ITS_SPAWNS, checked );
}

void GameOptsDialog::on_endSectorCountsForKill_toggled( bool checked )
{
	setFlag( END_SECTOR_COUNTS_FOR_KILL, checked );
}

void GameOptsDialog::on_weaponsStay_toggled( bool checked )
{
	setFlag( WEAPONS_STAY, checked );
}

void GameOptsDialog::on_allowPowerups_toggled( bool checked )
{
	setFlag( ALLOW_POWERUPS, checked );
}

void GameOptsDialog::on_allowHealth_toggled( bool checked )
{
	setFlag( ALLOW_HEALTH, checked );
}

void GameOptsDialog::on_allowArmor_toggled( bool checked )
{
	setFlag( ALLOW_ARMOR, checked );
}

void GameOptsDialog::on_spawnFarthest_toggled( bool checked )
{
	setFlag( SPAWN_FARTHEST, checked );
}

void GameOptsDialog::on_sameMap_toggled( bool checked )
{
	setFlag( SAME_MAP, checked );
}

void GameOptsDialog::on_forceRespawn_toggled( bool checked )
{
	setFlag( FORCE_RESPAWN, checked );
}

void GameOptsDialog::on_allowExit_toggled( bool checked )
{
	setFlag( ALLOW_EXIT, checked );
}

void GameOptsDialog::on_barrelsRespawn_toggled( bool checked )
{
	setFlag( BARRELS_RESPAWN, checked );
}

void GameOptsDialog::on_respawnProtection_toggled( bool checked )
{
	setFlag( RESPAWN_PROTECTION, checked );
}

void GameOptsDialog::on_loseFragIfFragged_toggled( bool checked )
{
	setFlag( LOSE_FRAG_IF_FRAGGED, checked );
}

void GameOptsDialog::on_keepFragsGained_toggled( bool checked )
{
	setFlag( KEEP_FRAGS_GAINED, checked );
}

void GameOptsDialog::on_noTeamSwitching_toggled( bool checked )
{
	setFlag( NO_TEAM_SWITCHING, checked );
}

void GameOptsDialog::on_spawnMultiWeapons_toggled( bool checked )
{
	setFlag( SPAWN_MULTI_WEAPONS, checked );
}

void GameOptsDialog::on_loseEntireInventory_toggled( bool checked )
{
	setFlag( LOSE_ENTIRE_INVENTORY, checked );
}

void GameOptsDialog::on_keepKeys_toggled( bool checked )
{
	setFlag( KEEP_KEYS, checked );
}

void GameOptsDialog::on_keepWeapons_toggled( bool checked )
{
	setFlag( KEEP_WEAPONS, checked );
}

void GameOptsDialog::on_keepArmor_toggled( bool checked )
{
	setFlag( KEEP_ARMOR, checked );
}

void GameOptsDialog::on_keepPowerups_toggled( bool checked )
{
	setFlag( KEEP_POWERUPS, checked );
}

void GameOptsDialog::on_keepAmmo_toggled( bool checked )
{
	setFlag( KEEP_AMMO, checked );
}

void GameOptsDialog::on_loseHalfAmmo_toggled( bool checked )
{
	setFlag( LOSE_HALF_AMMO, checked );
}

void GameOptsDialog::on_spawnWhereDied_toggled( bool checked )
{
	setFlag( SPAWN_WHERE_DIED, checked );
}


//----------------------------------------------------------------------------------------------------------------------

void GameOptsDialog::on_dmflags1_line_textEdited( const QString & )
{
	gameOpts.flags1 = ui->dmflags1_line->text().toInt();
	updateCheckboxes();
}

void GameOptsDialog::on_dmflags2_line_textEdited( const QString & )
{
	gameOpts.flags2 = ui->dmflags2_line->text().toInt();
	updateCheckboxes();
}

void GameOptsDialog::updateCheckboxes()
{
	ui->fallingDamage->setChecked( isEnabled( FALLING_DAMAGE ) );
	ui->dropWeapon->setChecked( isEnabled( DROP_WEAPON ) );
	ui->doubleAmmo->setChecked( isEnabled( DOUBLE_AMMO ) );
	ui->infAmmo->setChecked( isEnabled( INF_AMMO ) );
	ui->infInventory->setChecked( isEnabled( INF_INVENTORY ) );
	ui->noMonsters->setChecked( isEnabled( NO_MONSTERS ) );
	ui->monstersRespawn->setChecked( isEnabled( NO_MONSTERS_TO_EXIT ) );
	ui->monstersRespawn->setChecked( isEnabled( MONSTERS_RESPAWN ) );
	ui->noRespawn->setChecked( isEnabled( NO_RESPAWN ) );
	ui->itemsRespawn->setChecked( isEnabled( ITEMS_RESPAWN ) );
	ui->bigPowerupsRespawn->setChecked( isEnabled( BIG_POWERUPS_RESPAWN ) );
	ui->fastMonsters->setChecked( isEnabled( FAST_MONSTERS ) );
	ui->degeneration->setChecked( isEnabled( DEGENERATION ) );
	ui->allowAutoAim->setChecked( isEnabled( ALLOW_AUTO_AIM ) );
	ui->allowSuicide->setChecked( isEnabled( ALLOW_SUICIDE ) );
	if (isEnabled( ALLOW_JUMP_ALWAYS_OFF ))
		ui->allowJump->setCheckState( Qt::Unchecked );
	else if (isEnabled( ALLOW_JUMP_ALWAYS_ON ))
		ui->allowJump->setCheckState( Qt::Checked );
	else
		ui->allowJump->setCheckState( Qt::PartiallyChecked );
	if (isEnabled( ALLOW_CROUCH_ALWAYS_OFF ))
		ui->allowCrouch->setCheckState( Qt::Unchecked );
	else if (isEnabled( ALLOW_CROUCH_ALWAYS_ON ))
		ui->allowCrouch->setCheckState( Qt::Checked );
	else
		ui->allowCrouch->setCheckState( Qt::PartiallyChecked );
	if (isEnabled( ALLOW_FREELOOK_ALWAYS_OFF ))
		ui->allowFreelook->setCheckState( Qt::Unchecked );
	else if (isEnabled( ALLOW_FREELOOK_ALWAYS_ON ))
		ui->allowFreelook->setCheckState( Qt::Checked );
	else
		ui->allowFreelook->setCheckState( Qt::PartiallyChecked );
	ui->allowFOV->setChecked( isEnabled( ALLOW_FOV ) );
	ui->allowBFGAiming->setChecked( isEnabled( ALLOW_BFG_AIMING ) );
	ui->allowAutomap->setChecked( isEnabled( ALLOW_AUTOMAP ) );
	ui->automapAllies->setChecked( isEnabled( AUTOMAP_ALLIES ) );
	ui->allowSpying->setChecked( isEnabled( ALLOW_SPYING ) );
	ui->chasecamCheat->setChecked( isEnabled( CHASECAM_CHEAT ) );
	ui->checkAmmoForWeaponSwitch->setChecked( isEnabled( CHECK_AMMO_FOR_WEAPON_SWITCH ) );
	ui->iconsDeathKillsItsSpawns->setChecked( isEnabled( ICONS_DEATH_KILLS_ITS_SPAWNS ) );
	ui->endSectorCountsForKill->setChecked( isEnabled( END_SECTOR_COUNTS_FOR_KILL ) );
	ui->weaponsStay->setChecked( isEnabled( WEAPONS_STAY ) );
	ui->allowPowerups->setChecked( isEnabled( ALLOW_POWERUPS ) );
	ui->allowHealth->setChecked( isEnabled( ALLOW_HEALTH ) );
	ui->allowArmor->setChecked( isEnabled( ALLOW_ARMOR ) );
	ui->spawnFarthest->setChecked( isEnabled( SPAWN_FARTHEST ) );
	ui->sameMap->setChecked( isEnabled( SAME_MAP ) );
	ui->forceRespawn->setChecked( isEnabled( FORCE_RESPAWN ) );
	ui->allowExit->setChecked( isEnabled( ALLOW_EXIT ) );
	ui->barrelsRespawn->setChecked( isEnabled( BARRELS_RESPAWN ) );
	ui->respawnProtection->setChecked( isEnabled( RESPAWN_PROTECTION ) );
	ui->loseFragIfFragged->setChecked( isEnabled( LOSE_FRAG_IF_FRAGGED ) );
	ui->keepFragsGained->setChecked( isEnabled( KEEP_FRAGS_GAINED ) );
	ui->noTeamSwitching->setChecked( isEnabled( NO_TEAM_SWITCHING ) );
	ui->spawnMultiWeapons->setChecked( isEnabled( SPAWN_MULTI_WEAPONS ) );
	ui->loseEntireInventory->setChecked( isEnabled( LOSE_ENTIRE_INVENTORY ) );
	ui->keepKeys->setChecked( isEnabled( KEEP_KEYS ) );
	ui->keepWeapons->setChecked( isEnabled( KEEP_WEAPONS ) );
	ui->keepArmor->setChecked( isEnabled( KEEP_ARMOR ) );
	ui->keepPowerups->setChecked( isEnabled( KEEP_POWERUPS ) );
	ui->keepAmmo->setChecked( isEnabled( KEEP_AMMO ) );
	ui->loseHalfAmmo->setChecked( isEnabled( LOSE_HALF_AMMO ) );
	ui->spawnWhereDied->setChecked( isEnabled( SPAWN_WHERE_DIED ) );
}
