#ifndef VICTRONDEVICE_HPP
#define VICTRONDEVICE_HPP
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <nlohmann/json.hpp>
#include <libusbp.hpp>
#include <iostream>
#include <iomanip>
#include <pthread.h>
#include <sys/poll.h>
#include "victronadapter.hpp"
#include "vedirect.h"

class VictronAdapter;

class VictronDevice
{
private:
        nlohmann::json conf;
	uint16_t vendor_id  = 0x0001;
	uint16_t product_id = 0x0002;
	int spfd = 0; // filedescriptor for serial device
	char read_buf[256];
	struct pollfd fds[1];
	std::string dev_path;
	VictronAdapter* adapter;
	int run_loop = 0;
public:
	pthread_t thread;
	pthread_mutex_t serial_lock;
	pthread_mutex_t main_lock;
	pthread_cond_t serial_cond;

public:
	VictronDevice(void *ptr, std::string path);
	void initializeDevice();
	int openUSBSerialPort();
	unsigned int strToInt(const char *str, int base);
	int cleanUp();
	static void *readLoop(void *context);
	int get_block (FILE * term_f, ve_direct_block_t **block_p);
	int open_serial (const char *sport);
};

#endif

