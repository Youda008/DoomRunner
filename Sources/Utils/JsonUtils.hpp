//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: JSON parsing helpers that handle errors and simplify parsing code
//======================================================================================================================

#ifndef JSON_UTILS_INCLUDED
#define JSON_UTILS_INCLUDED


#include "Essential.hpp"
#include "CommonTypes.hpp"

#include <QString>
#include <QList>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>


//======================================================================================================================
// in order for the getEnum method to work, the author of the enum must specialize the following templates

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

namespace impl
{
	/// data related to an ongoing parsing process
	struct ParsingContext
	{
		QString filePath;
		bool dontShowAgain = false;  ///< whether to show "invalid element" errors to the user

		QString fileName() const;
	};
}

/// mechanisms common for JSON objects and arrays
class JsonValueCtx {

 protected:

	/// JSON key - either string key for objects or int index for arrays
	struct Key
	{
		enum Type
		{
			Uninitialized = 0,
			ObjectKey,
			ArrayIndex
		};
		Type type;
		QString key;
		int idx;

		Key() : type( Uninitialized ), key(), idx( -1 ) {}
		Key( const QString & key ) : type( ObjectKey ), key( key ), idx( -1 ) {}
		Key( int idx ) : type( ArrayIndex ), key(), idx( idx ) {}
	};

	impl::ParsingContext * _context;  ///< document-wide context shared among all elements of that document, the struct is stored in JsonDocumentCtx

	const JsonValueCtx * _parent;  ///< JSON element that contains this element
	Key _key;  ///< key or index that this element has in its parent element

 public:

	/// Constructs invalid JSON value.
	/** This should only be used to indicate missing element or failure.
	  * Anything else than isValid() or operator bool() is undefined. */
	JsonValueCtx()
		: _context( nullptr ), _parent( nullptr ), _key() {}

	/// Constructs a JSON value with no parent.
	/** This should only be used for creating a root element. */
	JsonValueCtx( impl::ParsingContext * context )
		: _context( context ), _parent( nullptr ), _key() {}

	/// Constructs a JSON value with a parent that is a JSON object.
	JsonValueCtx( impl::ParsingContext * context, const JsonValueCtx * parent, const QString & key )
		: _context( context ), _parent( parent ), _key( key ) {}

	/// Constructs a JSON value with a parent that is a JSON array.
	JsonValueCtx( impl::ParsingContext * context, const JsonValueCtx * parent, int index )
		: _context( context ), _parent( parent ), _key( index ) {}

	JsonValueCtx( const JsonValueCtx & ) = default;
	JsonValueCtx( JsonValueCtx && ) = default;
	JsonValueCtx & operator=( const JsonValueCtx & ) = default;
	JsonValueCtx & operator=( JsonValueCtx && ) = default;

	/// If this returns false, this object must not be used.
	bool isValid() const   { return _context != nullptr; }
	operator bool() const  { return isValid(); }

	bool isRoot() const    { return _parent == nullptr; }

	/// Reconstructs a path of this element in its JSON document.
	QString getJsonPath() const;

 protected:

	void constructJsonPathRecursively( QString & path ) const;

};

/// proxy class that solves cyclic dependancy between JsonObjectCtx and JsonArrayCtx
class JsonObjectCtxProxy : public JsonValueCtx {

 protected:

	QJsonObject _wrappedObject;

 public:

	JsonObjectCtxProxy()
		: JsonValueCtx(), _wrappedObject() {}

	JsonObjectCtxProxy( QJsonObject wrappedObject, impl::ParsingContext * context )
		: JsonValueCtx( context ), _wrappedObject( std::move(wrappedObject) ) {}

	JsonObjectCtxProxy( QJsonObject wrappedObject, impl::ParsingContext * context, const JsonValueCtx * parent, const QString & key )
		: JsonValueCtx( context, parent, key ), _wrappedObject( std::move(wrappedObject) ) {}

	JsonObjectCtxProxy( QJsonObject wrappedObject, impl::ParsingContext * context, const JsonValueCtx * parent, int index )
		: JsonValueCtx( context, parent, index ), _wrappedObject( std::move(wrappedObject) ) {}

	JsonObjectCtxProxy( const JsonObjectCtxProxy & ) = default;
	JsonObjectCtxProxy( JsonObjectCtxProxy && ) = default;
	JsonObjectCtxProxy & operator=( const JsonObjectCtxProxy & ) = default;
	JsonObjectCtxProxy & operator=( JsonObjectCtxProxy && ) = default;

};

/// proxy class that solves cyclic dependancy between JsonObjectCtx and JsonArrayCtx
class JsonArrayCtxProxy : public JsonValueCtx {

 protected:

	QJsonArray _wrappedArray;

 public:

	JsonArrayCtxProxy()
		: JsonValueCtx(), _wrappedArray() {}

	JsonArrayCtxProxy( QJsonArray wrappedArray, impl::ParsingContext * context, const JsonValueCtx * parent, const QString & key )
		: JsonValueCtx( context, parent, key ), _wrappedArray( std::move(wrappedArray) ) {}

	JsonArrayCtxProxy( QJsonArray wrappedArray, impl::ParsingContext * context, const JsonValueCtx * parent, int index )
		: JsonValueCtx( context, parent, index ), _wrappedArray( std::move(wrappedArray) ) {}

	JsonArrayCtxProxy( const JsonArrayCtxProxy & ) = default;
	JsonArrayCtxProxy( JsonArrayCtxProxy && ) = default;
	JsonArrayCtxProxy & operator=( const JsonArrayCtxProxy & ) = default;
	JsonArrayCtxProxy & operator=( JsonArrayCtxProxy && ) = default;

};

/** Wrapper around QJsonObject that knows its position in the JSON document and pops up an error messsage on invalid operations. */
class JsonObjectCtx : public JsonObjectCtxProxy {

 public:

	/// Constructs invalid JSON object wrapper.
	JsonObjectCtx() : JsonObjectCtxProxy() {}

	/// Constructs a JSON object wrapper with no parent, this should only be used for creating a root element.
	JsonObjectCtx( QJsonObject wrappedObject, impl::ParsingContext * context )
		: JsonObjectCtxProxy( std::move(wrappedObject), context ) {}

	/// Converts the temporary proxy object into the final object - workaround for cyclic dependancy.
	JsonObjectCtx( JsonObjectCtxProxy proxy )
		: JsonObjectCtxProxy( std::move(proxy) ) {}

	auto keys() const { return _wrappedObject.keys(); }

	bool hasMember( const QString & key ) const { return _wrappedObject.contains( key ); }

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
	/** If it doesn't exist it shows an error dialog and returns default value. */
	uint16_t getUInt16( const QString & key, uint16_t defaultVal, bool showError = true ) const;

	/// Returns an int64_t at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	int64_t getInt64( const QString & key, int64_t defaultVal, bool showError = true ) const;

	/// Returns a double at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	double getDouble( const QString & key, double defaultVal, bool showError = true ) const;

	/// Returns a string at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	QString getString( const QString & key, QString defaultVal = QString(), bool showError = true ) const;

	/// Returns an enum at a specified key.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	template< typename Enum >
	Enum getEnum( const QString & key, Enum defaultVal, bool showError = true ) const
	{
		uint intVal = getUInt( key, uint(defaultVal), showError );
		if (intVal <= enumSize< Enum >()) {
			return Enum( intVal );
		} else {
			invalidTypeAtKey( key, enumName< Enum >() );
			return defaultVal;
		}
	}

 protected:

	void missingKey( const QString & key, bool showError ) const;
	void invalidTypeAtKey( const QString & key, const QString & expectedType, bool showError = true ) const;
	QString elemPath( const QString & elemName ) const;

};

/** Wrapper around QJsonArray that knows its position in the JSON document and pops up an error messsage on invalid operations. */
class JsonArrayCtx : public JsonArrayCtxProxy {

 public:

	/// Constructs invalid JSON array wrapper.
	JsonArrayCtx() : JsonArrayCtxProxy() {}

	/// Converts the temporary proxy object into the final object - workaround for cyclic dependancy.
	JsonArrayCtx( JsonArrayCtxProxy proxy ) : JsonArrayCtxProxy( std::move(proxy) ) {}

	int size() const { return _wrappedArray.size(); }

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

	/// Returns an int64_t at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	int64_t getInt64( int index, int64_t defaultVal, bool showError = true ) const;

	/// Returns a double at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	double getDouble( int index, double defaultVal, bool showError = true ) const;

	/// Returns a string at a specified index.
	/** If it doesn't exist it shows an error dialog and returns default value. */
	QString getString( int index, QString defaultVal = QString(), bool showError = true ) const;

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
	void invalidTypeAtIdx( int index, const QString & expectedType, bool showError = true ) const;
	QString elemPath( int index ) const;

};

class JsonDocumentCtx {

	JsonObjectCtx _rootObject;

	mutable impl::ParsingContext _context;  ///< document-wide data related to an ongoing parsing process, each element has a pointer to this

 public:

	/// Constructs invalid JSON document.
	/** This should only be used to indicate a failure. Anything else than isValid() or operator bool() is undefined. */
	JsonDocumentCtx() : _rootObject() {}

	JsonDocumentCtx( QString filePath, const QJsonDocument & wrappedDoc )
		: _rootObject( wrappedDoc.object(), &_context )  { _context.filePath = std::move(filePath); }

	// _rootObject has ptr to _context, if this gets moved/copied, it will have pointer to a different (possibly temporary) object
	JsonDocumentCtx( const JsonDocumentCtx & ) = delete;
	JsonDocumentCtx( JsonDocumentCtx && ) = delete;
	JsonDocumentCtx & operator=( const JsonDocumentCtx & ) = delete;
	JsonDocumentCtx & operator=( JsonDocumentCtx && ) = delete;

	/// If this returns false, this object must not be used.
	bool isValid() const   { return _rootObject.isValid(); }
	operator bool() const  { return isValid(); }

	const QString & filePath() const { return _context.filePath; }
	      QString   fileName() const { return _context.fileName(); }

	const JsonObjectCtx & rootObject() const { return _rootObject; }

	void disableWarnings() const { _context.dontShowAgain = false; }

};


//======================================================================================================================
// generic utils

inline QJsonArray serializeStringVec( const QStringVec & vec )
{
	QJsonArray jsArray;
	for (const auto & elem : vec)
	{
		jsArray.append( elem );
	}
	return jsArray;
}

inline QStringVec deserializeStringVec( const JsonArrayCtx & jsVec )
{
	QStringVec vec;
	for (int i = 0; i < jsVec.size(); i++)
	{
		QString elem = jsVec.getString( i );
		if (!elem.isEmpty())
			vec.append( std::move(elem) );
	}
	return vec;
}

template< typename List >
QJsonArray serializeList( const List & list )
{
	QJsonArray jsArray;
	for (const auto & elem : list)
	{
		jsArray.append( serialize( elem ) );
	}
	return jsArray;
}

template< typename List >
void deserializeList( const JsonArrayCtx & jsList, List & list )
{
	for (int i = 0; i < jsList.size(); i++)
	{
		JsonObjectCtx jsElem = jsList.getObject( i );
		if (jsElem)
		{
			list.emplace();
			deserialize( jsElem, list.last() );
		}
	}
}

template< typename Elem >
QJsonObject serializeMap( const QHash< QString, Elem > & map )
{
	QJsonObject jsMap;
	for (auto iter = map.begin(); iter != map.end(); ++iter)
	{
		jsMap[ iter.key() ] = serialize( iter.value() );
	}
	return jsMap;
}

template< typename Elem >
void deserializeMap( const JsonObjectCtx & jsMap, QHash< QString, Elem > & map )
{
	auto keys = jsMap.keys();
	for (const QString & key : keys)
	{
		JsonObjectCtx jsElem = jsMap.getObject( key );
		if (jsElem)
		{
			Elem elem;
			deserialize( jsElem, elem );
			map.insert( key, elem );
		}
	}
}


//======================================================================================================================
// high-level file I/O helpers

bool writeJsonToFile( const QJsonDocument & jsonDoc, const QString & filePath, const QString & fileDesc );

inline constexpr bool IgnoreEmpty = true;
inline constexpr bool CheckIfEmpty = false;

/// Reads a text file and attempts to parse it as a JSON.
/** Returns nullptr if the file could not be opened or read, or invalid JsonDocument if it could not be parsed. */
std::unique_ptr< JsonDocumentCtx > readJsonFromFile( const QString & filePath, const QString & fileDesc, bool ignoreEmpty = false );


#endif // JSON_UTILS_INCLUDED
