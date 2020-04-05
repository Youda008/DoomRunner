//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  15.5.2019
// Description: JSON parsing helpers
//======================================================================================================================

#include "JsonHelper.hpp"

#include <QJsonValue>
#include <QString>
#include <QStringBuilder>
#include <QMessageBox>


//======================================================================================================================
//  getters using exceptions

//----------------------------------------------------------------------------------------------------------------------
//  JSON object variants
/*
bool getBool( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		throw JsonKeyMissing( key );
	QJsonValue val = json[ key ];
	if (!val.isBool())
		throw JsonInvalidTypeAtKey( key, "bool" );
	return val.toBool();
}

int getInt( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		throw JsonKeyMissing( key );
	QJsonValue val = json[ key ];
	if (!val.isDouble())
		throw JsonInvalidTypeAtKey( key, "int" );
	double d = val.toDouble();
	if (d < INT_MIN || d > INT_MAX)
		throw JsonInvalidTypeAtKey( key, "int" );
	return int(d);
}

uint getUInt( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		throw JsonKeyMissing( key );
	QJsonValue val = json[ key ];
	if (!val.isDouble())
		throw JsonInvalidTypeAtKey( key, "uint" );
	double d = val.toDouble();
	if (d < 0 || d > UINT_MAX)
		throw JsonInvalidTypeAtKey( key, "int" );
	return uint(d);
}

double getDouble( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		throw JsonKeyMissing( key );
	QJsonValue val = json[ key ];
	if (!val.isDouble())
		throw JsonInvalidTypeAtKey( key, "double" );
	return val.toDouble();
}

QString getString( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		throw JsonKeyMissing( key );
	QJsonValue val = json[ key ];
	if (!val.isString())
		throw JsonInvalidTypeAtKey( key, "string" );
	return val.toString();
}

QJsonObject getObject( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		throw JsonKeyMissing( key );
	QJsonValue val = json[ key ];
	if (!val.isObject())
		throw JsonInvalidTypeAtKey( key, "object" );
	return val.toObject();
}

QJsonArray getArray( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		throw JsonKeyMissing( key );
	QJsonValue val = json[ key ];
	if (!val.isArray())
		throw JsonInvalidTypeAtKey( key, "array" );
	return val.toArray();
}

//----------------------------------------------------------------------------------------------------------------------
//  JSON array variants

bool getBool( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isBool())
		throw JsonInvalidTypeAtIdx( index, "bool" );
	return val.toBool();
}

int getInt( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isDouble())
		throw JsonInvalidTypeAtIdx( index, "int" );
	double d = val.toDouble();
	if (d < INT_MIN || d > INT_MAX)
		throw JsonInvalidTypeAtIdx( index, "int" );
	return int(d);
}

uint getUInt( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isDouble())
		throw JsonInvalidTypeAtIdx( index, "uint" );
	double d = val.toDouble();
	if (d < 0 || d > UINT_MAX)
		throw JsonInvalidTypeAtIdx( index, "int" );
	return uint(d);
}

double getDouble( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isDouble())
		throw JsonInvalidTypeAtIdx( index, "double" );
	return val.toDouble();
}

QString getString( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isString())
		throw JsonInvalidTypeAtIdx( index, "string" );
	return val.toString();
}

QJsonObject getObject( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isObject())
		throw JsonInvalidTypeAtIdx( index, "object" );
	return val.toObject();
}

QJsonArray getArray( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isArray())
		throw JsonInvalidTypeAtIdx( index, "array" );
	return val.toArray();
}
*/

//======================================================================================================================
//  getters using default value

//----------------------------------------------------------------------------------------------------------------------
//  error handlers

template< typename RetType >
const RetType & jsonKeyMissing( const QString & key, const RetType & retVal )
{
	QMessageBox::warning( nullptr, "Error loading options file",
		"Element "%key%" is missing in the options file. Skipping this option. " ); // TODO: more info
	return retVal;
}

template< typename RetType >
const RetType & jsonInvalidTypeAtKey( const QString & key, const QString & expectedType, const RetType & retVal )
{
    QMessageBox::warning( nullptr, "Error loading options file",
		"Element "%key%" has invalid type, "%expectedType%" expected. Skipping this option." );
	return retVal;
}

template< typename RetType >
const RetType & jsonInvalidTypeAtIdx( int index, const QString & expectedType, const RetType & retVal )
{
    QMessageBox::warning( nullptr, "Error loading options file",
		"Element on index "%QString::number(index)%" has invalid type, "%expectedType%" expected. Skipping this option." );
	return retVal;
}

//----------------------------------------------------------------------------------------------------------------------
//  JSON object variants

// TODO: json context
bool getBool( const QJsonObject & json, const char * key, bool defaultVal )
{
	if (!json.contains( key ))
		return jsonKeyMissing( key, defaultVal );
	QJsonValue val = json[ key ];
	if (!val.isBool())
		return jsonInvalidTypeAtKey( key, "bool", defaultVal );
	return val.toBool();
}

int getInt( const QJsonObject & json, const char * key, int defaultVal )
{
	if (!json.contains( key ))
		return jsonKeyMissing( key, defaultVal );
	QJsonValue val = json[ key ];
	if (!val.isDouble())
		return jsonInvalidTypeAtKey( key, "int", defaultVal );
	double d = val.toDouble();
	if (d < INT_MIN || d > INT_MAX)
		return jsonInvalidTypeAtKey( key, "int", defaultVal );
	return int(d);
}

uint getUInt( const QJsonObject & json, const char * key, uint defaultVal )
{
	if (!json.contains( key ))
		return jsonKeyMissing( key, defaultVal );
	QJsonValue val = json[ key ];
	if (!val.isDouble())
		return jsonInvalidTypeAtKey( key, "uint", defaultVal );
	double d = val.toDouble();
	if (d < 0 || d > UINT_MAX)
		return jsonInvalidTypeAtKey( key, "int", defaultVal );
	return uint(d);
}

double getDouble( const QJsonObject & json, const char * key, double defaultVal )
{
	if (!json.contains( key ))
		return jsonKeyMissing( key, defaultVal );
	QJsonValue val = json[ key ];
	if (!val.isDouble())
		return jsonInvalidTypeAtKey( key, "double", defaultVal );
	return val.toDouble();
}

QString getString( const QJsonObject & json, const char * key, const QString & defaultVal )
{
	if (!json.contains( key ))
		return jsonKeyMissing( key, defaultVal );
	QJsonValue val = json[ key ];
	if (!val.isString())
		return jsonInvalidTypeAtKey( key, "string", defaultVal );
	return val.toString();
}

QJsonObject getObject( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		return jsonKeyMissing( key, QJsonObject() );
	QJsonValue val = json[ key ];
	if (!val.isObject())
		return jsonInvalidTypeAtKey( key, "object", QJsonObject() );
	return val.toObject();
}

QJsonArray getArray( const QJsonObject & json, const char * key )
{
	if (!json.contains( key ))
		return jsonKeyMissing( key, QJsonArray() );
	QJsonValue val = json[ key ];
	if (!val.isArray())
		return jsonInvalidTypeAtKey( key, "array", QJsonArray() );
	return val.toArray();
}


//----------------------------------------------------------------------------------------------------------------------
//  JSON array variants

bool getBool( const QJsonArray & json, int index, bool defaultVal )
{
	QJsonValue val = json[ index ];
	if (!val.isBool())
		return jsonInvalidTypeAtIdx( index, "bool", defaultVal );
	return val.toBool();
}

int getInt( const QJsonArray & json, int index, int defaultVal )
{
	QJsonValue val = json[ index ];
	if (!val.isDouble())
		return jsonInvalidTypeAtIdx( index, "int", defaultVal );
	double d = val.toDouble();
	if (d < INT_MIN || d > INT_MAX)
		return jsonInvalidTypeAtIdx( index, "int", defaultVal );
	return int(d);
}

uint getUInt( const QJsonArray & json, int index, uint defaultVal )
{
	QJsonValue val = json[ index ];
	if (!val.isDouble())
		return jsonInvalidTypeAtIdx( index, "uint", defaultVal );
	double d = val.toDouble();
	if (d < 0 || d > UINT_MAX)
		return jsonInvalidTypeAtIdx( index, "int", defaultVal );
	return uint(d);
}

double getDouble( const QJsonArray & json, int index, double defaultVal )
{
	QJsonValue val = json[ index ];
	if (!val.isDouble())
		return jsonInvalidTypeAtIdx( index, "double", defaultVal );
	return val.toDouble();
}

QString getString( const QJsonArray & json, int index, const QString & defaultVal )
{
	QJsonValue val = json[ index ];
	if (!val.isString())
		return jsonInvalidTypeAtIdx( index, "string", defaultVal );
	return val.toString();
}

QJsonObject getObject( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isObject())
		return jsonInvalidTypeAtIdx( index, "object", QJsonObject() );
	return val.toObject();
}

QJsonArray getArray( const QJsonArray & json, int index )
{
	QJsonValue val = json[ index ];
	if (!val.isArray())
		return jsonInvalidTypeAtIdx( index, "array", QJsonArray() );
	return val.toArray();
}
