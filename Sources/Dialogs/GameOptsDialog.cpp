//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the Game Options dialog
//======================================================================================================================

#include "GameOptsDialog.hpp"
#include "ui_GameOptsDialog.h"

#include "Utils/LangUtils.hpp"  // flag utils

#include <QString>
#include <QStringView>
#include <QLineEdit>
#include <QIntValidator>


//======================================================================================================================

namespace dmflags {

enum Field
{
	DmFlags1,
	DmFlags2,
	DmFlags3,
};

enum Effect
{
	Direct,     ///< means that setting the bit to 1 does exactly what the name says
	Inverted,   ///< means that setting the bit to 1 does the opposite of what the name says
};

struct FlagDef
{
	/// Which one of the dmflags fields this flag belongs to.
	Field field;

	/// Which one of the 32 bits this flag occupies.
	GameFlags bit;

	/// Whether the bit flag enables or disables what its name says.
	/** FlagEnables means the bit does exactly what the name says, FlagDisables means it does exactly the opposite.
	  * Example: When the flag name is "Allow exit", then setting the flags to 00000100 00000000 would "Disable exit".
	  * All bit flags are 0 by default. */
	Effect effect;

	/// Long description of what the flag does.
	QStringView description;
};

#define FLAG_DEF static constexpr FlagDef

// dmflags1
FLAG_DEF Allow_health                   = { DmFlags1, (1 <<  0), Inverted, u"When unchecked, no health items will be spawned on the next map loaded. (This also includes the berserk and the megasphere.)" };
FLAG_DEF Allow_powerups                 = { DmFlags1, (1 <<  1), Inverted, u"When unchecked, no powerups or artifacts will be spawned on the next map loaded." };
FLAG_DEF Weapons_stay                   = { DmFlags1, (1 <<  2), Direct,   u"Weapons will not disappear when a player picks them up. This does not apply to weapons dropped by monsters or other players." };
FLAG_DEF Falling_damage                 = { DmFlags1, (1 <<  3), Direct,   u"Damages the player when they fall too far; uses old ZDoom damage calculation" };
//FLAG_DEF Falling_damage_hexen         = { DmFlags1, (1 <<  4), Direct,   u"Falling too far hurts (Hexen style)" };
//FLAG_DEF Falling_damage_strife        = { DmFlags1, (1 <<  5), Direct,   u"Falling too far hurts (Strife style)" };
FLAG_DEF Same_map                       = { DmFlags1, (1 <<  6), Direct,   u"The level is restarted after the exit intermission, instead of moving on to the next map. The only way to go to a different level is with the changemap command." };
FLAG_DEF Spawn_farthest                 = { DmFlags1, (1 <<  7), Direct,   u"Game will attempt to respawn players at the spawn point the farthest away from other players." };
FLAG_DEF Force_respawn                  = { DmFlags1, (1 <<  8), Direct,   u"This automatically respawns players after a few seconds instead of letting them wait as long as they want." };
FLAG_DEF Allow_armor                    = { DmFlags1, (1 <<  9), Inverted, u"When unchecked, no armor will be spawned on the next map loaded. (This also includes the megasphere.)" };
FLAG_DEF Allow_exit                     = { DmFlags1, (1 << 10), Inverted, u"If exit is disallowed, activating an exit line (switch or teleporter) kills the player instead; the level can only be left once the time limit or frag limit hits." };
FLAG_DEF Infinite_ammo                  = { DmFlags1, (1 << 11), Direct,   u"Firing a weapon will not use any ammo." };
FLAG_DEF No_monsters                    = { DmFlags1, (1 << 12), Direct,   u"Enables or disables monsters in the level." };
FLAG_DEF Monsters_respawn               = { DmFlags1, (1 << 13), Direct,   u"Monsters will respawn after they have been killed, regardless of skill settings." };
FLAG_DEF Items_respawn                  = { DmFlags1, (1 << 14), Direct,   u"Items will respawn after they have been picked up." };
FLAG_DEF Fast_monsters                  = { DmFlags1, (1 << 15), Direct,   u"Monsters are more aggressive, and both they and projectiles use their FastSpeed property instead of their speed; as if a skill with the FastMonsters property was used." };
FLAG_DEF Allow_jump_always_off          = { DmFlags1, (1 << 16), Direct,   u"Allows or disallows jumping. When 'partially checked', it uses the MAPINFO settings, while 'checked' and 'unchecked' override them." };
FLAG_DEF Allow_jump_always_on           = { DmFlags1, (1 << 17), Direct,   u"" };
FLAG_DEF Allow_freelook_always_off      = { DmFlags1, (1 << 18), Direct,   u"Allows or disallows looking up and down. When 'partially checked', it uses the MAPINFO settings, while 'checked' and 'unchecked' override them." };
FLAG_DEF Allow_freelook_always_on       = { DmFlags1, (1 << 19), Direct,   u"" };
FLAG_DEF Allow_FOV                      = { DmFlags1, (1 << 20), Inverted, u"When unchecked, only the arbitrator will be able to set the horizontal field-of-view with the fov command, and the arbitrator's FOV will be used for all players." };
FLAG_DEF Spawn_multi_weapons            = { DmFlags1, (1 << 21), Inverted, u"Weapons that are flagged to appear only in multiplayer are not spawned when playing in cooperative mode." };
FLAG_DEF Allow_crouch_always_off        = { DmFlags1, (1 << 22), Direct,   u"Allows or disallows crouching. When 'partially checked', it uses the MAPINFO settings, while 'checked' and 'unchecked' override them." };
FLAG_DEF Allow_crouch_always_on         = { DmFlags1, (1 << 23), Direct,   u"" };
FLAG_DEF Lose_entire_inventory          = { DmFlags1, (1 << 24), Direct,   u"Player's inventory (including ammo, armor, keys, powerups, and weapons) is reset to normal starting conditions when respawning after death." };
FLAG_DEF Keep_keys                      = { DmFlags1, (1 << 25), Inverted, u"If unchecked, the respawning player's keys are reset to normal starting conditions in cooperative mode." };
FLAG_DEF Keep_weapons                   = { DmFlags1, (1 << 26), Inverted, u"If unchecked, the respawning player's weapons are reset to normal starting conditions in cooperative mode." };
FLAG_DEF Keep_armor                     = { DmFlags1, (1 << 27), Inverted, u"If unchecked, the respawning player's armor is reset to normal starting conditions in cooperative mode." };
FLAG_DEF Keep_powerups                  = { DmFlags1, (1 << 28), Inverted, u"If unchecked, the respawning player's powerups are reset to normal starting conditions in cooperative mode." };
FLAG_DEF Keep_ammo                      = { DmFlags1, (1 << 29), Inverted, u"If unchecked, the respawning player's ammo counts are reset to normal starting conditions in cooperative mode." };
FLAG_DEF Lose_half_ammo                 = { DmFlags1, (1 << 30), Direct,   u"Players respawn with half the ammo they had when they died (but not less than the normal starting amount)." };

// dmflags2
//FLAG_DEF Impaling                     = { DmFlags2, (1 <<  0), Direct,   u"Player gets impaled on MF2_IMPALE items" };
FLAG_DEF Drop_weapon                    = { DmFlags2, (1 <<  1), Direct,   u"Players will drop their weapons when they die." };
//FLAG_DEF No_runes                     = { DmFlags2, (1 <<  2), Direct,   u"Don't spawn runes" };
//FLAG_DEF Instant_return               = { DmFlags2, (1 <<  3), Direct,   u"Instantly return flags and skulls when player carrying it dies (ST/CTF)" };
FLAG_DEF No_team_switching              = { DmFlags2, (1 <<  4), Direct,   u"Players cannot change teams in a teamplay match after the map has started." };
//FLAG_DEF No_team_select               = { DmFlags2, (1 <<  5), Direct,   u"Player is automatically placed on a team." };
FLAG_DEF Double_ammo                    = { DmFlags2, (1 <<  6), Direct,   u"Ammo pickups provide twice as much ammo as normal. (The actual multiplier can be modified by the DoubleAmmoFactor property of custom skills.)" };
FLAG_DEF Degeneration                   = { DmFlags2, (1 <<  7), Direct,   u"A player's health above the normal maximum will decrease every second until it falls back to said normal maximum. Health degeneration is normally one point per second, but if the player's health is less than five points above the maximum, it will snap back instantly." };
FLAG_DEF Allow_BFG_aiming               = { DmFlags2, (1 <<  8), Inverted, u"When unchecked, prevents manual aiming of the BFG9000. It will still aim up or down if you shoot it at something, but you will not be able to, for instance, shoot it at the ground. (A common trick consists in aiming at the ground so the ball explodes sooner and opponents have less time to move away from the hitscan tracers.)" };
FLAG_DEF Barrels_respawn                = { DmFlags2, (1 <<  9), Direct,   u"Allows barrels, or any other actor calling A_BarrelDestroy to respawn after destruction." };
FLAG_DEF Respawn_protection             = { DmFlags2, (1 << 10), Direct,   u"This gives a few seconds of invulnerability to respawning players in order to prevent \"spawn camping\"." };
//FLAG_DEF Shotgun_start                = { DmFlags2, (1 << 11), Direct,   u"All playres start with a shotgun when they respawn" };
FLAG_DEF Spawn_where_died               = { DmFlags2, (1 << 12), Direct,   u"A player respawns at the place of death (unless it was in an instant-death sector) instead of at the player start." };
FLAG_DEF Keep_frags_gained              = { DmFlags2, (1 << 13), Direct,   u"Players keep their frag count from one map to the next." };
FLAG_DEF No_respawn                     = { DmFlags2, (1 << 14), Direct,   u"Dead players are not allowed to respawn." };
FLAG_DEF Lose_frag_if_fragged           = { DmFlags2, (1 << 15), Direct,   u"Player's frag count is decreased each time this player is killed." };
FLAG_DEF Infinite_inventory             = { DmFlags2, (1 << 16), Direct,   u"Using an inventory item will not expend it." };
FLAG_DEF No_monsters_to_exit            = { DmFlags2, (1 << 17), Direct,   u"Exiting the level is not possible as long as there remain monsters." };
FLAG_DEF Allow_automap                  = { DmFlags2, (1 << 18), Inverted, u"If unchecked, automap is disabled for all players." };
FLAG_DEF Automap_allies                 = { DmFlags2, (1 << 19), Inverted, u"Selects whether allies are shown on the automap." };
FLAG_DEF Allow_spying                   = { DmFlags2, (1 << 20), Inverted, u"Allows or disallows spying on other players." };
FLAG_DEF Chasecam_cheat                 = { DmFlags2, (1 << 21), Direct,   u"Permits to use the chasecam (third-person camera) even if sv_cheats is off." };
FLAG_DEF Allow_suicide                  = { DmFlags2, (1 << 22), Inverted, u"If disabled, forbids to use the 'kill' command to commit suicide." };
FLAG_DEF Allow_auto_aim                 = { DmFlags2, (1 << 23), Inverted, u"If unchecked, autoaim is disabled for all players." };
FLAG_DEF Check_ammo_for_weapon_switch   = { DmFlags2, (1 << 24), Inverted, u"Chooses whether having ammunition in your inventory is needed to be able to switch to a weapon." };
FLAG_DEF Icons_death_kills_its_spawns   = { DmFlags2, (1 << 25), Direct,   u"This option makes it so the death of the BossBrain kill all monsters created by the BossEye before ending the level, allowing a 100% kill tally on the intermission screen." };
FLAG_DEF End_sector_counts_for_kill     = { DmFlags2, (1 << 26), Inverted, u"This option makes monsters placed in sectors with the dDamage_End type (as used in Doom E1M8) not count towards the total." };
FLAG_DEF Big_powerups_respawn           = { DmFlags2, (1 << 27), Direct,   u"Items with the INVENTORY.BIGPOWERUP flag such as Doom's invulnerability sphere and blur sphere will be able to respawn like regular items." };

// new options added in GZDoom 4.11.0 and later
FLAG_DEF No_player_clip                 = { DmFlags3, (1 <<  0), Direct,   u"(Since GZDoom 4.11.0) Players can walk through and shoot through each other." };
FLAG_DEF Coop_shared_keys               = { DmFlags3, (1 <<  1), Direct,   u"(Since GZDoom 4.12.0) Picking up a key in cooperative mode will distribute it to all players." };
FLAG_DEF Local_items                    = { DmFlags3, (1 <<  2), Direct,   u"(Since GZDoom 4.12.0) Items are picked up client-side rather than fully taken by the client who picked it up." };
//FLAG_DEF No_local_drops                 = { DmFlags3, (1 <<  3), Direct,   u"(Since GZDoom 4.12.0) Drops from Actors aren't picked up locally." };
//FLAG_DEF No_coop_items                  = { DmFlags3, (1 <<  4), Direct,   u"(Since GZDoom 4.12.0) Items that only appear in co-op are disabled." };
//FLAG_DEF No_coop_things                 = { DmFlags3, (1 <<  5), Direct,   u"(Since GZDoom 4.12.0) Any Actor that only appears in co-op is disabled." };
//FLAG_DEF Remember_last_weapon           = { DmFlags3, (1 <<  6), Direct,   u"(Since GZDoom 4.12.0) When respawning in co-op, keep the last used weapon out instead of switching to the best new one." };
FLAG_DEF Pistol_start                   = { DmFlags3, (1 <<  7), Direct,   u"(Since GZDoom 4.12.2) Every level is a fresh start, with a pistol only." };

#undef FLAG_DEF

} // namespace dmflags


//======================================================================================================================
// GameOptsDialog

using namespace dmflags;

GameOptsDialog::GameOptsDialog( QWidget * parent, const GameplayDetails & gameplayDetails )
:
	QDialog( parent ),
	DialogCommon( this, u"GameOptsDialog" ),
	gameplayDetails( gameplayDetails )
{
	ui = new Ui::GameOptsDialog;
	ui->setupUi( this );
	setupTooltips();

	ui->dmflags1_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );
	ui->dmflags1_line->setText( QString::number( gameplayDetails.dmflags1 ) );

	ui->dmflags2_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );
	ui->dmflags2_line->setText( QString::number( gameplayDetails.dmflags2 ) );

	ui->dmflags3_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );
	ui->dmflags3_line->setText( QString::number( gameplayDetails.dmflags3 ) );

	updateCheckboxes();

	connect( ui->buttonBox, &QDialogButtonBox::accepted, this, &ThisClass::accept );
	connect( ui->buttonBox, &QDialogButtonBox::rejected, this, &ThisClass::reject );
}

GameOptsDialog::~GameOptsDialog()
{
	delete ui;
}

void GameOptsDialog::setupTooltips()
{
	// dmflags1
	ui->allowHealth->setToolTip( Allow_health.description.toString() );
	ui->allowPowerups->setToolTip( Allow_powerups.description.toString() );
	ui->weaponsStay->setToolTip( Weapons_stay.description.toString() );
	ui->fallingDamage->setToolTip( Falling_damage.description.toString() );
	ui->sameMap->setToolTip( Same_map.description.toString() );
	ui->spawnFarthest->setToolTip( Spawn_farthest.description.toString() );
	ui->forceRespawn->setToolTip( Force_respawn.description.toString() );
	ui->allowArmor->setToolTip( Allow_armor.description.toString() );
	ui->allowExit->setToolTip( Allow_exit.description.toString() );
	ui->infAmmo->setToolTip( Infinite_ammo.description.toString() );
	ui->noMonsters->setToolTip( No_monsters.description.toString() );
	ui->monstersRespawn->setToolTip( Monsters_respawn.description.toString() );
	ui->itemsRespawn->setToolTip( Items_respawn.description.toString() );
	ui->fastMonsters->setToolTip( Fast_monsters.description.toString() );
	ui->allowJump->setToolTip( Allow_jump_always_off.description.toString() );
	ui->allowFreelook->setToolTip( Allow_freelook_always_off.description.toString() );
	ui->allowFOV->setToolTip( Allow_FOV.description.toString() );
	ui->spawnMultiWeapons->setToolTip( Spawn_multi_weapons.description.toString() );
	ui->allowCrouch->setToolTip( Allow_crouch_always_off.description.toString() );
	ui->loseEntireInventory->setToolTip( Lose_entire_inventory.description.toString() );
	ui->keepKeys->setToolTip( Keep_keys.description.toString() );
	ui->keepWeapons->setToolTip( Keep_weapons.description.toString() );
	ui->keepArmor->setToolTip( Keep_armor.description.toString() );
	ui->keepPowerups->setToolTip( Keep_powerups.description.toString() );
	ui->keepAmmo->setToolTip( Keep_ammo.description.toString() );
	ui->loseHalfAmmo->setToolTip( Lose_half_ammo.description.toString() );

	// dmflags2
	ui->dropWeapon->setToolTip( Drop_weapon.description.toString() );
	ui->noTeamSwitching->setToolTip( No_team_switching.description.toString() );
	ui->doubleAmmo->setToolTip( Double_ammo.description.toString() );
	ui->degeneration->setToolTip( Degeneration.description.toString() );
	ui->allowBFGAiming->setToolTip( Allow_BFG_aiming.description.toString() );
	ui->barrelsRespawn->setToolTip( Barrels_respawn.description.toString() );
	ui->respawnProtection->setToolTip( Respawn_protection.description.toString() );
	ui->spawnWhereDied->setToolTip( Spawn_where_died.description.toString() );
	ui->keepFragsGained->setToolTip( Keep_frags_gained.description.toString() );
	ui->noRespawn->setToolTip( No_respawn.description.toString() );
	ui->loseFragIfFragged->setToolTip( Lose_frag_if_fragged.description.toString() );
	ui->infInventory->setToolTip( Infinite_inventory.description.toString() );
	ui->noMonstersToExit->setToolTip( No_monsters_to_exit.description.toString() );
	ui->allowAutomap->setToolTip( Allow_automap.description.toString() );
	ui->automapAllies->setToolTip( Automap_allies.description.toString() );
	ui->allowSpying->setToolTip( Allow_spying.description.toString() );
	ui->chasecamCheat->setToolTip( Chasecam_cheat.description.toString() );
	ui->allowSuicide->setToolTip( Allow_suicide.description.toString() );
	ui->allowAutoAim->setToolTip( Allow_auto_aim.description.toString() );
	ui->checkAmmoForWeaponSwitch->setToolTip( Check_ammo_for_weapon_switch.description.toString() );
	ui->iconsDeathKillsItsSpawns->setToolTip( Icons_death_kills_its_spawns.description.toString() );
	ui->endSectorCountsForKill->setToolTip( End_sector_counts_for_kill.description.toString() );
	ui->bigPowerupsRespawn->setToolTip( Big_powerups_respawn.description.toString() );

	// dmflags3
	ui->noPlayerClipping->setToolTip( No_player_clip.description.toString() );
	ui->shareKeys->setToolTip( Coop_shared_keys.description.toString() );
	ui->localItemPickups->setToolTip( Local_items.description.toString() );
	ui->pistolStart->setToolTip( Pistol_start.description.toString() );
}


//----------------------------------------------------------------------------------------------------------------------
// utils

static inline const GameFlags & getFlagsField( const GameplayDetails & gameplayDetails, const FlagDef & flag )
{
	// The GameplayDetails are just 3 consecutive int32_t fields, we can use it as an array to elegantly avoid branches.
	const GameFlags (& dmflagsFields)[3] = reinterpret_cast< const GameFlags (&)[3] >( gameplayDetails );  // reference to an array of 3 int32_t
	return dmflagsFields[ fut::to_underlying( flag.field ) ];
}
static inline GameFlags & getFlagsField( GameplayDetails & gameplayDetails, const FlagDef & flag )
{
	return unconst( getFlagsField( std::as_const( gameplayDetails ), flag ) );
}

static inline QLineEdit * getFlagsLine( Ui::GameOptsDialog * ui, const FlagDef & flag )
{
	QLineEdit * dmflagsLines [3] =
	{
		ui->dmflags1_line,
		ui->dmflags2_line,
		ui->dmflags3_line
	};
	return dmflagsLines[ fut::to_underlying( flag.field ) ];
}

void GameOptsDialog::toggleFlag( const FlagDef & flag, bool checked )
{
	GameFlags & dmflagsField = getFlagsField( gameplayDetails, flag );
	QLineEdit * dmflagsLine = getFlagsLine( ui, flag );

	bool newFlagState = checked != (flag.effect == Inverted);  // if flag inverts, then !checked, else checked
	toggleFlags( dmflagsField, flag.bit, newFlagState );

	dmflagsLine->setText( QString::number( dmflagsField ) );
}

bool GameOptsDialog::isChecked( const FlagDef & flag ) const
{
	const GameFlags & dmflagsField = getFlagsField( gameplayDetails, flag );

	bool flagState = areFlagsSet( dmflagsField, flag.bit );
	return flagState != (flag.effect == Inverted);  // if flag inverts, then !flagState, else flagState
}


//----------------------------------------------------------------------------------------------------------------------
// checkboxes to numbers conversion

void GameOptsDialog::on_allowHealth_toggled( bool checked )
{
	toggleFlag( Allow_health, checked );
}

void GameOptsDialog::on_allowPowerups_toggled( bool checked )
{
	toggleFlag( Allow_powerups, checked );
}

void GameOptsDialog::on_weaponsStay_toggled( bool checked )
{
	toggleFlag( Weapons_stay, checked );
}

void GameOptsDialog::on_fallingDamage_toggled( bool checked )
{
	toggleFlag( Falling_damage, checked );
}

void GameOptsDialog::on_sameMap_toggled( bool checked )
{
	toggleFlag( Same_map, checked );
}

void GameOptsDialog::on_spawnFarthest_toggled( bool checked )
{
	toggleFlag( Spawn_farthest, checked );
}

void GameOptsDialog::on_forceRespawn_toggled( bool checked )
{
	toggleFlag( Force_respawn, checked );
}

void GameOptsDialog::on_allowArmor_toggled( bool checked )
{
	toggleFlag( Allow_armor, checked );
}

void GameOptsDialog::on_allowExit_toggled( bool checked )
{
	toggleFlag( Allow_exit, checked );
}

void GameOptsDialog::on_infAmmo_toggled( bool checked )
{
	toggleFlag( Infinite_ammo, checked );
}

void GameOptsDialog::on_noMonsters_toggled( bool checked )
{
	toggleFlag( No_monsters, checked );
}

void GameOptsDialog::on_monstersRespawn_toggled( bool checked )
{
	toggleFlag( Monsters_respawn, checked );
}

void GameOptsDialog::on_itemsRespawn_toggled( bool checked )
{
	toggleFlag( Items_respawn, checked );
}

void GameOptsDialog::on_fastMonsters_toggled( bool checked )
{
	toggleFlag( Fast_monsters, checked );
}

void GameOptsDialog::on_allowJump_stateChanged( int state )
{
	if (state == Qt::Unchecked)
	{
		toggleFlag( Allow_jump_always_on, false );
		toggleFlag( Allow_jump_always_off, true );
	}
	else if (state == Qt::PartiallyChecked)
	{
		toggleFlag( Allow_jump_always_on, false );
		toggleFlag( Allow_jump_always_off, false );
	}
	else if (state == Qt::Checked)
	{
		toggleFlag( Allow_jump_always_on, true );
		toggleFlag( Allow_jump_always_off, false );
	}
}

void GameOptsDialog::on_allowFreelook_stateChanged( int state )
{
	if (state == Qt::Unchecked)
	{
		toggleFlag( Allow_freelook_always_on, false );
		toggleFlag( Allow_freelook_always_off, true );
	}
	else if (state == Qt::PartiallyChecked)
	{
		toggleFlag( Allow_freelook_always_on, false );
		toggleFlag( Allow_freelook_always_off, false );
	}
	else if (state == Qt::Checked)
	{
		toggleFlag( Allow_freelook_always_on, true );
		toggleFlag( Allow_freelook_always_off, false );
	}
}

void GameOptsDialog::on_allowFOV_toggled( bool checked )
{
	toggleFlag( Allow_FOV, checked );
}

void GameOptsDialog::on_spawnMultiWeapons_toggled( bool checked )
{
	toggleFlag( Spawn_multi_weapons, checked );
}

void GameOptsDialog::on_allowCrouch_stateChanged( int state )
{
	if (state == Qt::Unchecked)
	{
		toggleFlag( Allow_crouch_always_on, false );
		toggleFlag( Allow_crouch_always_off, true );
	}
	else if (state == Qt::PartiallyChecked)
	{
		toggleFlag( Allow_crouch_always_on, false );
		toggleFlag( Allow_crouch_always_off, false );
	}
	else if (state == Qt::Checked)
	{
		toggleFlag( Allow_crouch_always_on, true );
		toggleFlag( Allow_crouch_always_off, false );
	}
}

void GameOptsDialog::on_loseEntireInventory_toggled( bool checked )
{
	toggleFlag( Lose_entire_inventory, checked );
}

void GameOptsDialog::on_keepKeys_toggled( bool checked )
{
	toggleFlag( Keep_keys, checked );
}

void GameOptsDialog::on_keepWeapons_toggled( bool checked )
{
	toggleFlag( Keep_weapons, checked );
}

void GameOptsDialog::on_keepArmor_toggled( bool checked )
{
	toggleFlag( Keep_armor, checked );
}

void GameOptsDialog::on_keepPowerups_toggled( bool checked )
{
	toggleFlag( Keep_powerups, checked );
}

void GameOptsDialog::on_keepAmmo_toggled( bool checked )
{
	toggleFlag( Keep_ammo, checked );
}

void GameOptsDialog::on_loseHalfAmmo_toggled( bool checked )
{
	toggleFlag( Lose_half_ammo, checked );
}

void GameOptsDialog::on_dropWeapon_toggled( bool checked )
{
	toggleFlag( Drop_weapon, checked );
}

void GameOptsDialog::on_noTeamSwitching_toggled( bool checked )
{
	toggleFlag( No_team_switching, checked );
}

void GameOptsDialog::on_doubleAmmo_toggled( bool checked )
{
	toggleFlag( Double_ammo, checked );
}

void GameOptsDialog::on_degeneration_toggled( bool checked )
{
	toggleFlag( Degeneration, checked );
}

void GameOptsDialog::on_allowBFGAiming_toggled( bool checked )
{
	toggleFlag( Allow_BFG_aiming, checked );
}

void GameOptsDialog::on_barrelsRespawn_toggled( bool checked )
{
	toggleFlag( Barrels_respawn, checked );
}

void GameOptsDialog::on_respawnProtection_toggled( bool checked )
{
	toggleFlag( Respawn_protection, checked );
}

void GameOptsDialog::on_spawnWhereDied_toggled( bool checked )
{
	toggleFlag( Spawn_where_died, checked );
}

void GameOptsDialog::on_keepFragsGained_toggled( bool checked )
{
	toggleFlag( Keep_frags_gained, checked );
}

void GameOptsDialog::on_noRespawn_toggled( bool checked )
{
	toggleFlag( No_respawn, checked );
}

void GameOptsDialog::on_loseFragIfFragged_toggled( bool checked )
{
	toggleFlag( Lose_frag_if_fragged, checked );
}

void GameOptsDialog::on_infInventory_toggled( bool checked )
{
	toggleFlag( Infinite_inventory, checked );
}

void GameOptsDialog::on_noMonstersToExit_toggled( bool checked )
{
	toggleFlag( No_monsters_to_exit, checked );
}

void GameOptsDialog::on_allowAutomap_toggled( bool checked )
{
	toggleFlag( Allow_automap, checked );
}

void GameOptsDialog::on_automapAllies_toggled( bool checked )
{
	toggleFlag( Automap_allies, checked );
}

void GameOptsDialog::on_allowSpying_toggled( bool checked )
{
	toggleFlag( Allow_spying, checked );
}

void GameOptsDialog::on_chasecamCheat_toggled( bool checked )
{
	toggleFlag( Chasecam_cheat, checked );
}

void GameOptsDialog::on_allowSuicide_toggled( bool checked )
{
	toggleFlag( Allow_suicide, checked );
}

void GameOptsDialog::on_allowAutoAim_toggled( bool checked )
{
	toggleFlag( Allow_auto_aim, checked );
}

void GameOptsDialog::on_checkAmmoForWeaponSwitch_toggled( bool checked )
{
	toggleFlag( Check_ammo_for_weapon_switch, checked );
}

void GameOptsDialog::on_iconsDeathKillsItsSpawns_toggled( bool checked )
{
	toggleFlag( Icons_death_kills_its_spawns, checked );
}

void GameOptsDialog::on_endSectorCountsForKill_toggled( bool checked )
{
	toggleFlag( End_sector_counts_for_kill, checked );
}

void GameOptsDialog::on_bigPowerupsRespawn_toggled( bool checked )
{
	toggleFlag( Big_powerups_respawn, checked );
}

void GameOptsDialog::on_noPlayerClipping_toggled( bool checked )
{
	toggleFlag( No_player_clip, checked );
}

void GameOptsDialog::on_shareKeys_toggled( bool checked )
{
	toggleFlag( Coop_shared_keys, checked );
}

void GameOptsDialog::on_localItemPickups_toggled( bool checked )
{
	toggleFlag( Local_items, checked );
}

void GameOptsDialog::on_pistolStart_toggled( bool checked )
{
	toggleFlag( Pistol_start, checked );
}


//----------------------------------------------------------------------------------------------------------------------
// numbers to checkboxes conversion

void GameOptsDialog::on_dmflags1_line_textEdited( const QString & )
{
	gameplayDetails.dmflags1 = ui->dmflags1_line->text().toInt();
	updateCheckboxes();
}

void GameOptsDialog::on_dmflags2_line_textEdited( const QString & )
{
	gameplayDetails.dmflags2 = ui->dmflags2_line->text().toInt();
	updateCheckboxes();
}

void GameOptsDialog::on_dmflags3_line_textEdited( const QString & )
{
	gameplayDetails.dmflags3 = ui->dmflags3_line->text().toInt();
	updateCheckboxes();
}

void GameOptsDialog::updateCheckboxes()
{
	// dmflags1
	ui->allowHealth->setChecked( isChecked( Allow_health ) );
	ui->allowPowerups->setChecked( isChecked( Allow_powerups ) );
	ui->weaponsStay->setChecked( isChecked( Weapons_stay ) );
	ui->fallingDamage->setChecked( isChecked( Falling_damage ) );
	ui->sameMap->setChecked( isChecked( Same_map ) );
	ui->spawnFarthest->setChecked( isChecked( Spawn_farthest ) );
	ui->forceRespawn->setChecked( isChecked( Force_respawn ) );
	ui->allowArmor->setChecked( isChecked( Allow_armor ) );
	ui->allowExit->setChecked( isChecked( Allow_exit ) );
	ui->infAmmo->setChecked( isChecked( Infinite_ammo ) );
	ui->noMonsters->setChecked( isChecked( No_monsters ) );
	ui->monstersRespawn->setChecked( isChecked( Monsters_respawn ) );
	ui->itemsRespawn->setChecked( isChecked( Items_respawn ) );
	ui->fastMonsters->setChecked( isChecked( Fast_monsters ) );
	if (isChecked( Allow_jump_always_off ))
		ui->allowJump->setCheckState( Qt::Unchecked );
	else if (isChecked( Allow_jump_always_on ))
		ui->allowJump->setCheckState( Qt::Checked );
	else
		ui->allowJump->setCheckState( Qt::PartiallyChecked );
	if (isChecked( Allow_freelook_always_off ))
		ui->allowFreelook->setCheckState( Qt::Unchecked );
	else if (isChecked( Allow_freelook_always_on ))
		ui->allowFreelook->setCheckState( Qt::Checked );
	else
		ui->allowFreelook->setCheckState( Qt::PartiallyChecked );
	ui->allowFOV->setChecked( isChecked( Allow_FOV ) );
	ui->spawnMultiWeapons->setChecked( isChecked( Spawn_multi_weapons ) );
	if (isChecked( Allow_crouch_always_off ))
		ui->allowCrouch->setCheckState( Qt::Unchecked );
	else if (isChecked( Allow_crouch_always_on ))
		ui->allowCrouch->setCheckState( Qt::Checked );
	else
		ui->allowCrouch->setCheckState( Qt::PartiallyChecked );
	ui->loseEntireInventory->setChecked( isChecked( Lose_entire_inventory ) );
	ui->keepKeys->setChecked( isChecked( Keep_keys ) );
	ui->keepWeapons->setChecked( isChecked( Keep_weapons ) );
	ui->keepArmor->setChecked( isChecked( Keep_armor ) );
	ui->keepPowerups->setChecked( isChecked( Keep_powerups ) );
	ui->keepAmmo->setChecked( isChecked( Keep_ammo ) );
	ui->loseHalfAmmo->setChecked( isChecked( Lose_half_ammo ) );

	// dmflags2
	ui->dropWeapon->setChecked( isChecked( Drop_weapon ) );
	ui->noTeamSwitching->setChecked( isChecked( No_team_switching ) );
	ui->doubleAmmo->setChecked( isChecked( Double_ammo ) );
	ui->degeneration->setChecked( isChecked( Degeneration ) );
	ui->allowBFGAiming->setChecked( isChecked( Allow_BFG_aiming ) );
	ui->barrelsRespawn->setChecked( isChecked( Barrels_respawn ) );
	ui->respawnProtection->setChecked( isChecked( Respawn_protection ) );
	ui->spawnWhereDied->setChecked( isChecked( Spawn_where_died ) );
	ui->keepFragsGained->setChecked( isChecked( Keep_frags_gained ) );
	ui->noRespawn->setChecked( isChecked( No_respawn ) );
	ui->loseFragIfFragged->setChecked( isChecked( Lose_frag_if_fragged ) );
	ui->infInventory->setChecked( isChecked( Infinite_inventory ) );
	ui->noMonstersToExit->setChecked( isChecked( No_monsters_to_exit ) );
	ui->allowAutomap->setChecked( isChecked( Allow_automap ) );
	ui->automapAllies->setChecked( isChecked( Automap_allies ) );
	ui->allowSpying->setChecked( isChecked( Allow_spying ) );
	ui->chasecamCheat->setChecked( isChecked( Chasecam_cheat ) );
	ui->allowSuicide->setChecked( isChecked( Allow_suicide ) );
	ui->allowAutoAim->setChecked( isChecked( Allow_auto_aim ) );
	ui->checkAmmoForWeaponSwitch->setChecked( isChecked( Check_ammo_for_weapon_switch ) );
	ui->iconsDeathKillsItsSpawns->setChecked( isChecked( Icons_death_kills_its_spawns ) );
	ui->endSectorCountsForKill->setChecked( isChecked( End_sector_counts_for_kill ) );
	ui->bigPowerupsRespawn->setChecked( isChecked( Big_powerups_respawn ) );

	// dmflags3
	ui->noPlayerClipping->setChecked( isChecked( No_player_clip ) );
	ui->shareKeys->setChecked( isChecked( Coop_shared_keys ) );
	ui->localItemPickups->setChecked( isChecked( Local_items ) );
	ui->pistolStart->setChecked( isChecked( Pistol_start ) );
}
