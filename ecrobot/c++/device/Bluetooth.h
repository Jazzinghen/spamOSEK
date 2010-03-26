//
// Bluetooth.h
//
// Copyright 2009 by Takashi Chikamasa, Jon C. Martin and Robert W. Kramer
//

#ifndef BLUETOOTH_H_
#define BLUETOOTH_H_


extern "C"
{
	#include "ecrobot_interface.h"
	#include "rtoscalls.h"
};

namespace ecrobot
{
/**
 * Bluetooth communication class.
 *
 * Note:<BR>
 * User specified friendly device name using setFriendlyName seems to be appeared in the Windows<BR>
 * Bluetooth device dialog only when NXT BIOS is used as the firmware (why?).
 */
class Bluetooth
{
public:
	/**
	 * Maximum length of data in byte for send/receive.
	 */
	static const U32 MAX_BT_DATA_LENGTH = 254;

	/**
	 * Constructor.
	 * Note:<BR>
	 * This class must be constructed as a global object. Otherwise, a device assertion will be displayed<BR>
	 * in the LCD when the object is constructed as a non global object.
	 * When the object is destructed while the system is shut down, Bluetooth is closed automatically.
	 * @param -
	 * @return -
	 */
	Bluetooth(void);

	/**
	 * Destructor.
	 * Close the existing connection.
	 * @param -
	 * @return -
	 */
	~Bluetooth(void);

	/**
	 * Wait for a connection as a slave device.
	 * @param passkey Bluetooth passkey (up to 16 characters. I.e. "1234")
	 * @param duration Wait duration in msec (duration = 0 means wait forever)
	 * @return true:connected/false:not connected
	 */
	bool waitForConnection(const CHAR* passkey, U32 duration);

	/**
	 * Wait for a connection as a master device.
	 * @param passkey Bluetooth passkey (up to 16 characters. I.e. "1234")
	 * @param address Bluetooth Device Address to be connected (7bytes hex array data)
	 * @param duration Wait duration in msec (duration = 0 means wait forever)
	 * @return true:connected/false:not connected
	 */
	bool waitForConnection(const CHAR* passkey, const U8 address[7], U32 duration);

	/**
	 * Cancel of waiting for a connection.
	 * This member function must be invoked from the 1msec interrupt hook function (i.e. user_1ms_isr_type2 for nxtOSEK)\n
	 * to cancel of waiting for a connection. Otherwise, it does not work.
	 * @param -
	 * @return -
	 */
	void cancelWaitForConnection(void);

	/**
	 * Get the Bluetooth Device Address of the device.
	 * @param address Bluetooth Device Address (7bytes hex array data)
	 * @return true:succeeded/false:failed
	 */
	bool getDeviceAddress(U8 address[7]) const;

	/**
	 * Get the friendly name of the device.
	 * @param name Friendly name (max. 16 characters)
	 * @return true:succeeded/false:failed
	 */
	bool getFriendlyName(CHAR* name) const;

	/**
	 * Set the friendly name of the device.
	 * While a connection is established, device name can't be changed.
	 * @param name Friendly name (max. 16 characters)
	 * @return true:succeeded/false:failed
	 */
	bool setFriendlyName(const CHAR* name);

	/**
	 * Get connection status.
	 * @param -
	 * @return true:connected/false:not connected
	 */
	bool isConnected(void) const;

	/**
	 * Send data to the connected device.
	 * @param data Data to be sent
	 * @param length Length of data to be sent (up to 254)
	 * @return Length of sent data.
	 */
	U32 send(U8* data, U32 length);

	/**
	 * Receive data from the connected device.
	 * @param data Data to be received
	 * @param length Length of data to be received (up to 254)
	 * @return Length of received data.
	 */
	U32 receive(U8* data, U32 length) const;
	
	/**
	 * Reset all settings in the persistent settings in the BlueCore chip.
	 * The BlueCore chip should be restarted (remove the battery) after calling this function.<BR>
	 * Otherwise old values can be floating around the BlueCore chip causing<BR>
	 * unexpected behavior.
	 * @param -
	 * @return -
	 */
	 void setFactorySettings(void);

private:
	bool mIsConnected;
	bool mCancelConnection;
	void close(void);
};
}

#endif
