//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: support for Doom Mod Bundles - a batch of paths of files to load
//======================================================================================================================

#include "DoomModBundles.hpp"

#include "FileSystemUtils.hpp"
#include "FileInfoCache.hpp"
#include "ErrorHandling.hpp"

#include <QFile>
#include <QTextStream>


namespace dmb {


//======================================================================================================================

const QString fileSuffix = "dmb";


//----------------------------------------------------------------------------------------------------------------------

struct DMBContent
{
	QStringList entries;
};

using UncertainDMBContent = UncertainFileInfo< DMBContent >;

static UncertainDMBContent readContent( const QString & filePath )
{
	UncertainDMBContent content;

	QFile file( filePath );
	if (!file.open( QIODevice::Text | QIODevice::ReadOnly ))
	{
		// Here we don't want to pop up a message box, because that would show up too often and would be annoying.
		logRuntimeError() << "could not open file "%filePath%" for reading ("%file.errorString()%")";
		content.status = ReadStatus::CantOpen;
		return content;
	}

	PathRebaser rebaser( fs::currentDir, fs::getParentDir( filePath ) );

	QTextStream stream( &file );
	while (!stream.atEnd())
	{
		QString line = stream.readLine();
		// abort on error
		if (file.error() != QFile::NoError)
		{
			logRuntimeError() << "error occured while reading a file "%filePath%" ("%file.errorString()%")";
			content.status = ReadStatus::FailedToRead;
			return content;
		}
		// skip empty or commented lines
		if (line.isEmpty() || line.startsWith('#'))
		{
			continue;
		}
		// rebase the paths from the DMB's dir to our working dir
		QString entryPath = rebaser.rebaseBack( line );

		content.entries.append( std::move(entryPath) );
	}

	file.close();
	content.status = ReadStatus::Success;
	return content;
}

static bool writeContent( const QString & filePath, const DMBContent & content )
{
	QFile file( filePath );
	if (!file.open( QIODevice::Text | QIODevice::WriteOnly ))
	{
		reportRuntimeError( nullptr, "Cannot save Mod Bundle", "Could not open file "%filePath%" for writing ("%file.errorString()%")" );
		return false;
	}

	PathRebaser rebaser( fs::currentDir, fs::getParentDir( filePath ) );

	QTextStream stream( &file );
	for (const auto & entryPath : content.entries)
	{
		QString rebasedPath = rebaser.rebase( entryPath );
		stream << rebasedPath << '\n';
	}

	stream.flush();
	file.close();
	return true;
}


static FileInfoCache< DMBContent > g_cachedDMBInfo( readContent, writeContent );

std::optional< QStringList > getEntries( const QString & filePath )
{
	// We need this everytime the command is re-generated, which is pretty often, so we better cache it.
	auto uncertainContent = g_cachedDMBInfo.getFileInfo( filePath );
	if (uncertainContent.status == ReadStatus::Success)
		return std::move( uncertainContent.entries );
	else
		return std::nullopt;
}

bool saveEntries( const QString & filePath, QStringList entries )
{
	return g_cachedDMBInfo.setFileInfo( filePath, DMBContent{ std::move( entries ) } );
}


} // namespace dmb
