//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: support for DoomRunner Packs - a batch of paths of files to load
//======================================================================================================================

#include "DoomRunnerPacks.hpp"

#include "FileInfoCache.hpp"
#include "ErrorHandling.hpp"

#include <QFile>
#include <QTextStream>


namespace drp {


//======================================================================================================================

const QString fileSuffix = "drp";


//----------------------------------------------------------------------------------------------------------------------

struct DrpContent
{
	QStringList entries;
};

using UncertainDrpContent = UncertainFileInfo< DrpContent >;

UncertainDrpContent readContent( const QString & filePath )
{
	UncertainDrpContent content;

	QFile file( filePath );
	if (!file.open( QIODevice::Text | QIODevice::ReadOnly ))
	{
		reportRuntimeError( nullptr, "Cannot read DoomRunner Pack", "Could not open file "%filePath%" for reading ("%file.errorString()%")" );
		content.status = ReadStatus::CantOpen;
		return content;
	}

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
		content.entries.append( std::move(line) );
	}

	file.close();
	content.status = ReadStatus::Success;
	return content;
}

static FileInfoCache< DrpContent > g_cachedDrpInfo( readContent );

QStringList getEntries( const QString & filePath )
{
	// We need this everytime the command is re-generated, which is pretty often, so we better cache it.
	auto uncertainContent = g_cachedDrpInfo.getFileInfo( filePath );
	if (uncertainContent.status == ReadStatus::Success)
		return std::move( uncertainContent.entries );
	else
		return {};
}


} // namespace drp
