#include "SpecializedSerial.h"

serial::SpecializedSerial *serial::SpecializedSerial::m_instance = nullptr;

serial::SpecializedSerial *serial::SpecializedSerial::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new SpecializedSerial();
	}
	return m_instance;
}

serial::SpecializedSerial::SpecializedSerial() {}

bool serial::SpecializedSerial::sendDeviceId(uint32_t device_id)
{
	Serial.print(F("CMD+DEVICE_ID:"));
	Serial.println(device_id);
	return getResponse("RSP+OK", response_timeout_mils);
}

//Sends network info via serial and expects to read an RSP+OK response. Returns true
//if response received within time limit or false otherwise.
bool serial::SpecializedSerial::sendNetInfo(const String &ssid, int32_t rssi, const String &ip)
{
	Serial.print(F("CMD+INFO:"));
	Serial.print(ssid);
	Serial.print(F(","));
	Serial.print(rssi);
	Serial.print(F(","));
	Serial.println(ip);
	return getResponse("RSP+OK", response_timeout_mils);
}

//Sends a network disconnected message via serial and expects to read
//an RSP+WAITING_NETWORKS response. Returns true if response received
//within time limit or false otherwise.
bool serial::SpecializedSerial::sendNetNotConnected()
{
	Serial.println(F("CMD+DISCONNECTED"));
	return getResponse("RSP+OK", response_timeout_mils);
}

//Sends a network list after sending a start message with the ammount of networks
//that will be sent and followed by networks and an end message. All 3 functions
//return true if responses to the corrensponding message were received.
bool serial::SpecializedSerial::sendNetsStart(uint8_t networksCount)
{
	Serial.print(F("CMD+START_LIST:"));
	Serial.println(networksCount);
	return getResponse("RSP+OK", response_timeout_mils * 2);
}

bool serial::SpecializedSerial::sendNetsList(const String &ssid, uint8_t encryption, int32_t rssi)
{
	Serial.print(F("CMD+NETWORK:"));
	Serial.print(ssid);			//max 17(will check)
	Serial.print(',');			//1
	Serial.print(rssi);			//max 4
	Serial.print(',');			//1
	Serial.println(encryption); //1
	return getResponse("RSP+OK", response_timeout_mils * 2);
}

bool serial::SpecializedSerial::sendNetsEnd()
{
	Serial.println(F("CMD+END_LIST"));
	return getResponse("RSP+OK", response_timeout_mils * 2);
}

//Returns true if the command in the custom serial buffer is the net change command.
bool serial::SpecializedSerial::readNetChange()
{
	if (!m_serial_buffer->find("CHANGE"))
	{
		return false;
	}
	Serial.println(F("RSP+OK"));
	return true;
}

//Returns true if the command in the custom serial buffer is the retry command.
bool serial::SpecializedSerial::readNetRetry()
{
	if (!m_serial_buffer->find("RETRY"))
	{
		return false;
	}
	Serial.println(F("RSP+OK"));
	return true;
}

//Returns true if the command in the custom serial buffer is the reset command.
bool serial::SpecializedSerial::readReset()
{
	if (!m_serial_buffer->find("RESET"))
	{
		return false;
	}
	Serial.println(F("RSP+OK"));
	return true;
}

//Searches the buffer for the "CREDENTIALS" command. If the command is found
//and the values are within the desired range it returns the new wifi credentials.
//In any different case (couldn't fetch the command, bad values), it returns a null
//credentials object.
network::Credentials serial::SpecializedSerial::readCredentials()
{
	char *command = "CREDENTIALS";
	network::Credentials bad_credentials = {0, 0};
	if (!m_serial_buffer->find(command))
	{
		return bad_credentials;
	}

	network::Credentials credentials = {0, 0};
	//m_SerialBuffer is read begining after the command and the ':' character
	uint8_t array_index = 0;
	uint8_t buffer_index = strlen(command) + 1;
	//Copy characters until the break ',' character is read
	do
	{
		credentials.ssid[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		if (array_index > network::max_credential_length)
		{
			//Serial.println(F("RSP+BAD_SSID_LENGTH"));
			Serial.println(F("RSP+OK"));
			return bad_credentials;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != ',');

	array_index = 0;
	buffer_index++; //Skip the ',' character
	//Copy characters until the last character
	do
	{
		credentials.pass[array_index] = m_serial_buffer->getChar(buffer_index);
		array_index++;
		if (array_index > network::max_credential_length)
		{
			//Serial.println(F("RSP+BAD_PASSWORD_LENGTH"));
			Serial.println(F("RSP+OK"));
			return bad_credentials;
		}
		buffer_index++;
	} while (m_serial_buffer->getChar(buffer_index) != '\0');
	//Otherwise report OK
	Serial.println(F("RSP+OK"));
	return credentials;
}
