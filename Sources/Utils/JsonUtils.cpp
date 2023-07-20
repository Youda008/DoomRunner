//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: JSON parsing helpers that handle errors and simplify parsing code
//======================================================================================================================

#include "JsonUtils.hpp"

#include <QStringBuilder>
#include <QTextStream>
#include <QMessageBox>
#include <QCheckBox>
#include <QDebug>

//#include <QLinkedList> // obsolete? why???
#include <list>


//======================================================================================================================
//  JsonValueContext

void JsonValueCtx::prependPath( std::list< QString > & pathList ) const
{
	if (_parent)
	{
		if (_key.type == Key::ObjectKey)
			pathList.push_front( '/' + _key.key );
		else if (_key.type == Key::ArrayIndex)
			pathList.push_front( "/[" + QString::number( _key.idx ) + ']' );
		else
			pathList.push_front( "/<error>" );

		_parent->prependPath( pathList );
	}
}

QString JsonValueCtx::getPath() const
{
	std::list< QString > path;
	prependPath( path );

	// TODO: idiotic
	QString pathStr;
	QTextStream pathStream( &pathStr );
	for (const QString & part : path)
	{
		pathStream << part;
	}
	pathStream.flush();

	return pathStr;
}


//======================================================================================================================
//  JsonObjectCtx

JsonObjectCtxProxy JsonObjectCtx::getObject( const QString & key, bool showError ) const
{
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return JsonObjectCtxProxy();
	}
	QJsonValue val = (*_wrappedObject)[ key ];

	if (!val.isObject())
	{
		invalidTypeAtKey( key, "object" );
		return JsonObjectCtxProxy();
	}
	return JsonObjectCtxProxy( val.toObject(), _context, this, key );
}

JsonArrayCtxProxy JsonObjectCtx::getArray( const QString & key, bool showError ) const
{
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return JsonArrayCtxProxy();
	}
	QJsonValue val = (*_wrappedObject)[ key ];
	if (!val.isArray())
	{
		invalidTypeAtKey( key, "array" );
		return JsonArrayCtxProxy();
	}
	return JsonArrayCtxProxy( val.toArray(), _context, this, key );
}

bool JsonObjectCtx::getBool( const QString & key, bool defaultVal, bool showError ) const
{
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedObject)[ key ];
	if (!val.isBool())
	{
		invalidTypeAtKey( key, "bool" );
		return defaultVal;
	}
	return val.toBool();
}

int JsonObjectCtx::getInt( const QString & key, int defaultVal, bool showError ) const
{
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedObject)[ key ];
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
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedObject)[ key ];
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
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedObject)[ key ];
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
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedObject)[ key ];
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
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedObject)[ key ];
	if (!val.isDouble())
	{
		invalidTypeAtKey( key, "double" );
		return defaultVal;
	}
	return val.toDouble();
}

QString JsonObjectCtx::getString( const QString & key, QString defaultVal, bool showError ) const
{
	if (!_wrappedObject->contains( key ))
	{
		missingKey( key, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedObject)[ key ];
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
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return JsonObjectCtxProxy();
	}
	QJsonValue val = (*_wrappedArray)[ index ];
	if (!val.isObject())
	{
		invalidTypeAtIdx( index, "object" );
		return JsonObjectCtxProxy();
	}
	return JsonObjectCtxProxy( val.toObject(), _context, this, index );
}

JsonArrayCtxProxy JsonArrayCtx::getArray( int index, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return JsonArrayCtxProxy();
	}
	QJsonValue val = (*_wrappedArray)[ index ];
	if (!val.isArray())
	{
		invalidTypeAtIdx( index, "array" );
		return JsonArrayCtxProxy();
	}
	return JsonArrayCtxProxy( val.toArray(), _context, this, index );
}

bool JsonArrayCtx::getBool( int index, bool defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return false;
	}
	QJsonValue val = (*_wrappedArray)[ index ];
	if (!val.isBool())
	{
		invalidTypeAtIdx( index, "bool" );
		return defaultVal;
	}
	return val.toBool();

}

int JsonArrayCtx::getInt( int index, int defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedArray)[ index ];
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
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedArray)[ index ];
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
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedArray)[ index ];
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
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedArray)[ index ];
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
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedArray)[ index ];
	if (!val.isDouble())
	{
		invalidTypeAtIdx( index, "double" );
		return defaultVal;
	}
	return val.toDouble();
}

QString JsonArrayCtx::getString( int index, QString defaultVal, bool showError ) const
{
	if (index < 0 || index >= _wrappedArray->size())
	{
		indexOutOfBounds( index, showError );
		return defaultVal;
	}
	QJsonValue val = (*_wrappedArray)[ index ];
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
	QString message = "Element " % elemPath( key ) % " is missing in the options file, using default value.";

	if (!showError)
		return;
	else if (_context->dontShowAgain)
		qWarning() << message;
	else
		_context->dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading options file", message );
}

void JsonArrayCtx::indexOutOfBounds( int index, bool showError ) const
{
	QString message =
		"JSON array " % getPath() % " does not have index " % QString::number( index ) % ". "
		"This is a bug. Please make a copy of options.json before clicking Ok, "
		"and then create an issue on Github page with that file attached.";

	if (!showError)
		return;
	else if (_context->dontShowAgain)
		qWarning() << message;
	else
		_context->dontShowAgain = checkableMessageBox( QMessageBox::Critical, "Error loading options file", message );
}

void JsonObjectCtx::invalidTypeAtKey( const QString & key, const QString & expectedType, bool showError ) const
{
	QString actualType = typeStr[ (*_wrappedObject)[ key ].type() ];
	QString message =
		"Element " % elemPath( key ) % " has invalid type, expected " % expectedType % ", but found " % actualType % ". "
		"Skipping this entry.";

	if (!showError)
		return;
	else if (_context->dontShowAgain)
		qWarning() << message;
	else
		_context->dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading options file", message );
}

void JsonArrayCtx::invalidTypeAtIdx( int index, const QString & expectedType, bool showError ) const
{
	QString actualType = typeStr[ (*_wrappedArray)[ index ].type() ];
	QString message =
		"Element " % elemPath( index ) % " has invalid type. Expected " % expectedType % ", but found " % actualType % ". "
		"Skipping this entry.";

	if (!showError)
		return;
	else if (_context->dontShowAgain)
		qWarning() << message;
	else
		_context->dontShowAgain = checkableMessageBox( QMessageBox::Warning, "Error loading options file", message );
}

QString JsonObjectCtx::elemPath( const QString & elemName ) const
{
	return getPath() + '/' + elemName;
}

QString JsonArrayCtx::elemPath( int index ) const
{
	return getPath() + "/[" + QString::number( index ) + ']';
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
		QMessageBox::warning( nullptr, "Error saving "+fileDesc, error );
		return false;
	}

	return true;
}

bool readJsonFromFile( JsonDocumentCtx & jsonDocCtx, const QString & filePath, const QString & fileDesc )
{
	QByteArray bytes;
	QString readError = fs::readWholeFile( filePath, bytes );
	if (!readError.isEmpty())
	{
		QMessageBox::warning( nullptr, "Error loading "+fileDesc, readError );
		return false;
	}

	QJsonParseError parseError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson( bytes, &parseError );
	if (jsonDoc.isNull())
	{
		QMessageBox::warning( nullptr, "Error loading "+fileDesc,
			"Failed to parse "%fs::getFileNameFromPath(filePath)%": "%parseError.errorString()%"\n"
			"You can either open it in notepad and try to repair it, or delete it and start from scratch."
		);
		return false;
	}

	jsonDocCtx = JsonDocumentCtx( jsonDoc );
	return true;
}
