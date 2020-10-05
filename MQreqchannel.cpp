/*---------------------------------------------------------
 * MQreqchannel.cpp: Defines the methods for the 
 *   MQRequestChannel class
 * Dean Orenstein
 * UIN: 127008117
 * CSCE 313-909
 * Date: 10/04/2020
 *--------------------------------------------------------*/

#include "common.h"
#include "MQreqchannel.h"
#include <mqueue.h>
using namespace std;


MQRequestChannel::MQRequestChannel(const string _name, const Side _side, int _bufcap) : RequestChannel(_name, _side) {
	s1 = "/MQ_" + my_name + "1";
	s2 = "/MQ_" + my_name + "2";
		
	if (_side == SERVER_SIDE) {
		wfd = open_ipc(s1, O_RDWR|O_CREAT, _bufcap);
		rfd = open_ipc(s2, O_RDWR|O_CREAT, _bufcap);
	}
	else {
		wfd = open_ipc(s2, O_RDWR|O_CREAT, _bufcap);
		rfd = open_ipc(s1, O_RDWR|O_CREAT, _bufcap);
	}
}

MQRequestChannel::~MQRequestChannel() { 
	mq_close (wfd);
	mq_close (rfd);

	mq_unlink (s1.c_str());
	mq_unlink (s2.c_str());
}

int MQRequestChannel::open_ipc(string _pipe_name, int mode, int bufcap) {
	struct mq_attr attr {0, 1, bufcap, 0};
	int fd = (int) mq_open (_pipe_name.c_str(), O_RDWR | O_CREAT, 0600, &attr);
	if (fd < 0) {
		EXITONERROR(_pipe_name);
	}
	return fd;
}

int MQRequestChannel::cread(void* msgbuf, int bufcapacity) {
	return mq_receive (rfd, (char*) msgbuf, 8192, NULL); 
}

int MQRequestChannel::cwrite(void* msgbuf, int len) {
	return mq_send (wfd, (char*) msgbuf, len, 0);
}

