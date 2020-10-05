/*---------------------------------------------------------
 * FIFOreqchannel.cpp: Defines the methods for the 
 *   FIFORequestChannel class
 * Dean Orenstein
 * UIN: 127008117
 * CSCE 313-909
 * Date: 10/04/2020
 *--------------------------------------------------------*/

#include "common.h"
#include "FIFOreqchannel.h"
using namespace std;


FIFORequestChannel::FIFORequestChannel (const string _name, const Side _side) : RequestChannel(_name, _side) {
	s1 = "fifo_" + my_name + "1";
	s2 = "fifo_" + my_name + "2";
		
	if (_side == SERVER_SIDE) {
		wfd = open_ipc (s1, O_WRONLY);
		rfd = open_ipc (s2, O_RDONLY);
	}
	else {
		// Order of file descriptor opening matters
		rfd = open_ipc (s1, O_RDONLY);
		wfd = open_ipc (s2, O_WRONLY);
	}
}

FIFORequestChannel::~FIFORequestChannel () { 
	close (wfd);
	close (rfd);

	remove (s1.c_str ());
	remove (s2.c_str ());
}

int FIFORequestChannel::open_ipc (string _pipe_name, int mode) {
	mkfifo (_pipe_name.c_str (), 0600);
	int fd = open (_pipe_name.c_str (), mode);
	if (fd < 0) {
		cout << "Last errno is " <<  errno << "\n";
		EXITONERROR (_pipe_name);
	}
	return fd;
}

int FIFORequestChannel::cread (void* msgbuf, int bufcapacity) {
	return read (rfd, msgbuf, bufcapacity); 
}

int FIFORequestChannel::cwrite (void* msgbuf, int len) {
	return write (wfd, msgbuf, len);
}

