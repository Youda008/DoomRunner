//======================================================================================================================
// Project: DoomRunner
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: event filter that captures key presses and emits them as signals
//======================================================================================================================

#include "EventFilters.hpp"

#include <QEvent>
#include <QKeyEvent>


//======================================================================================================================
// ModifierHandler

template< KeyState state >
static inline uint8_t toggleModifiers( uint8_t currentModifiers, uint8_t newModifiers )
{
	if constexpr (state == KeyState::Pressed)
		return currentModifiers | newModifiers;
	else
		return currentModifiers & ~newModifiers;
}

template< KeyState state >
inline bool ModifierHandler::updateModifiers( int key )
{
	switch (key)
	{
	 case Qt::Key_Control:
		_pressedModifiers = toggleModifiers< state >( _pressedModifiers, Modifier::Ctrl );
		return true;
	 case Qt::Key_Alt:
		_pressedModifiers = toggleModifiers< state >( _pressedModifiers, Modifier::Alt );
		return true;
	 case Qt::Key_AltGr:
		_pressedModifiers = toggleModifiers< state >( _pressedModifiers, Modifier::Alt | Modifier::Ctrl );
		return true;
	 case Qt::Key_Shift:
		_pressedModifiers = toggleModifiers< state >( _pressedModifiers, Modifier::Shift );
		return true;
	 default:
		return false;
	}
}

bool ModifierHandler::updateModifiers_pressed( int key )
{
	return updateModifiers< KeyState::Pressed >( key );
}

bool ModifierHandler::updateModifiers_released( int key )
{
	return updateModifiers< KeyState::Released >( key );
}


//======================================================================================================================
// KeyPressFilter

bool KeyPressFilter::eventFilter( QObject * obj, QEvent * event )
{
	if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
	{
		QKeyEvent * keyEvent = static_cast< QKeyEvent * >( event );
		int key = keyEvent->key();
		int state = event->type();

		emit keyStateChanged( key, state == QEvent::KeyPress ? KeyState::Pressed : KeyState::Released );

		bool isModifier = (state == QEvent::KeyPress)
			? modifierHandler.updateModifiers_pressed( key )
			: modifierHandler.updateModifiers_released( key );

		if (!isModifier && state == QEvent::KeyPress)
			emit keyPressed( key, modifierHandler.pressedModifiers() );

		if (suppressKeyEvents)
			return true;
	}

	return QObject::eventFilter( obj, event );
}


//======================================================================================================================
// ConfirmationFilter

bool ConfirmationFilter::eventFilter( QObject * obj, QEvent * event )
{
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent * keyEvent = static_cast< QKeyEvent * >( event );

		if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return)
		{
			emit choiceConfirmed();
		}
	}

	return QObject::eventFilter( obj, event );
}
