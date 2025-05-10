//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: support for Doom Mod Bundles - a batch of paths of files to load
//======================================================================================================================

#ifndef DOOM_RUNNER_PACKS_INCLUDED
#define DOOM_RUNNER_PACKS_INCLUDED


#include "Essential.hpp"

#include <QString>
#include <QStringList>

#include <optional>


namespace dmb {


extern const QString fileSuffix;   ///< Doom Mod Bundle - a batch of paths of files to load

/// Reads all entries from a Doom Mod Bundle specified by \p filePath.
/** On error it pops up a message box and returns a nullopt. */
std::optional< QStringList > getEntries( const QString & filePath );

/// Saves the given entries into a Doom Mod Bundle specified by \p filePath.
/** On error it pops up a message box and returns false. */
bool saveEntries( const QString & filePath, QStringList entries );


} // namespace dmb


#endif // DOOM_RUNNER_PACKS_INCLUDED
