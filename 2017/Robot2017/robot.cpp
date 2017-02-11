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
#include "opencv/cv.h"

using namespace frc;
using namespace cv;

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

class TestRobot : public SampleRobot
{
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
	
	int server_socket;
	
public:
	void RobotInit();
	void Autonomous();
	void OperatorControl();
}

#include "dashcode.cpp"

void TestRobot::RobotInit()
{
	hardware[0] = HWMotor(6, "Intake");
	hardware[1] = HWMotor(8, "Climber");
	hardware[2] = HWMotor(0, 1, "Shifter");
	
	struct sockaddr_in server_info = {};
	server_info.sin_family = AF_INET;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	server_info.sin_port = htons(5800);
   
	server_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
}

void TestRobot::Autonomous()
{
	if(autonomous_program)
	{
		
	}
}

void HandlePackets(int server_socket)
{
	b32 has_packets = true;
    while(has_packets)
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
				char address_str[INET_ADDRSTRLEN] = {};
				inet_ntop(client_info.sin_family, &(client_info.sin_addr), address_str, INET_ADDRSTRLEN);
            
				u32 welcome_packet_size = sizeof(welcome_packet_header) +
										  sizeof(robot_hardware) * ArrayCount(hardware);
				u8 *welcome_packet = (u8 *) malloc(welcome_packet_size); 
				memset(welcome_packet, 0, welcome_packet_size);

				welcome_packet_header *welcome_header = (welcome_packet_header *) welcome_packet;
				welcome_header->header.size = welcome_packet_size;
				welcome_header->header.type = PACKET_TYPE_WELCOME;
				welcome_header->hardware_count = hardware_count;
				welcome_header->name = "ToasterOven";
				
				robot_hardware *hardware = (robot_hardware *)(welcome_header + 1);
            
				sendto(server_socket, (const char *) welcome_packet, welcome_packet_size, 0,
					   (struct sockaddr *) &client_info, client_info_size);
            
				free(welcome_packet);
			}
		}
	}
}

void TestRobot::OperatorControl()
{
	while(IsOperatorControl() && IsEnabled())
	{
		HandlePackets(server_socket);
	}
}