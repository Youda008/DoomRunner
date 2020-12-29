//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  29.12.2020
// Description: event filter that captures key presses and emits them as signals
//======================================================================================================================

#ifndef KEY_PRESS_EMITTER_INCLUDED
#define KEY_PRESS_EMITTER_INCLUDED


#include "Common.hpp"

#include <QObject>


//======================================================================================================================

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

/** event filter that captures key presses and emits them as signals */
class KeyPressEmitter : public QObject {

	Q_OBJECT

 public:

	KeyPressEmitter() {}
	virtual ~KeyPressEmitter() override {}

	virtual bool eventFilter( QObject * obj, QEvent * event ) override;

 signals:

	/// low-level control - notifies you about all key presses and releases, including modifiers
	void keyStateChanged( int key, KeyState state );

	/// high-level control - notifies you when a key is pressed and with which modifiers
	void keyPressed( int key, uint8_t modifiers );

 private:

	bool updateModifiers( int key, KeyState state );

	uint8_t pressedModifiers = 0;

};


#endif // KEY_PRESS_EMITTER_INCLUDED
