//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: properties and capabilities of different engines
//======================================================================================================================

// Dear Doom source port developers.
// If you cannot join your efforts and work on a single engine, can you kindly at least agree on
// a standardized set of command line parameters and ways of storing save files and use them everywhere?
// Trying to be compatible with all of your source ports and their older versions is a fucking nightmare.
// Thank you.

// Sources of information:
// relations between engines:  https://en.wikipedia.org/wiki/List_of_Doom_ports#/media/File:Doom_source_ports.svg
// generic parameters (1):     https://doomwiki.org/wiki/Source_port_parameters
// generic parameters (2):     https://doom.fandom.com/wiki/Parameter
// Chocolate Doom parameters:  https://www.chocolate-doom.org/wiki/index.php/Command_line_arguments
// Crispy Doom parameters:     https://github.com/fabiangreffrath/crispy-doom/wiki/New-Command-line-Parameters
// PrBoom+ parameters:         https://github.com/coelckers/prboom-plus/blob/master/prboom2/doc/README.command-line
// DSDA-Doom parameters:       https://manpages.ubuntu.com/manpages/lunar/man6/dsda-doom.6.html
// Woof parameters:            https://github.com/fabiangreffrath/woof/wiki/Command-Line-Parameters
// ZDoom parameters:           https://zdoom.org/wiki/Command_line_parameters


//======================================================================================================================

#include "EngineTraits.hpp"

#include "Utils/WADReader.hpp"        // g_cachedWadInfo
#include "Utils/ContainerUtils.hpp"   // find
#include "Utils/FileSystemUtils.hpp"  // getFileBasenameFromPath, PathRebaser
#include "Utils/OSUtils.hpp"          // getCachedPicturesDir

#include <QHash>
#include <QRegularExpression>


//======================================================================================================================
// engine definitions - add support for new engines here

static const char * const engineFamilyStrings [] =
{
	"ZDoom",
	"ChocolateDoom",
	"PrBoom",
	"MBF",
	"EDGE",
	"KEX",
};
static_assert( std::size(engineFamilyStrings) == size_t(EngineFamily::_EnumEnd), "Please update this table too" );

/// Engine families for known executable names.
/** The key is a normalizedName() - executable name in lower case without the .exe suffix. */
static const QHash< QString, EngineFamily > knownEngineFamilies =
{
	{ "chocolate-doom",    EngineFamily::ChocolateDoom },
	{ "chocolate-heretic", EngineFamily::ChocolateDoom },
	{ "chocolate-hexen",   EngineFamily::ChocolateDoom },
	{ "crispy-doom",       EngineFamily::ChocolateDoom },
	{ "crispy-heretic",    EngineFamily::ChocolateDoom },
	{ "crispy-hexen",      EngineFamily::ChocolateDoom },
	{ "doomretro",         EngineFamily::ChocolateDoom },
	{ "strife-ve",         EngineFamily::ChocolateDoom },
	{ "prboom",            EngineFamily::PrBoom },
	{ "prboom-plus",       EngineFamily::PrBoom },
	{ "glboom",            EngineFamily::PrBoom },
	{ "dsda-doom",         EngineFamily::PrBoom },
	{ "smmu",              EngineFamily::MBF },
	{ "eternity",          EngineFamily::MBF },
	{ "nugget-doom",       EngineFamily::MBF },
	{ "cherry-doom",       EngineFamily::MBF },
	{ "woof",              EngineFamily::MBF },
	{ "zdoom",             EngineFamily::ZDoom },
	{ "lzdoom",            EngineFamily::ZDoom },
	{ "gzdoom",            EngineFamily::ZDoom },
	{ "qzdoom",            EngineFamily::ZDoom },
	{ "vkdoom",            EngineFamily::ZDoom },
	{ "skulltag",          EngineFamily::ZDoom },
	{ "zandronum",         EngineFamily::ZDoom },
	{ "edge",              EngineFamily::EDGE },
	{ "3dge",              EngineFamily::EDGE },
	{ "edge-classic",      EngineFamily::EDGE },
	{ "doom_gog",          EngineFamily::KEX },
};

/// Engine families for known application names.
/** The key is exeAppName() - application name from the executable metadata (Windows only). */
static const QHash< QString, EngineFamily > knownEngineFamilies2 =
{
	{ "DOOM + DOOM II",    EngineFamily::KEX },
};

// Used when the auto-detection of the engine family fails.
// ChocolateDoom because the command line options that work here will probably work everywhere.
static const EngineFamily fallbackEngineFamily = EngineFamily::ChocolateDoom;

static const EngineFamilyTraits engineFamilyTraits [] =
{
	//ZDoom
	{
		.configFileSuffix = "ini",
		.saveFileSuffix = "zds",
		.saveDirParam = "-savedir",
		.multHostParam = nullptr,         // the player count is added to the host parameter:  -host <player count>
		.multPlayerCountParam = "-host",  // instead of having separate parameters like:       -host -players <count>
		.multJoinParam = "-join",
		.mapParamStyle = MapParamStyle::Map,
		.compatModeStyle = CompatModeStyle::ZDoom,
	},

	//Chocolate Doom
	{
		.configFileSuffix = "cfg",
		.saveFileSuffix = "dsg",
		.saveDirParam = "-savedir",
		.multHostParam = nullptr,
		.multPlayerCountParam = nullptr,
		.multJoinParam = nullptr,
		.mapParamStyle = MapParamStyle::Warp,
		.compatModeStyle = CompatModeStyle::None,
	},

	//PrBoom
	{
		.configFileSuffix = "cfg",
		.saveFileSuffix = "dsg",
		.saveDirParam = "-save",
		.multHostParam = nullptr,         // PrBoom has a separate executable for a multiplayer server
		.multPlayerCountParam = nullptr,  // with completely different parameters.
		.multJoinParam = nullptr,         // Not worth implementing unless somebody requests it.
		.mapParamStyle = MapParamStyle::Warp,
		.compatModeStyle = CompatModeStyle::PrBoom,
	},

	//MBF
	{
		.configFileSuffix = "cfg",
		.saveFileSuffix = "dsg",
		.saveDirParam = "-save",
		.multHostParam = "-server",
		.multPlayerCountParam = nullptr,  // there is no player count here, the number of players is arbitrary
		.multJoinParam = "-connect",
		.mapParamStyle = MapParamStyle::Warp,
		.compatModeStyle = CompatModeStyle::PrBoom,
	},

	//EDGE
	{
		.configFileSuffix = "cfg",
		.saveFileSuffix = "esg",  // EDGE stores saves completely differently than all the other engines, but screw it
		.saveDirParam = nullptr,
		.multHostParam = nullptr,
		.multPlayerCountParam = nullptr,
		.multJoinParam = nullptr,
		.mapParamStyle = MapParamStyle::Warp,
		.compatModeStyle = CompatModeStyle::None,
	},

	//KEX
	{
		// TODO: Where do i find any documentation for this?
		.configFileSuffix = "cfg",
		.saveFileSuffix = "sav",
		.saveDirParam = nullptr,
		.multHostParam = nullptr,
		.multPlayerCountParam = nullptr,
		.multJoinParam = nullptr,
		.mapParamStyle = MapParamStyle::Warp,
		.compatModeStyle = CompatModeStyle::None,
	},
};
static_assert( std::size(engineFamilyTraits) == std::size(engineFamilyStrings), "Please update this table too" );

static const QHash< QString, int > startingMonitorIndexes =
{
	// the key is an executable name in lower case without the .exe suffix
	{ "zdoom", 1 },
};

static const QStringList zdoomCompatModes =
{
	"0 - Default",        // All compatibility options are turned off.
	"1 - Doom",           // Enables a set of options that should allow nearly all maps made for vanilla Doom to work in ZDoom:
	                      //   crossdropoff, dehhealth, light, missileclip, nodoorlight, shorttex, soundtarget, spritesort, stairs, trace, useblocking, floormove, maskedmidtex
	"2 - Doom (Strict)",  // Sets all of the above options and also sets these:
	                      //   corpsegibs, hitscan, invisibility, limitpain, nopassover, notossdrop, wallrun
	"3 - Boom",           // Allows maps made specifically for Boom to function correctly by enabling the following options:
	                      //   boomscroll, missileclip, soundtarget, trace, maskedmidtex
	"4 - ZDoom 2.0.63",   // Sets the two following options to be true, restoring the behavior of version 2.0.63:
	                      //   light, soundtarget
	"5 - MBF",            // As Boom above, but also sets these for closer imitation of MBF behavior:
	                      //   mushroom, mbfmonstermove, noblockfriends, maskedmidtex
	"6 - Boom (Strict)",  // As Boom above, but also sets these:
	                      //   corpsegibs, hitscan, invisibility, nopassover, notossdrop, wallrun, maskedmidtex
    "7 - MBF (Strict)",
    "8 - MBF 21",
    "9 - MBF 21 (Strict)",
};

static const QStringList prboomCompatLevels =
{
	"0  - Doom v1.2",     // (note: flawed; use PrBoom+ 2.5.0.8 or higher instead if this complevel is desired)
	"1  - Doom v1.666",
	"2  - Doom v1.9",
	"3  - Ultimate Doom",
	"4  - Final Doom & Doom95",
	"5  - DOSDoom",
	"6  - TASDOOM",
	"7  - Boom's inaccurate vanilla",
	"8  - Boom v2.01",
	"9  - Boom v2.02",
	"10 - LxDoom",
	"11 - MBF",
	"12 - PrBoom (older version)",
	"13 - PrBoom (older version)",
	"14 - PrBoom (older version)",
	"15 - PrBoom (older version)",
	"16 - PrBoom (older version)",
	"17 - PrBoom (current)",
	"18 - unused",
	"19 - unused",
	"20 - unused",
	"21 - MBF21",
};

static const QStringList noCompatModes = {};


//======================================================================================================================
// implementation

//----------------------------------------------------------------------------------------------------------------------
// compat modes/levels

const QStringList & getCompatModes( CompatModeStyle style )
{
	if (style == CompatModeStyle::ZDoom)
		return zdoomCompatModes;
	else if (style == CompatModeStyle::PrBoom)
		return prboomCompatLevels;
	else
		return noCompatModes;
}

//----------------------------------------------------------------------------------------------------------------------
// EngineFamily

const char * familyToStr( EngineFamily family )
{
	if (size_t(family) < std::size(engineFamilyStrings))
		return engineFamilyStrings[ size_t(family) ];
	else
		return "<invalid>";
}

EngineFamily familyFromStr( const QString & familyStr )
{
	int idx = find( engineFamilyStrings, familyStr );
	if (idx >= 0)
		return EngineFamily( idx );
	else
		return EngineFamily::_EnumEnd;
}


//======================================================================================================================
// EngineTraits

//----------------------------------------------------------------------------------------------------------------------
// initialization

void EngineTraits::autoDetectTraits( const QString & executablePath )
{
	_appInfo = os::getAppInfo( executablePath );

	// EngineFamily is user-overridable in EngineDialog, but this is our default automatic detection.
	EngineFamily family = guessEngineFamily();
	setFamilyTraits( family );
}

EngineFamily EngineTraits::guessEngineFamily() const
{
	if (auto iter = knownEngineFamilies.find( normalizedName() ); iter != knownEngineFamilies.end())
	{
		return iter.value();
	}

 #if IS_WINDOWS
	// in case the executable name is not reliable (example: doom_gog.exe)
	if (auto iter = knownEngineFamilies2.find( exeAppName() ); iter != knownEngineFamilies2.end())
	{
		return iter.value();
	}
 #endif

	// Of course there has to be an exception that does it differently than everybody else for no reason.
	// Who the hell thinks that adding version number to the executable file name is a good idea?!
	if (normalizedName().startsWith("edge"))  // example: "edge135"
	{
		return EngineFamily::EDGE;
	}

	// fallback if everything fails
	return fallbackEngineFamily;
}

void EngineTraits::setFamilyTraits( EngineFamily family )
{
	_family = family;

	// assign family traits
	if (size_t(family) < std::size(engineFamilyTraits))
		_familyTraits = &engineFamilyTraits[ size_t(family) ];
	else
		_familyTraits = &engineFamilyTraits[ size_t(fallbackEngineFamily) ];

	// update all engine traits that might depend on family
	_commonSaveSubdir = getCommonSaveSubdir();  // pre-compute the common subdirectory for save files, so that we don't have to repeat it on every IWAD change
	_configFileName = getDefaultConfigFileName();
	_pistolStartOption = getPistolStartOption();
	_allowCheatsArgs = getAllowCheatsArgs();
	_screenshotDirParam = getScreenshotDirParam();
}

//----------------------------------------------------------------------------------------------------------------------
// default directories and path requirements

Version EngineTraits::getExeVersionOrAssumeLatest() const
{
	Version version = exeVersion();
	// if we can't read the version, assume the latest
	if (!version.isValid())
	{
		version = {255,255,255};
	}
	return version;
}

bool EngineTraits::isBasedOnGZDoomVersionOrLater( Version atLeastVersion ) const
{
	const QString & name = normalizedName();
	Version version = getExeVersionOrAssumeLatest();

	return _family == EngineFamily::ZDoom && (
		(name == "gzdoom" && version >= atLeastVersion) ||
		(name == "vkdoom")
	);
}

bool EngineTraits::isPortableZDoom() const
{
	QString dirOfExecutable = fs::getParentDir( exePath() );
	QString portableIniFilePath = fs::getPathFromFileName( dirOfExecutable, exeBaseName()+"_portable.ini" );
	return fs::isValidFile( portableIniFilePath );
}

QString EngineTraits::getDefaultConfigDir() const
{
 #if IS_WINDOWS

	// On Windows, engines usually store their config files in the directory of its binaries,
	// with the exception of GZDoom that started storing it to Documents\My Games\GZDoom
	if (isBasedOnGZDoomVersionOrLater({4,9,0}) && !isPortableZDoom())
		return os::getDocumentsDir()%"/My Games/"%exeAppName();        // -> C:/Users/Youda/Documents/My Games/GZDoom
	else if (_family == EngineFamily::KEX)
		return os::getSavedGamesDir()%"/Nightdive Studios/DOOM";       // -> C:/Users/Youda/Saved Games/Nightdive Studios/DOOM
	else
		return fs::getParentDir( exePath() );                          // -> E:/Youda/Games/Doom/GZDoom  (may be relative - based on exePath)

 #else

	// On Linux they store them in standard user's app data dir.
	if (_family == EngineFamily::ZDoom)
		return os::getConfigDirForApp( exePath() );                    // -> /home/youda/.config/gzdoom
	else if (_family == EngineFamily::ChocolateDoom)
		return os::getDataDirForApp( exePath() );                      // -> /home/youda/.local/share/crispy-doom
	else if (_family == EngineFamily::PrBoom)
		return os::getHomeDirForApp( exePath() )%"/."%exeBaseName();   // -> /home/youda/.prboom-plus
	else if (_family == EngineFamily::MBF)
		return os::getDataDirForApp( exePath() );                      // -> /home/youda/.local/share/woof
	else
		return os::getConfigDirForApp( exePath() );                    // -> /home/youda/.config/engine_name

 #endif
}

QString EngineTraits::getDefaultDataDir() const
{
 #if IS_WINDOWS

	// On Windows, engines usually store their data (saves, ...) in the directory of its binaries,
	// with the exception of GZDoom that started storing it to Saved Games\GZDoom
	if (isBasedOnGZDoomVersionOrLater({4,9,0}) && !isPortableZDoom())
		return os::getSavedGamesDir()%"/"%exeAppName();                // -> C:/Users/Youda/Saved Games/GZDoom
	else if (_family == EngineFamily::KEX)
		return os::getSavedGamesDir()%"/Nightdive Studios/DOOM";       // -> C:/Users/Youda/Saved Games/Nightdive Studios/DOOM
	else
		return fs::getParentDir( exePath() );                          // -> E:/Youda/Games/Doom/GZDoom  (may be relative - based on exePath)

 #else

	// On Linux they generally store them in the same dir as the configs.
	return getDefaultConfigDir();

 #endif
}

QString EngineTraits::getDefaultDemoDir() const
{
	// All the engines i tried expect a demo file path that is either absolute or relative to the current working dir.
	// That means if only a file name is specified, it will try to save to the executable's directory,
	// which works for portable installations, but fail when the engine is installed in a non-writeable location.
	// Not a single engine i tried, not even on Linux, tried to save the demo where it can write, such as home.
	return fs::getParentDir( exePath() );
}

QString EngineTraits::getDefaultScreenshotDir() const
{
 #if IS_WINDOWS

	if (isBasedOnGZDoomVersionOrLater({4,9,0}) && !isPortableZDoom())
		return os::getPicturesDir()%"/Screenshots/"%exeAppName();      // -> C:/Users/Youda/Pictures/Screenshots/GZDoom
	else
		return fs::getParentDir( exePath() );                          // -> E:/Youda/Games/Doom/GZDoom  (may be relative - based on exePath)

 #else

	QString screenshotDir = getDefaultDataDir();

	if (isBasedOnGZDoomVersionOrLater({4,9,0}) && !isPortableZDoom())  // only new non-portable GZDoom
	{
		screenshotDir += "/screenshots";
	}

	// this luckily doesn't depend on the selected IWAD

	return screenshotDir;

 #endif
}

// Dear Doom source port developers. Are you fucking kidding me?!
//
// Observed save file storage
//
// engine          OS        version   installation   subdirectory     IWAD-based subdirectory
//----------------------------------------------------------------------------------------------------------------------
// GZDoom          Windows   <  4.4    --             --               --
// GZDoom          Windows   >= 4.4    --             Save             --
// GZDoom          Windows   >= 4.9    portable       Save             doom.id.doom1.ultimate, ...
// GZDoom          Windows   >= 4.9    non-portable   --               doom.id.doom1.ultimate, ...
// GZDoom          Linux     <  4.9    --             --               --
// GZDoom          Linux     >= 4.9    --             --               doom.id.doom1.ultimate, ...
// GZDoom          Linux     >= 4.11   --             savegames        doom.id.doom1.ultimate, ...
// ChocolateDoom   Windows   latest    --             --               --
// ChocolateDoom   Linux     latest    --             savegames        doom.wad, doom2.wad, ...
// CrispyDoom      Windows   latest    --             --               --
// CrispyDoom      Linux     latest    --             savegames        doom.wad, doom2.wad, ...
// Doom Retro      Windows   latest    --             savegames        doom, doom2, ...
// Doom Retro      Linux     latest    --             ??               (Linux officially not supported)
// PrBoom+         Windows   latest    --             --               --
// PrBoom+         Linux     latest    --             ??               (no longer available in Linux repos)
// Woof            Windows   latest    --             savegames        Doom1.wad, Doom2.wad, Renamed.wad, ...
// Woof            Linux     latest    --             savegames        Doom1.wad, Doom2.wad, Renamed.wad, ...
// DSDA-Doom       Windows   latest    --             dsda_doom_data   doom1, doom2, renamed, ...
// DSDA-Doom       Linux     latest    --             dsda_doom_data   doom1, doom2, renamed, ...

// For some engines the directory for save files consists of 3 parts:
//   1. the data directory:                        /home/youda/.config/gzdoom
//   2. the common part of the save subdirectory:  savegames
//   3. the part that depends on the used IWAD:    doom.id.doom2.commercial
//   -> full save directory path:                  /home/youda/.config/gzdoom/savegames/doom.id.doom2.commercial
// This function returns the common part (2.)
QString EngineTraits::getCommonSaveSubdir() const
{
	QString saveSubdirBase;

	const QString & name = normalizedName();

	if (_family == EngineFamily::ZDoom && name == "gzdoom")
	{
		Version version = getExeVersionOrAssumeLatest();
		using v = Version;

		if constexpr (IS_WINDOWS)
		{
			if (version >= v{4,4,0} && (version < v{4,9,0} || isPortableZDoom()))
			{
				saveSubdirBase = "Save";
			}
		}
		else  // Linux, Mac
		{
			if (version >= v{4,11,0})
			{
				saveSubdirBase = "savegames";
			}
		}
	}
	else if (_family == EngineFamily::ChocolateDoom)
	{
		if (!IS_WINDOWS /* Chocolate/Crispy Doom */ || name == "doomretro")
		{
			saveSubdirBase = "savegames";
		}
	}
	else if (_family == EngineFamily::PrBoom)
	{
		if (name == "dsda-doom")
		{
			saveSubdirBase = "dsda_doom_data";
		}
	}
	else if (_family == EngineFamily::MBF)
	{
		if (name == "woof")
		{
			saveSubdirBase = "savegames";
		}
	}
	else if (_family == EngineFamily::EDGE)
	{
		saveSubdirBase = "savegame";
	}

	return saveSubdirBase;
}

QString EngineTraits::getDefaultSaveSubdir( const QString & IWADPath ) const
{
	assert( isInitialized() );

	QString saveDir = _commonSaveSubdir;

	const QString & name = normalizedName();

	// Some engines store their save files in a subdirectory named after the IWAD in use.
	if ((isBasedOnGZDoomVersionOrLater({4,9,0}))
	 || (_family == EngineFamily::ChocolateDoom && (!IS_WINDOWS /* Chocolate/Crispy Doom */ || name == "doomretro")))
	{
		QString gameID;
		if (!IWADPath.isEmpty())
		{
			const auto & iwadInfo = doom::g_cachedWadInfo.getFileInfo( IWADPath );
			if (iwadInfo.status == ReadStatus::Success)
			{
				if (_family == EngineFamily::ChocolateDoom && iwadInfo.game.chocolateID != nullptr)
					gameID = iwadInfo.game.chocolateID;
				else if (iwadInfo.game.gzdoomID != nullptr)
					gameID = iwadInfo.game.gzdoomID;
			}
		}
		if (gameID.isEmpty())
		{
			// fallback to Doom2 if the game cannot be identified, it has the best probability of being correct
			const doom::GameIdentification & defaultGame = doom::game::Doom2;
			gameID = _family == EngineFamily::ChocolateDoom ? defaultGame.chocolateID : defaultGame.gzdoomID;
		}
		if (_family == EngineFamily::ChocolateDoom && name != "doomretro")  // Chocolate or Crispy Doom
		{
			gameID += ".wad";
		}

		saveDir = fs::appendToPath( saveDir, gameID );
	}
	else if (_family == EngineFamily::PrBoom && name == "dsda-doom")
	{
		saveDir = fs::appendToPath( saveDir, fs::getFileBasenameFromPath( IWADPath ).toLower() );
	}
	else if (_family == EngineFamily::MBF && name == "woof")
	{
		saveDir = fs::appendToPath( saveDir, fs::getFileNameFromPath( IWADPath ) );
	}

	return saveDir;
}

bool EngineTraits::saveDirDependsOnIWAD() const
{
	assert( isInitialized() );

	const QString & name = normalizedName();

	return (isBasedOnGZDoomVersionOrLater({4,9,0}))
        || (_family == EngineFamily::ChocolateDoom && (!IS_WINDOWS /* Chocolate/Crispy Doom */ || name == "doomretro"))
        || (_family == EngineFamily::PrBoom && name == "dsda-doom")
        || (_family == EngineFamily::MBF && name == "woof");
}

QString EngineTraits::makeCmdSaveFilePath(
	const PathRebaser & runDirRebaser, const QString & saveDir, const QString & saveFileName
) const
{
	// the base dir for the save file parameter depends on the engine and its version
	if (isBasedOnGZDoomVersionOrLater({4,9,0}) || _family == EngineFamily::KEX)
	{
		// Path of the save file must be relative to the -savedir argument if present or the default save dir otherwise.
		// The path also cannot be absolute, because it is directly appended to the -savedir path and would produce nonsense.
		return runDirRebaser.maybeQuoted( saveFileName );
	}
	else
	{
		// Path of save file must be relative to the working directory.
		QString saveFilePath = fs::getPathFromFileName( saveDir, saveFileName );
		return runDirRebaser.makeRebasedCmdPath( saveFilePath );  // keep the path style of the saveDir
	}
}

QString EngineTraits::getSaveNumberFromFileName( const QString & saveFileName ) const
{
	static const QRegularExpression saveNumRegex("^[a-zA-Z_\\-]+(\\d+)\\.");
	if (auto match = saveNumRegex.match( saveFileName ); match.hasMatch())
	{
		return match.captured(1);
	}
	else if (_family == EngineFamily::MBF && normalizedName() == "woof" && saveFileName == "autosave.dsg")
	{
		return "255";
	}
	else
	{
		return "invalid_file_name";
	}
}

QString EngineTraits::getDefaultConfigFileName() const
{
	if (_family == EngineFamily::ZDoom)
	{
		// (╯°□°)╯︵ ┻━┻

		const QString & name = normalizedName();
		Version version = getExeVersionOrAssumeLatest();
		using v = Version;

		QString firstPart;
		if (name == "gzdoom" && version < v{2,1,0})
			firstPart = "zdoom";
		else
			firstPart = exeBaseName();

		if ((name == "gzdoom" && version >= v{4,9,0}) || (name == "vkdoom"))
			if (isPortableZDoom())
				return firstPart%"_portable.ini";
			else
				return firstPart%".ini";
		else
			return firstPart%"-"%os::getUserName()%".ini";   // -> gzdoom-Youda.ini
	}
	else
	{
		return exeBaseName()%".cfg";
	}
}

//----------------------------------------------------------------------------------------------------------------------
// command line parameters deduction

const char * EngineTraits::saveFileSuffix() const
{
	assert( _familyTraits );
	if (normalizedName() == "doomretro")
		return "save";  // ffs!
	else
		return _familyTraits->saveFileSuffix;
}

QStringList EngineTraits::getAllowCheatsArgs() const
{
	if (_family == EngineFamily::ZDoom)
		return { "+sv_cheats", "1" };
	else
		return {};
}

const char * EngineTraits::getPistolStartOption() const
{
	// https://doomwiki.org/wiki/Source_port_parameters#-pistolstart
	const QString & name = normalizedName();
	if (_family == EngineFamily::ChocolateDoom || _family == EngineFamily::PrBoom || name == "woof")
		return "-pistolstart";
	else
		return nullptr;
}

const char * EngineTraits::getScreenshotDirParam() const
{
	// https://doomwiki.org/wiki/Source_port_parameters#-shotdir_.3Cdirectory.3E
	if (_family == EngineFamily::ZDoom || _family == EngineFamily::PrBoom || normalizedName() == "doomretro")
		return "-shotdir";
	else
		return nullptr;
}

static const QRegularExpression doom1MapNameRegex("E(\\d+)M(\\d+)");
static const QRegularExpression doom2MapNameRegex("MAP(\\d+)");

QStringList EngineTraits::getMapArgs( int mapIdx, const QString & mapName ) const
{
	assert( isInitialized() );

	if (mapName.isEmpty())
	{
		return {};
	}

	if (mapParamStyle() == MapParamStyle::Map)  // this engine supports +map, we can use the map name directly
	{
		return { "+map", mapName };
	}
	else  // this engine only supports the old -warp, we must deduce map number
	{
		QRegularExpressionMatch match;
		if ((match = doom1MapNameRegex.match( mapName )).hasMatch())
		{
			return { "-warp", match.captured(1), match.captured(2) };
		}
		else if ((match = doom2MapNameRegex.match( mapName )).hasMatch())
		{
			return { "-warp", match.captured(1) };
		}
		else  // in case the WAD defines it's own map names, we have to resort to guessing the number by using its combo-box index
		{
			return { "-warp", QString::number( mapIdx + 1 ) };
		}
	}
}

QStringList EngineTraits::getLoadSavedGameArgs(
	const PathRebaser & runDirRebaser, const QString & saveDir, const QString & saveFileName
) const
{
	assert( isInitialized() );

	if (saveFileName.isEmpty())
	{
		return {};
	}

	if (_family == EngineFamily::ZDoom)
	{
		return { "-loadgame", makeCmdSaveFilePath( runDirRebaser, saveDir, saveFileName ) };
	}
	else
	{
		return { "-loadgame", getSaveNumberFromFileName( saveFileName ) };
	}
}

QStringList EngineTraits::getCompatModeArgs( int compatMode ) const
{
	assert( isInitialized() );

	// Properly working -compatmode is present only in GZDoom 4.8.0+,
	// for other ZDoom-based engines use at least something, even if it doesn't fully work.
	if (isBasedOnGZDoomVersionOrLater({4,8,0}))
	{
		return { "-compatmode", QString::number( compatMode ) };
	}
	else if (compatModeStyle() == CompatModeStyle::ZDoom)
	{
		return { "+compatmode", QString::number( compatMode ) };
	}
	else if (compatModeStyle() == CompatModeStyle::PrBoom)
	{
		return { "-complevel", QString::number( compatMode ) };
	}
	else
	{
		return {};
	}
}

QString EngineTraits::getCmdMonitorIndex( int ownIndex ) const
{
	assert( isInitialized() );

	int startingMonitorIndex = 0;
	if (auto iter = startingMonitorIndexes.find( exeBaseName() ); iter != startingMonitorIndexes.end())
		startingMonitorIndex = iter.value();

	return QString::number( startingMonitorIndex + ownIndex );
}
