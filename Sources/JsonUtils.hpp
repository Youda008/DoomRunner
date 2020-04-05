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
#include <QJsonObject>
#include <QJsonArray>


//======================================================================================================================
//  exceptions
/*
class JsonKeyMissing {
 public:
	JsonKeyMissing( const char * key ) : _key( key ) {}
	const QString & key() const { return _key; }
 private:
	QString _key;
};

class JsonInvalidTypeAtKey {
 public:
	JsonInvalidTypeAtKey( const QString & key, const QString & expectedType )
		: _key( key ), _expectedType( expectedType ) {}
	const QString & key() const { return _key; }
	const QString & expectedType() const { return _expectedType; }
 private:
	QString _key;
	QString _expectedType;
};

class JsonInvalidTypeAtIdx {
 public:
	JsonInvalidTypeAtIdx( int index, const QString & expectedType )
		: _index( index ), _expectedType( expectedType ) {}
	int index() const { return _index; }
	const QString & expectedType() const { return _expectedType; }
 private:
	int _index;
	QString _expectedType;
};
*/

//======================================================================================================================
//  getters using default value

//----------------------------------------------------------------------------------------------------------------------
//  JSON object variants

bool getBool( const QJsonObject & json, const char * key, bool defaultVal );
int getInt( const QJsonObject & json, const char * key, int defaultVal );
uint getUInt( const QJsonObject & json, const char * key, uint defaultVal );
double getDouble( const QJsonObject & json, const char * key, double defaultVal );
QString getString( const QJsonObject & json, const char * key, const QString & defaultVal = QString() );
QJsonObject getObject( const QJsonObject & json, const char * key );
QJsonArray getArray( const QJsonObject & json, const char * key );

//----------------------------------------------------------------------------------------------------------------------
//  JSON array variants

bool getBool( const QJsonArray & json, int index, bool defaultVal );
int getInt( const QJsonArray & json, int index, int defaultVal );
uint getUInt( const QJsonArray & json, int index, uint defaultVal );
double getDouble( const QJsonArray & json, int index, double defaultVal );
QString getString( const QJsonArray & json, int index, const QString & defaultVal = QString() );
QJsonObject getObject( const QJsonArray & json, int index );
QJsonArray getArray( const QJsonArray & json, int index );


#endif // JSON_HELPER_INCLUDED
