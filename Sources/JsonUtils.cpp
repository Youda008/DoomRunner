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
#include <QCheckBox>


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

uint16_t JsonContext::getUInt16( const char * key, uint16_t defaultVal )
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
	if (d < 0 || d > UINT16_MAX) {
		invalidTypeAtKey( key, "int" );
		return defaultVal;
	}
	return uint16_t(d);
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

uint16_t JsonContext::getUInt16( int index, uint16_t defaultVal )
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
	if (d < 0 || d > UINT16_MAX) {
		invalidTypeAtIdx( index, "int" );
		return defaultVal;
	}
	return uint16_t(d);
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

static bool checkableMessageBox( QMessageBox::Icon icon, const QString & title, const QString & message )
{
	QMessageBox msgBox( icon, title, message, QMessageBox::Ok );
	QCheckBox chkBox( "ignore the rest of these warnings" );
	msgBox.setCheckBox( &chkBox );

	msgBox.exec();

	return chkBox.isChecked();
}

static const char * typeStr [] = {
	"Null",
	"Bool",
	"Double",
	"String",
	"Array",
	"Object",
	"Undefined"
};

void JsonContext::invalidCurrentType( const QString & expectedType )
{
	if (dontShowAgain)
		return;

	QString actualType = typeStr[ entryStack.last().val.type() ];

	dontShowAgain = checkableMessageBox( QMessageBox::Critical, "Error loading options file",
		"Current element " % currentPath() % " has invalid type. "
		"Expected " % expectedType % ", but found " % actualType % ". "
		"This is a bug. Please make a copy of options.json before clicking Ok, "
		"and then create an issue on Github page with that file attached."
	);
}

void JsonContext::missingKey( const QString & key )
{
	if (dontShowAgain)
		return;

	dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading options file",
		"Element " % elemPath( key ) % " is missing in the options file, using default value. "
		"If you just updated to a newer version, you can ignore this warning."
	);
}

void JsonContext::indexOutOfBounds( int index )
{
	if (dontShowAgain)
		return;

	dontShowAgain = checkableMessageBox( QMessageBox::Critical, "Error loading options file",
		"JSON array " % currentPath() % " does not have index " % QString::number( index ) % ". "
		"This is a bug. Please make a copy of options.json before clicking Ok, "
		"and then create an issue on Github page with that file attached."
	);
}

void JsonContext::invalidTypeAtKey( const QString & key, const QString & expectedType )
{
	if (dontShowAgain)
		return;

	QString actualType = typeStr[ entryStack.last().val.toObject()[ key ].type() ];

	dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading options file",
		"Element " % elemPath( key ) % " has invalid type, expected " % expectedType % ", but found " % actualType % ". "
		"Skipping this entry."
	);
}

void JsonContext::invalidTypeAtIdx( int index, const QString & expectedType )
{
	if (dontShowAgain)
		return;

	QString actualType = typeStr[ entryStack.last().val.toArray()[ index ].type() ];

	dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading options file",
		"Element " % elemPath( index ) % " has invalid type. Expected " % expectedType % ", but found " % actualType % ". "
		"Skipping this entry."
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
