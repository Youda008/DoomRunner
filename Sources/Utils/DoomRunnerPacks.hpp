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

/// Reads all entries in the DoomRunner Pack specified by \p filePath.
/** If an error occurs, it pops up a message box and returns an empty list. */
QStringList getEntries( const QString & filePath );


} // namespace drp


#endif // DOOM_RUNNER_PACKS_INCLUDED
