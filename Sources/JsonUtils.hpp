//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  15.5.2019
// Description: JSON parsing helpers
//======================================================================================================================

#ifndef JSON_HELPER_INCLUDED
#define JSON_HELPER_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QList>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
class QWidget;


//======================================================================================================================
/** when parsing JSON file, this context stores info where we are, so that we can print more useful error messages */

class JsonContext {

 public:

	JsonContext( const QJsonObject & rootObject ) : dontShowAgain( false )
		{ entryStack.append({ QString(), rootObject }); }
	~JsonContext() {}

	// movement through JSON tree

	bool enterObject( const char * key );
	bool enterObject( int index );
	void exitObject();

	bool enterArray( const char * key );
	bool enterArray( int index );
	void exitArray();

	QString currentPath();

	int arraySize();

	// getters of elementary values

	bool getBool( const char * key, bool defaultVal );
	int getInt( const char * key, int defaultVal );
	uint getUInt( const char * key, uint defaultVal );
	double getDouble( const char * key, double defaultVal );
	QString getString( const char * key, const QString & defaultVal = QString() );

	bool getBool( int index, bool defaultVal );
	int getInt( int index, int defaultVal );
	uint getUInt( int index, uint defaultVal );
	double getDouble( int index, double defaultVal );
	QString getString( int index, const QString & defaultVal = QString() );

 private:

	void invalidCurrentType( const QString & expectedType );
	void missingKey( const QString & key );
	void indexOutOfBounds( int index );
	void invalidTypeAtKey( const QString & key, const QString & expectedType );
	void invalidTypeAtIdx( int index, const QString & expectedType );

	QString elemPath( const QString & elemName );
	QString elemPath( int index );
	QString pathWithoutArrays( const QString & elemName );
	QString pathWithoutArrays( int index );

 private:

	struct Key {
		enum Type {
			OTHER = 0,
			OBJECT_KEY,
			ARRAY_INDEX
		};
		Type type;
		QString key;
		int idx;

		Key( const QString & key ) : type( OBJECT_KEY ), key( key ), idx( -1 ) {}
		Key( int idx ) : type( ARRAY_INDEX ), key(), idx( idx ) {}
	};

	struct Entry {
		Key key;
		QJsonValue val;

		Entry( const QString & key, const QJsonValue & val ) : key( key ), val( val ) {}
		Entry( int idx, const QJsonValue & val ) : key( idx ), val( val ) {}
	};

	QList< Entry > entryStack;

	bool dontShowAgain;

};


#endif // JSON_HELPER_INCLUDED
