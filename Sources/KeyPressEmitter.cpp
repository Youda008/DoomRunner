//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  29.12.2020
// Description: event filter that captures key presses and emits them as signals
//======================================================================================================================

#include "KeyPressEmitter.hpp"

#include <QEvent>
#include <QKeyEvent>


//======================================================================================================================
//  KeyPressEmitter

static bool isArrow( int key )
{
	return key >= Qt::Key_Left && key <= Qt::Key_Down;
}

bool KeyPressEmitter::updateModifiers( int key, KeyState state )
{
	auto updateModifier = [ this ]( uint8_t modifier, KeyState state )
	{
		if (state == KeyState::PRESSED)
			pressedModifiers |= modifier;
		else
			pressedModifiers &= ~modifier;
	};

	switch (key)
	{
	 case Qt::Key_Control:
		updateModifier( Modifier::CTRL, state );
		return true;
	 case Qt::Key_Alt:
		updateModifier( Modifier::ALT, state );
		return true;
	 case Qt::Key_AltGr:
		updateModifier( Modifier::ALT | Modifier::CTRL, state );
		return true;
	 case Qt::Key_Shift:
		updateModifier( Modifier::SHIFT, state );
		return true;
	 default:
		return false;
	}
}

bool KeyPressEmitter::eventFilter( QObject * obj, QEvent * event )
{
	if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
	{
		QKeyEvent * keyEvent = static_cast< QKeyEvent * >( event );
		int key = keyEvent->key();
		KeyState state = event->type() == QEvent::KeyPress ? KeyState::PRESSED : KeyState::RELEASED;

		emit keyStateChanged( key, state );

		bool isModifier = updateModifiers( key, state );

		if (!isModifier && state == KeyState::PRESSED)
			emit keyPressed( key, pressedModifiers );

		// supress arrow key navigation when a modifier is pressed
		if (pressedModifiers != 0 && isArrow( key ))
			return true;
	}

	return QObject::eventFilter( obj, event );
}
