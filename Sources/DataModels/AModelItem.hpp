//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: abstract item for the GenericListModel
//              Has it's own header, because it's required in UserData.h which is included in almost everywhere.
//======================================================================================================================

#ifndef A_MODEL_ITEM_INCLUDED
#define A_MODEL_ITEM_INCLUDED

#include "Essential.hpp"

#include <QColor>
#include <QJsonObject>

#include <stdexcept>

class JsonObjectCtx;

class QString;
class QIcon;


//======================================================================================================================

#ifdef __GNUC__
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"  // std::optional sometimes makes false positive warnings
#endif

/// Abstract item for the GenericListModel.
/**
  * Every item type of ListModel must inherit from this struct to satisfy the requirements of the model. The dummy
  * methods denote what the model classes expect. Those methods should be overriden to point to the appropriate members
  * or implement the required functionality.
  *
  * The reason for the dummy implementation throwing exceptions is that different model configurations require different
  * item methods to be functional, and the configuration is currently run-time, so the compiler expects them to always
  * be there, even if they are not used.
  * We could try to convert the model configuration into template parameters, but then we would lose the ability
  * to let the view configure its model for what it needs, because the view can only access the non-template base class.
  *
  * Virtual methods (runtime polymorphism) are not needed here, because the item type is a template parameter
  * of the list model so the type is always fully known.
  */
struct AModelItem
{
	mutable std::optional< QColor > textColor;
	mutable std::optional< QColor > backgroundColor;
	bool isSeparator = false;  ///< true means this is a special item used to mark a section

	// methods required by read-only models

	// Should return an ID of this item that's unique within the list. Used for remembering selected items. Must always be implemented.
	const QString & getID() const;

	// Used for special purposes such as "Open File Location" action. Must be overriden when such action is enabled.
	const QString & getFilePath() const
	{
		throw std::logic_error(
			"File path has been requested, but getting Item's file path is not implemented. "
			"Either re-implement getFilePath() or disable actions requiring path in the view."
		);
	}

	// When icons are enabled, this must return the icon for this particular item.
	const QIcon & getIcon() const
	{
		throw std::logic_error(
			"Icon has been requested, but getting Item's icon is not implemented. "
			"Either re-implement getIcon() or disable icons in the view."
		);
	}

	// methods required by editable models

	bool isEditable() const
	{
		return false;
	}

	// When the model is set up to be editable, this must return the text to be edited in the view.
	const QString & getEditString() const
	{
		throw std::logic_error(
			"Edit has been requested, but editing this Item is not implemented. "
			"Either re-implement getEditString() or disable editing in the view."
		);
	}

	// When the model is set up to be editable, this must apply the user edit from the view.
	void setEditString( QString /*str*/ )
	{
		throw std::logic_error(
			"Edit has been requested, but editing this Item is not implemented. "
			"Either re-implement setEditString() or disable editing in the view."
		);
	}

	// methods required by models with checkable items

	// Whether this item has an active checkbox in the view.
	bool isCheckable() const
	{
		return false;
	}

	// When the model is set up to have checkboxes, this must return whether the checkbox should be displayed as checked.
	bool isChecked() const
	{
		throw std::logic_error(
			"Check state has been requested, but checking this Item is not implemented. "
			"Either re-implement isChecked() or disable checkable items in the view."
		);
	}

	// When the model is set up to have checkboxes, this must apply the new status of the checkbox.
	void setChecked( bool /*checked*/ ) const
	{
		throw std::logic_error(
			"Check state has been requested, but checking this Item is not implemented. "
			"Either re-implement setChecked() or disable checkable items in the view."
		);
	}
};

#ifdef __GNUC__
 #pragma GCC diagnostic pop
#endif


#endif // A_MODEL_ITEM_INCLUDED
