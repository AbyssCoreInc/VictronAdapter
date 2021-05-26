#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mosquittopp.h>
#include <cstring>
#include <cstdio>
#include <iostream>

#define MAX_PAYLOAD 50
#define DEFAULT_KEEP_ALIVE 0

class mqtt_client : public mosqpp::mosquittopp
{
private: 
	char *host;
	int  port;
public:
	mqtt_client (const char *id, char *host, int port);
	~mqtt_client();

	void on_connect(int rc);
	void on_message(const struct mosquitto_message *message);
	void on_subscribe(int mid, int qos_count, const int *granted_qos);
	void on_publish(int mid);
	void publish_sensor_data(std::string topic, std::string payload);
};

#endif
