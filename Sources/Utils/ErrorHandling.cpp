//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common error-handling routines
//======================================================================================================================

#include "ErrorHandling.hpp"

#include "LangUtils.hpp"
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

LogStream::LogStream( LogLevel level )
:
	_debugStream( debugStreamFromLogLevel( level ) ),
	_logFile( fs::getPathFromFileName( os::getCachedThisAppDataDir(), logFileName ) ),
	_logLevel( level )
{
	_debugStream.noquote().nospace();

	if (shouldWriteToFileStream())
	{
		if (_logFile.open( QIODevice::Append ))
			_fileStream.setDevice( &_logFile );
	}

	writeLineOpening();
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

void LogStream::writeLineOpening()
{
	auto logLevelStr = logLevelToStr( _logLevel );

	if (shouldWriteToDebugStream())
	{
		_debugStream << QStringLiteral("[%1] ").arg( logLevelStr, -7 );
	}

	if (shouldAndCanWriteToFileStream())
	{
		auto currentTime = QDateTime::currentDateTime().toString( Qt::DateFormat::ISODate );

		_fileStream << QStringLiteral("[%1] [%2] ").arg( currentTime ).arg( logLevelStr, -7 );
	}
}

} // namespace impl
