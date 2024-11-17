//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common error-handling routines
//======================================================================================================================

#include "ErrorHandling.hpp"

#include "WidgetUtils.hpp"  // HYPERLINK
//#include "StandardOutput.hpp"
#include "OSUtils.hpp"          // getThisAppDataDir
#include "FileSystemUtils.hpp"  // getPathFromFileName

#include <QStringBuilder>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>


//======================================================================================================================
//  displaying foreground errors

static const QString issuePageUrl = "https://github.com/Youda008/DoomRunner/issues";

void reportInformation( QWidget * parent, const QString & title, const QString & message )
{
	QMessageBox::information( parent, title, message );
}

void reportUserError( QWidget * parent, const QString & title, const QString & message )
{
	QMessageBox::warning( parent, title, message );
}

void reportRuntimeError( QWidget * parent, const QString & title, const QString & message )
{
	QMessageBox::warning( parent, title, message );
	logRuntimeError().noquote() << message;
}

void reportLogicError( QWidget * parent, const QString & title, const QString & message )
{
	QMessageBox::critical( parent, title,
		"<html><head/><body>"
		"<p>"
			%message%" This is a bug, please create an issue at " HYPERLINK( issuePageUrl, issuePageUrl )
		"</p>"
		"</body></html>"
	);
	logLogicError().noquote() << message;
}


//======================================================================================================================
//  logging background errors

namespace impl {

static const char * const LogLevelStrings [] =
{
	"DEBUG",
	"INFO",
	"FAILURE",
	"BUG",
};
static_assert( std::size(LogLevelStrings) == size_t(LogLevel::Bug) + 1, "Please update this table too" );

const char * logLevelToStr( LogLevel level )
{
	if (size_t(level) < std::size(LogLevelStrings))
		return LogLevelStrings[ size_t(level) ];
	else
		return "INVALID";
}

static const char * const logFileName = "errors.txt";

const QString & getCachedErrorFilePath()
{
	// local static variables are initialized under a mutex, so it should be save to use from multiple threads.
	static const QString logFilePath = fs::getPathFromFileName( os::getCachedThisAppDataDir(), logFileName );
	return logFilePath;
}

LogStream::LogStream( LogLevel level, const char * component )
:
	_debugStream( debugStreamFromLogLevel( level ) ),
	_logFile( getCachedErrorFilePath() ),
	_logLevel( level )
{
	_debugStream.noquote().nospace();

	if (shouldWriteToFileStream())
	{
		if (_logFile.open( QIODevice::Append ))
			_fileStream.setDevice( &_logFile );
	}

	writeLineOpening( level, component );
}

LogStream::~LogStream()
{
	if (shouldAndCanWriteToFileStream())
	{
		_fileStream << Qt::endl;
	}
	// log file is closed automatically
}

QDebug LogStream::debugStreamFromLogLevel( LogLevel level )
{
	switch (level)
	{
		case LogLevel::Debug:    return QMessageLogger().debug();
		case LogLevel::Failure:  return QMessageLogger().warning();
		case LogLevel::Info:     return QMessageLogger().info();
		case LogLevel::Bug:      return QMessageLogger().critical();
	}
	return QMessageLogger().critical();
}

void LogStream::writeLineOpening( LogLevel level, const char * component )
{
	auto logLevelStr = logLevelToStr( level );
	QString componentStr = component ? QStringLiteral("%1: ").arg( component ) : "";

	if (shouldWriteToDebugStream())
	{
		_debugStream << QStringLiteral("[%1] %2").arg( logLevelStr, -7 ).arg( componentStr );
	}

	if (shouldAndCanWriteToFileStream())
	{
		auto currentTime = QDateTime::currentDateTime().toString( Qt::DateFormat::ISODate );

		_fileStream << QStringLiteral("[%1] [%2] %3").arg( currentTime ).arg( logLevelStr, -7 ).arg( componentStr );
	}
}

} // namespace impl
