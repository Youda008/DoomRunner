//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: templates and common code for application's internal caches
//======================================================================================================================

#ifndef FILE_INFO_CACHE_INCLUDED
#define FILE_INFO_CACHE_INCLUDED


#include "Essential.hpp"

#include "FileInfoCacheTypes.hpp"

#include "JsonUtils.hpp"
#include "FileSystemUtils.hpp"  // isValidFile
#include "ErrorHandling.hpp"

#include <QString>
#include <QHash>
#include <QFileInfo>
#include <QDateTime>
#include <QElapsedTimer>


//======================================================================================================================
/// Template for arbitrary file info cache.
/** Implements caching of arbitrary data read from a file according to the file's last modification time. */

template< typename FileInfo >
class FileInfoCache : protected LoggingComponent {

	struct Entry
	{
		UncertainFileInfo< FileInfo > fileInfo;
		qint64 lastModified;
	};

	using ReadFileInfoFunc = UncertainFileInfo< FileInfo > (*)( const QString & );
	using WriteFileInfoFunc = bool (*)( const QString &, const FileInfo & );

	QHash< QString, Entry > _cache;
	ReadFileInfoFunc _readFileInfo;
	WriteFileInfoFunc _writeFileInfo;
	mutable bool _dirty = false;

	QElapsedTimer _timer;

 public:

	FileInfoCache( ReadFileInfoFunc readFileInfo, WriteFileInfoFunc writeFileInfo = nullptr )
		: LoggingComponent( u"FileInfoCache" ), _readFileInfo( readFileInfo ), _writeFileInfo( writeFileInfo ) {}

	/// Reads selected information from a file and stores it into a cache.
	/** If the file was already read earlier and was not modified since, it returns the cached info. */
	const UncertainFileInfo< FileInfo > & getFileInfo( const QString & filePath )
	{
		auto fileLastModified = QFileInfo( filePath ).lastModified().toSecsSinceEpoch();

		auto cacheIter = _cache.find( filePath );
		Entry * cacheEntry = cacheIter != _cache.end() ? &cacheIter.value() : nullptr;

		if (cacheEntry == nullptr)
		{
			logDebug() << "entry not found, reading info from file: " << filePath;
			cacheEntry = readFileInfoToCache( filePath, fileLastModified );
		}
		else if (cacheEntry->lastModified != fileLastModified)
		{
			logDebug() << "entry is outdated, reading info from file: " << filePath;
			cacheEntry = readFileInfoToCache( filePath, fileLastModified );
		}
		else if (cacheEntry->fileInfo.status == ReadStatus::CantOpen
			  || cacheEntry->fileInfo.status == ReadStatus::FailedToRead)
		{
			logDebug() << "reading file failed last time, trying again: " << filePath;
			cacheEntry = readFileInfoToCache( filePath, fileLastModified );
		}
		else if (cacheEntry->fileInfo.status == ReadStatus::Uninitialized)
		{
			logRuntimeError() << "entry is corrupted, reading info from file: " << filePath;
			cacheEntry = readFileInfoToCache( filePath, fileLastModified );
		}
		else
		{
			//logDebug() << "using cached info: " << filePath;
		}

		return cacheEntry->fileInfo;
	}

	/// Manually updates a record in the cache and writes the content to the corresponding file.
	/** Returns false if the content couldn't be written to the file. */
	bool setFileInfo( const QString & filePath, FileInfo fileInfo )
	{
		logDebug() << "writing info to cache and file: " << filePath;

		Entry & newEntry = _cache.insert( filePath, {} ).value();

		static_cast< FileInfo & >( newEntry.fileInfo ) = std::move( fileInfo );
		newEntry.fileInfo.status = ReadStatus::Success;
		newEntry.lastModified = QDateTime::currentSecsSinceEpoch();

		return _writeFileInfo( filePath, newEntry.fileInfo );
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
				logDebug() << "removing entry, file no longer exists: " << filePath;
				_dirty = true;
				continue;
			}

			JsonObjectCtx jsEntry = jsCache.getObject( filePath );
			if (!jsEntry)
			{
				logRuntimeError() << "removing corrupted entry (invalid JSON type): " << filePath;
				_dirty = true;
				continue;
			}

			Entry entry;
			deserialize( jsEntry, entry );
			if (entry.fileInfo.status == ReadStatus::Uninitialized || entry.lastModified == 0)
			{
				logRuntimeError() << "removing corrupted entry (vital fields missing): " << filePath;
				_dirty = true;
				continue;
			}

			_cache.insert( std::move(filePath), std::move(entry) );
		}
	}

 private:

	Entry * readFileInfoToCache( const QString & filePath, qint64 fileModifiedTimestamp )
	{
		Entry & newEntry = _cache.insert( filePath, {} ).value();

		_timer.restart();
		newEntry.fileInfo = _readFileInfo( filePath );
		auto elapsed = _timer.elapsed();

		if (newEntry.fileInfo.status == ReadStatus::Success)
		{
			logDebug() << " -> success (took "<<elapsed<<"ms)";
		}
		if (newEntry.fileInfo.status == ReadStatus::CantOpen)
		{
			logDebug() << " -> couldn't open file";
		}
		else if (newEntry.fileInfo.status == ReadStatus::FailedToRead)
		{
			logDebug() << " -> failed to read file";
		}
		else if (newEntry.fileInfo.status == ReadStatus::NotSupported)
		{
			logDebug() << " -> not implemented";
		}

		newEntry.lastModified = fileModifiedTimestamp;
		_dirty = true;

		return &newEntry;
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


//======================================================================================================================


#endif // FILE_INFO_CACHE
