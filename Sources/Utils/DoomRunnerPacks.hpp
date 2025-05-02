//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: support for DoomRunner Packs - a batch of paths of files to load
//======================================================================================================================

#ifndef DOOM_RUNNER_PACKS_INCLUDED
#define DOOM_RUNNER_PACKS_INCLUDED


#include "Essential.hpp"

#include <QString>
#include <QStringList>


namespace drp {


extern const QString fileSuffix;   ///< DoomRunner Pack - a batch of paths of files to load

/// Reads all entries from a DoomRunner Pack specified by \p filePath.
/** On error it pops up a message box and returns an empty list. */
QStringList getEntries( const QString & filePath );

/// Saves the given entries into a DoomRunner Pack specified by \p filePath.
/** On error it pops up a message box and returns false. */
bool saveEntries( const QString & filePath, QStringList entries );


} // namespace drp


#endif // DOOM_RUNNER_PACKS_INCLUDED
