//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: properties and capabilities of different engines
//======================================================================================================================

#include "EngineTraits.hpp"

#include "Utils/WADReader.hpp"        // g_cachedWadInfo
#include "Utils/ContainerUtils.hpp"   // find
#include "Utils/FileSystemUtils.hpp"  // getFileBasenameFromPath
#include "Utils/OSUtils.hpp"          // getCachedPicturesDir

#include <QHash>
#include <QRegularExpression>


//======================================================================================================================
// engine definitions - add support for new engines here

static const char * const engineFamilyStrings [] =
{
	"ZDoom",
	"PrBoom",
	"MBF",
	"ChocolateDoom",
	"EDGE",
};
static_assert( std::size(engineFamilyStrings) == size_t(EngineFamily::_EnumEnd), "Please update this table too" );

static const QHash< QString, EngineFamily > knownEngineFamilies =
{
	// the key is an executable name in lower case without the .exe suffix
	{ "zdoom",             EngineFamily::ZDoom },
	{ "lzdoom",            EngineFamily::ZDoom },
	{ "gzdoom",            EngineFamily::ZDoom },
	{ "qzdoom",            EngineFamily::ZDoom },
	{ "skulltag",          EngineFamily::ZDoom },
	{ "zandronum",         EngineFamily::ZDoom },
	{ "prboom",            EngineFamily::PrBoom },
	{ "prboom-plus",       EngineFamily::PrBoom },
	{ "glboom",            EngineFamily::PrBoom },
	{ "dsda-doom",         EngineFamily::PrBoom },
	{ "smmu",              EngineFamily::MBF },
	{ "eternity",          EngineFamily::MBF },
	{ "woof",              EngineFamily::MBF },
	{ "chocolate-doom",    EngineFamily::ChocolateDoom },
	{ "chocolate-heretic", EngineFamily::ChocolateDoom },
	{ "chocolate-hexen",   EngineFamily::ChocolateDoom },
	{ "crispy-doom",       EngineFamily::ChocolateDoom },
	{ "crispy-heretic",    EngineFamily::ChocolateDoom },
	{ "crispy-hexen",      EngineFamily::ChocolateDoom },
	{ "doomretro",         EngineFamily::ChocolateDoom },
	{ "strife-ve",         EngineFamily::ChocolateDoom },
	{ "edge",              EngineFamily::EDGE },
	{ "3dge",              EngineFamily::EDGE },
	{ "edge-classic",      EngineFamily::EDGE },
};

static const EngineFamilyTraits engineFamilyTraits [] =
{
	//ZDoom
	{
		.configFileSuffix = "ini",
		.saveFileSuffix = "zds",
		.saveDirParam = "-savedir",
		.mapParamStyle = MapParamStyle::Map,
		.compModeStyle = CompatModeStyle::ZDoom,
		.hasScreenshotDirParam = true,
		.needsStdoutParam = IS_WINDOWS,
	},

	//Chocolate Doom
	{
		.configFileSuffix = "cfg",
		.saveFileSuffix = "dsg",
		.saveDirParam = "-savedir",
		.mapParamStyle = MapParamStyle::Warp,
		.compModeStyle = CompatModeStyle::None,
		.hasScreenshotDirParam = false,
		.needsStdoutParam = false,
	},

	//PrBoom
	{
		.configFileSuffix = "cfg",
		.saveFileSuffix = "dsg",
		.saveDirParam = "-save",
		.mapParamStyle = MapParamStyle::Warp,
		.compModeStyle = CompatModeStyle::PrBoom,
		.hasScreenshotDirParam = false,
		.needsStdoutParam = false,
	},

	//MBF
	{
		.configFileSuffix = "cfg",
		.saveFileSuffix = "dsg",
		.saveDirParam = "-save",
		.mapParamStyle = MapParamStyle::Warp,
		.compModeStyle = CompatModeStyle::PrBoom,
		.hasScreenshotDirParam = false,
		.needsStdoutParam = false,
	},

	//EDGE
	{
		.configFileSuffix = "cfg",
		.saveFileSuffix = "esg",  // EDGE stores saves completely differently than all the other engines, but screw it
		.saveDirParam = "-savedir",  // not tested, probably doesn't work
		.mapParamStyle = MapParamStyle::Warp,
		.compModeStyle = CompatModeStyle::None,
		.hasScreenshotDirParam = false,
		.needsStdoutParam = false,
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
// code

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
	auto iter = knownEngineFamilies.find( normalizedName() );
	if (iter != knownEngineFamilies.end())
		return iter.value();
	// Of course there has to be an exception that does it differently than everybody else for no reason.
	// Who the hell thinks that adding version number to the executable file name is a good idea?!
	else if (normalizedName().startsWith("edge"))  // example: "edge135"
		return EngineFamily::EDGE;
	else
		return EngineFamily::ZDoom;
}

void EngineTraits::setFamilyTraits( EngineFamily family )
{
	_family = family;

	// assign family traits
	if (size_t(family) < std::size(engineFamilyTraits))
		_familyTraits = &engineFamilyTraits[ size_t(family) ];
	else
		_familyTraits = &engineFamilyTraits[ size_t(EngineFamily::ZDoom) ];  // use ZDoom traits as fallback

	// pre-compute the common subdirectory for save files, so that we don't have to repeat it on every IWAD change
	_commonSaveSubdir = getCommonSaveSubdir();
	_configFileName = getDefaultConfigFileName();
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
	else
		return fs::getParentDir( exePath() );                          // -> E:/Youda/Games/Doom/GZDoom  (may be relative - based on exePath)

 #else

	// On Linux they generally store them in the same dir as the configs.
	return getDefaultConfigDir();

 #endif
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

	if (isBasedOnGZDoomVersionOrLater({4,9,0}) && !isPortableInstallation())  // only new non-portable GZDoom
	{
		screenshotDir += "/screenshots";
	}

	// this luckily doesn't depend on the selected IWAD

	return screenshotDir;

 #endif
}

// For some engines the directory for save files consists of 3 parts:
//   1. the data directory:                        /home/youda/.config/gzdoom
//   2. the common part of the save subdirectory:  savegames
//   3. the part that depends on the used IWAD:    doom.id.doom2.commercial
//   -> full save directory path:                  /home/youda/.config/gzdoom/savegames/doom.id.doom2.commercial
// This function returns the common part (2.)
QString EngineTraits::getCommonSaveSubdir() const
{
	// engine          OS        version   installation   subdirectory
	//----------------------------------------------------------------
	// GZDoom          Windows   <  4.4    any
	// GZDoom          Windows   >= 4.4    any            Save
	// GZDoom          Windows   >= 4.9    portable       Save
	// GZDoom          Windows   >= 4.9    non-portable
	// GZDoom          Linux     <  4.11   any
	// GZDoom          Linux     >= 4.11   any            savegames
	// ChocolateDoom   Windows      any    any
	// ChocolateDoom   Linux        any    any            savegames
	// anything else   any          any    any

	// Thank you Graph! You're really making this world better and simpler.

	QString saveSubdirBase;

	if (_family == EngineFamily::ZDoom && normalizedName() == "gzdoom")
	{
		Version version = getExeVersionOrAssumeLatest();
		using v = Version;

		if (IS_WINDOWS)
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
		if (!IS_WINDOWS)
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

	// Some engines store their save files in a subdirectory named after the IWAD in use.
	if (saveDirDependsOnIWAD())
	{
		QString gameID;
		if (!IWADPath.isEmpty())
		{
			const auto & iwadInfo = doom::g_cachedWadInfo.getFileInfo( IWADPath );
			if (iwadInfo.status == ReadStatus::Success)
			{
				if (_family == EngineFamily::ChocolateDoom && iwadInfo.game.chocolateID != nullptr)
					gameID = iwadInfo.game.chocolateID;
				else if (isBasedOnGZDoomVersionOrLater({4,9,0}) && iwadInfo.game.gzdoomID != nullptr)
					gameID = iwadInfo.game.gzdoomID;
			}
		}
		if (gameID.isEmpty())
		{
			// fallback to Doom2 if the game cannot be identified, it has the best probability of being correct
			const doom::GameIdentification & defaultGame = doom::game::Doom2;
			gameID = _family == EngineFamily::ChocolateDoom ? defaultGame.chocolateID : defaultGame.gzdoomID;
		}

		saveDir = fs::appendToPath( saveDir, gameID );
	}

	return saveDir;
}

bool EngineTraits::saveDirDependsOnIWAD() const
{
	assert( isInitialized() );

	return isBasedOnGZDoomVersionOrLater({4,9,0}) || (_family == EngineFamily::ChocolateDoom && !IS_WINDOWS);
}

EngineTraits::SaveBaseDir EngineTraits::baseDirStyleForSaveFiles() const
{
	assert( isInitialized() );

	return isBasedOnGZDoomVersionOrLater({4,9,0}) ? SaveBaseDir::SaveDir : SaveBaseDir::WorkingDir;
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

static const QRegularExpression doom1MapNameRegex("E(\\d+)M(\\d+)");
static const QRegularExpression doom2MapNameRegex("MAP(\\d+)");

QStringVec EngineTraits::getMapArgs( int mapIdx, const QString & mapName ) const
{
	assert( isInitialized() );

	if (mapName.isEmpty())
	{
		return {};
	}

	if (_familyTraits->mapParamStyle == MapParamStyle::Map)  // this engine supports +map, we can use the map name directly
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

QStringVec EngineTraits::getCompatModeArgs( int compatMode ) const
{
	assert( isInitialized() );

	// Properly working -compatmode is present only in GZDoom 4.8.0+,
	// for other ZDoom-based engines use at least something, even if it doesn't fully work.
	if (isBasedOnGZDoomVersionOrLater({4,8,0}))
		return { "-compatmode", QString::number( compatMode ) };
	else if (_familyTraits->compModeStyle == CompatModeStyle::ZDoom)
		return { "+compatmode", QString::number( compatMode ) };
	else if (_familyTraits->compModeStyle == CompatModeStyle::PrBoom)
		return { "-complevel", QString::number( compatMode ) };
	else
		return {};
}

QString EngineTraits::getCmdMonitorIndex( int ownIndex ) const
{
	assert( isInitialized() );

	int startingMonitorIndex = 0;
	if (auto iter = startingMonitorIndexes.find( exeBaseName() ); iter != startingMonitorIndexes.end())
		startingMonitorIndex = iter.value();

	return QString::number( startingMonitorIndex + ownIndex );
}
