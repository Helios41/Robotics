#include "Network.h"

Network* Network::Instance = NULL;

Network::Network()
{
	SockAddrSize = sizeof(struct sockaddr_in);
	
}

void Network::Push(std::string data)
{

}

Network* Network::GetInstance()
{
	if(Instance == NULL)
		Instance = new Network();
	
	return Instance;
}

void Network::PushData()
{

}