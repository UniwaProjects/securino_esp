#include "UbidotsManager.h"

ubidots::UbidotsManager *ubidots::UbidotsManager::m_instance = nullptr;

ubidots::UbidotsManager *ubidots::UbidotsManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new UbidotsManager();
	}
	return m_instance;
}

ubidots::UbidotsManager::UbidotsManager()
{
	m_client = new Ubidots(account_token);
	//m_client->setDebug(true);
}

ubidots::UbidotsManager::~UbidotsManager()
{
	free(m_client);
}

//A connection attempt is made, with the given retries and intervals between them.
bool ubidots::UbidotsManager::wifiConnect(char *ssid, char *pass)
{
	bool connected = m_client->wifiConnect(ssid, pass);
	if (connected)
	{
		connected = m_client->serverConnected();
	}
	return connected;
}

//Ubidots only accepts float type values. The state will be converted
//to float before sending and to state after being received.
bool ubidots::UbidotsManager::sendStatus(alarm::Status current_status)
{
	//Return if wifi is disconnected
	if (!WiFi.isConnected())
	{
		return false;
	}
	m_client->add(state_label, (float)current_status.state);
	m_client->add(method_label, (float)current_status.method);
	m_client->add(sensor_label, (float)current_status.sensor);
	return m_client->send(device_label);
}

//Gets the variables from ubidots, they get converted to state the new state is
//returned. If there was an error in getting the variables, an error code
//is received. If that happens the old state is returned instead so that
//the program won't crash.
alarm::Status ubidots::UbidotsManager::getStatus(const alarm::Status &current_status)
{
	//Return if wifi is disconnected
	if (!WiFi.isConnected())
	{
		return current_status;
	}
	//Get values
	float state_value = m_client->get(device_label, state_label);
	float method_value = m_client->get(device_label, method_label);
	float sensor_value = m_client->get(device_label, sensor_label);
	//If the values are invalid, return
	if ((state_value == ERROR_VALUE) || (method_value == ERROR_VALUE) || (sensor_value == ERROR_VALUE))
	{
		return current_status;
	}
	//Get new values if everything is okey
	alarm::Status new_status = {alarm::state_disarmed, alarm::method_none, alarm::sensor_none_triggered};
	new_status.state = (alarm::arm_state_t)state_value;
	new_status.method = (alarm::arm_method_t)method_value;
	new_status.sensor = (alarm::sensor_state_t)sensor_value;
	return new_status;
}