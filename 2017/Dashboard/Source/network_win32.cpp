#include "packet_definitions.h"

struct net_main_params
{
	volatile b32 *running;
   volatile char *connect_to;
   volatile b32 *use_mdns;
   volatile b32 *reconnect;
   volatile b32 *connect_status;
   volatile DashboardState *dashstate;
   volatile HANDLE net_semaphore;
};

void RequestReconnect(network_settings *net_settings, net_main_params *params)
{
   if(!(*params->reconnect))
   {
      string connect_to_addr = String((char *) malloc(sizeof(char) * (net_settings->connect_to.length + 1)), net_settings->connect_to.length);
      CopyTo(net_settings->connect_to, connect_to_addr);
      connect_to_addr.text[connect_to_addr.length] = '\0';
      InterlockedExchangePointer((volatile PVOID *) &params->connect_to, connect_to_addr.text);
      InterlockedExchange(params->use_mdns, net_settings->is_mdns);
      InterlockedExchange(params->reconnect, true);
      ReleaseSemaphore(params->net_semaphore, 1, NULL);
   }
}

struct NetworkState
{
   SOCKET socket;
   struct sockaddr_in server_info;
};

void NetThread_Reconnect(NetworkState *net_state, net_main_params *params,
                         Notification *network_notification)
{  
   //TODO: make the notification system cache its messages so we dont have
   //      to worry about the lifetime of the strings we pass
   AddMessage((Console *) &params->dashstate->console,
            /*"Attempting to connect to: "*/ Literal((char *) params->connect_to),
            network_notification);
   
   char *host_ip = *params->use_mdns ? NULL : (char *) params->connect_to;
   
   if(*params->use_mdns)
   {
      hostent *robot_server = gethostbyname((char *) params->connect_to);
   
      if(robot_server)
      {
         host_ip = inet_ntoa(*((in_addr *)robot_server->h_addr));
         
         AddMessage((Console *) &params->dashstate->console,
                  /*"MDNS IP: "*/ Literal(host_ip),
                  network_notification);
      }
      else
      {
         AddMessage((Console *) &params->dashstate->console, Literal("MDNS IP address lookup FAILED"),
                  network_notification);
      }
   }
   
   AddMessage((Console *) &params->dashstate->console,
            host_ip ? /*concat "Host IP: "*/ Literal(host_ip) : Literal("Address: NULL"),
            network_notification);
   
   if(host_ip)
   {
      net_state->server_info.sin_family = AF_INET;
      net_state->server_info.sin_addr.s_addr = inet_addr(host_ip);
      net_state->server_info.sin_port = htons(5800);
      
      generic_packet_header packet = {};
      packet.size = sizeof(packet);
      packet.type = PACKET_TYPE_JOIN;
      
      sendto(net_state->socket, (const char *) &packet, sizeof(packet), 0,
             (struct sockaddr *) &net_state->server_info, sizeof(net_state->server_info));
   }
   
   //NOTE: we're not freeing here because we pass the connect_to string to the console
   //free((void *) params->connect_to);
   params->connect_to = NULL;
   
   //InterlockedExchange(params->connect_status, ?);
   InterlockedExchange(params->reconnect, false);
}

void HandlePacket(u8 *buffer)
{
   generic_packet_header *header = (generic_packet_header *) buffer;
    
   if(header->type == PACKET_TYPE_WELCOME)
   {
      /*
	  AddMessage((Console *) &params->dashstate->console,
                 Literal("Welcome Packet Recieved"), network_notification);
	  */
   }
}

void HandlePackets(NetworkState *net_state)
{
	b32 has_packets = true;
    while(has_packets)
    {
		char buffer[512] = {};
         
        struct sockaddr_in sender_info = {};
        int sender_info_size = sizeof(sender_info);
        
        int recv_return = recvfrom(net_state->socket, buffer, sizeof(buffer), 0,
                                   (struct sockaddr *) &sender_info, &sender_info_size);
        
        //TODO: check that sender_info equals net_state.server_info
        
        if(recv_return == SOCKET_ERROR)
        {
			int wsa_error = WSAGetLastError();
           
			if(wsa_error == WSAEWOULDBLOCK)
			{
				has_packets = false;
			}
			else if(wsa_error != 0)
			{
              
			}
        }
        else
        {
			HandlePacket((u8 *)buffer);
        }
	}	
}

//TODO: functions that send packets to the connected server

DWORD NetMain(LPVOID *data)
{
	net_main_params *params = (net_main_params *) data;

	WSADATA wsa_data = {};
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
   
   Notification *network_notification = AddNotification((Console *) &params->dashstate->console, Literal("Network"));
   
   AddMessage((Console *) &params->dashstate->console,
              Literal("Starting Network Thread"), network_notification);
   
   u_long non_blocking = true;
   NetworkState net_state = {};
   
   struct sockaddr_in client_info = {};
   client_info.sin_family = AF_INET;
   client_info.sin_addr.s_addr = htonl(INADDR_ANY);
   client_info.sin_port = htons(5800);
   
   net_state.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   ioctlsocket(net_state.socket, FIONBIO, &non_blocking);
	if(bind(net_state.socket, (struct sockaddr *) &client_info, sizeof(client_info)) == SOCKET_ERROR)
   {
      AddMessage((Console *) &params->dashstate->console,
              Literal("Bind Failed"), network_notification);
   }
   
   //TODO: this doesnt need to be multithreaded anymore by the looks of it, switch back to single thread
   while(*params->running)
	{
#if 0
      AddMessage((Console *) &params->dashstate->console,
                 Literal("Waiting On Semaphore"), network_notification);
      WaitForSingleObject(params->net_semaphore, INFINITE);
#endif 
      
      //TODO: packet send & recieve que
      /*
      while(work to do)
      {
         switch(work)
         {
            */
            if(*params->reconnect)
            {
               NetThread_Reconnect(&net_state, params, network_notification);
            }
            /*
         }
      }
      */

	  HandlePackets(&net_state);
	}
      

   /*
   shutdown(server_socket, SD_BOTH);
   closesocket(server_socket);
   */
   WSACleanup();
   
	return 0;
}