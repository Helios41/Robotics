#ifndef NETWORK_H_
#define NETWORK_H_

#include <string>
#include "inetLib.h"

class Network
{
private:
	static Network* Instance;

	struct sockaddr_in serverAddress;
	char Display;
	int SockAddrSize;
	int SocketFileDecs;
	int MessageLen;
	
	Network();
	
	void Push(std::string data);
	
public:
	static void PushData();
	static Network* GetInstance();
};

#endif