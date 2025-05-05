//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the Compatibility Options dialog
//======================================================================================================================

#include "CompatOptsDialog.hpp"
#include "ui_CompatOptsDialog.h"

#include "Utils/LangUtils.hpp"  // flag utils

#include <QString>
#include <QStringView>
#include <QTextStream>
#include <QLineEdit>
#include <QIntValidator>


//======================================================================================================================

namespace compatflags {

enum Field
{
	CompatFlags1,
	CompatFlags2,
};

struct FlagDef
{
	/// Which one of the compatflags fields this flag belongs to.
	Field field;

	/// Which one of the 32 bits this flag occupies.
	GameFlags bit;

	/// Which engine's CVar is used to enable this option.
	QStringView cvarName;

	/// Long description of what the option does.
	QStringView description;
};

#define FLAG_DEF static constexpr FlagDef

// compatflags 1
FLAG_DEF Find_shortest_textures    = { CompatFlags1, (1 <<  0), u"compat_shorttex",        u"Doom treats the first texture in the TEXTURE1 lump as no texture at all. But the function that looks for the shortest texture ignored that and checked texture n°0 as well. This option re-enables this buggy behavior for old WADs that require it." };
FLAG_DEF Use_buggier_stair         = { CompatFlags1, (1 <<  1), u"compat_stairs",          u"When looking for all tagged sectors to build stairs, Doom.exe resumed the search with the last sector of the current stair, not the one following the starting sector." };
FLAG_DEF Limit_Pain_Elem           = { CompatFlags1, (1 <<  2), u"compat_limitpain",       u"Vanilla Doom prevents a pain elemental from spawning lost souls if there are already 21 in the level. This limit has been removed in most source ports but there are maps that require it to work properly. For an example, see MAP19 of Hell Revealed." };
FLAG_DEF Dont_let_others_hear      = { CompatFlags1, (1 <<  3), u"compat_silentpickup",    u"Restores Doom's original behavior to play pickup sounds only for the player that is picking up an item. Obviously this option is only useful in multiplayer games." };
FLAG_DEF Actors_are_infinite       = { CompatFlags1, (1 <<  4), u"compat_nopassover",      u"Doom's collision code treated all actors as infinitely tall, i.e. it was impossible to jump over any solid object. Needless to say, with such a setup 3D bridges and similar effects are impossible to create. You can re-enable this effect with this option but be careful: Enabling it will seriously affect any map that expects proper z-sensitive object collision detection." };
FLAG_DEF Cripple_sound             = { CompatFlags1, (1 <<  5), u"compat_soundslots",      u"Use this only if you can't live without the silent BFG trick! This option seriously cripples the sound system so that each actor can only play one sound at once. Note that this will also introduce other sound glitches." };
FLAG_DEF Enable_wall_running       = { CompatFlags1, (1 <<  6), u"compat_wallrun",         u"Re-enables the wallrunning bug. It is not recommended to use this option unless some map absolutely requires it. As it depends on a bug it has serious side effects on the movement code and makes any fast movement become erratic.\nAdditionally, this option reestablish the original sliding code." };
FLAG_DEF Spawn_items_drops         = { CompatFlags1, (1 <<  7), u"compat_notossdrops",     u"Disables drop item tossing." };
FLAG_DEF All_special_lines         = { CompatFlags1, (1 <<  8), u"compat_useblocking",     u"Restores Doom's original behavior of any special line blocking use actions. ZDoom corrects this so that walkover actions or scrolling lines don't block uses." };
FLAG_DEF Disable_boom_door         = { CompatFlags1, (1 <<  9), u"compat_nodoorlight",     u"Disables the Boom/MBF light tag effect for doors. This is to allow maps with incorrectly tagged doors to play properly." };
FLAG_DEF Raven_scrollers           = { CompatFlags1, (1 << 10), u"compat_ravenscroll",     u"The scrolling floor specials in Heretic and Hexen move the player much faster than the actual texture scrolling speed. Enable this option to restore this effect." };
FLAG_DEF Use_original_sound        = { CompatFlags1, (1 << 11), u"compat_soundtarget",     u"ZDoom changed the sound alert handling for monsters in 2.0.90 so that the sound target is no longer preserved by a sector. While this allows more control over the actions taking place in a map it had severe side effects in many maps that spawn monsters while playing. These maps are counting on the monsters being alerted immediately and didn't bother with proper positioning because it didn't matter. This option restores the old alerting method from Doom.exe which was valid up to ZDoom 2.0.63a." };
FLAG_DEF DEH_health_settings       = { CompatFlags1, (1 << 12), u"compat_dehhealth",       u"Boom introduced a DeHackEd bug which applied the maximum health setting to medikits and stim packs. Originally it was only supposed to affect health bonuses. Many ZDoom maps, however, use it to limit the maximum overall health so an unconditional fix is no longer possible. Use this option to play vanilla Doom-compatible WADs with DEHACKED modifications that change the maximum health value." };
FLAG_DEF Self_ref_sectors          = { CompatFlags1, (1 << 13), u"compat_trace",           u"Doom's hitscan tracing code ignores all lines with both sides in the same sector. ZDoom's does not. This option reverts to the original but less precise behavior.\nThis option now also covers the original behavior or sight checks for such lines." };
FLAG_DEF Monsters_get_stuck        = { CompatFlags1, (1 << 14), u"compat_dropoff",         u"This option disables the logic that allows monsters to move away from positions where they hang over a tall dropoff (e.g. the edge of a lift.) Originally a monster got stuck in such a situation. Normally there shouldn't be any need to revert to the old behavior." };
FLAG_DEF Boom_scrollers            = { CompatFlags1, (1 << 15), u"compat_boomscroll",      u"The texture scrolling specials introduced in Boom were originally made to stack with each other. Enable this option to restore that behavior." };
FLAG_DEF Monsters_see_invisible    = { CompatFlags1, (1 << 16), u"compat_invisibility",    u"This option restores Doom's original behavior where monsters would always wake up when seeing a player who is using an invisibility powerup. ZDoom normally uses a more realistic routine where monsters will usually be unable to see those players." };
FLAG_DEF Instant_moving_floors     = { CompatFlags1, (1 << 17), u"compat_silentinstantfloors",  u"ZDoom normally disables the stop sound for floors that move instantly from one height to another. Enable this setting to make the stop sound be played in these special cases." };
FLAG_DEF Sector_sounds             = { CompatFlags1, (1 << 18), u"compat_sectorsounds",    u"Normally when sectors make noise, ZDoom uses the point of that sector which is closest to the listener as the source of the sound. (Ensuring that large sectors make sound all throughout) Enable this option to make ZDoom revert to the old behavior which caused these sounds to originate from the center of their sector." };
FLAG_DEF Use_Doom_heights          = { CompatFlags1, (1 << 19), u"compat_missileclip",     u"If enabled, actors use their original heights for the purposes of projectile collision. This allows for decorations to be pass-through for projectiles as they were originally in Doom while still blocking other actors correctly. Specifically, this affects actors with negative values defined for their ProjectilePassHeight property." };
FLAG_DEF Monsters_cannot_cross     = { CompatFlags1, (1 << 20), u"compat_crossdropoff",    u"The original Doom physics code prevented monsters from being thrown off of ledges that they couldn't step off of voluntarily, even when propelled by an outside force. ZDoom allows monsters to be pushed over these dropoffs by weapons and other forces. This options restores the vanilla Doom behavior." };
FLAG_DEF Allow_any_bossdeath       = { CompatFlags1, (1 << 21), u"compat_anybossdeath",    u"If enabled, any actor type which calls A_BossDeath triggers the level's special, even if they are not supposed to. This emulates a pre-Doom v1.9 behavior which is exploited by Doomsday of UAC" };
FLAG_DEF No_Minotaur_floor         = { CompatFlags1, (1 << 22), u"compat_minotaur",        u"If enabled, maulotaurs are unable to create their floor fire attack if their feet are clipped by water, sludge, lava or other terrain effect. Note that the flames can still travel across water; this was on the part of Raven Software's developers as it was a bug found in the original clipping code and not an attempt at realism as some may have believed." };
FLAG_DEF Original_A_Mushroom       = { CompatFlags1, (1 << 23), u"compat_mushroom",        u"If enabled, when the A_Mushroom codepointer is called from a state that was modified by a DeHackEd lump, it uses the original MBF behavior of the codepointer. This option does not affect states defined in DECORATE." };
FLAG_DEF Monster_movement          = { CompatFlags1, (1 << 24), u"compat_mbfmonstermove",  u"If enabled, monsters are affected by sector friction, wind and pusher/puller effects, as they are in MBF. By default, monsters are not subjected to friction and only affected by wind and pushers/pullers if they have the WINDTHRUST flag." };
FLAG_DEF Crushed_monsters          = { CompatFlags1, (1 << 25), u"compat_corpsegibs",      u"If enabled, corpses under a vertical door or crusher are changed into gibs, rather than replaced by a different actor, if they do not have a custom Crush state. This allows an arch-vile or similar monster to resurrect them. By default, actors without a custom Crush state are removed entirely and can therefore not be raised from the dead." };
FLAG_DEF Friendly_monsters         = { CompatFlags1, (1 << 26), u"compat_noblockfriends",  u"If enabled, friendly monsters are, like in MBF, not affected by lines with the \"block monsters\" flag, allowing them to follow the player all around a map. This option does not, however, block them at lines with the \"block player\" flag." };
FLAG_DEF Invert_sprite_sorting     = { CompatFlags1, (1 << 27), u"compat_spritesort",      u"If enabled, the original Doom sorting order for overlapping sprites is used." };
FLAG_DEF Use_Doom_hitscan          = { CompatFlags1, (1 << 28), u"compat_hitscan",         u"If enabled, the original Doom code for hitscan attacks is used. This reintroduces two bugs which makes hitscan attacks less likely to hit. The first is that it is a monster's cross-section, rather than its bounding box, that is used to check for impact; this makes attacks with a limited range (especially melee attacks) unlikely to hit very wide monsters. The second is the blockmap bug: if an actor crosses block boundaries and its center is in a different block than the one in which the impact happens, then there is no collision at all, letting attacks pass through it harmlessly." };
FLAG_DEF Find_neighboring_light    = { CompatFlags1, (1 << 29), u"compat_light",           u"If enabled, when a light level changes to the highest light level found in neighboring sectors, the search is made only for the first tagged sector, like in Doom." };
FLAG_DEF Draw_polyobjects          = { CompatFlags1, (1 << 30), u"compat_polyobj",         u"Uses the old flawed polyobject system, for maps that relied on its glitches." };
FLAG_DEF Ignore_Y_offsets          = { CompatFlags1, (1 << 31), u"compat_maskedmidtex",    u"This option emulates a vanilla renderer glitch by ignoring the Y locations of patches drawn on two-sided midtextures and instead always drawing them at the top of the texture." };

// compatflags 2
FLAG_DEF Cannot_travel_straight    = { CompatFlags2, (1 <<  0), u"compat_badangles",       u"This option emulates the error in the original engine's sine table by offsetting player angle when spawning or teleporting by one fineangle (approximatively 0.044°), preventing the player from facing directly in a cardinal direction." };
FLAG_DEF Use_Dooms_floor           = { CompatFlags2, (1 <<  1), u"compat_floormove",       u"This option undoes a Boom fix to floor movement logic. If this option is on, a floor may rise through the ceiling, or a ceiling may lower through a floor." };
FLAG_DEF Sounds_stop               = { CompatFlags2, (1 <<  2), u"compat_soundcutoff",     u"This option cuts a sound off if it lasted more than its source's \"life\" in the game world." };
FLAG_DEF Use_Dooms_point_on_line   = { CompatFlags2, (1 <<  3), u"compat_pointonline",     u"" };
FLAG_DEF Level_exit                = { CompatFlags2, (1 <<  4), u"compat_multiexit",       u"" };

#undef FLAG_DEF

} // namespace compatflags


//======================================================================================================================
// CompatOptsDialog

using namespace compatflags;

CompatOptsDialog::CompatOptsDialog( QWidget * parent, const CompatibilityDetails & compatDetails )
:
	QDialog( parent ),
	DialogCommon( this, u"CompatOptsDialog" ),
	compatDetails( compatDetails )
{
	ui = new Ui::CompatOptsDialog;
    ui->setupUi( this );
    setupTooltips();

	ui->compatflags1_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );
	ui->compatflags1_line->setText( QString::number( compatDetails.compatflags1 ) );

	ui->compatflags2_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );
	ui->compatflags2_line->setText( QString::number( compatDetails.compatflags2 ) );

	updateCheckboxes();

	connect( ui->buttonBox, &QDialogButtonBox::accepted, this, &ThisClass::accept );
	connect( ui->buttonBox, &QDialogButtonBox::rejected, this, &ThisClass::reject );
}

CompatOptsDialog::~CompatOptsDialog()
{
    delete ui;
}

void CompatOptsDialog::setupTooltips()
{
	// compatflags 1
	ui->findShortestTextures->setToolTip( Find_shortest_textures.description.toString() );
	ui->useBuggierStair->setToolTip( Use_buggier_stair.description.toString() );
	ui->limitPainElem->setToolTip( Limit_Pain_Elem.description.toString() );
	ui->dontLetOthersHear->setToolTip( Dont_let_others_hear.description.toString() );
	ui->actorsAreInfinite->setToolTip( Actors_are_infinite.description.toString() );
	ui->crippleSound->setToolTip( Cripple_sound.description.toString() );
	ui->enableWallRunning->setToolTip( Enable_wall_running.description.toString() );
	ui->spawnItemDrops->setToolTip( Spawn_items_drops.description.toString() );
	ui->allSpecialLines->setToolTip( All_special_lines.description.toString() );
	ui->disableBoomDoor->setToolTip( Disable_boom_door.description.toString() );
	ui->ravenScrollers->setToolTip( Raven_scrollers.description.toString() );
	ui->useOriginalSound->setToolTip( Use_original_sound.description.toString() );
	ui->dehHealthSettings->setToolTip( DEH_health_settings.description.toString() );
	ui->selfRefSectors->setToolTip( Self_ref_sectors.description.toString() );
	ui->monstersGetStuck->setToolTip( Monsters_get_stuck.description.toString() );
	ui->boomScrollers->setToolTip( Boom_scrollers.description.toString() );
	ui->monstersSeeInvisible->setToolTip( Monsters_see_invisible.description.toString() );
	ui->instantMovingFloors->setToolTip( Instant_moving_floors.description.toString() );
	ui->sectorSounds->setToolTip( Sector_sounds.description.toString() );
	ui->useDoomHeights->setToolTip( Use_Doom_heights.description.toString() );
	ui->monstersCannotCross->setToolTip( Monsters_cannot_cross.description.toString() );
	ui->allowAnyBossdeath->setToolTip( Allow_any_bossdeath.description.toString() );
	ui->noMinotaurFloor->setToolTip( No_Minotaur_floor.description.toString() );
	ui->originalAMushroom->setToolTip( Original_A_Mushroom.description.toString() );
	ui->monsterMovement->setToolTip( Monster_movement.description.toString() );
	ui->crushedMonsters->setToolTip( Crushed_monsters.description.toString() );
	ui->friendlyMonsters->setToolTip( Friendly_monsters.description.toString() );
	ui->invertSpriteSorting->setToolTip( Invert_sprite_sorting.description.toString() );
	ui->useDoomHitscan->setToolTip( Use_Doom_hitscan.description.toString() );
	ui->findNeighboringLight->setToolTip( Find_neighboring_light.description.toString() );
	ui->drawPolyobjects->setToolTip( Draw_polyobjects.description.toString() );
	ui->ignoreYoffsets->setToolTip( Ignore_Y_offsets.description.toString() );

	// compatflags 2
	ui->cannotTravelStraight->setToolTip( Cannot_travel_straight.description.toString() );
	ui->useDoomsFloor->setToolTip( Use_Dooms_floor.description.toString() );
	ui->soundsStop->setToolTip( Sounds_stop.description.toString() );
	ui->useDoomsPointOnLine->setToolTip( Use_Dooms_point_on_line.description.toString() );
	ui->levelExit->setToolTip( Level_exit.description.toString() );
}


//----------------------------------------------------------------------------------------------------------------------
// utils

static inline const GameFlags & getFlagsField( const CompatibilityDetails & compatDetails, const FlagDef & flag )
{
	// The GameplayDetails are just 2 consecutive int32_t fields, we can use it as an array to elegantly avoid branches.
	const GameFlags (& compatflagsFields)[2] = reinterpret_cast< const GameFlags (&)[2] >( compatDetails );  // reference to an array of 2 int32_t
	return compatflagsFields[ fut::to_underlying( flag.field ) ];
}
static inline GameFlags & getFlagsField( CompatibilityDetails & compatDetails, const FlagDef & flag )
{
	return unconst( getFlagsField( std::as_const( compatDetails ), flag ) );
}

static inline QLineEdit * getFlagsLine( Ui::CompatOptsDialog * ui, const FlagDef & flag )
{
	QLineEdit * compatflagsLines [2] =
	{
		ui->compatflags1_line,
		ui->compatflags2_line,
	};
	return compatflagsLines[ fut::to_underlying( flag.field ) ];
}

void CompatOptsDialog::toggleFlag( const FlagDef & flag, bool enabled )
{
	GameFlags & compatflagsField = getFlagsField( compatDetails, flag );
	QLineEdit * compatflagsLine = getFlagsLine( ui, flag );

	toggleFlags( compatflagsField, flag.bit, enabled );

	compatflagsLine->setText( QString::number( compatflagsField ) );
}

static bool isEnabled( const CompatibilityDetails & compatDetails, const FlagDef & flag )
{
	const GameFlags & dmflagsField = getFlagsField( compatDetails, flag );

	return areFlagsSet( dmflagsField, flag.bit );
}

bool CompatOptsDialog::isEnabled( const FlagDef & flag ) const
{
	return ::isEnabled( compatDetails, flag );
}


//----------------------------------------------------------------------------------------------------------------------
// checkboxes to numbers conversion

void CompatOptsDialog::on_findShortestTextures_toggled( bool checked )
{
	toggleFlag( Find_shortest_textures, checked );
}

void CompatOptsDialog::on_useBuggierStair_toggled( bool checked )
{
	toggleFlag( Use_buggier_stair, checked );
}

void CompatOptsDialog::on_limitPainElem_toggled( bool checked )
{
	toggleFlag( Limit_Pain_Elem, checked );
}

void CompatOptsDialog::on_dontLetOthersHear_toggled( bool checked )
{
	toggleFlag( Dont_let_others_hear, checked );
}

void CompatOptsDialog::on_actorsAreInfinite_toggled( bool checked )
{
	toggleFlag( Actors_are_infinite, checked );
}

void CompatOptsDialog::on_crippleSound_toggled( bool checked )
{
	toggleFlag( Cripple_sound, checked );
}

void CompatOptsDialog::on_enableWallRunning_toggled( bool checked )
{
	toggleFlag( Enable_wall_running, checked );
}

void CompatOptsDialog::on_spawnItemDrops_toggled( bool checked )
{
	toggleFlag( Spawn_items_drops, checked );
}

void CompatOptsDialog::on_allSpecialLines_toggled( bool checked )
{
	toggleFlag( All_special_lines, checked );
}

void CompatOptsDialog::on_disableBoomDoor_toggled( bool checked )
{
	toggleFlag( Disable_boom_door, checked );
}

void CompatOptsDialog::on_ravenScrollers_toggled( bool checked )
{
	toggleFlag( Raven_scrollers, checked );
}

void CompatOptsDialog::on_useOriginalSound_toggled( bool checked )
{
	toggleFlag( Use_original_sound, checked );
}

void CompatOptsDialog::on_dehHealthSettings_toggled( bool checked )
{
	toggleFlag( DEH_health_settings, checked );
}

void CompatOptsDialog::on_selfRefSectors_toggled( bool checked )
{
	toggleFlag( Self_ref_sectors, checked );
}

void CompatOptsDialog::on_monstersGetStuck_toggled( bool checked )
{
	toggleFlag( Monsters_get_stuck, checked );
}

void CompatOptsDialog::on_boomScrollers_toggled( bool checked )
{
	toggleFlag( Boom_scrollers, checked );
}

void CompatOptsDialog::on_monstersSeeInvisible_toggled( bool checked )
{
	toggleFlag( Monsters_see_invisible, checked );
}

void CompatOptsDialog::on_instantMovingFloors_toggled( bool checked )
{
	toggleFlag( Instant_moving_floors, checked );
}

void CompatOptsDialog::on_sectorSounds_toggled( bool checked )
{
	toggleFlag( Sector_sounds, checked );
}

void CompatOptsDialog::on_useDoomHeights_toggled( bool checked )
{
	toggleFlag( Use_Doom_heights, checked );
}

void CompatOptsDialog::on_monstersCannotCross_toggled( bool checked )
{
	toggleFlag( Monsters_cannot_cross, checked );
}

void CompatOptsDialog::on_allowAnyBossdeath_toggled( bool checked )
{
	toggleFlag( Allow_any_bossdeath, checked );
}

void CompatOptsDialog::on_noMinotaurFloor_toggled( bool checked )
{
	toggleFlag( No_Minotaur_floor, checked );
}

void CompatOptsDialog::on_originalAMushroom_toggled( bool checked )
{
	toggleFlag( Original_A_Mushroom, checked );
}

void CompatOptsDialog::on_monsterMovement_toggled( bool checked )
{
	toggleFlag( Monster_movement, checked );
}

void CompatOptsDialog::on_crushedMonsters_toggled( bool checked )
{
	toggleFlag( Crushed_monsters, checked );
}

void CompatOptsDialog::on_friendlyMonsters_toggled( bool checked )
{
	toggleFlag( Friendly_monsters, checked );
}

void CompatOptsDialog::on_invertSpriteSorting_toggled( bool checked )
{
	toggleFlag( Invert_sprite_sorting, checked );
}

void CompatOptsDialog::on_useDoomHitscan_toggled( bool checked )
{
	toggleFlag( Use_Doom_hitscan, checked );
}

void CompatOptsDialog::on_findNeighboringLight_toggled( bool checked )
{
	toggleFlag( Find_neighboring_light, checked );
}

void CompatOptsDialog::on_drawPolyobjects_toggled( bool checked )
{
	toggleFlag( Draw_polyobjects, checked );
}

void CompatOptsDialog::on_ignoreYoffsets_toggled( bool checked )
{
	toggleFlag( Ignore_Y_offsets, checked );
}

void CompatOptsDialog::on_cannotTravelStraight_toggled( bool checked )
{
	toggleFlag( Cannot_travel_straight, checked );
}

void CompatOptsDialog::on_useDoomsFloor_toggled( bool checked )
{
	toggleFlag( Use_Dooms_floor, checked );
}

void CompatOptsDialog::on_soundsStop_toggled( bool checked )
{
	toggleFlag( Sounds_stop, checked );
}

void CompatOptsDialog::on_useDoomsPointOnLine_toggled( bool checked )
{
	toggleFlag( Use_Dooms_point_on_line, checked );
}

void CompatOptsDialog::on_levelExit_toggled( bool checked )
{
	toggleFlag( Level_exit, checked );
}


//----------------------------------------------------------------------------------------------------------------------
// numbers to checkboxes conversion

void CompatOptsDialog::on_compatflags1_line_textEdited( const QString & )
{
	compatDetails.compatflags1 = ui->compatflags1_line->text().toInt();
	updateCheckboxes();
}

void CompatOptsDialog::on_compatflags2_line_textEdited( const QString & )
{
	compatDetails.compatflags2 = ui->compatflags2_line->text().toInt();
	updateCheckboxes();
}

void CompatOptsDialog::updateCheckboxes()
{
	// compatflags 1
	ui->findShortestTextures->setChecked( isEnabled( Find_shortest_textures ) );
	ui->useBuggierStair->setChecked( isEnabled( Use_buggier_stair ) );
	ui->limitPainElem->setChecked( isEnabled( Limit_Pain_Elem ) );
	ui->dontLetOthersHear->setChecked( isEnabled( Dont_let_others_hear ) );
	ui->actorsAreInfinite->setChecked( isEnabled( Actors_are_infinite ) );
	ui->crippleSound->setChecked( isEnabled( Cripple_sound ) );
	ui->enableWallRunning->setChecked( isEnabled( Enable_wall_running ) );
	ui->spawnItemDrops->setChecked( isEnabled( Spawn_items_drops ) );
	ui->allSpecialLines->setChecked( isEnabled( All_special_lines ) );
	ui->disableBoomDoor->setChecked( isEnabled( Disable_boom_door ) );
	ui->ravenScrollers->setChecked( isEnabled( Raven_scrollers ) );
	ui->useOriginalSound->setChecked( isEnabled( Use_original_sound ) );
	ui->dehHealthSettings->setChecked( isEnabled( DEH_health_settings ) );
	ui->selfRefSectors->setChecked( isEnabled( Self_ref_sectors ) );
	ui->monstersGetStuck->setChecked( isEnabled( Monsters_get_stuck ) );
	ui->boomScrollers->setChecked( isEnabled( Boom_scrollers ) );
	ui->monstersSeeInvisible->setChecked( isEnabled( Monsters_see_invisible ) );
	ui->instantMovingFloors->setChecked( isEnabled( Instant_moving_floors ) );
	ui->sectorSounds->setChecked( isEnabled( Sector_sounds ) );
	ui->useDoomHeights->setChecked( isEnabled( Use_Doom_heights ) );
	ui->monstersCannotCross->setChecked( isEnabled( Monsters_cannot_cross ) );
	ui->allowAnyBossdeath->setChecked( isEnabled( Allow_any_bossdeath ) );
	ui->noMinotaurFloor->setChecked( isEnabled( No_Minotaur_floor ) );
	ui->originalAMushroom->setChecked( isEnabled( Original_A_Mushroom ) );
	ui->monsterMovement->setChecked( isEnabled( Monster_movement ) );
	ui->crushedMonsters->setChecked( isEnabled( Crushed_monsters ) );
	ui->friendlyMonsters->setChecked( isEnabled( Friendly_monsters ) );
	ui->invertSpriteSorting->setChecked( isEnabled( Invert_sprite_sorting ) );
	ui->useDoomHitscan->setChecked( isEnabled( Use_Doom_hitscan ) );
	ui->findNeighboringLight->setChecked( isEnabled( Find_neighboring_light ) );
	ui->drawPolyobjects->setChecked( isEnabled( Draw_polyobjects ) );
	ui->ignoreYoffsets->setChecked( isEnabled( Ignore_Y_offsets ) );

	// compatflags 2
	ui->cannotTravelStraight->setChecked( isEnabled( Cannot_travel_straight ) );
	ui->useDoomsFloor->setChecked( isEnabled( Use_Dooms_floor ) );
	ui->soundsStop->setChecked( isEnabled( Sounds_stop ) );
	ui->useDoomsPointOnLine->setChecked( isEnabled( Use_Dooms_point_on_line ) );
	ui->levelExit->setChecked( isEnabled( Level_exit ) );
}


//----------------------------------------------------------------------------------------------------------------------
// command line options generation

static void addCmdLineOptionIfSet( QStringList & list, const CompatibilityDetails & compatDetails, const FlagDef & flag )
{
	if (isEnabled( compatDetails, flag ))
		list << ("+" % flag.cvarName) << "1";
}

QStringList CompatOptsDialog::getCmdArgsFromOptions( const CompatibilityDetails & compatDetails )
{
	QStringList cmdArgs;

	// compatflags 1
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Find_shortest_textures );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Use_buggier_stair );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Limit_Pain_Elem );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Dont_let_others_hear );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Actors_are_infinite );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Cripple_sound );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Enable_wall_running );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Spawn_items_drops );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, All_special_lines );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Disable_boom_door );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Raven_scrollers );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Use_original_sound );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, DEH_health_settings );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Self_ref_sectors );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Monsters_get_stuck );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Boom_scrollers );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Monsters_see_invisible );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Instant_moving_floors );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Sector_sounds );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Use_Doom_heights );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Monsters_cannot_cross );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Allow_any_bossdeath );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, No_Minotaur_floor );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Original_A_Mushroom );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Monster_movement );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Crushed_monsters );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Friendly_monsters );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Invert_sprite_sorting );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Use_Doom_hitscan );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Find_neighboring_light );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Draw_polyobjects );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Ignore_Y_offsets );

	// compatflags 2
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Cannot_travel_straight );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Use_Dooms_floor );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Sounds_stop );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Use_Dooms_point_on_line );
	addCmdLineOptionIfSet( cmdArgs, compatDetails, Level_exit );

	return cmdArgs;
}
