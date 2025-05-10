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
#include <QStringView>
#include <QtCore/qdebug.h>
#include <QTextStream>
#include <QFile>
class QWidget;

#include <memory>


//======================================================================================================================
// misc

#define assert_msg( condition, message ) assert(( (void)message, (condition) ))


//======================================================================================================================
// displaying foreground errors that directly thwart features requested by the user

/// Reports an event that is not necessarily an error, but is worth noting. (example: no update available)
/** \param parent Parent widget for the error message box. See https://doc.qt.io/qt-6/qdialog.html#QDialog */
void reportInformation( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is a result of an incorrect usage of the application. (example: no item selected)
/** \param parent Parent widget for the error message box. See https://doc.qt.io/qt-6/qdialog.html#QDialog */
void reportUserError( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is usually not the user's fault, but can happen time to time. (example: network error)
/** \param parent Parent widget for the error message box. See https://doc.qt.io/qt-6/qdialog.html#QDialog */
void reportRuntimeError( QWidget * parent, const QString & title, const QString & message );

/// Reports an error that is a result of mistake in the code and should be fixed. (example: index out of bounds)
/** \param parent Parent widget for the error message box. See https://doc.qt.io/qt-6/qdialog.html#QDialog
  * \param locationTag Short spaceless description of where the error occured. */
void reportLogicError( QWidget * parent, QStringView locationTag, const QString & title, const QString & message );


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
	std::unique_ptr< QDebug > _debugStream;
	QFile _logFile;
	QTextStream _fileStream;

	LogLevel _logLevel;
	bool _canLogToFile;
	bool _addQuotes = true;
	bool _addSpace = true;
	bool _firstTokenWritten = false;

 public:

	LogStream( LogLevel level, QStringView locationTag, bool canLogToFile = true );
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
	LogStream & space()
	{
		_addSpace = true;
		return *this;
	}
	LogStream & nospace()
	{
		_addSpace = false;
		return *this;
	}

	template< typename Obj >
	LogStream & operator<<( const Obj & obj )
	{
		if (_addSpace)
		{
			if (!_firstTokenWritten)
				_firstTokenWritten = true;
			else
				write(' ');
		}
		if (std::is_same_v< Obj, QString > && _addQuotes) write('"');
		write( obj );
		if (std::is_same_v< Obj, QString > && _addQuotes) write('"');
		return *this;
	}

	void flush()
	{
		// deleting the QDebug is the only way to flush the debug stream ..... really Qt? -_-
		_debugStream.reset();
		_fileStream.flush();
		_logFile.flush();
	}

 private:

	static std::unique_ptr< QDebug > debugStreamFromLogLevel( LogLevel level );

	void writeLineOpening( LogLevel level, QStringView locationTag );

	template< typename Obj >
	void write( const Obj & obj )
	{
		if (shouldWriteToDebugStream())
			*_debugStream << obj;
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
	DummyLogStream & space() { return *this; }
	DummyLogStream & nospace() { return *this; }

	template< typename Obj >
	DummyLogStream & operator<<( const Obj & ) { return *this; }

};

} // namespace impl


//----------------------------------------------------------------------------------------------------------------------
// top-level logging API

/// Logs a debugging message into stderr (in debug builds only).
inline auto logDebug( [[maybe_unused]] QStringView locationTag = {} )
{
 #if IS_DEBUG_BUILD
	return impl::LogStream( impl::LogLevel::Debug, locationTag );
 #else
	return impl::DummyLogStream();
 #endif
}

/// Logs a message about an event that is not necessarily an error, but is worth noting.
inline auto logInfo( QStringView locationTag = {} )
{
	return impl::LogStream( impl::LogLevel::Info, locationTag );
}

/// Logs a message about a non-critical background error into stderr and an error file.
inline auto logRuntimeError( QStringView locationTag = {} )
{
	return impl::LogStream( impl::LogLevel::Failure, locationTag );
}

/// Logs a message about a serious background error into stderr and an error file.
inline auto logLogicError( QStringView locationTag = {} )
{
	return impl::LogStream( impl::LogLevel::Bug, locationTag );
}

// Workaround that only prints the messages to console and doesn't write it the log file,
// in case a messages needs to be logged before the log file is successfully open.

inline auto printDebug( [[maybe_unused]] QStringView locationTag = {} )
{
 #if IS_DEBUG_BUILD
	return impl::LogStream( impl::LogLevel::Debug, locationTag, /*canLogToFile*/ false );
 #else
	return impl::DummyLogStream();
 #endif
}
inline auto printInfo( QStringView locationTag = {} )
{
	return impl::LogStream( impl::LogLevel::Info, locationTag, /*canLogToFile*/ false );
}
inline auto printRuntimeError( QStringView locationTag = {} )
{
	return impl::LogStream( impl::LogLevel::Failure, locationTag, /*canLogToFile*/ false );
}
inline auto printLogicError( QStringView locationTag = {} )
{
	return impl::LogStream( impl::LogLevel::Bug, locationTag, /*canLogToFile*/ false );
}


//----------------------------------------------------------------------------------------------------------------------
// logging helpers for simplifying logging even further

/// Abstract component that wants to log messages.
/** Any class that inherits from this will be able to log without having to write component name everytime. */
class LoggingComponent {

 protected:

	LoggingComponent( QStringView componentType, QStringView componentName = {} )
		: _componentType( componentType ), _componentName( componentName ) {}

	QStringView componentType() const  { return _componentType; }
	QStringView componentName() const  { return _componentName; }

	auto logDebug( QStringView funcName = {} ) const         { return ::logDebug( makeLocationTag( funcName ) ); }
	auto logInfo( QStringView funcName = {} ) const          { return ::logInfo( makeLocationTag( funcName ) ); }
	auto logRuntimeError( QStringView funcName = {} ) const  { return ::logRuntimeError( makeLocationTag( funcName ) ); }
	auto logLogicError( QStringView funcName = {} ) const    { return ::logLogicError( makeLocationTag( funcName ) ); }

 protected:

	QStringView _componentType;  // for example: "ListView"
	QStringView _componentName;  // for example: "iwadList"

	QString makeLocationTag( QStringView funcName ) const;

};

class ErrorReportingComponent : public LoggingComponent {

 protected:

	ErrorReportingComponent( QWidget * self, QStringView componentType, QStringView componentName = {} )
		: LoggingComponent( componentType, componentName ), _self( self ) {}

	// These don't need the source location tag, because they don't indicate a bug.
	void reportInformation( const QString & title, const QString & message ) const   { ::reportInformation( _self, title, message ); }
	void reportUserError( const QString & title, const QString & message ) const     { ::reportUserError( _self, title, message ); }
	void reportRuntimeError( const QString & title, const QString & message ) const  { ::reportRuntimeError( _self, title, message ); }

	// Logic errors should be more detailed, so that we have enough information to debug and fix them.
	void reportLogicError( QStringView funcName, const QString & title, const QString & message ) const
	{
		::reportLogicError( _self, makeLocationTag( funcName ), title, message );
	}

 protected:

	QWidget * _self;

};


#endif // ERROR_HANDLING_INCLUDED
