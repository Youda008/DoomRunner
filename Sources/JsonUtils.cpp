//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  15.5.2019
// Description: JSON parsing helpers
//======================================================================================================================

#include "JsonUtils.hpp"

#include <QStringBuilder>
#include <QMessageBox>


//======================================================================================================================
//  JsonContext

//----------------------------------------------------------------------------------------------------------------------
//  movement through JSON tree

bool JsonContext::enterObject( const char * key )
{
	if (!entryStack.last().val.isObject()) {
		invalidCurrentType( "object" );
		return false;
	}
	QJsonObject object = entryStack.last().val.toObject();
	if (!object.contains( key )) {
		missingKey( key );
		return false;
	}
	QJsonValue val = object[ key ];
	if (!val.isObject()) {
		invalidTypeAtKey( key, "object" );
		return false;
	}
	entryStack.push_back({ key, val });
	return true;
}

bool JsonContext::enterObject( int index )
{
	if (!entryStack.last().val.isArray()) {
		invalidCurrentType( "array" );
		return false;
	}
	QJsonArray array = entryStack.last().val.toArray();
	if (index < 0 || index >= array.size()) {
		indexOutOfBounds( index );
		return false;
	}
	QJsonValue val = array[ index ];
	if (!val.isObject()) {
		invalidTypeAtIdx( index, "object" );
		return false;
	}
	entryStack.push_back({ index, val });
	return true;
}

void JsonContext::exitObject()
{
	entryStack.pop_back();
}

bool JsonContext::enterArray( const char * key )
{
	if (!entryStack.last().val.isObject()) {
		invalidCurrentType( "object" );
		return false;
	}
	QJsonObject object = entryStack.last().val.toObject();
	if (!object.contains( key )) {
		missingKey( key );
		return false;
	}
	QJsonValue val = object[ key ];
	if (!val.isArray()) {
		invalidTypeAtKey( key, "array" );
		return false;
	}
	entryStack.push_back({ key, val });
	return true;
}

bool JsonContext::enterArray( int index )
{
	if (!entryStack.last().val.isArray()) {
		invalidCurrentType( "array" );
		return false;
	}
	QJsonArray array = entryStack.last().val.toArray();
	if (index < 0 || index >= array.size()) {
		indexOutOfBounds( index );
		return false;
	}
	QJsonValue val = array[ index ];
	if (!val.isArray()) {
		invalidTypeAtIdx( index, "array" );
		return false;
	}
	entryStack.push_back({ index, val });
	return true;
}

void JsonContext::exitArray()
{
	entryStack.pop_back();
}

QString JsonContext::currentPath()
{
	QString path;
	for (const Entry & entry : entryStack) {
		if (entry.key.type == Key::OBJECT_KEY)
			path += entry.key.key + '/';
		else if (entry.key.type == Key::ARRAY_INDEX)
			path += QString::number( entry.key.idx ) + '/';
		else
			path += "<error>/";
	}
	return path;
}

int JsonContext::arraySize()
{
	if (!entryStack.last().val.isArray()) {
		return 0;
	}
	QJsonArray array = entryStack.last().val.toArray();
	return array.size();
}

//----------------------------------------------------------------------------------------------------------------------
//  JSON object variants

bool JsonContext::getBool( const char * key, bool defaultVal )
{
	if (!entryStack.last().val.isObject()) {
		invalidCurrentType( "object" );
		return defaultVal;
	}
	QJsonObject object = entryStack.last().val.toObject();
	if (!object.contains( key )) {
		missingKey( key );
		return defaultVal;
	}
	QJsonValue val = object[ key ];
	if (!val.isBool()) {
		invalidTypeAtKey( key, "bool" );
		return defaultVal;
	}
	return val.toBool();
}

int JsonContext::getInt( const char * key, int defaultVal )
{
	if (!entryStack.last().val.isObject()) {
		invalidCurrentType( "object" );
		return defaultVal;
	}
	QJsonObject object = entryStack.last().val.toObject();
	if (!object.contains( key )) {
		missingKey( key );
		return defaultVal;
	}
	QJsonValue val = object[ key ];
	if (!val.isDouble()) {
		invalidTypeAtKey( key, "int" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < INT_MIN || d > INT_MAX) {
		invalidTypeAtKey( key, "int" );
		return defaultVal;
	}
	return int(d);
}

uint JsonContext::getUInt( const char * key, uint defaultVal )
{
	if (!entryStack.last().val.isObject()) {
		invalidCurrentType( "object" );
		return defaultVal;
	}
	QJsonObject object = entryStack.last().val.toObject();
	if (!object.contains( key )) {
		missingKey( key );
		return defaultVal;
	}
	QJsonValue val = object[ key ];
	if (!val.isDouble()) {
		invalidTypeAtKey( key, "uint" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < 0 || d > UINT_MAX) {
		invalidTypeAtKey( key, "int" );
		return defaultVal;
	}
	return uint(d);
}

double JsonContext::getDouble( const char * key, double defaultVal )
{
	if (!entryStack.last().val.isObject()) {
		invalidCurrentType( "object" );
		return defaultVal;
	}
	QJsonObject object = entryStack.last().val.toObject();
	if (!object.contains( key )) {
		missingKey( key );
		return defaultVal;
	}
	QJsonValue val = object[ key ];
	if (!val.isDouble()) {
		invalidTypeAtKey( key, "double" );
		return defaultVal;
	}
	return val.toDouble();
}

QString JsonContext::getString( const char * key, const QString & defaultVal )
{
	if (!entryStack.last().val.isObject()) {
		invalidCurrentType( "object" );
		return defaultVal;
	}
	QJsonObject object = entryStack.last().val.toObject();
	if (!object.contains( key )) {
		missingKey( key );
		return defaultVal;
	}
	QJsonValue val = object[ key ];
	if (!val.isString()) {
		invalidTypeAtKey( key, "string" );
		return defaultVal;
	}
	return val.toString();
}

//----------------------------------------------------------------------------------------------------------------------
//  JSON array variants

bool JsonContext::getBool( int index, bool defaultVal )
{
	if (!entryStack.last().val.isArray()) {
		invalidCurrentType( "array" );
		return false;
	}
	QJsonArray array = entryStack.last().val.toArray();
	if (index < 0 || index >= array.size()) {
		indexOutOfBounds( index );
		return false;
	}
	QJsonValue val = array[ index ];
	if (!val.isBool()) {
		invalidTypeAtIdx( index, "bool" );
		return defaultVal;
	}
	return val.toBool();

}
int JsonContext::getInt( int index, int defaultVal )
{
	if (!entryStack.last().val.isArray()) {
		invalidCurrentType( "array" );
		return defaultVal;
	}
	QJsonArray array = entryStack.last().val.toArray();
	if (index < 0 || index >= array.size()) {
		indexOutOfBounds( index );
		return defaultVal;
	}
	QJsonValue val = array[ index ];
	if (!val.isDouble()) {
		invalidTypeAtIdx( index, "int" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < INT_MIN || d > INT_MAX) {
		invalidTypeAtIdx( index, "int" );
		return defaultVal;
	}
	return int(d);
}
uint JsonContext::getUInt( int index, uint defaultVal )
{
	if (!entryStack.last().val.isArray()) {
		invalidCurrentType( "array" );
		return defaultVal;
	}
	QJsonArray array = entryStack.last().val.toArray();
	if (index < 0 || index >= array.size()) {
		indexOutOfBounds( index );
		return defaultVal;
	}
	QJsonValue val = array[ index ];
	if (!val.isDouble()) {
		invalidTypeAtIdx( index, "uint" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < 0 || d > UINT_MAX) {
		invalidTypeAtIdx( index, "int" );
		return defaultVal;
	}
	return uint(d);
}
double JsonContext::getDouble( int index, double defaultVal )
{
	if (!entryStack.last().val.isArray()) {
		invalidCurrentType( "array" );
		return defaultVal;
	}
	QJsonArray array = entryStack.last().val.toArray();
	if (index < 0 || index >= array.size()) {
		indexOutOfBounds( index );
		return defaultVal;
	}
	QJsonValue val = array[ index ];
	if (!val.isDouble()) {
		invalidTypeAtIdx( index, "double" );
		return defaultVal;
	}
	return val.toDouble();
}
QString JsonContext::getString( int index, const QString & defaultVal )
{
	if (!entryStack.last().val.isArray()) {
		invalidCurrentType( "array" );
		return defaultVal;
	}
	QJsonArray array = entryStack.last().val.toArray();
	if (index < 0 || index >= array.size()) {
		indexOutOfBounds( index );
		return defaultVal;
	}
	QJsonValue val = array[ index ];
	if (!val.isString()) {
		invalidTypeAtIdx( index, "string" );
		return defaultVal;
	}
	return val.toString();
}

//----------------------------------------------------------------------------------------------------------------------
//  error handlers

void JsonContext::invalidCurrentType( const QString & expectedType )
{
    QMessageBox::warning( parent, "Error loading options file",
		"Current element " % currentPath() % " has invalid type, " % expectedType % " expected. Skipping this option. "
		"If you just switched to a newer version, you can ignore this warning."
	);
}

void JsonContext::missingKey( const QString & key )
{
	QMessageBox::warning( parent, "Error loading options file",
		"Element " % elemPath( key ) % " is missing in the options file. Skipping this option. "
		"If you just switched to a newer version, you can ignore this warning."
	);
}

void JsonContext::indexOutOfBounds( int index )
{
	QMessageBox::critical( parent, "Error loading options file",
		"JSON array " % currentPath() % " does not have index " % QString::number( index ) % ". "
		"This shouldn't be happening and it is a bug. Please create an issue on Github page."
	);
}

void JsonContext::invalidTypeAtKey( const QString & key, const QString & expectedType )
{
    QMessageBox::warning( parent, "Error loading options file",
		"Element " % elemPath( key ) % " has invalid type, " % expectedType % " expected. Skipping this option. "
		"If you just switched to a newer version, you can ignore this warning."
	);
}

void JsonContext::invalidTypeAtIdx( int index, const QString & expectedType )
{
    QMessageBox::warning( parent, "Error loading options file",
		"Element " % elemPath( index ) % " has invalid type, " % expectedType % " expected. Skipping this option. "
		"If you just switched to a newer version, you can ignore this warning."
	);
}

QString JsonContext::elemPath( const QString & elemName )
{
	return currentPath() + elemName;
}

QString JsonContext::elemPath( int index )
{
	return currentPath() + QString::number( index );
}
