#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>

#include "packet_definitions.h"

#include <stdio.h>

#include <stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;
typedef float r32;
typedef double r64;
typedef uint32_t b32;

int main()
{
   WSADATA wsa_data = {};
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
   
   struct sockaddr_in server_info = {};
   server_info.sin_family = AF_INET;
   server_info.sin_addr.s_addr = htonl(INADDR_ANY);
   server_info.sin_port = htons(5800);
   
   u_long non_blocking = true;
   
   SOCKET server_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   ioctlsocket(server_socket, FIONBIO, &non_blocking);
   if(bind(server_socket, (struct sockaddr *) &server_info, sizeof(server_info)) == SOCKET_ERROR)
   {
      printf("Bind Failed\n");
   }
   
   b32 net_running = true;
   
   {
      char address_str[INET_ADDRSTRLEN] = {};
      inet_ntop(server_info.sin_family, &(server_info.sin_addr), address_str, INET_ADDRSTRLEN);
      printf("At %s\n", address_str);
   }
   
   while(net_running)
   {
      struct sockaddr_in client_info = {};
      int client_info_size = sizeof(client_info);
      
      char buffer[512] = {};
      
      int recv_return = recvfrom(server_socket, buffer, sizeof(buffer), 0,
                                 (struct sockaddr *) &client_info, &client_info_size);
      
      if(recv_return == SOCKET_ERROR)
      {
         int wsa_error = WSAGetLastError();
         
         if((wsa_error != WSAEWOULDBLOCK) &&
            (wsa_error != 0))
         {
            printf("Error\n");
         }
      }
      else
      {
         //PROCESS PACKET
         printf("Data recieved, Size = %u\n", recv_return);
         generic_packet_header *header = (generic_packet_header *) buffer;
         
         if(header->type == PACKET_TYPE_JOIN)
         {
            char address_str[INET_ADDRSTRLEN] = {};
            inet_ntop(client_info.sin_family, &(client_info.sin_addr), address_str, INET_ADDRSTRLEN);
            
            //Add address to list
            printf("Join Request From %s\n", address_str);
            
            generic_packet_header welcome_packet = {};
            welcome_packet.size = sizeof(welcome_packet);
            welcome_packet.type = PACKET_TYPE_WELCOME;
            
            sendto(server_socket, (const char *) &welcome_packet, sizeof(welcome_packet), 0,
                   (struct sockaddr *) &client_info, client_info_size);
         }
            
         
      }
      
      /*
      for()
      {
         
      }
      */
   }
   
   closesocket(server_socket);
   WSACleanup();
   
   return 0;
}