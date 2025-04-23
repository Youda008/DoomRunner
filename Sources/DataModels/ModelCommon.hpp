//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: data types and constants common to all models
//======================================================================================================================

#ifndef MODEL_COMMON_INCLUDED
#define MODEL_COMMON_INCLUDED


#include "Essential.hpp"

#include "Utils/LangUtils.hpp"  // makeBitMask


//======================================================================================================================

// specialize this template for whatever flags it is necessary
template< typename BoolFlags, typename BitFlags = uint32_t >
BoolFlags expandToBools( BitFlags flags );

enum class AccessStyle
{
	ReadOnly,   ///< The model only provides its content to the list view for displaying it in the UI, no modifications are allowed.
	Editable,   ///< The model accepts various modification requests from the UI as a result of some user interaction.
};

/// Implemented ways of serializing items for copying them or moving them around.
namespace ExportFormat { enum Values : uint
{
	None      = 0,

	FileUrls  = (1 << 0),   ///< List of URLs to local files. Can be imported to any model whose items are constructible from file paths.
	Indexes   = (1 << 1),   ///< List of indexes where the items are in the current model. Can be imported only to the same model.

	All       = makeBitMask(3)
};}
using ExportFormats = std::underlying_type_t< ExportFormat::Values >;

// Multipurpose Internet Mail Extensions
//   https://en.wikipedia.org/wiki/MIME
//   https://www.iana.org/assignments/top-level-media-types
//   https://www.iana.org/assignments/media-types
// Qt uses this to move data between widgets via drag&drop or to store data in a clipboard.
namespace MimeTypes
{
	static constexpr const char * const ModelPtr  = "application/x.qt-model+ptr";
	static constexpr const char * const UriList   = "text/uri-list";                   ///< MIME type for ExportFormat::FileUrls
	static constexpr const char * const Indexes   = "application/x.qt-model+indexes";  ///< MIME type for ExportFormat::Indexes
}

/* C++ has this annoying behaviour where if you derive a sub-class from a super-class that has template parameters,
 * you cannot directly access its members and you have to write either SuperClass::member or this->member,
 * and that applies for both variables and methods.
 * And because we inherit from one of the storage implementation classes, the code can get pretty messy.
 * So we define this set of super-class access helpers to make the code little less unreadable.
 *
 * The recommendations are:
 *  - when you need to access a property, regardless whether from the current class or from its super-class, use this->
 *  - when you need to explicitly call one of the documented methods of any of Qt's abstract model classes, use QBaseModel::
 *  - when you want to manipulate with our underlying model implementation (for example FilteredList), use listImpl().
 */
#define DECLARE_MODEL_SUPERCLASS_ACCESSORS( SuperClass, StorageImplClass, storageShortName ) \
	using QBaseModel = typename SuperClass::QBaseModel; \
	      auto & storageShortName##Impl()       { return static_cast<       StorageImplClass & >( *this ); } \
	const auto & storageShortName##Impl() const { return static_cast< const StorageImplClass & >( *this ); } \


#endif // MODEL_COMMON_INCLUDED
