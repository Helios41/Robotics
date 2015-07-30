#include "Network.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <pthread.h>

#define Kilobyte(BYTE) BYTE * 1024
#define Megabyte(BYTE) Kilobyte(BYTE) * 1024
#define Gigabyte(BYTE) Megabyte(BYTE) * 1024
#define Terabyte(BYTE) Gigabyte(BYTE) * 1024

#define RECIVE_BUFFER_SIZE Kilobyte(1)

struct NetworkInfo
{
   s32 socketID;
   u8 reciveBuffer[RECIVE_BUFFER_SIZE];
};

b32 initialized = false;
const char[] ServerIP = "10.46.18.2";
pthread_t network_thread = {0};
NetworkInfo net_thread_info = {0}; 

volatile b32 network_thread_running = false;

void *network_thread_func(void *param)
{
   NetworkInfo *net_info = (NetworkInfo *) param;
   
   while(network_thread_running)
   {
      
      sleep(1); //make this less (http://stackoverflow.com/questions/1157209/is-there-an-alternative-sleep-function-in-c-to-milliseconds)
   }
   
   return NULL;
}

/*
Problem:
   First makes you set a static IP,
   both the laptop and robot are set to the same IP
*/

namespace Network
{
   b32 InitNetwork()
   {
      if(!initialized)
      {
         s32 socketID = 0;
         struct sockaddr_in server_address = {0};
         
         if((socketID = socket(AF_INET, SOCK_STREAM, 0)) < 0)
         {
            //TODO: log error to driver station
            return false;
         }
         
         server_address.sin_family = AF_INET;
         server_address.sin_port = htons(Network::Port);
      
         if(inet_pton(AF_INET, ServerIP, &server, &server_address.sin_addr) <= 0)
         {
            //TODO: log error to driver station
            return false;
         }
      
         if(connect(socketID, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
         {
            //TODO: log error to driver station
            return false;
         }
      
         net_thread_info.socketID = socketID;
      
         __sync_lock_test_and_set(&network_thread_running, true);
      
         if(pthread_create(&network_thread, NULL, network_thread_func, &net_thread_info))
         {
            //TODO: log error to driver station
            return false;
         }
      
         initialized = true;
      }
      return true;
   }
   
   void StopNetwork(void)
   {
      __sync_lock_test_and_set(&network_thread_running, false);
   }
}
