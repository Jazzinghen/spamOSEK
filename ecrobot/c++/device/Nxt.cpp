//
// Nxt.cpp
//
// Copyright 2009 by Takashi Chikamasa, Jon C. Martin and Robert W. Kramer
//

#include "Nxt.h"
using namespace ecrobot;


//=============================================================================
// Constructor
Nxt::Nxt(void){}

//=============================================================================
// get battery voltage in mV
S16 Nxt::getBattMv(void) const
{
	return static_cast<S16>(ecrobot_get_battery_voltage());
}

//=============================================================================
// NXT buttons status
Nxt::eButton Nxt::getButtons(void) const
{
	U8 buttons = 0x00;

	if (ecrobot_is_RUN_button_pressed() == 1)
	{
		buttons |= Nxt::RUN_ON;
	}
	
	if (ecrobot_is_ENTER_button_pressed() == 1)
	{
		buttons |= Nxt::ENTR_ON;
	}
	return static_cast<Nxt::eButton>(buttons);
}

