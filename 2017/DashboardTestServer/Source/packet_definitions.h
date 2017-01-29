#ifndef PACKET_DEFINITIONS_H
#define PACKET_DEFINITIONS_H

#include "stdint.h"

#pragma pack(push, 1)

#define PACKET_TYPE_INVALID 0

#define PACKET_TYPE_JOIN 1 //client -> server
#define PACKET_TYPE_WELCOME 2 //server -> client
#define PACKET_TYPE_HARDWARE_SAMPLE 3 //server -> client

#define PACKET_TYPE_UPLOAD_AUTONOMOUS 4 //client -> server
#define PACKET_TYPE_UPLOAD_CONTROLS 5 //client -> server
#define PACKET_TYPE_UPLOAD_VISION_CONFIG 6 //client -> server

#define PACKET_TYPE_REQUEST_UPLOADED_STATE 7 //client -> server
#define PACKET_TYPE_UPLOADED_STATE 8 //server -> client 

#define PACKET_TYPE_CAMERA_FEED 9 //server -> client 

#define PACKET_TYPE_DEBUG_MESSAGE 10 //server -> client

#define PACKET_TYPE_PING 11

struct generic_packet_header
{
   //TODO: maybe switch this back to u32 cuz our video packets may be huge
   //NOTE: bandwith is limited to 7Mbit/sec
	uint16_t size;
	uint8_t type;
};

//NOTE: PACKET_TYPE_JOIN uses generic_packet_header, no additional information

//NOTE: PACKET_TYPE_WELCOME
struct welcome_packet_header
{
   generic_packet_header header;
   
   char name[16];
   uint8_t hardware_count;
   uint8_t function_count;
};

#define HARDWARE_TYPE_INVALID 0
#define HARDWARE_TYPE_MOTOR 1
#define HARDWARE_TYPE_SOLENOID 2
#define HARDWARE_TYPE_DRIVE 3
#define HARDWARE_TYPE_SWITCH 4
#define HARDWARE_TYPE_CAMERA 5  
#define HARDWARE_TYPE_DISTANCE_SENSOR 6
#define HARDWARE_TYPE_LIGHT 7

struct robot_hardware
{
   //TODO: dynamic size on these
   char name[16];
	uint8_t id;
	uint8_t type;
};

struct robot_function
{
   char name[16];
	uint8_t id;
};

//NOTE: PACKET_TYPE_HARDWARE_SAMPLE
struct hardware_sample_packet_header
{
   generic_packet_header header;
	
   uint8_t id;
   uint8_t type;
   
   uint64_t timestamp; //NOTE: call time(NULL) and cast to u64
   
   union
   {
      float motor;
	  uint32_t solenoid;
	  struct
	  {
	     float forward;
		 float rotate;
	  };
	  uint32_t _switch;
      float distance_sensor;
	  uint32_t light;
	};
};

//NOTE: PACKET_TYPE_UPLOAD_AUTONOMOUS
struct upload_autonomous_packet_header
{
   generic_packet_header header;
   
   char name[32];
   uint8_t block_count;
};

#define VALUE_BLOCK_INVALID 0
#define VALUE_BLOCK_CONSTANT_UINT 1
#define VALUE_BLOCK_CONSTANT_BOOL 2
#define VALUE_BLOCK_CONSTANT_FLOAT 3
#define VALUE_BLOCK_CONSTANT_VEC2 4
#define VALUE_BLOCK_CONTROLLER_BOOL 5
#define VALUE_BLOCK_CONTROLLER_FLOAT 6
#define VALUE_BLOCK_CONTROLLER_VEC2 7
#define VALUE_BLOCK_SENSOR_BOOL 8
#define VALUE_BLOCK_SENSOR_FLOAT 9

struct value_block
{
   uint8_t type;
   
   union
   {
      uint32_t uint_param;
      uint32_t bool_param;
      float float_param;
      struct
      {
         float vec2_param_x;
         float vec2_param_y;
      };
      struct 
      {
         uint32_t id;
         uint32_t button_or_axis;
      } controller;
      struct 
      {
         uint32_t id;
         uint32_t x_axis;
         uint32_t y_axis;
      } twoaxis_controller;
      uint32_t sensor_id;
   };
};

#define FUNCTION_BLOCK_INVALID 0
#define FUNCTION_BLOCK_WAIT 1
#define FUNCTION_BLOCK_SET 2
#define FUNCTION_BLOCK_BUILTIN 3
#define FUNCTION_BLOCK_VISION 4
#define FUNCTION_BLOCK_GOTO 5

struct function_block
{
   uint8_t type;
   uint8_t id; //NOTE: unused for wait and maybe soon goto 
   value_block param;
};

//NOTE: PACKET_TYPE_UPLOAD_CONTROLS
//WIP

//NOTE: PACKET_TYPE_UPLOAD_VISION_CONFIG
//WIP

//NOTE: PACKET_TYPE_REQUEST_UPLOADED_STATE uses generic_packet_header, no additional information

//NOTE: PACKET_TYPE_UPLOADED_STATE
struct uploaded_state_packet_header
{
   generic_packet_header header;
   
   uint8_t has_autonomous;
   char autonomous_name[32];
   
   //TODO: data currently on the robot for controls
   //eg. allows us to outline the buttons the robot is currently polling
   
   //TODO: data currently on the robot for vision
};

//NOTE: PACKET_TYPE_CAMERA_FEED
//WIP

//NOTE: PACKET_TYPE_DEBUG_MESSAGE
struct debug_message_packet_header
{
	generic_packet_header header;
   
	char text[32];
};

//NOTE: PACKET_TYPE_PING uses generic_packet_header, no additional information

#pragma pack(pop)

#endif
