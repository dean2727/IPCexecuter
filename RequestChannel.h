/*---------------------------------------------------------
 * RequestChannel.h: RequestChannel class, base class for
 *   the 3 interprocess communication classes used in this
 *   package. Defines communication attributes and abstract
 * 	 methods to be defined in said classes
 * Dean Orenstein
 * UIN: 127008117
 * CSCE 313-909
 * Date: 10/04/2020
 *--------------------------------------------------------*/

#ifndef _reqchannel_H_
#define _reqchannel_H_

#include "common.h"

class RequestChannel {
public:
	enum Side {SERVER_SIDE, CLIENT_SIDE};
	enum Mode {READ_MODE, WRITE_MODE};
	
protected:
	/*  The current implementation uses named pipes. */
	string my_name;
	Side my_side;
	
	int wfd;
	int rfd;
	
	string s1, s2;
	virtual int open_ipc(string name, int mode){;}
	
public:
	RequestChannel(const string _name, const Side _side) : my_name(_name), my_side(_side) {}
	virtual ~RequestChannel() {}

	virtual int cread (void* msgbuf, int bufcapacity) = 0;
	virtual int cwrite (void *msgbuf , int msglen) = 0;

	string name() {
        return my_name;
    }
};

#endif