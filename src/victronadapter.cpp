#include <fstream>
#include <algorithm>
#include "victronadapter.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <csignal>
#include "mqtt-client.hpp"
#define NUM_THREADS 2

using json = nlohmann::json;


VictronAdapter::VictronAdapter(std::string path)
{
	this->mqtt_clientid = "1";
	std::cout<<" Victron VE.Direct Fiware Adapther \n";
	this->readConfigFile(path);
	// initialize read buffer
	memset(&this->read_buf, '\0', sizeof(this->read_buf));
	this->iot_client = new mqtt_client(this->mqtt_clientid.c_str(), const_cast<char*>(this->mqtt_host.c_str()), this->strToInt(this->mqtt_port.c_str(),10));
	std::cout<<"Victron Adapter ready"<<std::endl;

	//Mutex
	if (pthread_mutex_init(&this->main_lock, NULL) != 0)
	{
		std::cout<<"main mutex init failed"<<std::endl;
	}
	pthread_mutex_lock(&this->main_lock);

	// Polling stuff
	//this->fds[0].fd = this->spfd;
	//this->fds[0].events = POLLIN;
}

void VictronAdapter::initAdapter()
{
	std::vector<libusbp::device> devs = this->findUSBDevices(this->vendor_id, this->product_id);
	std::cout<<"number of interfaces "<<devs.size()<<std::endl;
	for (int i = 0; i < devs.size(); i++)
	{
		libusbp::serial_port port(devs[i]);
	        std::string port_name = port.get_name();
		std::cout<<"\t"<<port_name<<std::endl;
		this->vdevs.push_back(new VictronDevice(this, port_name));
	}
	// initialize objects
	for (int i = 0; i < devs.size(); i++)
	{
		this->vdevs[i]->initializeDevice();
	}

	// Create device thread per each port
	for (int i = 0; i < devs.size(); i++)
	{
		pthread_create(&(this->vdevs[i]->thread), NULL, &vdevs[i]->readLoop, (void *)this->vdevs[i]);
	}
}

void VictronAdapter::sendData(std::string sentence)
{
	std::string str = sentence.substr (0,sentence.length());
	std::cout<<"sendSentence: "<<str<<std::endl;
	//nlohmann::json jsonsent = this->interpreter->convertToJSON(str);
	//std::cout<<"sendSentence json: "<<jsonsent<<std::endl;
	//this->sendMQTTPacket(jsonsent);
}

unsigned int VictronAdapter::strToInt(const char *str, int base)
{
    const char digits[] = "0123456789abcdef";
    unsigned int result = 0;

    while(*str)
    {
        result *= base;
        result += strchr(digits, *str++) - digits;
    }
    return result;
}
/*
std::string VictronAdapter::readSentence()
{
	memset(&this->read_buf, '\0', sizeof(this->read_buf));
	int pollrc = poll( this->fds, 1, 5000);
	if (pollrc < 0)
	{
		std::cout<<"readSentence pollrc error: "<<pollrc<<std::endl;
    		//perror("poll");
	}
	else if( pollrc > 0)
	{
    		if( this->fds[0].revents & POLLIN )
		{
			ssize_t rc = read(this->spfd, &this->read_buf, sizeof(this->read_buf) );
			if (rc > 0)
			{
				// You've got rc characters. do something with buff 
				std::string ret(this->read_buf,sizeof(this->read_buf));
				ret.erase(std::remove_if(ret.begin(), ret.end(), ::isspace), ret.end());
				return ret;
			}
		}
	}
	else
	{
		std::cout<<"readSentence timeout "<<std::endl;
	}
	return NULL;
}
*/
std::vector<libusbp::device> VictronAdapter::findUSBDevices(uint16_t vendor_id, uint16_t product_id)
{
	std::vector<libusbp::device> devices;
	std::vector<libusbp::device> fullist = libusbp::list_connected_devices();

	for(size_t i = 0; i < fullist.size(); i++)
 	{
		libusbp::device candidate = fullist[i];
		if (fullist[i].get_vendor_id() == vendor_id && fullist[i].get_product_id() == product_id)
		{
			devices.push_back(fullist[i]);
		}
	}
	//delete(fullist);
	return devices;
}
/*
int VictronAdapter::openUSBSerialPort()
{
	struct termios options;

	libusbp::serial_port port(device);
	std::string port_name = port.get_name();
	std::cout << port_name << std::endl;

	this->spfd = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK | O_ASYNC);
	if (this->spfd == -1)
	{
		// Could not open the port. 
		perror("open_port: Unable to open /dev/ttyf1 - ");
		return 2;
	}
	fcntl(this->spfd, F_SETFL, 0);

	tcgetattr(this->spfd, &options);
	cfsetispeed(&options, B4800); // 4800 is the baudrate of NMEA0193
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 5;
	tcsetattr(this->spfd, TCSANOW, &options);
	return 0;
}
*/
int VictronAdapter::readConfigFile(std::string path)
{

	std::ifstream file(path);
	std::cout<<" Parse configuration file "<<path<<"\n";
	this->conf = json::parse(file);
	std::cout<<this->conf<<"\n";

	std::string s_vid = this->conf["vendor_id"];
	std::string s_pid = this->conf["product_id"];
	std::cout<<"vendor_id: "<<s_vid<<std::endl;
	std::cout<<"product_id: "<<s_pid<<std::endl;

	this->mqtt_host = this->conf["mqtt_host"];
	this->mqtt_port = this->conf["mqtt_port"];
	this->vendor_id = this->strToInt(s_vid.c_str(),16);
	this->product_id = this->strToInt(s_pid.c_str(),16);

	return 0;
}

void VictronAdapter::sendMQTTPacket(nlohmann::json data)
{
	std::string topic = "/";
	topic.append(this->conf["api_key"]);
	topic.append("/");
	topic.append(data["sentence"]);
	topic.append("/attrs");
	std::cout<<"topic: "<<topic<<std::endl;
	data.erase("sentence");
	data.erase("source"); // WTF I shoudl do with this field is there a situation that there might be same sentence from two different devices?! 
	std::cout<<"data: "<<data<<std::endl;
	this->iot_client->publish_sensor_data(topic, data.dump());
}

/**
* This is global for cleanup function
*/
VictronAdapter* adapter;

void signalHandler( int signum ) {
	std::cout << "Interrupt signal (" << signum << ") received.\n";
	delete adapter;
	exit(signum);  
}

int main(int argc, char const *argv[])
{
	std::string conffile = "/etc/victronadapter.conf";
	signal(SIGINT, signalHandler);
	// read in the json file
	if (argc > 1)
	{
		std::cout<<"argv[1] "<<argv[1]<<" argv[1] "<<argv[2];
		if (strcmp(argv[1],"-c")==0)
			conffile = argv[2];
	}

	adapter = new VictronAdapter(conffile);
	adapter->initAdapter();
	// Create Thread for serial reader
	//int rc = pthread_create(&adapter->serial_thread, NULL, &NMEA_Adapter::readLoop, (void *) adapter);

	nlohmann::json temp;
	pthread_cond_t cond; 
	pthread_cond_init(&cond, NULL);
	pthread_mutex_lock(&adapter->main_lock);
	pthread_cond_wait(&cond, &adapter->main_lock);

	adapter->cleanUp();
	return 0;
}


int VictronAdapter::cleanUp()
{
	pthread_mutex_destroy(&this->serial_lock);
	pthread_mutex_unlock(&this->main_lock);
	pthread_mutex_destroy(&this->main_lock);
	pthread_cancel(this->serial_thread);
	return 0;
}
