//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: templates and common code for application's internal caches
//======================================================================================================================

#ifndef FILE_INFO_CACHE_INCLUDED
#define FILE_INFO_CACHE_INCLUDED


#include "Essential.hpp"

#include "Utils/JsonUtils.hpp"
#include "Utils/FileSystemUtils.hpp"  // isValidFile

#include <QString>
#include <QHash>
#include <QFileInfo>
#include <QDateTime>

#include <QDebug>


//======================================================================================================================
//  templates for arbitrary file info cache

enum class ReadStatus
{
	Success,
	NotSupported,
	CantOpen,
	FailedToRead,
	InvalidFormat,
	InfoNotPresent,

	Uninitialized,
};
const char * statusToStr( ReadStatus status );
ReadStatus statusFromStr( const QString & statusStr );

template< typename FileInfo >
struct UncertainFileInfo : public FileInfo
{
	ReadStatus status = ReadStatus::Uninitialized;
};

template< typename FileInfo >
class FileInfoCache {

	struct Entry
	{
		UncertainFileInfo< FileInfo > fileInfo;
		qint64 lastModified;
	};

	using ReadFileInfoFunc = UncertainFileInfo< FileInfo > (*)( const QString & );

	QHash< QString, Entry > _cache;
	ReadFileInfoFunc _readFileInfo;
	mutable bool _dirty = false;

 public:


	FileInfoCache( ReadFileInfoFunc readFileInfo ) : _readFileInfo( readFileInfo ) {}

	/// Reads selected information from a file and stores it into a cache.
	/** If the file was already read earlier and was not modified since, it returns the cached info. */
	const UncertainFileInfo< FileInfo > & getFileInfo( const QString & filePath )
	{
		auto fileLastModified = QFileInfo( filePath ).lastModified().toSecsSinceEpoch();

		auto cacheIter = _cache.find( filePath );
		if (cacheIter == _cache.end())
		{
			//qDebug() << "FileInfoCache: entry for" << filePath << "not found, reading info from file";
			cacheIter = readFileInfoToCache( filePath, fileLastModified );
		}
		else if (cacheIter->lastModified != fileLastModified)
		{
			//qDebug() << "FileInfoCache: entry for" << filePath << "is outdated, reading info from file";
			cacheIter = readFileInfoToCache( filePath, fileLastModified );
		}
		else if (cacheIter->fileInfo.status == ReadStatus::CantOpen
			  || cacheIter->fileInfo.status == ReadStatus::FailedToRead)
		{
			//qDebug() << "FileInfoCache: reading file" << filePath << "failed last time, trying again";
			cacheIter = readFileInfoToCache( filePath, fileLastModified );
		}
		else if (cacheIter->fileInfo.status == ReadStatus::Uninitialized)
		{
			qWarning() << "FileInfoCache: entry for" << filePath << "is corrupted, reading info from file";
			cacheIter = readFileInfoToCache( filePath, fileLastModified );
		}
		else
		{
			//qDebug() << "FileInfoCache: using cached info for" << filePath;
		}

		return cacheIter->fileInfo;
	}

	/// Indicates whether the cache has been modified since the last time it was loaded from file or dumped to file.
	bool isDirty() const  { return _dirty; }

	QJsonObject serialize() const
	{
		QJsonObject jsMap;

		for (auto iter = _cache.begin(); iter != _cache.end(); ++iter)
		{
			// don't save invalid or empty entries
			if (iter->fileInfo.status == ReadStatus::Uninitialized || iter->fileInfo.status == ReadStatus::NotSupported)
			{
				continue;
			}

			jsMap[ iter.key() ] = serialize( iter.value() );
		}

		_dirty = false;

		return jsMap;
	}

	void deserialize( const JsonObjectCtx & jsCache )
	{
		_dirty = false;

		auto keys = jsCache.keys();
		for (QString & filePath : keys)
		{
			if (!fs::isValidFile( filePath ))
			{
				qDebug() << "FileInfoCache: removing entry for" << filePath << ", file no longer exists";
				_dirty = true;
				continue;
			}

			JsonObjectCtx jsEntry = jsCache.getObject( filePath );
			if (!jsEntry)
			{
				qWarning() << "FileInfoCache: removing corrupted entry for" << filePath << ", invalid JSON type";
				_dirty = true;
				continue;
			}

			Entry entry;
			deserialize( jsEntry, entry );
			if (entry.fileInfo.status == ReadStatus::Uninitialized || entry.lastModified == 0)
			{
				qWarning() << "FileInfoCache: removing corrupted entry for" << filePath << ", vital fields missing";
				_dirty = true;
				continue;
			}

			_cache.insert( std::move(filePath), std::move(entry) );
		}
	}

 private:

	auto readFileInfoToCache( const QString & filePath, qint64 fileModifiedTimestamp )
	{
		Entry newEntry;

		newEntry.fileInfo = _readFileInfo( filePath );
		newEntry.lastModified = fileModifiedTimestamp;

		if (newEntry.fileInfo.status == ReadStatus::CantOpen)
		{
			qInfo() << "FileInfoCache: couldn't open file" << filePath;
		}
		else if (newEntry.fileInfo.status == ReadStatus::FailedToRead)
		{
			qWarning() << "FileInfoCache: failed to read file" << filePath;
		}
		else if (newEntry.fileInfo.status == ReadStatus::FailedToRead)
		{
			//qDebug() << "FileInfoCache: file info for" << filePath << "not implemented";
		}

		_dirty = true;
		return _cache.insert( filePath, std::move(newEntry) );
	}

	static QJsonObject serialize( const Entry & cacheEntry )
	{
		QJsonObject jsFileInfo;

		jsFileInfo["status"] = statusToStr( cacheEntry.fileInfo.status );
		jsFileInfo["last_modified"] = cacheEntry.lastModified;

		cacheEntry.fileInfo.serialize( jsFileInfo );

		return jsFileInfo;
	}

	static void deserialize( const JsonObjectCtx & jsFileInfo, Entry & cacheEntry )
	{
		cacheEntry.fileInfo.status = statusFromStr( jsFileInfo.getString( "status" ) );
		cacheEntry.lastModified = jsFileInfo.getInt( "last_modified", 0 );

		cacheEntry.fileInfo.deserialize( jsFileInfo );
	}

};


#endif // FILE_INFO_CACHE
