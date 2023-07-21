#ifndef TIME_STATS_INCLUDED
#define TIME_STATS_INCLUDED

#include <QElapsedTimer>
#include <QDebug>
#include <QFile>

class TimeStats
{
	QElapsedTimer timer;
	qint64 lastVal;
	int counter;
	QFile logFile;

public:

	TimeStats( const QString & fileName )
		: logFile( fileName )
	{
		logFile.open( QFile::WriteOnly );
		reset();
	}

	~TimeStats()
	{
		logFile.close();
	}

	void reset()
	{
		timer.restart();
		lastVal = 0;
		counter = 0;
	}

	auto totalElapsed() const
	{
		return timer.elapsed();
	}

	void updateLastVal()
	{
		lastVal = timer.elapsed();
	}

	void logTimePoint( const char * activityDesc )
	{
		auto elapsed = timer.elapsed();
		auto delta = elapsed - lastVal;
		auto message = QStringLiteral("  #%1: %2 took %3ms").arg( counter, -2 ).arg( activityDesc, -27 ).arg( delta, 3 );
		qDebug().noquote().nospace() << message;
		logFile.write( (message+'\n').toLatin1() );
		logFile.flush();
		lastVal = elapsed;
		++counter;
	}

	void log( const QString & message )
	{
		qDebug().noquote().nospace() << message;
		logFile.write( (message+'\n').toLatin1() );
		logFile.flush();
	}
};

#endif // TIME_STATS_INCLUDED
