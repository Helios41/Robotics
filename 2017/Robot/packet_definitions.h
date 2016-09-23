#ifndef PACKET_DEFINITIONS_H
#define PACKET_DEFINITIONS_H

#pragma pack(push, 1)

#define PACKET_TYPE_INVALID 0
#define PACKET_TYPE_HARDWARE_DETAILS 1
#define PACKET_TYPE_HARDWARE_UPDATE 2
#define PACKET_TYPE_WELCOME 3
#define PACKET_TYPE_SEND_AUTONOMOUS 4
#define PACKET_TYPE_DEBUG 5
#define PACKET_TYPE_GET_AUTONOMOUS_STATE 6
#define PACKET_TYPE_RETURN_AUTONOMOUS_STATE 7
#define PACKET_TYPE_GET_TIMESTAMP 8
#define PACKET_TYPE_RETURN_TIMESTAMP 9

struct generic_packet_header
{
	uint32_t packet_size;
	uint16_t packet_type;
};

#define HARDWARE_TYPE_INVALID 0
#define HARDWARE_TYPE_MOTOR_CONTROLLER 1
#define HARDWARE_TYPE_SOLENOID 2
#define HARDWARE_TYPE_FLOAT_SENSOR 3
#define HARDWARE_TYPE_BOOL_SENSOR 4
#define HARDWARE_TYPE_DRIVE 5
#define HARDWARE_TYPE_CAMERA 6

struct hardware_details_header
{
	generic_packet_header header;
	uint32_t hw_count;
};

struct hardware_component
{
	char name[16];
	uint32_t hw_id;
	uint16_t hw_type;
};

#define SOLENOID_STATE_INVALID 0
#define SOLENOID_STATE_EXTENDED 1
#define SOLENOID_STATE_RETRACTED 2
#define SOLENOID_STATE_STOPPED 3
#define SOLENOID_STATE_TOGGLE 3

struct hardware_update_header
{
	generic_packet_header header;
	uint32_t hw_id;
	uint16_t hw_type;
   uint64_t timestamp; //NOTE: call time(NULL) and cast to u64
	union
	{
		float motor_value;
		uint32_t solenoid_value;
		float float_sensor_value;
		uint32_t bool_sensor_value;

		struct
		{
			float forward_value;
			float rotate_value;
		};
	};
};

struct welcome_header
{
	generic_packet_header header;
	char name[32];
};

struct send_autonomous_header
{
	generic_packet_header header;
	uint32_t auto_operation_count;
	char name[32];
};

struct auto_operation
{
	uint32_t hw_id;
	uint16_t hw_type;
	float time;

	union
	{
		float motor_value;
		uint32_t solenoid_value;
		struct
		{
			float forward_value;
			float rotate_value;
		};
	};
};

#define DEBUG_TYPE_INVALID 0
#define DEBUG_TYPE_MESSAGE 1
#define DEBUG_TYPE_PACKET 2

struct debug_header
{
	generic_packet_header header;
	uint16_t type;
	char text[32];
};

struct get_autonomous_state_header
{
	generic_packet_header header;
};

struct return_autonomous_state_header
{
	generic_packet_header header;
	int16_t has_autonomous;
	char name[32];
};

struct get_timestamp_header
{
   generic_packet_header header;
};

struct return_timestamp_header
{
   generic_packet_header header;
   uint64_t timestamp;
};

#pragma pack(pop)

#endif