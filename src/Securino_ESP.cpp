/*
Note: The yield() function is used through out loops that extend to an
unknown amount of seconds. If the loop extends to more than 50ms, calling
a delay or yield() (which is equal to delay(0);) is recommended as it keeps
the WiFi stack running smoothly. TCP/IP and WiFi operations are handled only
at the end of each loop() and during delays.
*/
#include <ESP8266WiFi.h>
#include "common/alarmtypes.h"
#include "common/networktypes.h"
#include "common/Timer.h"
#include "SpecializedSerial.h"
#include "UbidotsManager.h"
#include "SavedData.h"

#pragma region Constants
const uint16_t upload_state_secs = 120;
const uint16_t download_state_secs = 120;
#pragma endregion

#pragma region Global
// Class Instances
serial::SpecializedSerial *g_serial = serial::SpecializedSerial::getInstance();
ubidots::UbidotsManager *g_ubidots_client = ubidots::UbidotsManager::getInstance();
data::SavedData *g_saved_data = data::SavedData::getInstance();

// Variables
alarm::Status g_status = {alarm::state_disarmed, alarm::method_none, alarm::sensor_none_triggered};
network::Credentials g_credentials = {0, 0};
Timer g_upload_timer = Timer(upload_state_secs);	 //Timer to update ubidots variables
Timer g_download_timer = Timer(download_state_secs); //Timer to check for online variable changes
#pragma endregion

#pragma region Forward Declarations
void getNetworkCredentials();
bool connectNetwork();
void onBootConnectNetwork();
uint8_t sendScannedNets();
bool checkLocalStateChange();
bool checkNetworkChange();
void checkReset();
void checkOnlineStateChange();
void timedStateUpdate();
#pragma endregion

void setup()
{
	//Set ESP8266 to client mode
	WiFi.mode(WIFI_STA);

	// Initialize serial com
	g_serial->init();

	// Initialize the EEPROM memory
	g_saved_data->initializeMemory();

	// Send the device ID
	uint32_t unique_id = ESP.getChipId();
	while (!g_serial->sendDeviceId(unique_id))
	{
		yield();
	}

	//Copy credentials from EEPROM
	g_credentials = g_saved_data->readWifiCredentials();

	//If a connection cannot be established, probably because the ssid was not found,
	//a new list of networks is sent.	New ssid and password is expected from user.
	onBootConnectNetwork();

	//Send the state to ubidots
	g_ubidots_client->sendStatus(g_status);
}

//If there is an established wifi connection then the ESP8266 checks
//for changes(made from the mobile app) on the ubidots platform,
//for changes on the state of the arduino and lastly it perfoms an
//update every 5 minutes as an indicator that the alarm is still powered on
//and online.
void loop()
{

	//Checks if there is a received serial command, gets the command to the custom
	//serial buffer and then compares it with possible commands. Then clears the buffer.
	if (g_serial->getCommand())
	{
		checkLocalStateChange();
		checkNetworkChange();
		checkReset();
		g_serial->clearSerial();
	}

	//If Wifi is connected, every few seconds the program checks for an online state
	//change and if such a change	has occured, the change is sent to the arduino.
	//Also every few seconds the online variables are updated to indicate
	//that the alarm is online and working.
	if (WiFi.isConnected())
	{
		checkOnlineStateChange();
		timedStateUpdate();
	}
	else
	{
		//Else a troubleshooting takes place depending on the state of the alarm.
		//If the alarm is disarmed attempts are made to reconnect. Otherwise it constantly
		//tries a reconnect to the previous network.
		if (g_status.state == alarm::state_disarmed)
		{
			g_serial->sendNetNotConnected();
		}
		connectNetwork();
	}
}

//Reads the command and if its not a credentials command, it tries until credentials
//are given. If it is a credentials command, if valid credentials were given,
//it replaces the current credentials.
void getNetworkCredentials()
{
	bool done = false;
	while (!done)
	{
		if (g_serial->getCommand())
		{
			network::Credentials new_credentials = {0, 0};
			new_credentials = g_serial->readCredentials();
			if (strlen(new_credentials.ssid) > 0)
			{
				g_credentials = new_credentials;
				done = true;
			}
			g_serial->clearSerial();
		}
		yield();
	}
	g_saved_data->saveWifiCredentials(g_credentials);
}

//Attempts to establish a wifi connection with the global credentials. If a
//connection is established, the network info (SSID, RSSI, Local IP), are sent to
//the arduino.
bool connectNetwork()
{
	//Disconnects from the connected network
	WiFi.disconnect();

	//Delay ensures disconnection happens before trying to reconnect
	delay(500);

	//Use the credentials to establish a connection via ubidots
	g_ubidots_client->wifiConnect(g_credentials.ssid, g_credentials.pass);

	//Frequent delays are needed for the wifi libraries, without a delay here
	//the WiFi library won't have time to refresh its contents and wifi will
	//report connected. This happens because it's been some time from the last
	//end of loop at which point those libraries do any pending operations.
	delay(500);

	//If not connected send network info to arduino, exit
	if (!WiFi.isConnected())
	{
		return false;
	}

	bool sent = false;
	while (!sent)
	{
		sent = g_serial->sendNetInfo(
			WiFi.SSID(),
			WiFi.RSSI(),
			WiFi.localIP().toString());
		yield();
	}
	return sent;
}

//Tries to connect on the previous network during boot. If unable to connect,
//it sends the disconneced command via serial to the arduino, and listens for network
//change command while retrying to connect.
void onBootConnectNetwork()
{
	bool connected = connectNetwork(); //Connect to wifi
	while (!connected)
	{
		if (g_serial->sendNetNotConnected())
		{
			while (!g_serial->getCommand())
			{
				yield();
			}
			checkNetworkChange();
			g_serial->clearSerial();
		}
		connected = connectNetwork();
		yield();
	}
}

//Scans networks and if the returned count is equal to 0 or better, a list
//is sent back to the arduino. -1 is defined as "scan running" and -2 as "scan failed".
uint8_t sendScannedNets()
{
	int8_t networksCount = WiFi.scanNetworks();
	if (networksCount >= 0)
	{
		while (!g_serial->sendNetsStart(networksCount))
		{
			yield();
		}
		if (networksCount > 0)
		{
			for (uint8_t i = 0; i < networksCount; i++)
			{
				bool sent = false;
				while (!sent)
				{
					sent = g_serial->sendNetsList(
						WiFi.ESP8266WiFiScanClass::SSID(i),
						WiFi.ESP8266WiFiScanClass::encryptionType(i),
						WiFi.ESP8266WiFiScanClass::RSSI(i));
					yield();
				}
				yield();
			}
			while (!g_serial->sendNetsEnd())
			{
				yield();
			}
		}
	}
	return networksCount;
}

//Reads the state command from serial and compares it to the current state.
//If the current state is the same with the new one, then nothing happens.
//Otherwise the new state overides the old and ubidots variables are updated.
bool checkLocalStateChange()
{
	alarm::Status new_status = g_serial->readStatus(g_status);
	if ((g_status.state == new_status.state) && (g_status.method == new_status.method) && (g_status.sensor == new_status.sensor))
	{
		return false;
	}
	g_status = new_status;
	g_ubidots_client->sendStatus(g_status);
	return true;
}

//If a network change is requested, a scan of the networks is sent,
//credntials are received and a connection to the network is attempted.
//Whether the it connects to the network or not as well as the reattempts
//are handled inside the main loop().
bool checkNetworkChange()
{
	if (g_serial->readNetChange())
	{
		if (sendScannedNets() > 0)
		{
			getNetworkCredentials();
			connectNetwork();
		}
		return true;
	}
	return false;
}

//It restarts the ESP upon request from the arduino.
void checkReset()
{
	if (g_serial->readReset())
	{
		ESP.restart();
	}
}

//This function is responsible for reading the ubidots variables every 30 seconds
//and then compare them to the existing state. If the variables are different from the
//stored ones then that means that a user changed the state from the mobile app.
//The function then stores the variables, replacing the excisting ones and
//sends a state change command to the arduino.
void checkOnlineStateChange()
{
	//If 30 seconds have passed
	if (g_download_timer.timeout())
	{
		//Storing the existing state
		alarm::Status new_status = g_ubidots_client->getStatus(g_status);
		//If the previous state isnot the same with the currently read,
		//then replace the current state and send a serial command
		//to the arduino for the state to change.
		//Last, reset the timer.
		if ((g_status.state != new_status.state) || (g_status.method != new_status.method) || (g_status.sensor != new_status.sensor))
		{
			g_status = new_status;
			g_serial->sendStatus(g_status);
		}
		g_download_timer.reset();
	}
}

//Every 5 minutes updates the online variables, indicating that the system is
//on and online. It then resets the timer.
void timedStateUpdate()
{
	if (g_upload_timer.timeout())
	{
		g_ubidots_client->sendStatus(g_status);
		g_upload_timer.reset();
	}
}