//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common error-handling routines
//======================================================================================================================

#ifndef ERROR_HANDLING_INCLUDED
#define ERROR_HANDLING_INCLUDED


#include "Essential.hpp"

#include "LangUtils.hpp"

#include <QString>
#include <QtCore/qdebug.h>
#include <QTextStream>
#include <QFile>

class QWidget;


//======================================================================================================================
//  displaying foreground errors that directly thwart features requested by the user

/// Reports an event that is not necessarily an error, but is worth noting. (example: no update available)
void reportInformation( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is a result of an incorrect usage of the application. (example: no item selected)
void reportUserError( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is usually not the user's fault, but can happen time to time. (example: network error)
void reportRuntimeError( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is a result of mistake in the code and should be fixed. (example: index out of bounds)
void reportLogicError( QWidget * parent, const QString & title, const QString & message );


//======================================================================================================================
//  logging background errors that don't directly impact features requested by the user

namespace impl {

enum class LogLevel
{
	Debug,
	Info,
	Failure,
	Bug,
};

/// Stream wrapper that logs to multiple streams depending on log level and build type
class LogStream
{
	QDebug _debugStream;
	QFile _logFile;
	QTextStream _fileStream;

	LogLevel _logLevel;
	bool _addQuotes = true;
	//bool _addSpace = true;
	bool _firstTokenWritten = false;

 public:

	LogStream( LogLevel level );
	~LogStream();

	LogStream & quote()
	{
		_addQuotes = true;
		return *this;
	}
	LogStream & noquote()
	{
		_addQuotes = false;
		return *this;
	}
	/*LogStream & space()
	{
		_addSpace = true;
		return *this;
	}
	LogStream & nospace()
	{
		_addSpace = false;
		return *this;
	}*/

	template< typename Obj >
	LogStream & operator<<( const Obj & obj )
	{
		/*if (_addSpace)
		{
			if (!_firstTokenWritten)
				_firstTokenWritten = true;
			else
				write(' ');
		}*/
		if (std::is_same_v< Obj, QString > && _addQuotes) write('"');
		write( obj );
		if (std::is_same_v< Obj, QString > && _addQuotes) write('"');
		return *this;
	}

 private:

	static QDebug debugStreamFromLogLevel( LogLevel level );

	void writeLineOpening();

	template< typename Obj >
	void write( const Obj & obj )
	{
		if (shouldWriteToDebugStream())
			_debugStream << obj;
		if (shouldAndCanWriteToFileStream())
			_fileStream << obj;
	}

	inline constexpr bool shouldWriteToDebugStream() const
	{
		return fut::to_underlying( _logLevel ) >= fut::to_underlying( LogLevel::Info ) || IS_DEBUG_BUILD;
	}

	inline constexpr bool shouldWriteToFileStream() const
	{
		return fut::to_underlying( _logLevel ) >= fut::to_underlying( LogLevel::Failure );
	}

	inline bool shouldAndCanWriteToFileStream() const
	{
		return _logFile.isOpen(); //&& shouldWriteToFileStream()  redundant, log file is always open when should write
	}
};

/// Stream wrapper that does nothing (used to eliminate debug messages in release builds)
class DummyLogStream
{
 public:

	DummyLogStream( LogLevel ) {}

	DummyLogStream & space() { return *this; }
	DummyLogStream & nospace() { return *this; }

	template< typename Obj >
	DummyLogStream & operator<<( const Obj & ) { return *this; }

};

} // namespace impl


//----------------------------------------------------------------------------------------------------------------------
//  top-level logging API

/// Writes a debugging message into stderr (in debug builds only).
inline auto logDebug()
{
 #if IS_DEBUG_BUILD
	return impl::LogStream( impl::LogLevel::Debug );
 #else
	return impl::DummyLogStream( impl::LogLevel::Debug );
 #endif
}

/// Writes a message about an event that is not necessarily an error, but is worth noting.
inline auto logInfo()
{
	return impl::LogStream( impl::LogLevel::Info );
}

/// Writes a message about a non-critical background error into stderr and an error file.
inline auto logRuntimeError()
{
	return impl::LogStream( impl::LogLevel::Failure );
}

/// Writes a message about a serious background error into stderr and an error file.
inline auto logLogicError()
{
	return impl::LogStream( impl::LogLevel::Bug );
}


#endif // ERROR_HANDLING_INCLUDED
