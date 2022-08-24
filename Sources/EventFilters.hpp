//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: classes that capture and report various events
//======================================================================================================================

#ifndef EVENT_FILTERS_INCLUDED
#define EVENT_FILTERS_INCLUDED


#include "Common.hpp"

#include <QObject>


//======================================================================================================================
//  common types

enum class KeyState
{
	PRESSED,
	RELEASED
};

enum Modifier : uint8_t
{
	CTRL  = 1 << 0,
	ALT   = 1 << 1,
	SHIFT = 1 << 2
};


//======================================================================================================================
//  this is extracted into a separate class so it can be used inside individual widgets

class ModifierHandler {

 public:

	bool updateModifiers_pressed( int key );
	bool updateModifiers_released( int key );
	bool updateModifiers( int key, KeyState state );

	uint8_t pressedModifiers() const { return _pressedModifiers; }

 private:

	template< KeyState state >
	bool updateModifiers( int key );

	uint8_t _pressedModifiers = 0;

};


//======================================================================================================================
/** event filter that captures key presses and emits them as signals */

class KeyPressFilter : public QObject {

	Q_OBJECT

 public:

	KeyPressFilter() {}
	virtual ~KeyPressFilter() override {}

	virtual bool eventFilter( QObject * obj, QEvent * event ) override;

 signals:

	/// low-level control - notifies you about all key presses and releases, including modifiers
	void keyStateChanged( int key, KeyState state );

	/// high-level control - notifies you when a key is pressed and with which modifiers
	void keyPressed( int key, uint8_t modifiers );

 private:

	ModifierHandler modifierHandler;

};


//======================================================================================================================
/** event filter that captures enter presses and emits them as signal */

class ConfirmationFilter : public QObject {

	Q_OBJECT

 public:

	ConfirmationFilter() {}
	virtual ~ConfirmationFilter() override {}

	virtual bool eventFilter( QObject * obj, QEvent * event ) override;

 signals:

	void choiceConfirmed();

};


#endif // EVENT_FILTERS_INCLUDED
