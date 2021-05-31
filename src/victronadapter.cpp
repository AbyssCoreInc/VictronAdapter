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
	
	//HTTP Headers
	std::string header = std::string("Fiware-Service: ");
	header.append(this->conf["fiware_service"]);
	this->headers.push_back(header.c_str());
	header = "Fiware-ServicePath: ";
	header.append(this->conf["fiware_servicepath"]);
	this->headers.push_back(header.c_str());
	this->headers.push_back("Content-Type: application/json");
	this->headers.push_back("Cache-Control: no-cache");

}

void VictronAdapter::sendData(ve_direct_block_t *block)
{
	//std::string str = sentence.substr (0,sentence.length());
	std::cout<<"sendSentence: "<<block->device_info->name<<std::endl;

	this->convertToJSON(block);
	//std::cout<<"sendSentence json: "<<jsonsent<<std::endl;
	//this->sendMQTTPacket(jsonsent);
}

void VictronAdapter::convertToJSON(ve_direct_block_t *b)
{
	nlohmann::json ret;
	nlohmann::json values;
	ve_direct_fields_t  *fields_p;
	if (b->device_info->type == ve_direct_device_type_mppt)
	{
		std::cout<<"parse MPPT device"<<std::endl;
		std::ifstream file(this->conf["model_path"].get<std::string>().append("/mppt.json"));
		ret = json::parse(file);

		for (fields_p=b->fields; fields_p!=NULL; fields_p=fields_p->next) {
			//printf("\t\t%s:\t", fields_p->name);
			if (strcmp(fields_p->name,"SER#")==0)
			{
				//printf("%s\n", (char *)fields_p->value);
				ret["devices"][0]["device_id"] = (char *)fields_p->value;
			}
			else if (strcmp(fields_p->name,"PID")==0)
			{
				//printf("0x%0X\n", *(int *)fields_p->value);
				for(auto &array : ret["devices"][0]["static_attributes"])
				{
					std::string field = array["name"]; 
					//std::cout<<"field:"<<field<<std::endl;
					if (strcmp(field.c_str(),"DevicePID")==0) {
						char temp[8];
						sprintf(temp,"0x%0X", *(int *)fields_p->value);
						//std::cout<<"temp:"<<temp<<std::endl;
						array["value"] = std::string(temp);
					}
					if (strcmp(field.c_str(),"DeviceName")==0) {
						array["value"] = std::string(b->device_info->name);
					}
				}
			}
		}
		//std::cout<<"hep"<<std::endl;
		int exist = 0;
		std::string devid = ret["devices"][0]["device_id"];
		std::string tempdev;
		for (int i = 0; i < this->mpptDevices.size(); i++)
		{
			tempdev = this->mpptDevices.at(i)["devices"][0]["device_id"];
			if (tempdev.compare(devid)==0)
				exist = 1;
		}
		if (exist)
		{
			for (fields_p=b->fields; fields_p!=NULL; fields_p=fields_p->next) {
				// make values json
				if (strcmp(fields_p->name,"V")==0)
				{
					//printf("%i\n", *(int *)fields_p->value);
					values["V"]= *(int *)fields_p->value;
				}
				else if (strcmp(fields_p->name,"I")==0)
				{
					//printf("%i\n", *(int *)fields_p->value);
					values["I"]= *(int *)fields_p->value;
				}
				else if (strcmp(fields_p->name,"VPV")==0)
				{
					//printf("%i\n", *(int *)fields_p->value);
					values["VPV"]= *(int *)fields_p->value;
				}
				else if (strcmp(fields_p->name,"PPV")==0)
				{
					//printf("%i\n", *(int *)fields_p->value);
					values["PPV"]= *(int *)fields_p->value;
				}
				else if (strcmp(fields_p->name,"IL")==0)
				{
					//printf("%i\n", *(int *)fields_p->value);
					values["IL"]= *(int *)fields_p->value;
				}
			}
			// update data
			this->updateData(ret["devices"][0]["device_id"], values);
		}
		else
		{
			this->mpptDevices.push_back(ret);
			this->createDevice(ret);
		}
		std::cout<<"parsed MPPT device fields"<<std::endl;

	}
	//std::cout<<ret<<std::endl;
}

void VictronAdapter::createDevice(nlohmann::json dev)
{
	std::cout<<"createDevice:"<<dev<<std::endl;
	// curlpp stuff
	curlpp::Cleanup myCleanup;
	curlpp::Easy request;
	std::string body = dev.dump();
	std::string uri = this->conf["agent_url"];
	request.setOpt(new curlpp::options::Url(uri));
	request.setOpt(new curlpp::options::Verbose(true));
	request.setOpt(new curlpp::options::HttpHeader(headers));
	request.setOpt(new curlpp::options::PostFields(body));
	request.setOpt(new curlpp::options::PostFieldSize(body.length()));
	request.perform();

}
void VictronAdapter::updateData(std::string id, nlohmann::json dev)
{
	std::cout<<"updateData id:"<<id<<"="<<dev<<std::endl;
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
	return devices;
}

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
