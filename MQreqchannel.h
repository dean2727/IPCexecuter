/*---------------------------------------------------------
 * MQreqchannel.h: MQRequestChannel class, inherits from
 *   RequestChannel and uses message queue methods
 * Dean Orenstein
 * UIN: 127008117
 * CSCE 313-909
 * Date: 10/04/2020
 *--------------------------------------------------------*/

#ifndef _MQreqchannel_H_
#define _MQreqchannel_H_

#include "common.h"
#include "RequestChannel.h"

class MQRequestChannel : public RequestChannel {
private:
	
public:
	MQRequestChannel(const string _name, const Side _side, int _bufcap);
	~MQRequestChannel();
	int cread (void* msgbuf, int bufcapacity);
	int cwrite (void *msgbuf , int msglen);
	int open_ipc(string name, int mode, int bufcap);
};

#endif
