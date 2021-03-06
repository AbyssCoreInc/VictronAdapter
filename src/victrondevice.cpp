// Vittutttuuu
#include "victrondevice.hpp"

VictronDevice::VictronDevice(void *ptr, std::string path) 
{
	this->dev_path = path;
	this->adapter = (VictronAdapter*)ptr;
}

void VictronDevice::initializeDevice()
{
	this->run_loop = 1;
}

void *VictronDevice::readLoop(void *context)
{
	int serial_state;
	FILE *term_f;
	int term_fd;
	ve_direct_block_t *block;
	term_fd = ((VictronDevice*)context)->open_serial (((VictronDevice*)context)->dev_path.c_str());
	term_f = fdopen (term_fd, "r");
	while(1)
	{
		if (!(serial_state = ((VictronDevice*)context)->get_block (term_f, &block)))
		{
			if (block->device_info == NULL)
			{
				// fprintf(stderr, "Warning: skipping block w/o device_info :-/\n");
				ve_direct_free_block(block);
				continue;
			}
			//ve_direct_print_block(block);
			((VictronDevice*)context)->adapter->sendData(block);
		}
	}
	return NULL;
}

int VictronDevice::cleanUp()
{
        close(this->spfd);
        pthread_cancel(this->thread);
        return 0;
}

int VictronDevice::get_block (FILE * term_f, ve_direct_block_t **block_p)
{
	//TODO add custom parameters to log
	char block_buf[1025];
	int l=0;
	int fd;
	int a;
	fd_set set;
	int to;
	struct timeval timeout;
	size_t len;

	ve_direct_block_t *b;

	fd = fileno (term_f);
	FD_ZERO (&set);
	FD_SET (fd, &set);
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	while ((a = getc (term_f)) != EOF)
	{
		to = select (FD_SETSIZE, &set, NULL, NULL, &timeout);
		len = 0;
		ioctl (fd, FIONREAD, &len);
		if (to == 0) {
			fprintf (stderr,"Read timeout, please check serial input...\n");
			return 1;
		}
		if (this->run_loop == 0)
			return 1;
		if (len == 0) {
			fprintf (stderr,"Device disconnected, retrying in 10 sec...\n");
			return 2;
		}
		if (l > 1024) {
			fprintf (stderr, "Error: Exhausted block read buffer!\n");
			return 1;
		}
		block_buf[l]=a;
		/* skip hex protocol frames */
		if (block_buf[0] == ':' && block_buf[l] == '\n')
		{
			l = 0;
			continue;
		}
		if (l < 9)
		{
			l++;
			continue;
		}
		block_buf[l+1]='\0'; // add null terminator after last byte in buffer
		// test for end of block
		if (strstr(&block_buf[l-9], "Checksum") == &block_buf[l-9])
		{
			l = 0;
			b = ve_direct_parse_block(&block_buf[0]);
			if (b == NULL)
				return 1;
			*block_p = b;
			return 0;
		}
		l++;
	}

	fprintf (stderr, "Error: got EOF from serial stream!\n");
	return 1;
}

int VictronDevice::open_serial (const char *sport)
{
	struct termios uio;
	int term_fd;
	memset (&uio, 0, sizeof (uio));
	cfsetispeed (&uio, B19200);
	term_fd = open (sport, O_RDONLY);
	uio.c_lflag = ICANON;
	uio.c_oflag &= ~OPOST;
	uio.c_cflag &= ~PARENB;
	uio.c_cflag &= ~CSIZE;
	uio.c_cflag |= CS8 | CREAD | CLOCAL;
	uio.c_cc[VMIN] = 0;
	uio.c_cc[VTIME] = 5;
	uio.c_iflag &= ~(IXON | IXOFF | IXANY);
	if (term_fd == -1)
	{
		perror ("Error opening serial port");
		return 1;
	}
	tcsetattr (term_fd, TCSANOW, &uio);
	tcflush (term_fd, TCIOFLUSH);
	return term_fd;
}
