/*
Handles the communication for the Ubidots IOT platform. Responsible for the upload
and download of Ubidot's variables, as well as the wifi connection. The IDs need to
match those of the corresponding Ubidots variables.
*/
#pragma once

#include "Ubidots.h"
#include "common/alarmtypes.h"

namespace ubidots
{
	class UbidotsManager
	{
	public:
		UbidotsManager(UbidotsManager const &) = delete;
		void operator=(UbidotsManager const &) = delete;
		static UbidotsManager *getInstance();
		bool sendStatus(alarm::Status current_status);
		alarm::Status getStatus(const alarm::Status &current_status);
		bool wifiConnect(char *ssid, char *pass);

	private:
		//Methods
		UbidotsManager();
		~UbidotsManager();
		//Variables
		static UbidotsManager *m_instance;
		Ubidots *m_client;
		//Ubidots variables, used for communicating with ubidots
		const char *account_token = "BBFF-P7j4JqKbWLSIpDA4u6kGjxUxJmORLf";
		const char *device_label = "securino";
		const char *state_label = "state";
		const char *method_label = "method";
		const char *sensor_label = "sensor";
	};
} // namespace ubidots
