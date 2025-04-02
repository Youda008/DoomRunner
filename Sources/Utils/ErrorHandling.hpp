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
// misc

#define assert_msg( condition, message ) assert(( (void)message, (condition) ))


//======================================================================================================================
// displaying foreground errors that directly thwart features requested by the user

/// Reports an event that is not necessarily an error, but is worth noting. (example: no update available)
void reportInformation( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is a result of an incorrect usage of the application. (example: no item selected)
void reportUserError( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is usually not the user's fault, but can happen time to time. (example: network error)
void reportRuntimeError( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is a result of mistake in the code and should be fixed. (example: index out of bounds)
void reportLogicError( QWidget * parent, const QString & title, const QString & message );


//======================================================================================================================
// logging background errors that don't directly impact features requested by the user

namespace impl {

enum class LogLevel
{
	Debug,
	Info,
	Failure,
	Bug,
};
const char * logLevelToStr( LogLevel level );

/// Stream wrapper that logs to multiple streams depending on log level and build type
class LogStream
{
	// A helper class that aborts the program if it's a debug build and the level is high enough,
	// but only after all streams are already closed and flushed.
	struct Aborter
	{
		LogLevel _logLevel;
		Aborter( LogLevel logLevel ) : _logLevel( logLevel ) {}
		~Aborter();
	};

	Aborter _aborter;
	QDebug _debugStream;
	QFile _logFile;
	QTextStream _fileStream;

	LogLevel _logLevel;
	bool _canLogToFile;
	bool _addQuotes = true;
	//bool _addSpace = true;
	//bool _firstTokenWritten = false;

 public:

	LogStream( LogLevel level, const char * componentName, bool canLogToFile = true );
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

	void writeLineOpening( LogLevel level, const char * componentName );

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
		return IS_DEBUG_BUILD || fut::to_underlying( _logLevel ) >= fut::to_underlying( LogLevel::Info );
	}

	inline constexpr bool shouldWriteToFileStream() const
	{
		return _canLogToFile && fut::to_underlying( _logLevel ) >= fut::to_underlying( LogLevel::Failure );
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

	DummyLogStream() {}

	DummyLogStream & quote() { return *this; }
	DummyLogStream & noquote() { return *this; }
	//DummyLogStream & space() { return *this; }
	//DummyLogStream & nospace() { return *this; }

	template< typename Obj >
	DummyLogStream & operator<<( const Obj & ) { return *this; }

};

} // namespace impl


//----------------------------------------------------------------------------------------------------------------------
// top-level logging API

/// Logs a debugging message into stderr (in debug builds only).
inline auto logDebug( [[maybe_unused]] const char * componentName = nullptr )
{
 #if IS_DEBUG_BUILD
	return impl::LogStream( impl::LogLevel::Debug, componentName );
 #else
	return impl::DummyLogStream();
 #endif
}

/// Logs a message about an event that is not necessarily an error, but is worth noting.
inline auto logInfo( const char * componentName = nullptr )
{
	return impl::LogStream( impl::LogLevel::Info, componentName );
}

/// Logs a message about a non-critical background error into stderr and an error file.
inline auto logRuntimeError( const char * componentName = nullptr )
{
	return impl::LogStream( impl::LogLevel::Failure, componentName );
}

/// Logs a message about a serious background error into stderr and an error file.
inline auto logLogicError( const char * componentName = nullptr )
{
	return impl::LogStream( impl::LogLevel::Bug, componentName );
}

// Workaround that only prints the messages to console and doesn't write it the log file,
// in case a messages needs to be logged before the log file is successfully open.

inline auto printDebug( [[maybe_unused]] const char * componentName = nullptr )
{
 #if IS_DEBUG_BUILD
	return impl::LogStream( impl::LogLevel::Debug, componentName, /*canLogToFile*/ false );
 #else
	return impl::DummyLogStream();
 #endif
}
inline auto printInfo( const char * componentName = nullptr )
{
	return impl::LogStream( impl::LogLevel::Info, componentName, /*canLogToFile*/ false );
}
inline auto printRuntimeError( const char * componentName = nullptr )
{
	return impl::LogStream( impl::LogLevel::Failure, componentName, /*canLogToFile*/ false );
}
inline auto printLogicError( const char * componentName = nullptr )
{
	return impl::LogStream( impl::LogLevel::Bug, componentName, /*canLogToFile*/ false );
}


//----------------------------------------------------------------------------------------------------------------------
// logging helpers for simplifying logging even further

/// Abstract component that wants to log messages.
/** Any class that inherits from this will be able to log without having to write component name everytime. */
class LoggingComponent {

 protected:

	LoggingComponent( const char * componentName ) : _componentName( componentName ) {}

	auto logLogicError() const     { return ::logLogicError( _componentName ); }
	auto logRuntimeError() const   { return ::logRuntimeError( _componentName ); }
	auto logInfo() const           { return ::logInfo( _componentName ); }
	auto logDebug() const          { return ::logDebug( _componentName ); }

 private:

	const char * _componentName;

};


#endif // ERROR_HANDLING_INCLUDED
