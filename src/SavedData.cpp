#include "SavedData.h"

data::SavedData *data::SavedData::m_instance = nullptr;

data::SavedData *data::SavedData::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new SavedData();
	}
	return m_instance;
}

data::SavedData::SavedData() {}

//Note: ESP8266 contains no EEPROM, it emulates the EEPROM through the flash memory.
//That means that EEPROM memory must be declared as EEPROM.begin(SIZE); and an
//additional EEPROM.commit() must be called after calls to the write funtion.
void data::SavedData::initializeMemory()
{
	EEPROM.begin(eeprom_memory);

	uint8_t value = EEPROM.read(memoryInitAddress);
	if (value == memoryInitValue)
	{
		return;
	}

	network::Credentials new_credentials = {"Default", "Default"};
	if (saveWifiCredentials(new_credentials))
	{
		EEPROM.write(memoryInitAddress, memoryInitValue);
		EEPROM.commit();
	}
}

//SSID extends from the first byte of EEPROM memory to the 17th(0 through 16).
//It can have up to 16 characters and the semicolon delimeter, rest of the bytes will be filled
//with zeroes. A string is returned.
String data::SavedData::readWifiSSID()
{
	char ssid_buffer[network::max_credential_length + 1] = {0};
	int eeprom_index = 0;
	for (int i = 0; i < network::max_credential_length; i++)
	{
		char c = (char)EEPROM.read(eeprom_index + i);
		if (c == ';')
		{
			break;
		}
		ssid_buffer[i] = c;
	}
	return String(ssid_buffer);
}

//Password extends from the 18th byte of EEPROM memory to the 35th(17 through 34).
//It can have up to 16 characters and the semicolon delimeter, rest of the bytes will be filled
//with zeroes. A string is returned.
String data::SavedData::readWifiPass()
{
	char pass_buffer[network::max_credential_length + 1] = {0};
	uint8_t eeprom_index = network::max_credential_length + 1;
	for (int i = 0; i < network::max_credential_length; i++)
	{
		char c = (char)EEPROM.read(eeprom_index + i);
		if (c == ';')
		{
			break;
		}
		pass_buffer[i] = c;
	}
	return String(pass_buffer);
}

//A way to save the network::Credentials object directly.
//If the length of the given credentials is within range the credentials are saved.
bool data::SavedData::saveWifiCredentials(network::Credentials credentials)
{
	if (strlen(credentials.ssid) >= network::max_credential_length + 1 || strlen(credentials.pass) >= network::max_credential_length + 1)
	{
		return false;
	}
	saveWifiSSID(credentials.ssid);
	saveWifiPass(credentials.pass);
	return true;
}

//A way to read to a network::Credentials object directly.
network::Credentials data::SavedData::readWifiCredentials()
{
	network::Credentials credentials = {0, 0};
	strncpy(credentials.ssid, readWifiSSID().c_str(), network::max_credential_length + 1);
	strncpy(credentials.pass, readWifiPass().c_str(), network::max_credential_length + 1);
	return credentials;
}

//Writes the ssid starting at position 0 of the EEPRROM followed by the semicolon delimiter.
//Fills the rest of the 17 positions for the ssid with zeroes.
//Calls commit after writing for the reasons stated in the note.
void data::SavedData::saveWifiSSID(const char *ssid)
{
	uint8_t ssid_length = strlen(ssid);
	uint8_t eeprom_index = 0;
	for (uint8_t i = 0; i < ssid_length; i++)
	{
		EEPROM.write(eeprom_index + i, ssid[i]);
	}
	EEPROM.write(eeprom_index + ssid_length, ';');
	for (uint8_t i = ssid_length + 1; i < network::max_credential_length + 1; i++)
	{
		EEPROM.write(eeprom_index + i, 0);
	}
	EEPROM.commit();
}

//Writes the ssid starting at position network::max_credential_length+1 of the EEPRROM followed by the semicolon delimiter.
//Fills the rest of the 17 positions for the ssid with zeroes.
//Calls commit after writing for the reasons stated in the note.
void data::SavedData::saveWifiPass(const char *pass)
{
	uint8_t pass_length = strlen(pass);
	uint8_t eeprom_index = network::max_credential_length + 1;
	for (uint8_t i = 0; i < pass_length; i++)
	{
		EEPROM.write(eeprom_index + i, pass[i]);
	}
	EEPROM.write(eeprom_index + pass_length, ';');
	for (uint8_t i = pass_length + 1; i < network::max_credential_length + 1; i++)
	{
		EEPROM.write(eeprom_index + i, 0);
	}
	EEPROM.commit();
}