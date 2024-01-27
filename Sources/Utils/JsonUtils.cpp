//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: JSON parsing helpers that handle errors and simplify parsing code
//======================================================================================================================

#include "JsonUtils.hpp"

#include "Utils/ErrorHandling.hpp"
#include "Utils/FileSystemUtils.hpp"  // getFileNameFromPath

#include <QStringBuilder>
#include <QTextStream>
#include <QMessageBox>
#include <QCheckBox>
#include <QDebug>


//======================================================================================================================
//  JsonValueContext

void JsonValueCtx::constructJsonPathRecursively( QString & path ) const
{
	// start the path construction at the root element (the bottom of the recursion)
	if (!_parent)
	{
		path.clear();
		return;
	}

	// recursively construct path from the root element up to this element
	_parent->constructJsonPathRecursively( path );

	// append key/index of this element
	if (_key.type == Key::ObjectKey)
		path.append('/').append( _key.key );
	else if (_key.type == Key::ArrayIndex)
		path.append("/[").append( QString::number( _key.idx ) ).append(']');
	else
		path.append("/<error>");
}

QString JsonValueCtx::getJsonPath() const
{
	QString path;
	constructJsonPathRecursively( path );
	return path;
}


//======================================================================================================================
//  JsonObjectCtx

JsonObjectCtxProxy JsonObjectCtx::getObject( const QString & key, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return JsonObjectCtxProxy();
	}
	QJsonValue val = _wrappedObject[ key ];

	if (!val.isObject())
	{
		invalidTypeAtKey( key, "object" );
		return JsonObjectCtxProxy();
	}
	return JsonObjectCtxProxy( val.toObject(), _context, this, key );
}

JsonArrayCtxProxy JsonObjectCtx::getArray( const QString & key, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return JsonArrayCtxProxy();
	}
	QJsonValue val = _wrappedObject[ key ];
	if (!val.isArray())
	{
		invalidTypeAtKey( key, "array" );
		return JsonArrayCtxProxy();
	}
	return JsonArrayCtxProxy( val.toArray(), _context, this, key );
}

bool JsonObjectCtx::getBool( const QString & key, bool defaultVal, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedObject[ key ];
	if (!val.isBool())
	{
		invalidTypeAtKey( key, "bool" );
		return defaultVal;
	}
	return val.toBool();
}

int JsonObjectCtx::getInt( const QString & key, int defaultVal, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedObject[ key ];
	if (!val.isDouble())
	{
		invalidTypeAtKey( key, "int" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < INT_MIN || d > INT_MAX)
	{
		invalidTypeAtKey( key, "int" );
		return defaultVal;
	}
	return int(d);
}

uint JsonObjectCtx::getUInt( const QString & key, uint defaultVal, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedObject[ key ];
	if (!val.isDouble())
	{
		invalidTypeAtKey( key, "uint" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < 0 || d > UINT_MAX)
	{
		invalidTypeAtKey( key, "uint" );
		return defaultVal;
	}
	return uint(d);
}

uint16_t JsonObjectCtx::getUInt16( const QString & key, uint16_t defaultVal, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedObject[ key ];
	if (!val.isDouble())
	{
		invalidTypeAtKey( key, "uint16" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < 0 || d > UINT16_MAX)
	{
		invalidTypeAtKey( key, "uint16" );
		return defaultVal;
	}
	return uint16_t(d);
}

int64_t JsonObjectCtx::getInt64( const QString & key, int64_t defaultVal, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedObject[ key ];
	if (!val.isDouble())
	{
		invalidTypeAtKey( key, "int64" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < (INT64_MIN>>10) || d > (INT64_MAX>>10))
	{
		invalidTypeAtKey( key, "int64" );
		return defaultVal;
	}
	return int64_t(d);
}

double JsonObjectCtx::getDouble( const QString & key, double defaultVal, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedObject[ key ];
	if (!val.isDouble())
	{
		invalidTypeAtKey( key, "double" );
		return defaultVal;
	}
	return val.toDouble();
}

QString JsonObjectCtx::getString( const QString & key, QString defaultVal, bool showError ) const
{
	if (!_wrappedObject.contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedObject[ key ];
	if (val.isNull())
	{
		invalidTypeAtKey( key, "string", showError );
		return defaultVal;
	}
	else if (!val.isString())
	{
		invalidTypeAtKey( key, "string" );
		return defaultVal;
	}
	return val.toString();
}


//======================================================================================================================
//  JsonArrayCtx

JsonObjectCtxProxy JsonArrayCtx::getObject( int index, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return JsonObjectCtxProxy();
	}
	QJsonValue val = _wrappedArray[ index ];
	if (!val.isObject())
	{
		invalidTypeAtIdx( index, "object" );
		return JsonObjectCtxProxy();
	}
	return JsonObjectCtxProxy( val.toObject(), _context, this, index );
}

JsonArrayCtxProxy JsonArrayCtx::getArray( int index, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return JsonArrayCtxProxy();
	}
	QJsonValue val = _wrappedArray[ index ];
	if (!val.isArray())
	{
		invalidTypeAtIdx( index, "array" );
		return JsonArrayCtxProxy();
	}
	return JsonArrayCtxProxy( val.toArray(), _context, this, index );
}

bool JsonArrayCtx::getBool( int index, bool defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return false;
	}
	QJsonValue val = _wrappedArray[ index ];
	if (!val.isBool())
	{
		invalidTypeAtIdx( index, "bool" );
		return defaultVal;
	}
	return val.toBool();

}

int JsonArrayCtx::getInt( int index, int defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedArray[ index ];
	if (!val.isDouble())
	{
		invalidTypeAtIdx( index, "int" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < INT_MIN || d > INT_MAX)
	{
		invalidTypeAtIdx( index, "int" );
		return defaultVal;
	}
	return int(d);
}

uint JsonArrayCtx::getUInt( int index, uint defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedArray[ index ];
	if (!val.isDouble())
	{
		invalidTypeAtIdx( index, "uint" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < 0 || d > UINT_MAX)
	{
		invalidTypeAtIdx( index, "int" );
		return defaultVal;
	}
	return uint(d);
}

uint16_t JsonArrayCtx::getUInt16( int index, uint16_t defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedArray[ index ];
	if (!val.isDouble())
	{
		invalidTypeAtIdx( index, "uint" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < 0 || d > UINT16_MAX)
	{
		invalidTypeAtIdx( index, "int" );
		return defaultVal;
	}
	return uint16_t(d);
}

int64_t JsonArrayCtx::getInt64( int index, int64_t defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedArray[ index ];
	if (!val.isDouble())
	{
		invalidTypeAtIdx( index, "int64" );
		return defaultVal;
	}
	double d = val.toDouble();
	if (d < (INT64_MIN>>10) || d > (INT64_MAX>>10))
	{
		invalidTypeAtIdx( index, "int64" );
		return defaultVal;
	}
	return int64_t(d);
}

double JsonArrayCtx::getDouble( int index, double defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedArray[ index ];
	if (!val.isDouble())
	{
		invalidTypeAtIdx( index, "double" );
		return defaultVal;
	}
	return val.toDouble();
}

QString JsonArrayCtx::getString( int index, QString defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray.size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = _wrappedArray[ index ];
	if (val.isNull())
	{
		invalidTypeAtIdx( index, "string", showError );
		return defaultVal;
	}
	else if (!val.isString())
	{
		invalidTypeAtIdx( index, "string" );
		return defaultVal;
	}
	return val.toString();
}


//======================================================================================================================
//  error handling

static bool checkableMessageBox( QMessageBox::Icon icon, const QString & title, const QString & message )
{
	QMessageBox msgBox( icon, title, message, QMessageBox::Ok );
	QCheckBox * chkBox = new QCheckBox( "ignore the rest of these warnings" );
	msgBox.setCheckBox( chkBox );  // msgBox takes ownership of chkBox

	msgBox.exec();

	return chkBox->isChecked();
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

void JsonObjectCtx::missingKey( const QString & key, bool showError ) const
{
	QString message = "Element "%elemPath( key )%" is missing in "%_context->fileName()%",\nusing default value.";

	if (showError)
	{
		if (!_context->dontShowAgain)
			_context->dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading JSON file", message );
		logRuntimeError("JsonValueCtx").noquote() << message;
	}
}

void JsonArrayCtx::indexOutOfBounds( int index, bool showError ) const
{
	QString message =
		"JSON array "%getJsonPath()%" does not have index "%QString::number( index )%".\n"
		"This is a bug. Please make a copy of "%_context->fileName()%" before clicking Ok, "
		"and then create an issue on Github page with that file attached.";

	if (showError)
	{
		if (!_context->dontShowAgain)
			_context->dontShowAgain = checkableMessageBox( QMessageBox::Critical, "Error loading JSON file", message );
		logRuntimeError("JsonValueCtx").noquote() << message;
	}
}

void JsonObjectCtx::invalidTypeAtKey( const QString & key, const QString & expectedType, bool showError ) const
{
	QString actualType = typeStr[ _wrappedObject[ key ].type() ];
	QString message =
		"Element "%elemPath( key )%" in "%_context->fileName()%" has invalid type. "
		"Expected "%expectedType%", but found "%actualType%". Skipping this entry.";

	if (showError)
	{
		if (!_context->dontShowAgain)
			_context->dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading JSON file", message );
		logRuntimeError("JsonValueCtx").noquote() << message;
	}
}

void JsonArrayCtx::invalidTypeAtIdx( int index, const QString & expectedType, bool showError ) const
{
	QString actualType = typeStr[ _wrappedArray[ index ].type() ];
	QString message =
		"Element "%elemPath( index )%" in "%_context->fileName()%" has invalid type. "
		"Expected "%expectedType%", but found "%actualType%". Skipping this entry.";

	if (showError)
	{
		if (!_context->dontShowAgain)
			_context->dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading JSON file", message );
		logRuntimeError("JsonValueCtx").noquote() << message;
	}
}

QString JsonObjectCtx::elemPath( const QString & elemName ) const
{
	return getJsonPath() + '/' + elemName;
}

QString JsonArrayCtx::elemPath( int index ) const
{
	return getJsonPath() + "/[" + QString::number( index ) + ']';
}

QString _ParsingContext::fileName() const
{
	return fs::getFileNameFromPath( filePath );
}


//======================================================================================================================
//  file writing helpers

#include "FileSystemUtils.hpp"

bool writeJsonToFile( const QJsonDocument & jsonDoc, const QString & filePath, const QString & fileDesc )
{
	QByteArray bytes = jsonDoc.toJson();

	QString error = fs::updateFileSafely( filePath, bytes );
	if (!error.isEmpty())
	{
		reportRuntimeError( nullptr, "Error saving "+fileDesc, error );
		return false;
	}

	return true;
}

JsonDocumentCtx readJsonFromFile( const QString & filePath, const QString & fileDesc, bool ignoreEmpty )
{
	QByteArray bytes;
	QString readError = fs::readWholeFile( filePath, bytes );
	if (!readError.isEmpty())
	{
		reportRuntimeError( nullptr, "Error loading "+fileDesc, readError );
		return JsonDocumentCtx();
	}

	if (bytes.isEmpty())
	{
		if (!ignoreEmpty)
			reportRuntimeError( nullptr, "Error loading "+fileDesc, fileDesc+" file is empty." );
		return JsonDocumentCtx();
	}

	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson( bytes, &parseError );
	if (jsonDoc.isNull())
	{
		reportRuntimeError( nullptr, "Error loading "+fileDesc,
			"Failed to parse \""%fs::getFileNameFromPath(filePath)%"\": "%parseError.errorString()%"\n"
			"You can either open it in notepad and try to repair it, or delete it and start from scratch."
		);
		return JsonDocumentCtx();
	}

	return JsonDocumentCtx( filePath, jsonDoc );
}
