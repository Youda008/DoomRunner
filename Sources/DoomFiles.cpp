//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: Doom file type recognition and known WAD detection
//======================================================================================================================

#include "DoomFiles.hpp"

#include "Utils/DoomModBundles.hpp"  // fileSuffix
#include "Utils/FileSystemUtils.hpp"
#include "Utils/LangUtils.hpp"

#include <QHash>
#include <QSet>
#include <QFileInfo>
#include <QRegularExpression>


namespace doom {


//======================================================================================================================
// file type recognition

const QString demoFileSuffix = "lmp";  // this seems to be universal across different engines

// elemental lists of suffixes of known Doom file types
static const QString wadSuffix  = "wad";
static const QString iwadSuffix = "iwad";
static const QString pwadSuffix = "pwad";
static const QStringList wadSuffixes = { wadSuffix, iwadSuffix, pwadSuffix };
static const QStringList zipSuffixes = { "pk3", "pkz", "zip" };
static const QStringList _7zSuffixes = { "pk7", "7z" };
static const QStringList patchSuffixes = { "deh"/* DeHackEd patch */, "bex"/* deh for Boom */, "hhe"/* Heretic Hack Editor */ };
static const QStringList archIWADSuffixes = { "ipk3", "ipk7" };
static const QStringList extraModSuffixes = {
    "pke"/* pk3 for Eternity */, "epk"/* pk3 for EDGE and 3DGE */, "vwad"/* pk3 for K8Vavoom */
};
static const QStringList dukeSuffixes = { "grp", "rff" };

// top-level lists for main application logic like filtering files on the drive
// Because these static lists depend on other static variables, which in C++ are not guaranteed to be initialized
// before these ones, we cannot initialize these lists here, and have to do it manually later.
static QStringList possibleIWADSuffixes;
static QStringList possibleModSuffixes;

// optimization for faster search
static QSet< QString > possibleIWADSuffixes_set;

QSet< QString > makeStringSet( const QStringList & list )
{
	return { list.begin(), list.end() };
}

void initFileNameSuffixes()
{
	using L = QStringList;
	possibleIWADSuffixes = L{ wadSuffix, iwadSuffix } + archIWADSuffixes + dukeSuffixes;
	possibleModSuffixes  = L{ wadSuffix, pwadSuffix } + patchSuffixes + zipSuffixes + _7zSuffixes + extraModSuffixes
	                     + dukeSuffixes + L{ dmb::fileSuffix };

	possibleIWADSuffixes_set = makeStringSet( possibleIWADSuffixes );
}

const QStringList & getIWADSuffixes()
{
	return possibleIWADSuffixes;
}

const QStringList & getModSuffixes()
{
	return possibleModSuffixes;
}

// The correct way would be to recognize the type by file header, but there are incorrectly made mods
// that present themselfs as IWADs, so in order to support those we need to use the file suffix

bool isWAD( const QFileInfo & file )
{
	// for such small number of items, linear search is probably faster
	return wadSuffixes.contains( file.suffix().toLower() );
}

bool isZip( const QFileInfo & file )
{
	// for such small number of items, linear search is probably faster
	return zipSuffixes.contains( file.suffix().toLower() );
}

bool canBeIWAD( const QFileInfo & file )
{
	// given that this is called every tick on potentially large number of files, it should better be a hash search
	return possibleIWADSuffixes_set.contains( file.suffix().toLower() );
}


//======================================================================================================================
// known WAD info


//----------------------------------------------------------------------------------------------------------------------
// known games

namespace game {

//-- Doom 1 --------------------------------------------------------------------

static const GameIdentification Doom1_Shareware =
{
	.name = "DOOM Shareware",
	.gzdoomID = "doom.id.doom1.shareware",
	.chocolateID = "doom",
};

static const GameIdentification Doom1_Registered =
{
	.name = "DOOM Registered",
	.gzdoomID = "doom.id.doom1.registered",
	.chocolateID = "doom",
};

static const GameIdentification Doom1_Ultimate =
{
	.name = "The Ultimate DOOM",
	.gzdoomID = "doom.id.doom1.ultimate",
	.chocolateID = "doom",
};

static const GameIdentification Doom1_Ultimate_XBox =
{
	.name = "DOOM: XBox Edition",
	.gzdoomID = "doom.id.doom1.ultimate.xbox",
	.chocolateID = "doom",
};

static const GameIdentification Doom1_BFG =
{
	.name = "DOOM: BFG Edition",
	.gzdoomID = "doom.id.doom1.bfg",
	.chocolateID = "doom",
};

static const GameIdentification Doom1_KEX =
{
	.name = "DOOM: KEX Edition",
	.gzdoomID = "doom.id.doom1.kex",
	.chocolateID = "doom",
};

static const GameIdentification Doom1_Unity =
{
	.name = "DOOM: Unity Edition",
	.gzdoomID = "doom.id.doom1.unity",
	.chocolateID = "doom",
};

//-- Doom 2 --------------------------------------------------------------------

const GameIdentification Doom2 =
{
	.name = "DOOM 2: Hell on Earth",
	.gzdoomID = "doom.id.doom2.commercial",
	.chocolateID = "doom2",
};

static const GameIdentification Doom2_XBox =
{
	.name = "DOOM 2: XBox Edition",
	.gzdoomID = "doom.id.doom2.commercial.xbox",
	.chocolateID = "doom2",
};

static const GameIdentification Doom2_BFG =
{
	.name = "DOOM 2: BFG Edition",
	.gzdoomID = "doom.id.doom2.bfg",
	.chocolateID = "doom2",
};

static const GameIdentification Doom2_KEX =
{
	.name = "DOOM 2: KEX Edition",
	.gzdoomID = "doom.id.doom2.kex",
	.chocolateID = "doom2",
};

static const GameIdentification Doom2_Unity =
{
	.name = "DOOM 2: Unity Edition",
	.gzdoomID = "doom.id.doom2.unity",
	.chocolateID = "doom2",
};

//-- Final Doom ----------------------------------------------------------------

static const GameIdentification Doom2_TNT =
{
	.name = "Final Doom: TNT - Evilution",
	.gzdoomID = "doom.id.doom2.tnt",
	.chocolateID = "tnt",
};

static const GameIdentification Doom2_TNT_KEX =
{
	.name = "Final Doom: TNT - Evilution: KEX Edition",
	.gzdoomID = "doom.id.doom2.tnt.kex",
	.chocolateID = "tnt",
};

static const GameIdentification Doom2_TNT_Unity =
{
	.name = "Final Doom: TNT - Evilution: Unity Edition",
	.gzdoomID = "doom.id.doom2.tnt.unity",
	.chocolateID = "tnt",
};

static const GameIdentification Doom2_Plutonia =
{
	.name = "Final Doom: Plutonia Experiment",
	.gzdoomID = "doom.id.doom2.plutonia",
	.chocolateID = "plutonia",
};

static const GameIdentification Doom2_Plutonia_KEX =
{
	.name = "Final Doom: Plutonia Experiment: KEX Edition",
	.gzdoomID = "doom.id.doom2.plutonia.kex",
	.chocolateID = "plutonia",
};

static const GameIdentification Doom2_Plutonia_Unity =
{
	.name = "Final Doom: Plutonia Experiment: Unity Edition",
	.gzdoomID = "doom.id.doom2.plutonia.unity",
	.chocolateID = "plutonia",
};

//-- Heretic -------------------------------------------------------------------

static const GameIdentification Heretic_Shareware =
{
	.name = "Heretic Shareware",
	.gzdoomID = "heretic.shareware",
	.chocolateID = "heretic1",
};

static const GameIdentification Heretic =
{
	.name = "Heretic",
	.gzdoomID = "heretic.heretic",
	.chocolateID = "heretic",
};

//-- Hexen ---------------------------------------------------------------------

static const GameIdentification Hexen_Shareware =
{
	.name = "Hexen: Demo Version",
	.gzdoomID = "hexen.shareware",
	.chocolateID = "hexen",
};

static const GameIdentification Hexen =
{
	.name = "Hexen: Beyond Heretic",
	.gzdoomID = "hexen.hexen",
	.chocolateID = "hexen",
};

static const GameIdentification Hexen_Deathkings =
{
	.name = "Hexen: Deathkings of the Dark Citadel",
	.gzdoomID = "hexen.deathkings",
	.chocolateID = "hexen",
};

//-- FreeDoom and "free Heretic" -----------------------------------------------

static const GameIdentification Freedoom_Demo =
{
	.name = "Freedoom: Demo Version",
	.gzdoomID = "doom.freedoom.demo",
	.chocolateID = "freedoom1",
};

static const GameIdentification Freedoom_Phase1 =
{
	.name = "Freedoom: Phase 1",
	.gzdoomID = "doom.freedoom.phase1",
	.chocolateID = "freedoom1",
};

static const GameIdentification Freedoom_Phase2 =
{
	.name = "Freedoom: Phase 2",
	.gzdoomID = "doom.freedoom.phase2",
	.chocolateID = "freedoom2",
};

static const GameIdentification FreeDM =
{
	.name = "FreeDM",
	.gzdoomID = "doom.freedoom.freedm",
	.chocolateID = "freedm",
};

static const GameIdentification Blasphemer =
{
	.name = "Blasphemer",
	.gzdoomID = "blasphemer",
	.chocolateID = "heretic",
};

//-- other games ---------------------------------------------------------------

static const GameIdentification Strife =
{
	.name = "Strife: Quest for the Sigil",
	.gzdoomID = "strife.strife",
	.chocolateID = "strife1",
};

static const GameIdentification Strife_Veteran =
{
	.name = "Strife: Veteran Edition",
	.gzdoomID = "strife.veteran",
	.chocolateID = "strife1",
};

static const GameIdentification Chex_Quest =
{
	.name = "Chex(R) Quest",
	.gzdoomID = "chex.chex1",
	.chocolateID = "chex",
};

static const GameIdentification Chex_Quest3 =
{
	.name = "Chex(R) Quest 3",
	.gzdoomID = "chex.chex3",
	.chocolateID = "chex",
};

static const GameIdentification Harmony =
{
	.name = "Harmony",
	.gzdoomID = "harmony",
	.chocolateID = "unknown",
};

} // namespace game


//----------------------------------------------------------------------------------------------------------------------
// detection of known games from IWAD

template< size_t count >
static bool containsAllOf( const QSet< QString > & set, const std::array< const char *, count > & elems )
{
	for (const char * elem : elems)
		if (!set.contains( elem ))
			return false;
	return true;
}

GameIdentification identifyGame( const QSet< QString > & lumps )
{
	// Hand-crafted decision tree for detecting IWADs based on the lumps they contain.
	// Based on https://github.com/ZDoom/gzdoom/blob/master/wadsrc_extra/static/iwadinfo.txt
	//
	// NOTE: These conditions should be sorted from the least common to the most.
	// It may be slightly slower, but the other way around we risk misclassifying items with only few specific lumps like:
	// strife.veteran:  "MAP35", "I_RELB", "FXAA_F"

	if (lumps.contains("I_RELB") && lumps.contains("FXAA_F") && lumps.contains("MAP35"))
	{
		return game::Strife_Veteran;
	}
	else if (lumps.contains("TITLE"))  // Heretic & Hexen
	{
		if (lumps.contains("BLASPHEM"))
		{
			return game::Blasphemer;
		}
		else if (lumps.contains("MUS_E1M1"))
		{
			if (lumps.contains("E2M1"))
			{
				return game::Heretic;
			}
			else  // only first episode
			{
				return game::Heretic_Shareware;
			}
		}
		else if (lumps.contains("MAP60") && lumps.contains("CLUS1MSG"))
		{
			return game::Hexen_Deathkings;
		}
		else if (lumps.contains("MAP01") && lumps.contains("WINNOWR"))
		{
			if (lumps.contains("MAP40"))
			{
				return game::Hexen;
			}
			else
			{
				return game::Hexen_Shareware;
			}
		}
	}
	else if (lumps.contains("E1M1"))  // Doom1-based games
	{
		if (lumps.contains("FREEDOOM"))
		{
			if (lumps.contains("E2M1"))
			{
				return game::Freedoom_Phase1;
			}
			else  // only first episode
			{
				return game::Freedoom_Demo;
			}
		}
		else if (lumps.contains("CYCLA1") && lumps.contains("FLMBA1") && lumps.contains("MAPINFO"))
		{
			return game::Chex_Quest3;
		}
		else if (lumps.contains("W94_1") && lumps.contains("POSSH0M0") && lumps.contains("E4M1"))
		{
			return game::Chex_Quest;
		}
		else if (containsAllOf<3>( lumps, { "E2M1", "DPHOOF", "BFGGA0" } ))  // full Doom1 variants - can add "E3M1", "HEADA1", "CYBRA1", "SPIDA1D1" for additional verification
		{
			if (lumps.contains("E4M2"))  // with 4th episode
			{
				if (lumps.contains("E1M10") && lumps.contains("SEWERS"))
				{
					return game::Doom1_Ultimate_XBox;
				}
				else if (lumps.contains("DMENUPIC"))  // re-releases
				{
					if (containsAllOf<4>( lumps, { "M_ACPT", "M_CAN", "M_EXITO", "M_CHG" } ))
					{
						return game::Doom1_BFG;
					}
					else if (lumps.contains("GAMECONF"))  // KEX
					{
						return game::Doom1_KEX;
					}
					else
					{
						return game::Doom1_Unity;
					}
				}
				else  // original
				{
					return game::Doom1_Ultimate;
				}
			}
			else
			{
				return game::Doom1_Registered;
			}
		}
		else
		{
			return game::Doom1_Shareware;
		}
	}
	else if (lumps.contains("MAP01"))  // Doom2-based games
	{
		if (lumps.contains("ENDSTRF") && lumps.contains("MAP33"))
		{
			return game::Strife;
		}
		else if (lumps.contains("0HAWK01") && lumps.contains("0CARA3") && lumps.contains("0NOSE1"))
		{
			return game::Harmony;
		}
		else if (lumps.contains("FREEDOOM"))
		{
			return game::Freedoom_Phase2;
		}
		else if (lumps.contains("FREEDM"))
		{
			return game::FreeDM;
		}
		else if (lumps.contains("REDTNT2"))  // TNT
		{
			if (lumps.contains("GAMECONF"))  // KEX
			{
				return game::Doom2_TNT_KEX;
			}
			else if (lumps.contains("DMAPINFO"))  // Unity
			{
				return game::Doom2_TNT_Unity;
			}
			else
			{
				return game::Doom2_TNT;
			}
		}
		else if (lumps.contains("CAMO1"))  // Plutonia
		{
			if (lumps.contains("GAMECONF"))  // KEX
			{
				return game::Doom2_Plutonia_KEX;
			}
			else if (lumps.contains("DMAPINFO"))  // Unity
			{
				return game::Doom2_Plutonia_Unity;
			}
			else
			{
				return game::Doom2_Plutonia;
			}
		}
		else  // Doom2 variants
		{
			if (lumps.contains("CWILV32") && lumps.contains("MAP33"))
			{
				return game::Doom2_XBox;
			}
			else if (lumps.contains("DMENUPIC"))  // re-releases
			{
				if (containsAllOf<4>( lumps, { "M_ACPT", "M_CAN", "M_EXITO", "M_CHG" } ))
				{
					return game::Doom2_BFG;
				}
				else if (lumps.contains("GAMECONF"))  // KEX
				{
					return game::Doom2_KEX;
				}
				else
				{
					return game::Doom2_Unity;
				}
			}
			else
			{
				return game::Doom2;
			}
		}
	}
	return { {}, {} };  // it's not any of the games we know
}


//----------------------------------------------------------------------------------------------------------------------
// map names

QStringList getStandardMapNames( const QString & iwadFilePath )
{
	QStringList mapNames;

	QString iwadFileBaseNameLower = fs::getFileBasenameFromPath( iwadFilePath ).toLower();

	if (iwadFileBaseNameLower == "doom" || iwadFileBaseNameLower == "doom1")
	{
		mapNames.reserve( 4 * 9 );
		for (int e = 1; e <= 4; e++)
			for (int m = 1; m <= 9; m++)
				mapNames.append( QStringLiteral("E%1M%2").arg(e).arg(m) );
	}
	else
	{
		mapNames.reserve( 32 );
		for (int i = 1; i <= 32; i++)
			mapNames.append( QStringLiteral("MAP%1").arg( i, 2, 10, QChar('0') ) );
	}

	return mapNames;
}


//----------------------------------------------------------------------------------------------------------------------
// starting maps

// fast lookup table that can be used for WADs whose name can be matched exactly
static const QHash< QString, QString > startingMapsLookup =
{
	// MasterLevels
	{ "virgil.wad",    "MAP03" },
	{ "minos.wad",     "MAP05" },
	{ "bloodsea.wad",  "MAP07" },
	{ "mephisto.wad",  "MAP07" },
	{ "nessus.wad",    "MAP07" },
	{ "geryon.wad",    "MAP08" },
	{ "vesperas.wad",  "MAP09" },
	{ "blacktwr.wad",  "MAP25" },
	{ "teeth.wad",     "MAP31" },

	// unofficial MasterLevels
	{ "dante25.wad",   "MAP02" },
	{ "derelict.wad",  "MAP02" },
	{ "achron22.wad",  "MAP03" },
	{ "flood.wad",     "MAP03" },
	{ "twm01.wad",     "MAP03" },
	{ "watchtwr.wad",  "MAP04" },
	{ "todeath.wad",   "MAP05" },
	{ "arena.wad",     "MAP06" },
	{ "storm.wad",     "MAP09" },
	{ "the_evil.wad",  "MAP30" },

	// Also include the MasterLevels that start from MAP01, because otherwise when user switches from non-MAP01 level
	// to MAP01 level, the launcher will retain its previous values, which will be incorrect.
	{ "attack.wad",    "MAP01" },
	{ "canyon.wad",    "MAP01" },
	{ "catwalk.wad",   "MAP01" },
	{ "combine.wad",   "MAP01" },
	{ "fistula.wad",   "MAP01" },
	{ "garrison.wad",  "MAP01" },
	{ "manor.wad",     "MAP01" },
	{ "paradox.wad",   "MAP01" },
	{ "subspace.wad",  "MAP01" },
	{ "subterra.wad",  "MAP01" },
	{ "ttrap.wad",     "MAP01" },

	// unofficial MasterLevels starting from MAP01
	{ "anomaly.wad",   "MAP01" },
	{ "cdk_fury.wad",  "MAP01" },
	{ "cpu.wad",       "MAP01" },
	{ "device_1.wad",  "MAP01" },
	{ "dmz.wad",       "MAP01" },
	{ "e_inside.wad",  "MAP01" },
	{ "farside.wad",   "MAP01" },
	{ "hive.wad",      "MAP01" },
	{ "mines.wad",     "MAP01" },
	{ "trouble.wad",   "MAP01" },
};

// slow regex search for WADs whose name follows specfic format, for example those with postfixed version number
inline constexpr auto CaseSensitive   = QRegularExpression::NoPatternOption;
inline constexpr auto CaseInsensitive = QRegularExpression::CaseInsensitiveOption;
static const QPair< QRegularExpression, QString > startingMapsRegexes [] =
{
	{ QRegularExpression("SIGIL_COMPAT[^.]*\\.wad",  CaseInsensitive), "E3M1" },  // SIGIL_COMPAT_v1_21.wad, SIGIL_COMPAT_95.WAD
	{ QRegularExpression("SIGIL_II[^.]*\\.wad",      CaseInsensitive), "E6M1" },  // SIGIL_II_V1_0.WAD
	{ QRegularExpression("SIGIL[^.]*\\.wad",         CaseInsensitive), "E5M1" },  // SIGIL_v1_21.wad
};

QString getStartingMap( const QString & wadFileName )
{
	QString wadFileNameLower = wadFileName.toLower();

	// first do a fast search if the file name can be matched directly
	auto iter = startingMapsLookup.find( wadFileNameLower );
	if (iter != startingMapsLookup.end())
		return iter.value();

	// if not found, do a slow search if it's in one of known formats
	for (const auto & regexPair : startingMapsRegexes)
		if (regexPair.first.match( wadFileNameLower ).hasMatch())
			return regexPair.second;

	return {};
}


//======================================================================================================================


} // namespace doom
