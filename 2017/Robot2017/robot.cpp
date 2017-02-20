#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include "stdint.h"

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

#include "packet_definitions.h"
#include "WPILib.h"

using namespace frc;

struct ErrorFunction
{
	int avg_span;
	float coeff;
	float dcoeff;

	float target;
	float acc;
	int inc;
	float avg;
	float speed;
};

ErrorFunction EFunction(int avg_span, float coeff, float dcoeff)
{
	ErrorFunction result = {};

	result.avg_span = avg_span;
	result.coeff = coeff;
	result.dcoeff = dcoeff;

	return result;
}

float Square(float x)
{
	return x * x;
}

float Absolute(float x)
{
	return (x < 0.0f) ? -x : x;
}

float Clamp(float min, float value, float max)
{
	float result = value;

	if(result > max)
		result = max;

	if(result < min)
		result = min;

	return result;
}

void Update(ErrorFunction *function, float rate)
{
	function->inc++;
	function->acc += rate;

	if(function->inc >= function->avg_span)
	{
		function->avg = function->acc / (float)function->inc;

		float diff = (function->target - function->avg);
		float c = function->coeff * Square(function->dcoeff * diff);
		function->speed += c * diff;
		function->speed = Clamp(-1, function->speed, 1);

		function->acc = 0.0f;
		function->inc = 0;
	}
}

struct RobotHardware
{
	RobotHardwareType type;
	union
	{
		struct
		{
			ErrorFunction error_function;
			Encoder *encoder;
			Victor *motor;
		} encoder_motor;
		
		Victor *motor;
		DoubleSolenoid *solenoid;
		DigitalInput *_switch;
	};
	
	char name[16];
	r32 multiplier;
};

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

struct ButtonState
{
	b32 up;
	b32 down;
};

struct network_state
{
	int socket;
	struct sockaddr_in client_info;
	socklen_t client_info_size;
};

class TestRobot : public SampleRobot
{
public:
	Joystick driver_controller { 0 };
	Joystick op_controller { 0 };
	
	RobotHardware hardware[3] = {};
	
	Victor right_front { 0 };
	Victor right_back { 1 };
	Encoder right_encoder {0, 1};
	Victor left_front { 2 };
	Victor left_back { 3 };
	Encoder left_encoder {2, 3};
	r32 drive_multiplier = 1.0f;
	
	char auto_name[32] = {};
	u32 auto_length = 0;
	FunctionBlock *auto_program = NULL;
	
	network_state net_state = {};
	
	void RobotInit();
	void Autonomous();
	void OperatorControl();
};

#include <dashcode.h>

void TestRobot::RobotInit()
{
	hardware[0] = HWMotor(6, "Intake");
	hardware[1] = HWMotor(8, "Climber");
	hardware[2] = HWSolenoid(0, 1, "Shifter");
	
	struct sockaddr_in server_info = {};
	server_info.sin_family = AF_INET;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	server_info.sin_port = htons(5800);
   
	net_state.socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

	bind(net_state.socket, (struct sockaddr *) &server_info, sizeof(server_info));
}

void TestRobot::Autonomous()
{
	if(auto_program)
	{
		
	}
}

void SendWelcomePacket(network_state *net_state, RobotHardware *hardware, u32 hardware_count)
{
	u32 welcome_packet_size = sizeof(welcome_packet_header) +
							  sizeof(robot_hardware) * hardware_count;
	u8 *welcome_packet = (u8 *) malloc(welcome_packet_size);
	memset(welcome_packet, 0, welcome_packet_size);

	welcome_packet_header *welcome_header = (welcome_packet_header *) welcome_packet;
	welcome_header->header.size = welcome_packet_size;
	welcome_header->header.type = PACKET_TYPE_WELCOME;
	welcome_header->hardware_count = hardware_count;
	welcome_header->drive_encoder = false;
	strcpy(welcome_header->name, "ToasterOven");

	robot_hardware *send_hardware = (robot_hardware *)(welcome_header + 1);

	for(u32 i = 0;
		i < hardware_count;
		i++)
	{
		strcpy(send_hardware[0].name, hardware[0].name);
		send_hardware[0].type = (u8) hardware[0].type;
	}

	sendto(net_state->socket, (const char *) welcome_packet, welcome_packet_size, 0,
		   (struct sockaddr *) &net_state->client_info, net_state->client_info_size);

	free(welcome_packet);
}

void HandlePackets(network_state *net_state, TestRobot *robot)
{
	b32 has_packets = true;
    while(has_packets)
	{
		struct sockaddr_in client_info = {};
		socklen_t client_info_size = sizeof(client_info);
      
		char buffer[512] = {};
      
		int recv_return = recvfrom(net_state->socket, buffer, sizeof(buffer), 0,
                                   (struct sockaddr *) &client_info, &client_info_size);
      
		if(recv_return == -1)
		{
			int recv_error = errno;
			if((recv_error != EWOULDBLOCK) &&
			   (recv_error != EAGAIN))
			{
				has_packets = false;
			}
		}
		else
		{
			generic_packet_header *header = (generic_packet_header *) buffer;
         
			if(header->type == PACKET_TYPE_JOIN)
			{
				net_state->client_info = client_info;
				net_state->client_info_size = client_info_size;

				SendWelcomePacket(net_state, robot->hardware, ArrayCount(robot->hardware));
			}
			else if(header->type == PACKET_TYPE_SET_FLOAT)
			{
				set_float_packet_header *set_float = (set_float_packet_header *) header;

				FunctionBlock set_float_function = {};
				set_float_function.type = FunctionBlock_SetFloat;
				set_float_function.set_float.value = set_float->value;
				set_float_function.set_float.hardware_index = set_float->index;
				ExecuteBlocklangFunction(&set_float_function, robot);
			}
			else if(header->type == PACKET_TYPE_SET_MULTIPLIER)
			{
				set_multiplier_packet_header *set_multiplier = (set_multiplier_packet_header *) header;

				FunctionBlock set_multiplier_function = {};
				set_multiplier_function.type = FunctionBlock_SetMultiplier;
				set_multiplier_function.set_multiplier.value = set_multiplier->multiplier;
				set_multiplier_function.set_multiplier.hardware_index = set_multiplier->index;
				ExecuteBlocklangFunction(&set_multiplier_function, robot);
			}
		}
	}
}

void TestRobot::OperatorControl()
{
	ButtonState buttons = {};


	while(IsOperatorControl() && IsEnabled())
	{
		buttons.up = buttons.up && !driver_controller.GetRawButton(2);
		buttons.down = driver_controller.GetRawButton(2);

		FunctionBlock drive = {};
		drive.type = FunctionBlock_ArcadeDrive;
		drive.arcade_drive.power = driver_controller.GetRawAxis(1);
		drive.arcade_drive.rotate = driver_controller.GetRawAxis(4);
		ExecuteBlocklangFunction(&drive, this);

		FunctionBlock intake = {};
		intake.type = FunctionBlock_SetFloat;
		intake.set_float.value = op_controller.GetRawButton(3) ? 1.0f : (op_controller.GetRawButton(2) ? -1.0f : 0.0f);
		intake.set_float.hardware_index = 0;
		ExecuteBlocklangFunction(&intake, this);

		FunctionBlock climber = {};
		climber.type = FunctionBlock_SetFloat;
		climber.set_float.value = op_controller.GetRawAxis(1);
		climber.set_float.hardware_index = 1;
		ExecuteBlocklangFunction(&climber, this);

		if(buttons/*[2]*/.up)
		{
			FunctionBlock ball_shifter = {};
			ball_shifter.type = FunctionBlock_SetBool;
			ball_shifter.set_float.value = BooleanOp_Not;
			ball_shifter.set_float.hardware_index = 2;
			ExecuteBlocklangFunction(&ball_shifter, this);
		}

		HandlePackets(&net_state, this);
		Wait(0.05f);
	}
}

START_ROBOT_CLASS(TestRobot);
