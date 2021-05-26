#include "mqtt-client.hpp"
#define PUBLISH_TOPIC "EXAMPLE_TOPIC"
#define DEBUG
#ifdef DEBUG
#include <iostream>
#endif

mqtt_client::mqtt_client(const char *id, char *host, int port) : mosquittopp(id)
{
	int keepalive = DEFAULT_KEEP_ALIVE;
	std::cout<<"MQTT Client connect to: "<<host<<":"<<port<<std::endl;
	mosqpp::lib_init();
	this->host = host;
	this->port = port;
	//connect(host, port, keepalive);
	connect_async(this->host, this->port, keepalive);
//	if (loop_start() != MOSQ_ERR_SUCCESS) {
//		std::cout << "loop_start failed" << std::endl;
//	}
}

mqtt_client::~mqtt_client()
{
	disconnect();
	loop_stop();
	mosqpp::lib_cleanup();
}

void mqtt_client::on_connect(int rc)
{
    if (!rc)
    {
        #ifdef DEBUG
            std::cout << "Connected - code " << rc << std::endl;
        #endif
    }
}

void mqtt_client::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
    #ifdef DEBUG
        std::cout << "Subscription succeeded." << std::endl;
    #endif
}

void mqtt_client::on_publish(int mid) {
	//disconnect();
	std::cout << "Published message with id: " << mid << std::endl;
}

void mqtt_client::on_message(const struct mosquitto_message *message)
{
	int payload_size = MAX_PAYLOAD + 1;
	char buf[payload_size];

	#ifdef DEBUG
	std::cout << "on_message: " << buf <<std::endl;
	#endif
	if(!strcmp(message->topic, PUBLISH_TOPIC))
	{
		memset(buf, 0, payload_size * sizeof(char));

		/* Copy N-1 bytes to ensure always 0 terminated. */
		memcpy(buf, message->payload, MAX_PAYLOAD * sizeof(char));

        #ifdef DEBUG
            std::cout << buf << std::endl;
        #endif

		// Examples of messages for M2M communications...
		if (!strcmp(buf, "STATUS"))
        {
            snprintf(buf, payload_size, "This is a Status Message...");
            publish(NULL, PUBLISH_TOPIC, strlen(buf), buf);
            #ifdef DEBUG
                std::cout << "Status Request Recieved." << std::endl;
            #endif
		}
		if (!strcmp(buf, "ON"))
		{
			snprintf(buf, payload_size, "Turning on...");
			publish(NULL, PUBLISH_TOPIC, strlen(buf), buf);
			#ifdef DEBUG
			std::cout << "Request to turn on." << std::endl;
			#endif
		}

		if (!strcmp(buf, "OFF"))
		{
			snprintf(buf, payload_size, "Turning off...");
			publish(NULL, PUBLISH_TOPIC, strlen(buf), buf);
			#ifdef DEBUG
			std::cout << "Request to turn off." << std:: endl;
			#endif
		}
	}
}

void mqtt_client::publish_sensor_data(std::string topic, std::string payload)
{
	//connect(this->host, this->port, DEFAULT_KEEP_ALIVE);
	/* Publish the message */
	int ret = publish(NULL,topic.c_str(),payload.length(),payload.c_str(),1,false);
	std::cout<<"mqtt message published:"<<ret<<std::endl;
}
