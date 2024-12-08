//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: the data user enters into the launcher
//======================================================================================================================

#include "UserData.hpp"

#include <QFileIconProvider>
#include <QString>
#include <QHash>


//----------------------------------------------------------------------------------------------------------------------
// icons

static const QIcon emptyIcon;

// Not sure how QIcon is implemented and how heavy is its construction and copying,
// so we better cache it and return references.
// Especially on Windows the icon loading via QFileIconProvider seems notably slow.
static QHash< QString, QIcon > g_filesystemIconCache;

// better construct it only once rather than in every call
static std::optional< QFileIconProvider > g_iconProvider;

const QIcon & Mod::getIcon() const
{
	if (isCmdArg)
	{
		return emptyIcon;
	}

	if (!g_iconProvider)
	{
		g_iconProvider.emplace();
		g_iconProvider->setOptions( QFileIconProvider::DontUseCustomDirectoryIcons );  // custom dir icons might cause freezes
	}

	// File icons are mostly determined by file suffix, so we can cache the icons only for the suffixes to load less
	// icons in total. The only exception is when a file has no suffix on Linux, then the icon can be determined
	// by file header, but those files will not be used as mods, so we can ignore that.
	// Special handling is needed for directories, because they don't have suffixes (usually),
	// but we want to display them differently than files without suffixes.

	QFileInfo entryInfo( this->path );
	QString entryID = entryInfo.isDir() ? "<dir>" : entryInfo.suffix().toLower();

	auto iter = g_filesystemIconCache.find( entryID );
	if (iter == g_filesystemIconCache.end())
	{
		QIcon origIcon = g_iconProvider->icon( entryInfo );

		// strip the icon from unnecessary high-res variants that slow down the painting process
		QSize smallestSize = origIcon.availableSizes().at(0);
		QPixmap pixmap = origIcon.pixmap( smallestSize );

		iter = g_filesystemIconCache.insert( std::move( entryID ), QIcon( pixmap ) );
	}
	return iter.value();
}
