#ifndef PACKET_DEFINITIONS_H
#define PACKET_DEFINITIONS_H

//NOTE: not for drive or camera 
struct RobotHardwareSample
{
   union
   {
	   r32 motor;
	   b32 solenoid;
       b32 _switch; //switch is used in C so ¯\_(ツ)_/¯
   };
   
   r32 multiplier; //NOTE: currently only used for motor types;
   u64 timestamp; //NOTE: call time(NULL) and cast to u64
};

//NOTE: for drive, duh
struct RobotDriveSample
{
	r32 forward;
	r32 rotate;
	r32 multiplier;
	
	u64 timestamp;
};

enum RobotHardwareType
{
   Hardware_Motor = 1,
   Hardware_EncoderMotor = 2,
   Hardware_Solenoid = 3,
   Hardware_Switch = 4,
   Hardware_Camera = 5
};

enum FunctionBlockType 
{
   FunctionBlock_Wait = 1,
   FunctionBlock_SetFloat = 2, //NOTE: this is for motor types
   FunctionBlock_SetBool = 3,
   FunctionBlock_SetMultiplier = 4,  //NOTE: this is for motor types
   FunctionBlock_Vision = 5,
   FunctionBlock_ArcadeDrive = 6,
   FunctionBlock_SetDriveMultiplier = 7
};

enum BooleanOperation
{
	BooleanOp_True = 1,
	BooleanOp_False = 2,
	BooleanOp_Not = 3
};

struct FunctionBlock
{
   FunctionBlockType type;
   
   union
   {
	   struct
	   {
		   r32 duration;
	   } wait;
	   
	   struct
	   {
		   u32 hardware_index;
		   r32 value;
	   } set_float;
	   
	   struct
	   {
		   u32 hardware_index;
		   BooleanOperation op;
	   } set_bool;
	   
	   struct
	   {
		   u32 hardware_index;
		   r32 value;
	   } set_multiplier;
	   
	   struct
	   {
		   u32 index;
	   } vision;
	   
	   struct
	   {
		   r32 power;
		   r32 rotate;
	   } arcade_drive;
	   
	   struct
	   {
		   r32 value;
	   } set_drive_multiplier;
   };
};

enum ControlType
{
	Control_ButtonUp = 1,
	Control_ButtonDown = 2,
	Control_Axis = 3
};

#pragma pack(push, 1)

#define PACKET_TYPE_INVALID 0

#define PACKET_TYPE_JOIN 1 //client -> server
#define PACKET_TYPE_WELCOME 2 //server -> client
#define PACKET_TYPE_HARDWARE_SAMPLE 3 //server -> client
#define PACKET_TYPE_DRIVE_SAMPLE 4 //server -> client

#define PACKET_TYPE_UPLOAD_AUTONOMOUS 5 //client -> server
#define PACKET_TYPE_UPLOAD_CONTROLS 6 //client -> server
#define PACKET_TYPE_UPLOAD_VISION_CONFIG 7 //client -> server

#define PACKET_TYPE_REQUEST_UPLOADED_STATE 8 //client -> server
#define PACKET_TYPE_UPLOADED_STATE 9 //server -> client 

#define PACKET_TYPE_CAMERA_FEED 10 //server -> client 

#define PACKET_TYPE_DEBUG_MESSAGE 11 //server -> client

#define PACKET_TYPE_SET_FLOAT 12 //client -> server
#define PACKET_TYPE_SET_MULTIPLIER 13 //client -> server

#define PACKET_TYPE_PING 14

struct generic_packet_header
{
   //TODO: maybe switch this back to u32 cuz our video packets may be huge
   //NOTE: bandwith is limited to 7Mbit/sec
	u16 size;
	u8 type;
};

//NOTE: PACKET_TYPE_JOIN uses generic_packet_header, no additional information

//NOTE: PACKET_TYPE_WELCOME
struct welcome_packet_header
{
   generic_packet_header header;
   
   char name[16];
   u8 hardware_count;
   
   b32 drive_encoder;
};

struct robot_hardware
{
   char name[16];
   u8 type; //Cast to RobotHardwareType
};

//NOTE: PACKET_TYPE_HARDWARE_SAMPLE
struct hardware_sample_packet_header
{
   generic_packet_header header;
   u8 index;
   
   RobotHardwareSample sample;
};

//NOTE: PACKET_TYPE_UPLOAD_AUTONOMOUS
struct upload_autonomous_packet_header
{
   generic_packet_header header;
   
   char name[32];
   u8 block_count;
};

//NOTE: PACKET_TYPE_UPLOAD_CONTROLS
struct upload_controls_packet_header
{
   generic_packet_header header;
   u8 driver; //NOTE: if true controls for driver, if false controls for operator
   
   u8 control_count;
};

struct control_config
{
	u8 type; //Cast to ControlType
	u8 button_or_axis;
	u8 block_count;
};

//NOTE: PACKET_TYPE_UPLOAD_VISION_CONFIG
//WIP

//NOTE: PACKET_TYPE_REQUEST_UPLOADED_STATE uses generic_packet_header, no additional information

//NOTE: PACKET_TYPE_UPLOADED_STATE
struct uploaded_state_packet_header
{
   generic_packet_header header;
   
   u8 has_autonomous;
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

//NOTE: PACKET_TYPE_SET_FLOAT
struct set_float_packet_header
{
	generic_packet_header header;

	u32 index;
	r32 value;
};

//NOTE: PACKET_TYPE_SET_MULTIPLIER
struct set_multiplier_packet_header
{
	generic_packet_header header;

	u32 index;
	r32 multiplier;
};

//NOTE: PACKET_TYPE_PING uses generic_packet_header, no additional information

#pragma pack(pop)

#endif
