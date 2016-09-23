#include <winsock2.h>
#include <Ws2tcpip.h>

struct net_main_params
{
	volatile b32 *running;
   volatile b32 *reconnect;
};

DWORD NetMain(LPVOID *data)
{
	net_main_params *params = (net_main_params *) data;

	WSADATA wsa_data = {};
	WSAStartup(MAKEWORD(2, 2), &wsa_data);

#if 0
	while(*params->running)
	{
		if(*params->reconnect)
		{
			hostent *robot_server = gethostbyname("roborio-4618-frc.local");

			if (robot_server)
			{
				char *address = inet_ntoa(*((in_addr *)robot_server->h_addr));
				MessageBoxA(NULL, address, "Address", MB_OK);

			}
			else
			{
				MessageBoxA(NULL, "Not Found", "Address", MB_OK);
			}

			InterlockedCompareExchange(params->reconnect, false, true);
		}
	}
#endif

	return 0;
}