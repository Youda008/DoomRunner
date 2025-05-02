//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: support for DoomRunner Packs - a batch of paths of files to load
//======================================================================================================================

#include "DoomRunnerPacks.hpp"

#include "FileSystemUtils.hpp"
#include "FileInfoCache.hpp"
#include "ErrorHandling.hpp"

#include <QFile>
#include <QTextStream>


namespace drp {


//======================================================================================================================

const QString fileSuffix = "drp";


//----------------------------------------------------------------------------------------------------------------------

struct DRPContent
{
	QStringList entries;
};

using UncertainDrpContent = UncertainFileInfo< DRPContent >;

static UncertainDrpContent readContent( const QString & filePath )
{
	UncertainDrpContent content;

	QFile file( filePath );
	if (!file.open( QIODevice::Text | QIODevice::ReadOnly ))
	{
		reportRuntimeError( nullptr, "Cannot read DoomRunner Pack", "Could not open file "%filePath%" for reading ("%file.errorString()%")" );
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
			reportRuntimeError( nullptr, "Cannot read DoomRunner Pack", "Error occured while reading a file "%filePath%" ("%file.errorString()%")" );
			content.status = ReadStatus::FailedToRead;
			return content;
		}
		// skip empty or commented lines
		if (line.isEmpty() || line.startsWith('#'))
		{
			continue;
		}
		// rebase the paths from the DRP's dir to our working dir
		QString entryPath = rebaser.rebaseBack( line );

		content.entries.append( std::move(entryPath) );
	}

	file.close();
	content.status = ReadStatus::Success;
	return content;
}

static bool writeContent( const QString & filePath, const DRPContent & content )
{
	QFile file( filePath );
	if (!file.open( QIODevice::Text | QIODevice::WriteOnly ))
	{
		reportRuntimeError( nullptr, "Cannot save DoomRunner Pack", "Could not open file "%filePath%" for writing ("%file.errorString()%")" );
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


static FileInfoCache< DRPContent > g_cachedDrpInfo( readContent, writeContent );

QStringList getEntries( const QString & filePath )
{
	// We need this everytime the command is re-generated, which is pretty often, so we better cache it.
	auto uncertainContent = g_cachedDrpInfo.getFileInfo( filePath );
	if (uncertainContent.status == ReadStatus::Success)
		return std::move( uncertainContent.entries );
	else
		return {};
}

bool saveEntries( const QString & filePath, QStringList entries )
{
	return g_cachedDrpInfo.setFileInfo( filePath, DRPContent{ std::move( entries ) } );
}


} // namespace drp
