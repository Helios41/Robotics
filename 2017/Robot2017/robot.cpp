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
	
	RobotHardware hardware[9] = {};
	
	Victor right_front { 0 };
	Victor right_back { 1 };
	//Encoder right_encoder {0, 1};
	Victor left_front { 2 };
	Victor left_back { 3 };
	//Encoder left_encoder {6, 7};
	r32 drive_multiplier = 1.0f;
	
	AnalogInput turntable_potentiometer { 0 };

	char auto_name[32] = {};
	u32 auto_length = 0;
	FunctionBlock *auto_program = NULL;
	
	network_state net_state = {};

	void RobotInit();
	void Autonomous();
	void OperatorControl();
};

#include <dashcode.h>

void DisableAllHardware(TestRobot *robot)
{
	for(u32 i = 0;
		i < ArrayCount(robot->hardware);
		i++)
	{
		if((robot->hardware[i].type == Hardware_Motor) &&
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

#define HW_TURNTABLE 0
#define HW_SHOOTER 1
#define HW_INTAKE 2
#define HW_INDEXER 3
#define HW_CLIMBER 4
#define HW_SHOOTER_SWIVEL 5
#define HW_BALL_SHIFTER 6
#define HW_PUNCH 7
#define HW_GEAR_HOLDER 8

void TestRobot::RobotInit()
{
	hardware[HW_TURNTABLE] = HWMotor_Victor(4, (char *)"Turntable");
	hardware[HW_SHOOTER] = HWMotor_EncoderVictor(5, 4, 5, EFunction(10, 0.0005, 0.000005), (char *)"Shooter");
	hardware[HW_INTAKE] = HWMotor_Victor(6, (char *)"Intake");
	hardware[HW_INDEXER] = HWMotor_Victor(7, (char *)"Indexer");
	hardware[HW_CLIMBER] = HWMotor_Victor(8, (char *)"Climber");
	hardware[HW_SHOOTER_SWIVEL] = HWMotor_Spark(9, (char *)"Shooter Swivel");
	hardware[HW_BALL_SHIFTER] = HWSolenoid(0, 1, (char *)"Ball Shifter");
	hardware[HW_PUNCH] = HWSolenoid(6, 7, (char *)"Punch");
	hardware[HW_GEAR_HOLDER] = HWSolenoid(2, 3, (char *)"Gear Holder");

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

	(new Compressor(0))->Stop();
}

struct blocklang_runtime
{
	FunctionBlock *functions;
	u32 function_count;
	r32 wait_time;
	u32 curr_function;
};

b32 IsRunning(blocklang_runtime *auto_rt)
{
	return (auto_rt->curr_function != auto_rt->function_count) || (auto_rt->wait_time != 0.0);
}

void TestRobot::Autonomous()
{
	DisableAllHardware(this);

	/*
 	if(auto_program)
	{
		
	}
	*/

	FunctionBlock test_auto[6] = {};

	test_auto[0].type = FunctionBlock_ArcadeDriveConst;
	test_auto[0].arcade_drive_const.power = 0.25;
	test_auto[0].arcade_drive_const.rotate = 0.0;

	test_auto[1].type = FunctionBlock_Wait;
	test_auto[1].wait.duration = 5;

	test_auto[2].type = FunctionBlock_ArcadeDriveConst;
	test_auto[2].arcade_drive_const.power = 0.0;
	test_auto[2].arcade_drive_const.rotate = 0.0;

	test_auto[3].type = FunctionBlock_Wait;
	test_auto[3].wait.duration = 2.5;

	test_auto[4].type = FunctionBlock_ArcadeDriveConst;
	test_auto[4].arcade_drive_const.power = 0.40;
	test_auto[4].arcade_drive_const.rotate = 0.0;

	test_auto[5].type = FunctionBlock_Wait;
	test_auto[5].wait.duration = 2.5;

	blocklang_runtime auto_rt = {};
	auto_rt.function_count = ArrayCount(test_auto);
	auto_rt.functions = test_auto;

	u64 timer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	while(IsAutonomous() && IsEnabled() && IsRunning(&auto_rt))
	{
		u64 curr_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - timer;
		timer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		if(auto_rt.wait_time == 0.0)
		{
			while(auto_rt.curr_function < auto_rt.function_count)
			{
				FunctionBlock function = auto_rt.functions[auto_rt.curr_function++];
				if(function.type == FunctionBlock_Wait)
				{
					auto_rt.wait_time = function.wait.duration * 1000;
					break;
				}
				else
				{
					ExecuteBlocklangFunction(function, this);
				}
			}
		}

		auto_rt.wait_time -= curr_time;
		auto_rt.wait_time = (auto_rt.wait_time < 0.0) ? 0.0 : auto_rt.wait_time;

		SmartDashboard::PutNumber("Auto RT Wait Time", auto_rt.wait_time);
		SmartDashboard::PutNumber("Auto RT Curr Timer", curr_time);
		SmartDashboard::PutNumber("Auto RT Timer", timer);
		SmartDashboard::PutNumber("Auto IPointer", auto_rt.curr_function);

		Wait(0.05f);
	}

	DisableAllHardware(this);
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
		}
	}
}

void SendSamples(network_state *net_state, TestRobot *robot)
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
		}

		if(send_sample)
		{
			SendHardwareSample(net_state, i, sample);
			hardware->last_sample = sample;
		}
	}
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

void TestRobot::OperatorControl()
{
	ButtonState driver_buttons[8] = {};
	ButtonState op_buttons[11] = {};

	DisableAllHardware(this);

	b32 shooter_ready = false;

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
		r32 spot2 = 15000 * ((op_controller.GetRawAxis(2) - 1) / 2) - 70000;

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

		HandlePackets(&net_state, this);
		SendSamples(&net_state, this);

		/*
		SmartDashboard::PutNumber("Left Raw", left_encoder.GetRaw());
		SmartDashboard::PutNumber("Left Rate", left_encoder.GetRate());
		SmartDashboard::PutNumber("Left Get", left_encoder.Get());

		SmartDashboard::PutNumber("Right Raw", right_encoder.GetRaw());
		SmartDashboard::PutNumber("Right Rate", right_encoder.GetRate());
		SmartDashboard::PutNumber("Right Get", right_encoder.Get());
		 */

		for(u32 i = 0;
			i < ArrayCount(hardware);
			i++)
		{
			if(hardware[i].type == Hardware_EncoderMotor)
			{
				Update(&hardware[i].encoder_motor.error_function, hardware[i].encoder_motor.encoder->GetRate());

				SmartDashboard::PutNumber("Rate " + std::string(hardware[i].name), hardware[i].encoder_motor.encoder->GetRate());
				SmartDashboard::PutNumber("Speed " + std::string(hardware[i].name), hardware[i].encoder_motor.error_function.speed);
				SmartDashboard::PutNumber("Target " + std::string(hardware[i].name), hardware[i].encoder_motor.error_function.target);
				SmartDashboard::PutNumber("Average " + std::string(hardware[i].name), hardware[i].encoder_motor.error_function.avg);

				if(Absolute(hardware[i].encoder_motor.error_function.target) < 0.1)
				{
					hardware[i].encoder_motor.motor->Set(0);
				}
				else
				{
					hardware[i].encoder_motor.motor->Set(hardware[i].encoder_motor.error_function.speed);
				}
			}
		}

		Wait(0.05f);
	}

	DisableAllHardware(this);
}

START_ROBOT_CLASS(TestRobot);
