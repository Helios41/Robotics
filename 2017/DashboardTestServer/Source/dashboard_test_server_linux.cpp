#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "packet_definitions.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv/cv.h"

using namespace cv;

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
   VideoCapture cap(1);

   if(!cap.isOpened())
   {
      printf("Camera open failed\n");
      return 0;
   }

   namedWindow("visiontest", CV_WINDOW_AUTOSIZE);

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

            u32 hardware_count = 7;
            u32 function_count = 1;

            u32 welcome_packet_size = sizeof(welcome_packet_header) +
                                      sizeof(robot_hardware) * hardware_count +
                                      sizeof(robot_function) * function_count;
            u8 *welcome_packet = (u8 *) malloc(welcome_packet_size); 
            memset(welcome_packet, 0, welcome_packet_size);

            welcome_packet_header *welcome_header = (welcome_packet_header *) welcome_packet;
            welcome_header->header.size = welcome_packet_size;
            welcome_header->header.type = PACKET_TYPE_WELCOME;
            welcome_header->hardware_count = hardware_count;
            welcome_header->function_count = function_count;

            char robot_name[16] = "Robot2017";
            memcpy(welcome_header->name, robot_name, 16);
            
            robot_hardware *hardware = (robot_hardware *)(welcome_header + 1);

            char hardware0_name[16] = "Drive";
            memcpy(hardware[0].name, hardware0_name, 16);
            hardware[0].type = HARDWARE_TYPE_DRIVE;
            hardware[0].id = 0;
            
            char hardware1_name[16] = "Shooter";
            memcpy(hardware[1].name, hardware1_name, 16);
            hardware[1].type = HARDWARE_TYPE_MOTOR;
            hardware[1].id = 1;

            char hardware2_name[16] = "Indexer";
            memcpy(hardware[2].name, hardware2_name, 16);
            hardware[2].type = HARDWARE_TYPE_MOTOR;
            hardware[2].id = 2;
            
            char hardware3_name[16] = "Turntable";
            memcpy(hardware[3].name, hardware3_name, 16);
            hardware[3].type = HARDWARE_TYPE_MOTOR;
            hardware[3].id = 3;

            char hardware4_name[16] = "Shooter Camera";
            memcpy(hardware[4].name, hardware4_name, 16);
            hardware[4].type = HARDWARE_TYPE_CAMERA;
            hardware[4].id = 4;

            char hardware5_name[16] = "Shooter Base";
            memcpy(hardware[5].name, hardware5_name, 16);
            hardware[5].type = HARDWARE_TYPE_MOTOR;
            hardware[5].id = 5;

            char hardware6_name[16] = "SCamera Light";
            memcpy(hardware[6].name, hardware6_name, 16);
            hardware[6].type = HARDWARE_TYPE_LIGHT;
            hardware[6].id = 0;

            robot_function *functions = (robot_function *)(hardware + hardware_count);

            char function0_name[16] = "Invert Drive"; 
            memcpy(functions[0].name, function0_name, 16);

            printf("Sending welcome, Size = %i, Type = %i\n", ((generic_packet_header *) welcome_packet)->size,
                                                              ((generic_packet_header *) welcome_packet)->type);

            sendto(server_socket, (const char *) welcome_packet, welcome_packet_size, 0,
                   (struct sockaddr *) &client_info, client_info_size);
            
            free(welcome_packet);
         }
            
         
      }
      
      /*
      for()
      {
         
      }
      */

      Mat frame;
      if(!cap.read(frame))
      {
         printf("Missed Frame\n");
      }
      //frame = frame.reshape(0, 1);
      imshow("visiontest", frame);

      u32 frame_size = frame.total() * frame.elemSize(); 
      //printf("Frame Size = %i\n", frame_size);

      int key_code = waitKey(30) % 256;

      if(key_code == 27)
      {
         net_running = false;
      }
   }
   
   close(server_socket);
   
   return 0;
}
