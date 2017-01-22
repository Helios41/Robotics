#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

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
   struct sockaddr_in server_info = {};
   server_info.sin_family = AF_INET;
   server_info.sin_addr.s_addr = htonl(INADDR_ANY);
   server_info.sin_port = htons(5800);
   
   int server_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
   
   if(bind(server_socket, (struct sockaddr *) &server_info, sizeof(server_info)) == -1)
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
      socklen_t client_info_size = sizeof(client_info);
      
      char buffer[512] = {};
      
      int recv_return = recvfrom(server_socket, buffer, sizeof(buffer), 0,
                                 (struct sockaddr *) &client_info, &client_info_size);
      
      if(recv_return == -1)
      {
         int recv_error = errno;
         
         if((recv_error != EWOULDBLOCK) &&
            (recv_error != EAGAIN) &&
            (recv_error != 0))
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
   
   close(server_socket);
   
   return 0;
}
