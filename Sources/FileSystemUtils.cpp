//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  5.4.2020
// Description: utilities concerning paths, directories and files
//======================================================================================================================

#include "FileSystemUtils.hpp"

#include <QDirIterator>
#include <QFile>
#include <QStringBuilder>
#include <QTextStream>
#include <QDesktopServices>
#include <QProcess>
#include <QUrl>


//======================================================================================================================

void traverseDirectory(
	const QString & dir, bool recursively, EntryTypes typesToVisit,
	const PathContext & pathContext, const std::function< void ( const QFileInfo & entry ) > & visitEntry
)
{
	if (dir.isEmpty())
		return;

	QDir dir_( dir );
	if (!dir_.exists())
		return;

	QDirIterator dirIt( dir_ );
	while (dirIt.hasNext())
	{
		QString entryPath = pathContext.convertPath( dirIt.next() );
		QFileInfo entry( entryPath );
		if (entry.isDir())
		{
			QString dirName = dirIt.fileName();  // we need the original entry name including "." and "..", entry is already converted
			if (dirName != "." && dirName != "..")
			{
				if (typesToVisit.isSet( EntryType::DIR ))
					visitEntry( entry );
				if (recursively)
					traverseDirectory( entry.filePath(), recursively, typesToVisit, pathContext, visitEntry );
			}
		}
		else
		{
			if (typesToVisit.isSet( EntryType::FILE ))
				visitEntry( entry );
		}
	}
}

QString updateFile( const QString & filePath, const QByteArray & newContent )
{
	// Write to a different file than the original and after it's done and closed, replace the original with the new.
	// This is done to prevent data loss, when the program (or OS) crashes during writing to disc.

	QString newFilePath = filePath+".new";
	QFile newFile( newFilePath );
	if (!newFile.open( QIODevice::WriteOnly ))
	{
		return "Could not open file "%newFilePath%" for writing: "%newFile.errorString();
	}

	newFile.write( newContent );
	if (newFile.error() != QFile::NoError)
	{
		return "Could not write to file "%newFilePath%": "%newFile.errorString();
	}

	newFile.close();

	QFile oldFile( filePath );
	if (oldFile.exists())
	{
		if (!oldFile.remove())
		{
			return "Could not delete the previous options file "%filePath%": "%newFile.errorString();
		}
	}

	if (!newFile.rename( filePath ))
	{
		return "Could rename new file "%newFilePath%" back to "%filePath%": "%newFile.errorString();
	}

	return {};
}

bool openFileLocation( const QString & filePath )
{
	// based on answers at https://stackoverflow.com/questions/3490336/how-to-reveal-in-finder-or-show-in-explorer-with-qt

	const QFileInfo fileInfo( filePath );

#if defined(Q_OS_WIN)

	QStringList args;
	if (!fileInfo.isDir())
		args += "/select,";
	args += QDir::toNativeSeparators( fileInfo.canonicalFilePath() );
	return QProcess::startDetached( "explorer.exe", args );

#elif defined(Q_OS_MAC)

	QStringList args;
	args << "-e";
	args << "tell application \"Finder\"";
	args << "-e";
	args << "activate";
	args << "-e";
	args << "select POSIX file \"" + fileInfo.canonicalFilePath() + "\"";
	args << "-e";
	args << "end tell";
	args << "-e";
	args << "return";
	return QProcess::execute( "/usr/bin/osascript", args );

#else

	// We cannot select a file here, because no file browser really supports it.
	return QDesktopServices::openUrl( QUrl::fromLocalFile( fileInfo.isDir()? fileInfo.filePath() : fileInfo.path() ) );

#endif
}

QString makeFileFilter( const char * filterName, const QVector<QString> & suffixes )
{
	QString filter;
	QTextStream filterStream( &filter, QIODevice::WriteOnly );

	filterStream << filterName << " (";
	for (const QString & suffix : suffixes)
		if (&suffix == &suffixes[0])
			filterStream <<  "*." << suffix << " *." << suffix.toUpper();
		else
			filterStream << " *." << suffix << " *." << suffix.toUpper();
	filterStream << ");;";

	filterStream.flush();
	return filter;
}
