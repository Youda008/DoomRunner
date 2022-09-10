//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: JSON parsing helpers that handle errors and simplify parsing code
//======================================================================================================================

#ifndef JSON_UTILS_INCLUDED
#define JSON_UTILS_INCLUDED


#include "Common.hpp"

#include <QString>
#include <QList>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include <list>


//======================================================================================================================
//  in order for the getEnum method to work, the author of the enum must specialize the following templates

template< typename Enum >
const char * enumName() { return "unknown"; }

template< typename Enum >
uint enumSize() { return 0; }


//======================================================================================================================
// wrappers around JsonObject and JsonArray containing a parsing context that enables us to show useful error messages
//
// implementation notes:
//
// 1. JSON path reconstruction - getPath()
//    Each JsonObject/JsonArray wrapper knows its parent and its key, and is therefore able to reconstruct its path
//    in the JSON document by traversing the JSON tree from leaf to root, so that we can tell the user exactly which
//    element is broken.
//
// 2. common parsing context
//    Each wrapper has access to a context that is shared between all elements of a particular JSON Document and that
//    contains data related to the parsing process of this document. The context struct is stored in JsonDocumentCtx
//    and all its elements get a pointer.
//
// 3. JsonObjectCtxProxy/JsonArrayCtxProxy
//    These proxy classes exist only because there is a cyclic dependancy between JsonObjectCtx and JsonArrayCtx
//    (JsonObjectCtx::getArray returns JsonArrayCtx and JsonArrayCtx::getObject returns JsonObjectCtx) and we can't
//    declare one before the other. Therefore getObject/getArray return a proxy class that is declared before both and
//    that is then automatically converted (by a constructor) to the final JsonObjectCtx/JsonArrayCtx.


constexpr bool ShowError = true;
constexpr bool DontShowError = false;

/// data related to an ongoing parsing process
struct _ParsingContext
{
	bool dontShowAgain = false;  ///< whether to show "invalid element" errors to the user
};

/// mechanisms common for JSON objects and arrays
class JsonValueCtx {

 protected:

	/// JSON key - either string key for objects or int index for arrays
	struct Key
	{
		enum Type
		{
			Other = 0,
			ObjectKey,
			ArrayIndex
		};
		Type type;
		QString key; // TODO: std::variant
		int idx;

		Key() : type( Other ), key(), idx( -1 ) {}
		Key( const QString & key ) : type( ObjectKey ), key( key ), idx( -1 ) {}
		Key( int idx ) : type( ArrayIndex ), key(), idx( idx ) {}
	};

	_ParsingContext * _context;  ///< document-wide context shared among all elements of that document, the struct is stored in JsonDocumentCtx

	const JsonValueCtx * _parent;  ///< JSON element that contains this element
	Key _key;  ///< key or index that this element has in its parent element

 public:

	/// Constructs a JSON value with no parent.
	/** This should be only used for creating a root element. */
	JsonValueCtx( _ParsingContext * context )
		: _context( context ), _parent( nullptr ), _key() {}

	/// Constructs a JSON value with a parent that is a JSON object.
	JsonValueCtx( _ParsingContext * context, const JsonValueCtx * parent, const QString & key )
		: _context( context ), _parent( parent ), _key( key ) {}

	/// Constructs a JSON value with a parent that is a JSON array.
	JsonValueCtx( _ParsingContext * context, const JsonValueCtx * parent, int index )
		: _context( context ), _parent( parent ), _key( index ) {}

	/// copy constructor
	JsonValueCtx( const JsonValueCtx & other ) = default;

	/// Builds a path of this element in its JSON document.
	QString getPath() const;

 protected:

	void prependPath( std::list< QString > & pathList ) const;

};

/// proxy class that solves cyclic dependancy between JsonObjectCtx and JsonArrayCtx
class JsonObjectCtxProxy : public JsonValueCtx {

 protected:

	std::optional< QJsonObject > _wrappedObject;

 public:

	JsonObjectCtxProxy()
		: JsonValueCtx( nullptr ), _wrappedObject( std::nullopt ) {}

	JsonObjectCtxProxy( const QJsonObject & wrappedObject, _ParsingContext * context )
		: JsonValueCtx( context ), _wrappedObject( wrappedObject ) {}

	JsonObjectCtxProxy( const QJsonObject & wrappedObject, _ParsingContext * context, const JsonValueCtx * parent, const QString & key )
		: JsonValueCtx( context, parent, key ), _wrappedObject( wrappedObject ) {}

	JsonObjectCtxProxy( const QJsonObject & wrappedObject, _ParsingContext * context, const JsonValueCtx * parent, int index )
		: JsonValueCtx( context, parent, index ), _wrappedObject( wrappedObject ) {}

	JsonObjectCtxProxy( const JsonObjectCtxProxy & other ) = default;

	operator bool() const { return _wrappedObject.has_value(); }

};

/// proxy class that solves cyclic dependancy between JsonObjectCtx and JsonArrayCtx
class JsonArrayCtxProxy : public JsonValueCtx {

 protected:

	std::optional< QJsonArray > _wrappedArray;

 public:

	JsonArrayCtxProxy()
		: JsonValueCtx( nullptr ), _wrappedArray( std::nullopt ) {}

	JsonArrayCtxProxy( const QJsonArray & wrappedArray, _ParsingContext * context, const JsonValueCtx * parent, const QString & key )
		: JsonValueCtx( context, parent, key ), _wrappedArray( wrappedArray ) {}

	JsonArrayCtxProxy( const QJsonArray & wrappedArray, _ParsingContext * context, const JsonValueCtx * parent, int index )
		: JsonValueCtx( context, parent, index ), _wrappedArray( wrappedArray ) {}

	JsonArrayCtxProxy( const JsonArrayCtxProxy & other ) = default;

	operator bool() const { return _wrappedArray.has_value(); }

};

/** Wrapper around QJsonObject that knows its position in the JSON document and pops up an error messsage on invalid operations. */
class JsonObjectCtx : public JsonObjectCtxProxy {

 public:

	/// Constructs invalid JSON object wrapper.
	JsonObjectCtx()
		: JsonObjectCtxProxy() {}

	/// Constructs a JSON object wrapper with no parent, this should be only used for creating a root element.
	JsonObjectCtx( const QJsonObject & wrappedObject, _ParsingContext * context )
		: JsonObjectCtxProxy( wrappedObject, context ) {}

	/// Converts the temporary proxy object into the final object - workaround for cyclic dependancy.
	JsonObjectCtx( const JsonObjectCtxProxy & proxy )
		: JsonObjectCtxProxy( proxy ) {}

	/// Returns a sub-object at a specified key.
	/** If it doesn't exist it shows an error dialog and returns invalid object. */
	JsonObjectCtxProxy getObject( const QString & key, bool showError = true ) const;

	/// Returns a sub-array at a specified key.
	/** If it doesn't exist it shows an error dialog and returns invalid object. */
	JsonArrayCtxProxy getArray( const QString & key, bool showError = true ) const;

	/// Returns a bool at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	bool getBool( const QString & key, bool defaultVal, bool showError = true ) const;

	/// Returns an int at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	int getInt( const QString & key, int defaultVal, bool showError = true ) const;

	/// Returns an uint at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	uint getUInt( const QString & key, uint defaultVal, bool showError = true ) const;

	/// Returns an uint16_t at a specified key.
	uint16_t getUInt16( const QString & key, uint16_t defaultVal, bool showError = true ) const;

	/// Returns a double at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	double getDouble( const QString & key, double defaultVal, bool showError = true ) const;

	/// Returns a string at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	QString getString( const QString & key, const QString & defaultVal = QString(), bool showError = true ) const;

	/// Returns an enum at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	template< typename Enum >
	Enum getEnum( const QString & key, Enum defaultVal, bool showError = true ) const
	{
		uint intVal = getUInt( key, defaultVal, showError );
		if (intVal <= enumSize< Enum >()) {
			return Enum( intVal );
		} else {
			invalidTypeAtKey( key, enumName< Enum >() );
			return defaultVal;
		}
	}

 protected:

	void missingKey( const QString & key, bool showError ) const;
	void invalidTypeAtKey( const QString & key, const QString & expectedType ) const;
	QString elemPath( const QString & elemName ) const;

};

/** Wrapper around QJsonArray that knows its position in the JSON document and pops up an error messsage on invalid operations. */
class JsonArrayCtx : public JsonArrayCtxProxy {

 public:

	/// Constructs invalid JSON array wrapper.
	JsonArrayCtx() : JsonArrayCtxProxy() {}

	/// Converts the temporary proxy object into the final object - workaround for cyclic dependancy.
	JsonArrayCtx( const JsonArrayCtxProxy & proxy ) : JsonArrayCtxProxy( proxy ) {}

	int size() const { return _wrappedArray->size(); }

	/// Returns a sub-object at a specified index.
	/** If it doesn't exist it shows an error dialog and returns invalid array. */
	JsonObjectCtxProxy getObject( int index, bool showError = true ) const;

	/// Returns a sub-array at a specified index.
	/** If it doesn't exist it shows an error dialog and returns invalid array. */
	JsonArrayCtxProxy getArray( int index, bool showError = true ) const;

	/// Returns a bool at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	bool getBool( int index, bool defaultVal, bool showError = true ) const;

	/// Returns an int at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	int getInt( int index, int defaultVal, bool showError = true ) const;

	/// Returns an uint at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	uint getUInt( int index, uint defaultVal, bool showError = true ) const;

	/// Returns an uint16_t at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	uint16_t getUInt16( int index, uint16_t defaultVal, bool showError = true ) const;

	/// Returns a double at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	double getDouble( int index, double defaultVal, bool showError = true ) const;

	/// Returns a string at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	QString getString( int index, const QString & defaultVal = QString(), bool showError = true ) const;

	/// Returns a sub-object at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	template< typename Enum >
	Enum getEnum( int index, Enum defaultVal, bool showError = true ) const
	{
		uint intVal = getUInt( index, defaultVal, showError );
		if (intVal <= enumSize< Enum >()) {
			return Enum( intVal );
		} else {
			invalidTypeAtKey( index, enumName< Enum >() );
			return defaultVal;
		}
	}

 protected:

	void indexOutOfBounds( int index, bool showError ) const;
	void invalidTypeAtIdx( int index, const QString & expectedType ) const;
	QString elemPath( int index ) const;

};

class JsonDocumentCtx {

	//QJsonDocument & _wrappedDoc;
	JsonObjectCtx _rootObject;

	_ParsingContext _context;  ///< document-wide data related to an ongoing parsing process, each element has a pointer to this

 public:

	JsonDocumentCtx( QJsonDocument & wrappedDoc )
		: /*_wrappedDoc( wrappedDoc ),*/ _rootObject( wrappedDoc.object(), &_context ) {}

	JsonObjectCtx & rootObject() { return _rootObject; }

	void toggleWarnings( bool enable ) { _context.dontShowAgain = !enable; }

};


#endif // JSON_UTILS_INCLUDED
