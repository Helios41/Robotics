#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

#include <chrono>

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

float Sign(float x)
{
	return (x < 0.0f) ? -1 : 1;
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
		
		struct
		{
			union
			{
				Victor *victor;
				Spark *spark;
			};
			b32 is_victor;
		} motor;

		DoubleSolenoid *solenoid;
		DigitalInput *_switch;
		AnalogInput *potentiometer;
	};
	
	char name[16];
	r32 multiplier;
	RobotHardwareSample last_sample;
};

b32 HitTarget(ErrorFunction efunction, r32 threshold)
{
	return Absolute(efunction.avg - efunction.target) < threshold;
}

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
	Joystick op_controller { 1 };
	
	RobotHardware hardware[11] = {};
	
	Victor right_front { 0 };
	Victor right_back { 1 };
	Encoder right_encoder {0, 1};
	Victor left_front { 2 };
	Victor left_back { 3 };
	Encoder left_encoder {2, 3};
	r32 drive_multiplier = 1.0f;
	
	RobotDriveSample last_drive_sample;

	u32 current_auto_slot = 0;

	char *auto_name[4] = {(char *) malloc(sizeof(char) * 32), (char *) malloc(sizeof(char) * 32), (char *) malloc(sizeof(char) * 32), (char *) malloc(sizeof(char) * 32)};
	u32 auto_length[4] = {0, 0, 0, 0};
	FunctionBlock *auto_program[4] = {NULL, NULL, NULL, NULL};
	
	network_state net_state = {};

	void RobotInit();
	void Disabled();
	void Autonomous();
	void OperatorControl();
};

void SendDebugMessagePacket(network_state *net_state, const char *text, b32 auto_debug);

#include <dashcode.h>

void DisableAllHardware(TestRobot *robot)
{
	for(u32 i = 0;
		i < ArrayCount(robot->hardware);
		i++)
	{
		if((robot->hardware[i].type == Hardware_Motor) ||
		   (robot->hardware[i].type == Hardware_EncoderMotor))
		{
			FunctionBlock reset_motor = {};
			reset_motor.type = FunctionBlock_SetFloatConst;
			reset_motor.set_float_const.value = 0.0f;
			reset_motor.set_float_const.hardware_index = i;
			ExecuteBlocklangFunction(reset_motor, robot);

			FunctionBlock reset_multiplier = {};
			reset_multiplier.type = FunctionBlock_SetMultiplier;
			reset_multiplier.set_multiplier.value = 1.0f;
			reset_multiplier.set_multiplier.hardware_index = i;
			ExecuteBlocklangFunction(reset_multiplier, robot);
		}
		else if(robot->hardware[i].type == Hardware_Solenoid)
		{
			FunctionBlock reset_solenoid = {};
			reset_solenoid.type = FunctionBlock_SetBool;
			reset_solenoid.set_bool.op = BooleanOp_False;
			reset_solenoid.set_bool.hardware_index = i;
			ExecuteBlocklangFunction(reset_solenoid, robot);
		}
	}

	FunctionBlock reset_drive = {};
	reset_drive.type = FunctionBlock_ArcadeDriveConst;
	reset_drive.arcade_drive_const.power = 0.0f;
	reset_drive.arcade_drive_const.rotate = 0.0f;
	ExecuteBlocklangFunction(reset_drive, robot);

	FunctionBlock reset_drive_multiplier = {};
	reset_drive_multiplier.type = FunctionBlock_SetDriveMultiplier;
	reset_drive_multiplier.set_drive_multiplier.value = 1.0f;
	ExecuteBlocklangFunction(reset_drive_multiplier, robot);
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
	welcome_header->drive_encoder = false; //TODO: change this to true when we get drive encoders working
	strcpy(welcome_header->name, "ToasterOven");

	robot_hardware *send_hardware = (robot_hardware *)(welcome_header + 1);

	for(u32 i = 0;
		i < hardware_count;
		i++)
	{
		strcpy(send_hardware[i].name, hardware[i].name);
		send_hardware[i].type = (u8) hardware[i].type;
	}

	sendto(net_state->socket, (const char *) welcome_packet, welcome_packet_size, 0,
		   (struct sockaddr *) &net_state->client_info, net_state->client_info_size);

	free(welcome_packet);
}

void SendDebugMessagePacket(network_state *net_state, const char *text, b32 auto_debug)
{
	debug_message_packet_header debug_message_header = {};
	debug_message_header.header.size = sizeof(debug_message_header);
	debug_message_header.header.type = PACKET_TYPE_DEBUG_MESSAGE;
	strncpy(debug_message_header.text, text, strlen(text));
	debug_message_header.is_autonomous_debug = auto_debug;

	sendto(net_state->socket, (const char *) &debug_message_header, sizeof(debug_message_header), 0,
		   (struct sockaddr *) &net_state->client_info, net_state->client_info_size);
}

void SendHardwareSample(network_state *net_state, u32 index, RobotHardwareSample sample)
{
	hardware_sample_packet_header hardware_packet_header = {};
	hardware_packet_header.header.size = sizeof(hardware_packet_header);
	hardware_packet_header.header.type = PACKET_TYPE_HARDWARE_SAMPLE;
	hardware_packet_header.index = index;
	hardware_packet_header.sample = sample;

	sendto(net_state->socket, (const char *) &hardware_packet_header , sizeof(hardware_packet_header), 0,
		   (struct sockaddr *) &net_state->client_info, net_state->client_info_size);
}

void SendDriveSample(network_state *net_state, RobotDriveSample sample)
{
	drive_sample_packet_header drive_packet_header = {};
	drive_packet_header.header.size = sizeof(drive_packet_header);
	drive_packet_header.header.type = PACKET_TYPE_DRIVE_SAMPLE;
	drive_packet_header.sample = sample;

	sendto(net_state->socket, (const char *) &drive_packet_header, sizeof(drive_packet_header), 0,
		   (struct sockaddr *) &net_state->client_info, net_state->client_info_size);
}

void SendUploadedState(network_state *net_state, TestRobot *robot)
{
	uploaded_state_packet_header uploaded_state_packet = {};
	uploaded_state_packet.header.size = sizeof(uploaded_state_packet);
	uploaded_state_packet.header.type = PACKET_TYPE_UPLOADED_STATE;

	strcpy(uploaded_state_packet.autonomous_name, robot->auto_name[robot->current_auto_slot]);
	uploaded_state_packet.has_autonomous = (robot->auto_length[robot->current_auto_slot] > 0);

	sendto(net_state->socket, (const char *) &uploaded_state_packet, sizeof(uploaded_state_packet), 0,
		   (struct sockaddr *) &net_state->client_info, net_state->client_info_size);
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
			if((recv_error == EWOULDBLOCK) ||
			   (recv_error == EAGAIN))
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
				set_float_function.type = FunctionBlock_SetFloatConst;
				set_float_function.set_float_const.value = set_float->value;
				set_float_function.set_float_const.hardware_index = set_float->index;
				ExecuteBlocklangFunction(set_float_function, robot);
			}
			else if(header->type == PACKET_TYPE_SET_MULTIPLIER)
			{
				set_multiplier_packet_header *set_multiplier = (set_multiplier_packet_header *) header;

				FunctionBlock set_multiplier_function = {};
				set_multiplier_function.type = FunctionBlock_SetMultiplier;
				set_multiplier_function.set_multiplier.value = set_multiplier->multiplier;
				set_multiplier_function.set_multiplier.hardware_index = set_multiplier->index;
				ExecuteBlocklangFunction(set_multiplier_function, robot);
			}
			else if(header->type == PACKET_TYPE_UPLOAD_AUTONOMOUS)
			{
				SendDebugMessagePacket(net_state, "Recieved Autonomous", true);

				upload_autonomous_packet_header *upload_autonomous = (upload_autonomous_packet_header *) header;
				FunctionBlock *blocks = (FunctionBlock *)(upload_autonomous + 1);

				strncpy(robot->auto_name[upload_autonomous->slot], upload_autonomous->name, strlen(upload_autonomous->name));
				robot->auto_length[upload_autonomous->slot] = upload_autonomous->block_count;
				free(robot->auto_program[upload_autonomous->slot]);
				robot->auto_program[upload_autonomous->slot] = (FunctionBlock *) malloc(sizeof(FunctionBlock) * robot->auto_length[upload_autonomous->slot]);

				for(u32 i = 0;
					i < robot->auto_length[upload_autonomous->slot];
					i++)
				{
					robot->auto_program[upload_autonomous->slot][i] = blocks[i];
				}

				SendUploadedState(net_state, robot);

				SaveAutoFile(robot, "/home/lvuser/auto0.rabin2", 0);
				SaveAutoFile(robot, "/home/lvuser/auto1.rabin2", 1);
				SaveAutoFile(robot, "/home/lvuser/auto2.rabin2", 2);
				SaveAutoFile(robot, "/home/lvuser/auto3.rabin2", 3);
			}
			else if(header->type == PACKET_TYPE_REQUEST_UPLOADED_STATE)
			{
				SendDebugMessagePacket(net_state, "Request State", false);
				SendUploadedState(net_state, robot);
			}
		}
	}
}

void SendSamples(network_state *net_state, TestRobot *robot, b32 force_send = false)
{
	for(u32 i = 0;
		i < ArrayCount(robot->hardware);
		i++)
	{
		RobotHardware *hardware = robot->hardware + i;

		RobotHardwareSample sample = {};
		sample.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		bool send_sample = false;

		switch(hardware->type)
		{
			case Hardware_Motor:
			{
				sample.multiplier = hardware->multiplier;
				sample.motor = hardware->motor.is_victor ? hardware->motor.victor->Get() : hardware->motor.spark->Get();

				send_sample = (Absolute(sample.motor - hardware->last_sample.motor) > 0.01f) ||
							  (Absolute(sample.multiplier - hardware->last_sample.multiplier) > 0.01f);
			}
			break;

			case Hardware_EncoderMotor:
			{
				sample.multiplier = hardware->multiplier;
				sample.motor = hardware->encoder_motor.encoder->GetRate();

				send_sample = (Absolute(sample.motor - hardware->last_sample.motor) > 0.01f) ||
							  (Absolute(sample.multiplier - hardware->last_sample.multiplier) > 0.01f);
			}
			break;

			case Hardware_Solenoid:
			{
				sample.solenoid = (hardware->solenoid->Get() == DoubleSolenoid::Value::kForward) ? true : false;
				send_sample = (sample.solenoid != hardware->last_sample.solenoid);
			}
			break;

			case Hardware_Switch:
			{
				sample._switch = hardware->_switch->Get();
				send_sample = (sample._switch != hardware->last_sample._switch);
			}
			break;

			case Hardware_Potentiometer:
			{
				sample.potentiometer = hardware->potentiometer->GetValue();
				send_sample = (Absolute(sample.potentiometer - hardware->last_sample.potentiometer) > 5);
			}
			break;
		}

		if(send_sample || force_send)
		{
			SendHardwareSample(net_state, i, sample);
			hardware->last_sample = sample;
		}
	}

	RobotDriveSample drive_sample = {};
	drive_sample.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	drive_sample.multiplier = robot->drive_multiplier;
	drive_sample.left = robot->left_encoder.GetRate();
	drive_sample.right = robot->right_encoder.GetRate();

	b32 send_drive_sample = (Absolute(drive_sample.multiplier - robot->last_drive_sample.multiplier) > 0.01f) ||
							(Absolute(drive_sample.left - robot->last_drive_sample.left) > 0.01f) ||
							(Absolute(drive_sample.right - robot->last_drive_sample.right) > 0.01f) ;

	if(send_drive_sample || force_send)
	{
		SendDriveSample(net_state, drive_sample);
		robot->last_drive_sample = drive_sample;
	}
}

void UpdateErrorFunction(TestRobot *robot)
{
	for(u32 i = 0;
		i < ArrayCount(robot->hardware);
		i++)
	{
		if(robot->hardware[i].type == Hardware_EncoderMotor)
		{
			r32 rate = robot->hardware[i].encoder_motor.encoder->GetRate();

			Update(&robot->hardware[i].encoder_motor.error_function, rate);

			SmartDashboard::PutNumber("Rate " + std::string(robot->hardware[i].name), rate);
			SmartDashboard::PutNumber("Speed " + std::string(robot->hardware[i].name), robot->hardware[i].encoder_motor.error_function.speed);
			SmartDashboard::PutNumber("Target " + std::string(robot->hardware[i].name), robot->hardware[i].encoder_motor.error_function.target);
			SmartDashboard::PutNumber("Average " + std::string(robot->hardware[i].name), robot->hardware[i].encoder_motor.error_function.avg);

			robot->hardware[i].encoder_motor.error_function.speed = Clamp(-0.83, robot->hardware[i].encoder_motor.error_function.speed, 0);

			if(Absolute(robot->hardware[i].encoder_motor.error_function.target) < 0.1)
			{
				robot->hardware[i].encoder_motor.motor->Set(0);
			}
			else
			{
				robot->hardware[i].encoder_motor.motor->Set(robot->hardware[i].encoder_motor.error_function.speed);
			}
		}
	}
}

#define HW_TURNTABLE 0
#define HW_SHOOTER 1
#define HW_INTAKE 2
#define HW_INDEXER 3
#define HW_CLIMBER 4
#define HW_SHOOTER_SWIVEL 5
#define HW_BALL_SHIFTER 6
#define HW_PUNCH 7
#define HW_GEAR_HOLDER 8
#define HW_POT 9
#define HW_HOPPER_FLAP 10

void TestRobot::RobotInit()
{
	hardware[HW_TURNTABLE] = HWMotor_Victor(4, (char *)"Turntable");
	hardware[HW_SHOOTER] = HWMotor_EncoderVictor(5, 4, 5, EFunction(10, 0.001, 0.0000088), (char *)"Shooter");
	hardware[HW_INTAKE] = HWMotor_Victor(6, (char *)"Intake");
	hardware[HW_INDEXER] = HWMotor_Victor(7, (char *)"Indexer");
	hardware[HW_CLIMBER] = HWMotor_Victor(8, (char *)"Climber");
	hardware[HW_SHOOTER_SWIVEL] = HWMotor_Spark(9, (char *)"Shooter Swivel");
	hardware[HW_BALL_SHIFTER] = HWSolenoid(0, 1, (char *)"Ball Shifter");
	hardware[HW_PUNCH] = HWSolenoid(6, 7, (char *)"Punch");
	hardware[HW_GEAR_HOLDER] = HWSolenoid(2, 3, (char *)"Gear Holder");
	hardware[HW_POT] = HWPotentiometer(0, (char *)"Swivel Pot");
	hardware[HW_HOPPER_FLAP] = HWSolenoid(4, 5, (char *)"Hopper Flap");

	struct sockaddr_in server_info = {};
	server_info.sin_family = AF_INET;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	server_info.sin_port = htons(5800);

	net_state.socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);

	{
		int flags = fcntl(net_state.socket, F_GETFL);
		fcntl(net_state.socket, F_SETFL, flags | O_NONBLOCK);
	}

	bind(net_state.socket, (struct sockaddr *) &server_info, sizeof(server_info));

	LoadAutoFile(this, "/home/lvuser/auto0.rabin2", 0);
	LoadAutoFile(this, "/home/lvuser/auto1.rabin2", 1);
	LoadAutoFile(this, "/home/lvuser/auto2.rabin2", 2);
	LoadAutoFile(this, "/home/lvuser/auto3.rabin2", 3);

	//(new Compressor(0))->Stop();
}

void TestRobot::Disabled()
{
	SendDebugMessagePacket(&net_state, "Disabled", false);

	LoadAutoFile(this, "/home/lvuser/auto0.rabin2", 0);
	LoadAutoFile(this, "/home/lvuser/auto1.rabin2", 1);
	LoadAutoFile(this, "/home/lvuser/auto2.rabin2", 2);
	LoadAutoFile(this, "/home/lvuser/auto3.rabin2", 3);

	while(!IsEnabled())
	{
		u32 prev_auto_slot = current_auto_slot;

		if(op_controller.GetRawButton(6))
		{
			current_auto_slot = 0;
		}
		else if(op_controller.GetRawButton(7))
		{
			current_auto_slot = 1;
		}
		else if(op_controller.GetRawButton(11))
		{
			current_auto_slot = 2;
		}
		else if(op_controller.GetRawButton(10))
		{
			current_auto_slot = 3;
		}

		if(prev_auto_slot != current_auto_slot)
		{
			SendUploadedState(&net_state, this);
		}

		HandlePackets(&net_state, this);
		SendSamples(&net_state, this);
	}
}

void TestRobot::Autonomous()
{
	DisableAllHardware(this);

#if 1
	b32 auto_rt_setup = (auto_program[current_auto_slot] != NULL);
	blocklang_runtime auto_rt = SetupRuntime(auto_program[current_auto_slot], auto_length[current_auto_slot]);

	b32 flip_turns = (DriverStation::GetInstance().GetAlliance() != DriverStation::Alliance::kRed);

	SendDebugMessagePacket(&net_state, auto_rt_setup ? "Start Auto" : "No Auto Run", true);

 	while(IsAutonomous() && IsEnabled())
 	{
 		if(auto_rt_setup)
 		{
 			ExecuteBlocklangRuntime(&auto_rt, this, flip_turns);

 			SmartDashboard::PutNumber("Auto RT Wait Time", auto_rt.wait_time);
 			SmartDashboard::PutNumber("Auto RT Timer", auto_rt.timer);
 			SmartDashboard::PutNumber("Auto IPointer", auto_rt.curr_function);
 		}

 		HandlePackets(&net_state, this);
 		SendSamples(&net_state, this);
 		UpdateErrorFunction(this);

 		Wait(0.05f);
 	}
#else

	FunctionBlock test_auto[11] = {};

	test_auto[0].type = FunctionBlock_DriveDistance;
	test_auto[0].drive_distance.left_distance = 6.2;
	test_auto[0].drive_distance.right_distance = 6.2;

	test_auto[1].type = FunctionBlock_SetBool;
	test_auto[1].set_bool.hardware_index = HW_PUNCH;
	test_auto[1].set_bool.op = BooleanOp_True;

	test_auto[2].type = FunctionBlock_Wait;
	test_auto[2].wait.duration = 0.05;

	test_auto[3].type = FunctionBlock_SetBool;
	test_auto[3].set_bool.hardware_index = HW_PUNCH;
	test_auto[3].set_bool.op = BooleanOp_False;

	test_auto[4].type = FunctionBlock_Wait;
	test_auto[4].wait.duration = 0.1;

	test_auto[5].type = FunctionBlock_ArcadeDriveConst;
	test_auto[5].arcade_drive_const.power = -0.7;
	test_auto[5].arcade_drive_const.rotate = 0;

	test_auto[6].type = FunctionBlock_Wait;
	test_auto[6].wait.duration = 0.75;

	test_auto[7].type = FunctionBlock_ArcadeDriveConst;
	test_auto[7].arcade_drive_const.power = 0;
	test_auto[7].arcade_drive_const.rotate = -1;

	test_auto[8].type = FunctionBlock_Wait;
	test_auto[8].wait.duration = 0.55;

	test_auto[9].type = FunctionBlock_ArcadeDriveConst;
	test_auto[9].arcade_drive_const.power = -1;
	test_auto[9].arcade_drive_const.rotate = 0;

	test_auto[10].type = FunctionBlock_Wait;
	test_auto[10].wait.duration = 0.6;

	blocklang_runtime auto_rt = SetupRuntime(test_auto, ArrayCount(test_auto));

	while(IsAutonomous() && IsEnabled() && IsRunning(&auto_rt))
	{
		ExecuteBlocklangRuntime(&auto_rt, this);

		SmartDashboard::PutNumber("Auto RT Wait Time", auto_rt.wait_time);
		SmartDashboard::PutNumber("Auto RT Timer", auto_rt.timer);
		SmartDashboard::PutNumber("Auto IPointer", auto_rt.curr_function);

		Wait(0.05f);
	}
#endif

	/*
	left_encoder.Reset();
	right_encoder.Reset();

	while(IsAutonomous() && IsEnabled())
	{
		r32 left_distance = left_encoder.Get() / 1173.0;
		r32 right_distance = right_encoder.Get() / 1173.0;

		b32 left_remaining = (left_distance + 6.2) > 0.01;
		b32 right_remaining = (right_distance + 6.2) > 0.01;

		r32 speed = (left_remaining || right_remaining) ? 0.35 : 0;

		SmartDashboard::PutBoolean("Left Remaining", left_remaining);
		SmartDashboard::PutBoolean("Right Remaining", right_remaining);

		FunctionBlock distance_drive = {};
		distance_drive.type = FunctionBlock_ArcadeDriveConst;
		distance_drive.arcade_drive_const.power = speed / 2.0;
		distance_drive.arcade_drive_const.rotate = 0.0f;
		ExecuteBlocklangFunction(distance_drive, this);

		SmartDashboard::PutNumber("Encoder Test Speed", speed);
		SmartDashboard::PutNumber("Left Distance", left_distance);
		SmartDashboard::PutNumber("Right Distance", right_distance);


		SmartDashboard::PutBoolean("Rate Stopped", (Absolute(left_encoder.GetRate()) < 10) || (Absolute(right_encoder.GetRate()) < 10));
		SmartDashboard::PutBoolean("Moved", (left_distance > 2) && (right_distance > 2));

		if((!left_remaining && !right_remaining) || not_moving)
		{
			break;
		}
	}

	FunctionBlock punch_out = {};
	punch_out.type = FunctionBlock_SetBool;
	punch_out.set_bool.hardware_index = HW_PUNCH;
	punch_out.set_bool.op = BooleanOp_True;
	ExecuteBlocklangFunction(punch_out, this);

	Wait(0.05);

	FunctionBlock punch_in = {};
	punch_in.type = FunctionBlock_SetBool;
	punch_in.set_bool.hardware_index = HW_PUNCH;
	punch_in.set_bool.op = BooleanOp_False;
	ExecuteBlocklangFunction(punch_in, this);

	Wait(0.1);

	FunctionBlock drive = {};
	drive.type = FunctionBlock_ArcadeDriveConst;
	drive.arcade_drive_const.power = -1;
	drive.arcade_drive_const.rotate = 0;
	ExecuteBlocklangFunction(drive, this);

	Wait(0.75);

	FunctionBlock turn = {};
	turn.type = FunctionBlock_ArcadeDriveConst;
	turn.arcade_drive_const.power = 0;
	turn.arcade_drive_const.rotate = -1;
	ExecuteBlocklangFunction(turn, this);

	Wait(0.55);

	FunctionBlock forwards = {};
	forwards.type = FunctionBlock_ArcadeDriveConst;
	forwards.arcade_drive_const.power = -1;
	forwards.arcade_drive_const.rotate = 0;
	ExecuteBlocklangFunction(forwards, this);

	Wait(0.6);
	 */

	DisableAllHardware(this);
}

#define D_BUTTON_A 0
#define D_BUTTON_B 1
#define D_BUTTON_X 2
#define D_BUTTON_Y 3
#define D_BUTTON_LBUMP 4
#define D_BUTTON_RBUMP 5
#define D_BUTTON_BACK 6
#define D_BUTTON_START 7

#define OP_BUTTON_TRIGGER 0
#define OP_BUTTON_2 1
#define OP_BUTTON_3 2
#define OP_BUTTON_4 3
#define OP_BUTTON_5 4
#define OP_BUTTON_6 5
#define OP_BUTTON_7 6
#define OP_BUTTON_8 7
#define OP_BUTTON_9 8
#define OP_BUTTON_10 9
#define OP_BUTTON_11 10

#define Min(a, b) (((a) > (b)) ? (b) : (a))

void TestRobot::OperatorControl()
{
	ButtonState driver_buttons[8] = {};
	ButtonState op_buttons[11] = {};

	DisableAllHardware(this);
	SendSamples(&net_state, this, true);

	b32 shooter_ready = false;

	hardware[HW_SHOOTER].encoder_motor.error_function.speed = -0.65;

	SendDebugMessagePacket(&net_state, "Start Teleop", false);

	while(IsOperatorControl() && IsEnabled())
	{
		for(u32 i = 0;
			i < ArrayCount(driver_buttons);
			i++)
		{
			driver_buttons[i].up = driver_buttons[i].down && !driver_controller.GetRawButton(i + 1);
			driver_buttons[i].down = driver_controller.GetRawButton(i + 1);
		}

		for(u32 i = 0;
			i < ArrayCount(op_buttons);
			i++)
		{
			op_buttons[i].up = op_buttons[i].down && !op_controller.GetRawButton(i + 1);
			op_buttons[i].down = op_controller.GetRawButton(i + 1);
		}

		FunctionBlock drive = {};
		drive.type = FunctionBlock_ArcadeDriveController;
		drive.arcade_drive_controller.power_axis_index = 1;
		drive.arcade_drive_controller.rotate_axis_index = 4;
		ExecuteBlocklangFunction(drive, this);

		FunctionBlock intake = {};
		intake.type = FunctionBlock_SetFloatConst;
		intake.set_float_const.value = op_buttons[OP_BUTTON_3].down ? 1.0f : (op_buttons[OP_BUTTON_2].down ? -1.0f : 0.0f);
		intake.set_float_const.hardware_index = HW_INTAKE;
		ExecuteBlocklangFunction(intake, this);

		FunctionBlock climber = {};
		climber.type = FunctionBlock_SetFloatController;
		climber.set_float_controller.axis_index = 1;
		climber.set_float_controller.is_op = true;
		climber.set_float_controller.hardware_index = HW_CLIMBER;
		ExecuteBlocklangFunction(climber, this);

		FunctionBlock ball_shifter = {};
		ball_shifter.type = FunctionBlock_SetBool;
		ball_shifter.set_bool.op = driver_buttons[D_BUTTON_LBUMP].down ? BooleanOp_True : BooleanOp_False;
		ball_shifter.set_bool.hardware_index = HW_BALL_SHIFTER;
		ExecuteBlocklangFunction(ball_shifter, this);

		FunctionBlock gear_punch = {};
		gear_punch.type = FunctionBlock_SetBool;
		gear_punch.set_bool.op = driver_buttons[D_BUTTON_Y].down ? BooleanOp_True : BooleanOp_False;
		gear_punch.set_bool.hardware_index = HW_PUNCH;
		ExecuteBlocklangFunction(gear_punch, this);

		if(driver_buttons[D_BUTTON_X].up)
		{
			FunctionBlock gear_holder = {};
			gear_holder.type = FunctionBlock_SetBool;
			gear_holder.set_bool.op = BooleanOp_Not;
			gear_holder.set_bool.hardware_index = HW_GEAR_HOLDER;
			ExecuteBlocklangFunction(gear_holder, this);
		}

		r32 spot1 = -59414;
		r32 spot2 = -67000;

		SmartDashboard::PutNumber("Spot 1", spot1);
		SmartDashboard::PutNumber("Spot 2", spot2);

		FunctionBlock shooter = {};
		shooter.type = FunctionBlock_SetFloatConst;
		shooter.set_float_const.value = op_buttons[OP_BUTTON_4].down ? spot1 : (op_buttons[OP_BUTTON_5].down ? spot2 : 0);
		shooter.set_float_const.hardware_index = HW_SHOOTER;
		ExecuteBlocklangFunction(shooter, this);

		b32 new_shooter_ready = HitTarget(hardware[HW_SHOOTER].encoder_motor.error_function, 6000) && op_buttons[OP_BUTTON_TRIGGER].down;
		b32 shooter_became_ready = new_shooter_ready && !shooter_ready;
		b32 shooter_became_unready = !new_shooter_ready && shooter_ready;
		shooter_ready = new_shooter_ready;

		/*
		if(shooter_became_ready)
		{
			FunctionBlock indexer = {};
			indexer.type = FunctionBlock_SetFloatConst;
			indexer.set_float_const.value = -0.65;
			indexer.set_float_const.hardware_index = HW_INDEXER;
			ExecuteBlocklangFunction(indexer, this);

			FunctionBlock turntable = {};
			turntable.type = FunctionBlock_SetFloatConst;
			turntable.set_float_const.value = 1;
			turntable.set_float_const.hardware_index = HW_TURNTABLE;
			ExecuteBlocklangFunction(turntable, this);
		}

		if(shooter_became_unready)
		{
			FunctionBlock indexer = {};
			indexer.type = FunctionBlock_SetFloatConst;
			indexer.set_float_const.value = 0;
			indexer.set_float_const.hardware_index = HW_INDEXER;
			ExecuteBlocklangFunction(indexer, this);

			FunctionBlock turntable = {};
			turntable.type = FunctionBlock_SetFloatConst;
			turntable.set_float_const.value = 0;
			turntable.set_float_const.hardware_index = HW_TURNTABLE;
			ExecuteBlocklangFunction(turntable, this);
		}
		 */

		if(op_buttons[OP_BUTTON_7].up)
		{
			FunctionBlock toggle_flap = {};
			toggle_flap.type = FunctionBlock_SetBool;
			toggle_flap.set_bool.op = BooleanOp_Not;
			toggle_flap.set_bool.hardware_index = HW_HOPPER_FLAP;
			ExecuteBlocklangFunction(toggle_flap, this);
		}

		HandlePackets(&net_state, this);
		SendSamples(&net_state, this);
		UpdateErrorFunction(this);

		SmartDashboard::PutNumber("Left Distance", left_encoder.Get() / 1173.0);
		SmartDashboard::PutNumber("Right Distance", right_encoder.Get() / 1173.0);

		Wait(0.05f);
	}

	DisableAllHardware(this);
}

START_ROBOT_CLASS(TestRobot);
