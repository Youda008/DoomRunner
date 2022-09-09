//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: logic of the Compatibility Options dialog
//======================================================================================================================

#include "CompatOptsDialog.hpp"
#include "ui_CompatOptsDialog.h"

#include <QString>
#include <QTextStream>
#include <QLineEdit>
#include <QIntValidator>


//======================================================================================================================

typedef uint flagsIdx;
const flagsIdx COMPAT_FLAGS_1 = 0;
const flagsIdx COMPAT_FLAGS_2 = 1;

struct CompatFlag {
	QString cvarName;
	flagsIdx flags;
	int32_t bit;
};

static const CompatFlag CRUSHED_MONSTERS        = { "compat_corpsegibs",          COMPAT_FLAGS_1, 33554432 };
static const CompatFlag FRIENDLY_MONSTERS       = { "compat_noblockfriends",      COMPAT_FLAGS_1, 67108864 };
static const CompatFlag LIMIT_PAIN_ELEM         = { "compat_limitpain",           COMPAT_FLAGS_1, 4 };
static const CompatFlag MONSTER_MOVEMENT        = { "compat_mbfmonstermove",      COMPAT_FLAGS_1, 16777216 };
static const CompatFlag MONSTERS_CANNOT_CROSS   = { "compat_crossdropoff",        COMPAT_FLAGS_1, 1048576 };
static const CompatFlag MONSTERS_GET_STUCK      = { "compat_dropoff",             COMPAT_FLAGS_1, 16384 };
static const CompatFlag MONSTERS_SEE_INVISIBLE  = { "compat_invisibility",        COMPAT_FLAGS_1, 65536 };
static const CompatFlag NO_MINOTAUR_FLOOR       = { "compat_minotaur",            COMPAT_FLAGS_1, 4194304 };
static const CompatFlag SPAWN_ITEMS_DROPS       = { "compat_notossdrops",         COMPAT_FLAGS_1, 128 };

static const CompatFlag DEH_HEALTH_SETTINGS     = { "compat_dehhealth",           COMPAT_FLAGS_1, 4096 };
static const CompatFlag ORIGINAL_A_MUSHROOM     = { "compat_mushroom",            COMPAT_FLAGS_1, 8388608 };

static const CompatFlag ALL_SPECIAL_LINES       = { "compat_useblocking",         COMPAT_FLAGS_1, 256 };
static const CompatFlag ALLOW_ANY_BOSSDEATH     = { "compat_anybossdeath",        COMPAT_FLAGS_1, 2097152 };
static const CompatFlag DISABLE_BOOM_DOOR       = { "compat_nodoorlight",         COMPAT_FLAGS_1, 512 };
static const CompatFlag FIND_NEIGHBORING_LIGHT  = { "compat_light",               COMPAT_FLAGS_1, 536870912 };
static const CompatFlag FIND_SHORTEST_TEXTURES  = { "compat_shorttex",            COMPAT_FLAGS_1, 1 };
static const CompatFlag USE_BUGGIER_STAIR       = { "compat_stairs",              COMPAT_FLAGS_1, 2 };
static const CompatFlag USE_DOOMS_FLOOR         = { "compat_floormove",           COMPAT_FLAGS_2, 2 };
static const CompatFlag USE_DOOMS_POINT_ON_LINE = { "compat_pointonline",         COMPAT_FLAGS_2, 8 };
static const CompatFlag LEVEL_EXIT              = { "compat_multiexit",           COMPAT_FLAGS_2, 16 };

static const CompatFlag ACTORS_ARE_INFINITE     = { "compat_nopassover",          COMPAT_FLAGS_1, 16 };
static const CompatFlag BOOM_SCROLLERS          = { "compat_boomscroll",          COMPAT_FLAGS_1, 32768 };
static const CompatFlag CANNOT_TRAVEL_STRAIGHT  = { "compat_badangles",           COMPAT_FLAGS_2, 1 };
static const CompatFlag ENABLE_WALL_RUNNING     = { "compat_wallrun",             COMPAT_FLAGS_1, 64 };
static const CompatFlag RAVEN_SCROLLERS         = { "compat_ravenscroll",         COMPAT_FLAGS_1, 1024 };
static const CompatFlag SELF_REF_SECTORS        = { "compat_trace",               COMPAT_FLAGS_1, 8192 };
static const CompatFlag USE_DOOM_HITSCAN        = { "compat_hitscan",             COMPAT_FLAGS_1, 268435456 };
static const CompatFlag USE_DOOM_HEIGHTS        = { "compat_missileclip",         COMPAT_FLAGS_1, 524288 };

static const CompatFlag DRAW_POLYOBJECTS        = { "compat_polyobj",             COMPAT_FLAGS_1, 1073741824 };
static const CompatFlag IGNORE_Y_OFFSETS        = { "compat_maskedmidtex",        COMPAT_FLAGS_1, -2147483648 };
static const CompatFlag INVERT_SPRITE_SORTING   = { "compat_spritesort",          COMPAT_FLAGS_1, 134217728 };

static const CompatFlag CRIPPLE_SOUND           = { "compat_soundslots",          COMPAT_FLAGS_1, 32 };
static const CompatFlag DONT_LET_OTHERS         = { "compat_silentpickup",        COMPAT_FLAGS_1, 8 };
static const CompatFlag INSTANT_MOVING_FLOORS   = { "compat_silentinstantfloors", COMPAT_FLAGS_1, 131072 };
static const CompatFlag SECTOR_SOUNDS           = { "compat_sectorsounds",        COMPAT_FLAGS_1, 262144 };
static const CompatFlag SOUNDS_STOP             = { "compat_soundcutoff",         COMPAT_FLAGS_2, 4 };
static const CompatFlag USE_ORIGINAL_SOUND      = { "compat_soundtarget",         COMPAT_FLAGS_1, 2048 };


//======================================================================================================================

CompatOptsDialog::CompatOptsDialog( QWidget * parent, const CompatibilityOptions & compatOpts )
:
	QDialog( parent ),
	compatOpts( compatOpts )
{
	ui = new Ui::CompatOptsDialog;
    ui->setupUi( this );

	ui->compatflags1_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );
	ui->compatflags2_line->setValidator( new QIntValidator( INT32_MIN, INT32_MAX, this ) );

	ui->compatflags1_line->setText( QString::number( compatOpts.flags1 ) );
	ui->compatflags2_line->setText( QString::number( compatOpts.flags2 ) );

	updateCheckboxes();

	connect( ui->buttonBox, &QDialogButtonBox::accepted, this, &thisClass::accept );
	connect( ui->buttonBox, &QDialogButtonBox::rejected, this, &thisClass::reject );
}

CompatOptsDialog::~CompatOptsDialog()
{
    delete ui;
}

void CompatOptsDialog::setFlag( const CompatFlag & flag, bool enabled )
{
	int32_t * flags;
	QLineEdit * line;

	if (flag.flags == COMPAT_FLAGS_1)
	{
		flags = &compatOpts.flags1;
		line = ui->compatflags1_line;
	}
	else
	{
		flags = &compatOpts.flags2;
		line = ui->compatflags2_line;
	}

	if (enabled)
		*flags |= flag.bit;
	else
		*flags &= ~flag.bit;

	line->setText( QString::number( *flags ) );
}

static bool isEnabled( const CompatibilityOptions & compatOpts, const CompatFlag & flag )
{
	const int32_t * flags;

	if (flag.flags == COMPAT_FLAGS_1)
		flags = &compatOpts.flags1;
	else
		flags = &compatOpts.flags2;

	return (*flags & flag.bit) != 0;
}

bool CompatOptsDialog::isEnabled( const CompatFlag & flag ) const
{
	return ::isEnabled( compatOpts, flag );
}


//----------------------------------------------------------------------------------------------------------------------

void CompatOptsDialog::on_crushedMonsters_toggled( bool checked )
{
	setFlag( CRUSHED_MONSTERS, checked );
}

void CompatOptsDialog::on_friendlyMonsters_toggled( bool checked )
{
	setFlag( FRIENDLY_MONSTERS, checked );
}

void CompatOptsDialog::on_limitPainElem_toggled( bool checked )
{
	setFlag( LIMIT_PAIN_ELEM, checked );
}

void CompatOptsDialog::on_monsterMovement_toggled( bool checked )
{
	setFlag( MONSTER_MOVEMENT, checked );
}

void CompatOptsDialog::on_monstersCannotCross_toggled( bool checked )
{
	setFlag( MONSTERS_CANNOT_CROSS, checked );
}

void CompatOptsDialog::on_monstersGetStuck_toggled( bool checked )
{
	setFlag( MONSTERS_GET_STUCK, checked );
}

void CompatOptsDialog::on_monstersSeeInvisible_toggled( bool checked )
{
	setFlag( MONSTERS_SEE_INVISIBLE, checked );
}

void CompatOptsDialog::on_noMinotaurFloor_toggled( bool checked )
{
	setFlag( NO_MINOTAUR_FLOOR, checked );
}

void CompatOptsDialog::on_spawnItemDrops_toggled( bool checked )
{
	setFlag( SPAWN_ITEMS_DROPS, checked );
}

void CompatOptsDialog::on_dehHealthSettings_toggled( bool checked )
{
	setFlag( DEH_HEALTH_SETTINGS, checked );
}

void CompatOptsDialog::on_originalAMushroom_toggled( bool checked )
{
	setFlag( ORIGINAL_A_MUSHROOM, checked );
}

void CompatOptsDialog::on_allSpecialLines_toggled( bool checked )
{
	setFlag( ALL_SPECIAL_LINES, checked );
}

void CompatOptsDialog::on_allowAnyBossdeath_toggled( bool checked )
{
	setFlag( ALLOW_ANY_BOSSDEATH, checked );
}

void CompatOptsDialog::on_disableBoomDoor_toggled( bool checked )
{
	setFlag( DISABLE_BOOM_DOOR, checked );
}

void CompatOptsDialog::on_findNeighboringLight_toggled( bool checked )
{
	setFlag( FIND_NEIGHBORING_LIGHT, checked );
}

void CompatOptsDialog::on_findShortestTextures_toggled( bool checked )
{
	setFlag( FIND_SHORTEST_TEXTURES, checked );
}

void CompatOptsDialog::on_useBuggierStair_toggled( bool checked )
{
	setFlag( USE_BUGGIER_STAIR, checked );
}

void CompatOptsDialog::on_useDoomsFloor_toggled( bool checked )
{
	setFlag( USE_DOOMS_FLOOR, checked );
}

void CompatOptsDialog::on_useDoomsPointOnLine_toggled( bool checked )
{
	setFlag( USE_DOOMS_POINT_ON_LINE, checked );
}

void CompatOptsDialog::on_levelExit_toggled( bool checked )
{
	setFlag( LEVEL_EXIT, checked );
}

void CompatOptsDialog::on_actorsAreInfinite_toggled( bool checked )
{
	setFlag( ACTORS_ARE_INFINITE, checked );
}

void CompatOptsDialog::on_boomScrollers_toggled( bool checked )
{
	setFlag( BOOM_SCROLLERS, checked );
}

void CompatOptsDialog::on_cannotTravelStraight_toggled( bool checked )
{
	setFlag( CANNOT_TRAVEL_STRAIGHT, checked );
}

void CompatOptsDialog::on_enableWallRunning_toggled( bool checked )
{
	setFlag( ENABLE_WALL_RUNNING, checked );
}

void CompatOptsDialog::on_ravenScrollers_toggled( bool checked )
{
	setFlag( RAVEN_SCROLLERS, checked );
}

void CompatOptsDialog::on_selfRefSectors_toggled( bool checked )
{
	setFlag( SELF_REF_SECTORS, checked );
}

void CompatOptsDialog::on_useDoomHitscan_toggled( bool checked )
{
	setFlag( USE_DOOM_HITSCAN, checked );
}

void CompatOptsDialog::on_useDoomHeights_toggled( bool checked )
{
	setFlag( USE_DOOM_HEIGHTS, checked );
}

void CompatOptsDialog::on_drawPolyobjects_toggled( bool checked )
{
	setFlag( DRAW_POLYOBJECTS, checked );
}

void CompatOptsDialog::on_ignoreYoffsets_toggled( bool checked )
{
	setFlag( IGNORE_Y_OFFSETS, checked );
}

void CompatOptsDialog::on_invertSpriteSorting_toggled( bool checked )
{
	setFlag( INVERT_SPRITE_SORTING, checked );
}

void CompatOptsDialog::on_crippleSound_toggled( bool checked )
{
	setFlag( CRIPPLE_SOUND, checked );
}

void CompatOptsDialog::on_dontLetOthersHear_toggled( bool checked )
{
	setFlag( DONT_LET_OTHERS, checked );
}

void CompatOptsDialog::on_instantMovingFloors_toggled( bool checked )
{
	setFlag( INSTANT_MOVING_FLOORS, checked );
}

void CompatOptsDialog::on_sectorSounds_toggled( bool checked )
{
	setFlag( SECTOR_SOUNDS, checked );
}

void CompatOptsDialog::on_soundsStop_toggled( bool checked )
{
	setFlag( SOUNDS_STOP, checked );
}

void CompatOptsDialog::on_useOriginalSound_toggled( bool checked )
{
	setFlag( USE_ORIGINAL_SOUND, checked );
}


//----------------------------------------------------------------------------------------------------------------------

void CompatOptsDialog::on_compatflags1_line_textEdited( const QString & )
{
	compatOpts.flags1 = ui->compatflags1_line->text().toInt();
	updateCheckboxes();
}

void CompatOptsDialog::on_compatflags2_line_textEdited( const QString & )
{
	compatOpts.flags2 = ui->compatflags2_line->text().toInt();
	updateCheckboxes();
}

void CompatOptsDialog::updateCheckboxes()
{
	ui->crushedMonsters->setChecked( isEnabled( CRUSHED_MONSTERS ) );
	ui->friendlyMonsters->setChecked( isEnabled( FRIENDLY_MONSTERS ) );
	ui->limitPainElem->setChecked( isEnabled( LIMIT_PAIN_ELEM ) );
	ui->monsterMovement->setChecked( isEnabled( MONSTER_MOVEMENT ) );
	ui->monstersCannotCross->setChecked( isEnabled( MONSTERS_CANNOT_CROSS ) );
	ui->monstersGetStuck->setChecked( isEnabled( MONSTERS_GET_STUCK ) );
	ui->monstersSeeInvisible->setChecked( isEnabled( MONSTERS_SEE_INVISIBLE ) );
	ui->noMinotaurFloor->setChecked( isEnabled( NO_MINOTAUR_FLOOR ) );
	ui->spawnItemDrops->setChecked( isEnabled( SPAWN_ITEMS_DROPS ) );

	ui->dehHealthSettings->setChecked( isEnabled( DEH_HEALTH_SETTINGS ) );
	ui->originalAMushroom->setChecked( isEnabled( ORIGINAL_A_MUSHROOM ) );

	ui->allSpecialLines->setChecked( isEnabled( ALL_SPECIAL_LINES ) );
	ui->allowAnyBossdeath->setChecked( isEnabled( ALLOW_ANY_BOSSDEATH ) );
	ui->disableBoomDoor->setChecked( isEnabled( DISABLE_BOOM_DOOR ) );
	ui->disableBoomDoor->setChecked( isEnabled( DISABLE_BOOM_DOOR ) );
	ui->findNeighboringLight->setChecked( isEnabled( FIND_NEIGHBORING_LIGHT ) );
	ui->findShortestTextures->setChecked( isEnabled( FIND_SHORTEST_TEXTURES ) );
	ui->useBuggierStair->setChecked( isEnabled( USE_BUGGIER_STAIR ) );
	ui->useDoomsFloor->setChecked( isEnabled( USE_DOOMS_FLOOR ) );
	ui->useDoomsPointOnLine->setChecked( isEnabled( USE_DOOMS_POINT_ON_LINE ) );
	ui->levelExit->setChecked( isEnabled( LEVEL_EXIT ) );

	ui->actorsAreInfinite->setChecked( isEnabled( ACTORS_ARE_INFINITE ) );
	ui->boomScrollers->setChecked( isEnabled( BOOM_SCROLLERS ) );
	ui->cannotTravelStraight->setChecked( isEnabled( CANNOT_TRAVEL_STRAIGHT ) );
	ui->enableWallRunning->setChecked( isEnabled( ENABLE_WALL_RUNNING ) );
	ui->ravenScrollers->setChecked( isEnabled( RAVEN_SCROLLERS ) );
	ui->selfRefSectors->setChecked( isEnabled( SELF_REF_SECTORS ) );
	ui->useDoomHitscan->setChecked( isEnabled( USE_DOOM_HITSCAN ) );
	ui->useDoomHeights->setChecked( isEnabled( USE_DOOM_HEIGHTS ) );

	ui->drawPolyobjects->setChecked( isEnabled( DRAW_POLYOBJECTS ) );
	ui->ignoreYoffsets->setChecked( isEnabled( IGNORE_Y_OFFSETS ) );
	ui->invertSpriteSorting->setChecked( isEnabled( INVERT_SPRITE_SORTING ) );

	ui->crippleSound->setChecked( isEnabled( CRIPPLE_SOUND ) );
	ui->dontLetOthersHear->setChecked( isEnabled( DONT_LET_OTHERS ) );
	ui->instantMovingFloors->setChecked( isEnabled( INSTANT_MOVING_FLOORS ) );
	ui->sectorSounds->setChecked( isEnabled( SECTOR_SOUNDS ) );
	ui->soundsStop->setChecked( isEnabled( SOUNDS_STOP ) );
	ui->useOriginalSound->setChecked( isEnabled( USE_ORIGINAL_SOUND ) );
}


//----------------------------------------------------------------------------------------------------------------------

static void addCmdLineOptionIfSet( QStringList & list, const CompatibilityOptions & compatOpts, const CompatFlag & flag )
{
	if (isEnabled( compatOpts, flag ))
		list << "+" + flag.cvarName << "1";
}

QStringList CompatOptsDialog::getCmdArgsFromOptions( const CompatibilityOptions & compatOpts )
{
	QStringList cmdArgs;

	addCmdLineOptionIfSet( cmdArgs, compatOpts, CRUSHED_MONSTERS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, FRIENDLY_MONSTERS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, LIMIT_PAIN_ELEM );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, MONSTER_MOVEMENT );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, MONSTERS_CANNOT_CROSS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, MONSTERS_GET_STUCK );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, MONSTERS_SEE_INVISIBLE );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, NO_MINOTAUR_FLOOR );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, SPAWN_ITEMS_DROPS );

	addCmdLineOptionIfSet( cmdArgs, compatOpts, DEH_HEALTH_SETTINGS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, ORIGINAL_A_MUSHROOM );

	addCmdLineOptionIfSet( cmdArgs, compatOpts, ALL_SPECIAL_LINES );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, ALLOW_ANY_BOSSDEATH );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, DISABLE_BOOM_DOOR );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, FIND_NEIGHBORING_LIGHT );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, FIND_SHORTEST_TEXTURES );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, USE_BUGGIER_STAIR );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, USE_DOOMS_FLOOR );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, USE_DOOMS_POINT_ON_LINE );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, LEVEL_EXIT );

	addCmdLineOptionIfSet( cmdArgs, compatOpts, ACTORS_ARE_INFINITE );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, BOOM_SCROLLERS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, CANNOT_TRAVEL_STRAIGHT );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, ENABLE_WALL_RUNNING );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, RAVEN_SCROLLERS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, SELF_REF_SECTORS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, USE_DOOM_HITSCAN );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, USE_DOOM_HEIGHTS );

	addCmdLineOptionIfSet( cmdArgs, compatOpts, DRAW_POLYOBJECTS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, IGNORE_Y_OFFSETS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, INVERT_SPRITE_SORTING );

	addCmdLineOptionIfSet( cmdArgs, compatOpts, CRIPPLE_SOUND );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, DONT_LET_OTHERS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, INSTANT_MOVING_FLOORS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, SECTOR_SOUNDS );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, SOUNDS_STOP );
	addCmdLineOptionIfSet( cmdArgs, compatOpts, USE_ORIGINAL_SOUND );

	return cmdArgs;
}
