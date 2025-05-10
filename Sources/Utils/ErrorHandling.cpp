//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: common error-handling routines
//======================================================================================================================

#include "ErrorHandling.hpp"

#include "WidgetUtils.hpp"      // HYPERLINK
#include "OSUtils.hpp"          // getThisLauncherDataDir
#include "FileSystemUtils.hpp"  // getPathFromFileName

#include <QStringBuilder>
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>


//======================================================================================================================
// displaying foreground errors

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
	auto logStream = logRuntimeError();
	logStream.noquote() << message;
	logStream.flush();

	QMessageBox::warning( parent, title, message );
}

// Logic errors should be more detailed, so that we have enough information to debug and fix them.
void reportLogicError( QWidget * parent, QStringView locationTag, const QString & title, const QString & message )
{
	auto logStream = logLogicError( locationTag );
	logStream.noquote() << message;
	logStream.flush();

	QMessageBox::critical( parent, !locationTag.isEmpty() ? (locationTag%": "%title) : title,
		"<html><head/><body>"
		"<p>"
			%message%" This is a bug, please create an issue at " HYPERLINK( issuePageUrl, issuePageUrl )
		"</p>"
		"</body></html>"
	);
}


//======================================================================================================================
// logging background errors

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
	// local static variables are initialized under a mutex, so it should be save to use from multiple threads
	static const QString logFilePath = fs::getPathFromFileName( os::getThisLauncherDataDir(), logFileName );
	return logFilePath;
}

LogStream::LogStream( LogLevel level, QStringView locationTag, bool canLogToFile )
:
	_aborter( level ),
	_debugStream( debugStreamFromLogLevel( level ) ),
	_logFile(),
	_fileStream(),
	_logLevel( level ),
	_canLogToFile( canLogToFile )
{
	_debugStream->noquote().nospace();

	if (shouldWriteToFileStream())
	{
		_logFile.setFileName( getCachedErrorFilePath() );
		if (_logFile.open( QIODevice::Append ))
			_fileStream.setDevice( &_logFile );
	}

	writeLineOpening( level, locationTag );
}

LogStream::~LogStream()
{
	if (shouldAndCanWriteToFileStream())
	{
		_fileStream << Qt::endl;
	}
	// log file is closed automatically
}

LogStream::Aborter::~Aborter()
{
	if constexpr (IS_DEBUG_BUILD)
	{
		if (fut::to_underlying( _logLevel ) >= fut::to_underlying( LogLevel::Bug ))
		{
			assert_msg( false, "This error that deserves your attention" );
			// To get more info, either setup a breakpoint here, or check errors.txt
		}
	}
}

std::unique_ptr< QDebug > LogStream::debugStreamFromLogLevel( LogLevel level )
{
	switch (level)
	{
		case LogLevel::Debug:    return std::make_unique< QDebug >( QtMsgType::QtDebugMsg );
		case LogLevel::Info:     return std::make_unique< QDebug >( QtMsgType::QtInfoMsg );
		case LogLevel::Failure:  return std::make_unique< QDebug >( QtMsgType::QtWarningMsg );
		case LogLevel::Bug:      return std::make_unique< QDebug >( QtMsgType::QtCriticalMsg );
		default:                 return std::make_unique< QDebug >( QtMsgType::QtCriticalMsg );
	}
}

void LogStream::writeLineOpening( LogLevel level, QStringView locationTag )
{
	auto logLevelStr = logLevelToStr( level );
	QString messagePrefix = !locationTag.isEmpty() ? QStringLiteral("%1: ").arg( locationTag ) : "";

	if (shouldWriteToDebugStream())
	{
		// with the workaround in initStdStreams(), this should write to stdout even on Windows
		*_debugStream << QStringLiteral("[%1] %2").arg( logLevelStr, -7 ).arg( messagePrefix );
	}

	if (shouldAndCanWriteToFileStream())
	{
		auto currentTime = QDateTime::currentDateTime().toString( Qt::DateFormat::ISODate );

		_fileStream << QStringLiteral("[%1] [%2] %3").arg( currentTime ).arg( logLevelStr, -7 ).arg( messagePrefix );
	}
}

} // namespace impl


//----------------------------------------------------------------------------------------------------------------------
// logging helpers

QString LoggingComponent::makeLocationTag( QStringView funcName ) const
{
	QString tag = _componentType.toString();
	if (!_componentName.isEmpty())
		tag = tag%"("%_componentName%")";
	if (!funcName.isEmpty())
		tag = tag%"::"%funcName;
	return tag;
}
