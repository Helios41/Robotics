#include <winsock2.h>
#include <Ws2tcpip.h>

struct net_main_params
{
	volatile b32 *running;
   volatile char *connect_to;
   volatile b32 *use_mdns;
   volatile b32 *reconnect;
   volatile b32 *connect_status;
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
   }
}

DWORD NetMain(LPVOID *data)
{
	net_main_params *params = (net_main_params *) data;

	WSADATA wsa_data = {};
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

	while(*params->running)
	{
		if(*params->reconnect)
		{
         //TODO: remove these message boxes, send messages to the console
         //      ***Make the console fully thread safe, notifications included***
         MessageBoxA(NULL, (char *) params->connect_to, "Address", MB_OK);
			char *host_ip = params->use_mdns ? NULL : (char *) params->connect_to;
         
         if(params->use_mdns)
         {
            hostent *robot_server = gethostbyname((char *) params->connect_to);
         
            if(robot_server)
            {
               host_ip = inet_ntoa(*((in_addr *)robot_server->h_addr));
               MessageBoxA(NULL, host_ip, "Address", MB_OK);
            }
            else
            {
               MessageBoxA(NULL, "Not Found", "Address", MB_OK);
            }
         }
         
         MessageBoxA(NULL, host_ip ? host_ip : "NULL", "Address", MB_OK);
			
         if(host_ip)
         {
            SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            b32 is_nonblocking = true;
            ioctlsocket(server_socket, FIONBIO, (u_long *)&is_nonblocking);

            struct sockaddr_in server;
            server.sin_family = AF_INET;
         
            server.sin_addr.s_addr = inet_addr(host_ip);
            server.sin_port = htons(8089);
         
            connect(server_socket, (struct sockaddr *)&server, sizeof(server));
         }
         
         free((void *) params->connect_to);
         params->connect_to = NULL;
			
         //connect_status = ?;
         InterlockedExchange(params->reconnect, false);
		}
	}

	return 0;
}