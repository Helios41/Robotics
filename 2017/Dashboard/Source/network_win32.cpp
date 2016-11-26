struct net_main_params
{
	volatile b32 *running;
   volatile char *connect_to;
   volatile b32 *use_mdns;
   volatile b32 *reconnect;
   volatile b32 *connect_status;
   volatile DashboardState *dashstate;
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

//TODO: functions that send packets to the connected server

DWORD NetMain(LPVOID *data)
{
	net_main_params *params = (net_main_params *) data;

	WSADATA wsa_data = {};
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
   
   Notification *network_notification = AddNotification((Console *) &params->dashstate->console, Literal("Network"));
   
   AddMessage((Console *) &params->dashstate->console,
              Literal("Starting Network Thread"), network_notification);
   
	while(*params->running)
	{
		if(*params->reconnect)
		{
         AddMessage((Console *) &params->dashstate->console,
                    /*"Attempting to connect to: "*/ Literal((char *) params->connect_to),
                    network_notification);
         
			char *host_ip = params->use_mdns ? NULL : (char *) params->connect_to;
         
         if(params->use_mdns)
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
            SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            b32 is_nonblocking = true;
            ioctlsocket(server_socket, FIONBIO, (u_long *)&is_nonblocking);

            struct sockaddr_in server;
            server.sin_family = AF_INET;
         
            server.sin_addr.s_addr = inet_addr(host_ip);
            server.sin_port = htons(8089);
         
            connect(server_socket, (struct sockaddr *)&server, sizeof(server));
         }
         
         //NOTE: we're not freeing here because we pass the connect_to string to the console
         //free((void *) params->connect_to);
         params->connect_to = NULL;
			
         //connect_status = ?;
         InterlockedExchange(params->reconnect, false);
		}
	}

	return 0;
}