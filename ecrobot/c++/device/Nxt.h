//
// Nxt.h
//
// Copyright 2009 by Takashi Chikamasa, Jon C. Martin and Robert W. Kramer
//

#ifndef NXT_H_
#define NXT_H_

extern "C"
{
	#include "ecrobot_interface.h"
	#include "rtoscalls.h"
};

namespace ecrobot
{
/**
 * NXT intelligent block class.
 */
class Nxt
{
public:
	/**
	 * NXT Button enum.
	 */
	enum eButton
	{
		RUN_ON = 0x01,                    /**< RUN (right triangle) button is ON */
		ENTR_ON = 0x02,                   /**< ENTR(orange rectangle) button is ON */
		RUN_ENTR_ON = (RUN_ON | ENTR_ON), /**< RUN and ENTR buttons are ON */
		BUTTONS_OFF = 0x00                /**< RUN and ENTR buttons are OFF */
	};

	/**
	 * Constructor.
	 * @param -
	 * @return -
	 */
	Nxt(void);

	/**
	 * Get NXT buttons status.
	 * @param -
	 * @return Status of RUN/ENTR buttons
	 */
	eButton getButtons(void) const;

	/**
	 * Get battery voltage in mV.
	 * @param -
	 * @return Battery voltage in mV
	 */
	S16 getBattMv(void) const;
};
}

#endif
