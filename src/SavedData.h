/*
Saves SSID and Password of the connected WiFi network to the EEPROM memory or
to be more precise, to the emulated EEPROM memory. ESP8266 has no EEPROM so part of
the flash memory is used to emulate an EEPROM.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include <EEPROM.h>
#include "common/networktypes.h"

namespace data
{
	// Address of the memory init cookie
	const uint16_t memoryInitAddress = (network::max_credential_length + 1) * 2;
	const uint16_t memoryInitAddressLength = 1;
	const uint8_t memoryInitValue = 128;
	const uint8_t eeprom_memory = memoryInitAddressLength + (network::max_credential_length + 1) * 2;

	class SavedData
	{
	public:
		SavedData(SavedData const &) = delete;
		void operator=(SavedData const &) = delete;
		static SavedData *getInstance();
		void initializeMemory();
		bool saveWifiCredentials(network::Credentials credentials);
		network::Credentials readWifiCredentials();

	private:
		//Methods
		SavedData();
		void saveWifiSSID(const char *ssid);
		void saveWifiPass(const char *pass);
		String readWifiSSID();
		String readWifiPass();
		//Variables
		static SavedData *m_instance;
	};
} // namespace data