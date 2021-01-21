/*
Complementing the SerialManager for this device's purpose, it handles additional
commands exchange fitting to this device's needs.
*/
#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "common/SerialManager.h"
#include "common/networktypes.h"

namespace serial {
	class SpecializedSerial : public SerialManager {
	public:
		SpecializedSerial(SpecializedSerial const&) = delete;
		void operator=(SpecializedSerial const&) = delete;
		static SpecializedSerial* getInstance();
		bool sendDeviceId(uint32_t device_id);
		bool sendNetInfo(const String& ssid, int32_t rssi, const String& ip);
		bool sendNetNotConnected();
		bool sendNetsStart(uint8_t networksCount);
		bool sendNetsList(const String& ssid, uint8_t encryption, int32_t rssi);
		bool sendNetsEnd();
		bool readNetChange();
		bool readNetRetry();
		bool readReset();
		network::Credentials readCredentials();

	private:
		//Methods
		SpecializedSerial();
		//Variables
		static SpecializedSerial* m_instance;
	};
}