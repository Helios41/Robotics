#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include "Dashboard.h"
#include <stdio.h>
#define MAXPENDING 10

//#define CAMERA_ENABLED

#include "WPILib.h"

class BobRoss : public SampleRobot
{
public:
	Joystick drive_controller;
	Joystick op_controller;
	Compressor compressor;
	robot_hardware_array hardware;
	DoubleSolenoid light_relay;
	pthread_t network_thread;

	BobRoss(void);
	~BobRoss(void);
	void OperatorControl(void);
	void Autonomous(void);
	void RobotInit(void);
};

void StringCopy(char *src, char *dest)
{
   while(*src)
   {
      *dest = *src;
      dest++;
      src++;
   }
   *dest = '\0';
}

char *IntToString(int value, char *str)
{
   sprintf(str, "%i", value);
   return str;
}

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

volatile int server_socket = 0;
volatile bool net_running = true;

volatile int autonomous_operation_count = 0;
volatile auto_operation *autonomous_operations = NULL;
volatile char *autonomous_name = NULL;

volatile int autonomous_error = false;

void SendHardwareDetailsPacket(int client_socket, hardware_component *components, int components_count)
{
	int hardware_details_packet_size = sizeof(hardware_details_header) + (sizeof(hardware_component) * components_count);
	void *hardware_details_packet = malloc(hardware_details_packet_size);

	hardware_details_header *header = (hardware_details_header *) hardware_details_packet;
	header->header.packet_type = PACKET_TYPE_HARDWARE_DETAILS;
	header->header.packet_size = hardware_details_packet_size;
	header->hw_count = components_count;
	hardware_component *hardware_components = (hardware_component *)(header + 1);

	for(int i = 0; i < components_count; i++)
	{
		StringCopy(components[i].name, (hardware_components + i)->name);
		(hardware_components + i)->hw_type = components[i].hw_type;
		(hardware_components + i)->hw_id = components[i].hw_id;
	}

	send(client_socket, hardware_details_packet, hardware_details_packet_size, 0);
	free(hardware_details_packet);
}

void SendHardwareUpdatePacket_Motor(int client_socket, int hw_id, float value)
{
	int hardware_update_packet_size = sizeof(hardware_update_header);
	void *hardware_update_packet = malloc(hardware_update_packet_size);

	hardware_update_header *header = (hardware_update_header *) hardware_update_packet;
	header->header.packet_type = PACKET_TYPE_HARDWARE_UPDATE;
	header->header.packet_size = hardware_update_packet_size;
	header->hw_type = HARDWARE_TYPE_MOTOR_CONTROLLER;
	header->hw_id = hw_id;
	header->motor_value = value;

	send(client_socket, hardware_update_packet, hardware_update_packet_size, 0);
	free(hardware_update_packet);
}

void SendHardwareUpdatePacket_Solenoid(int client_socket, int hw_id, bool extended)
{
	hardware_update_header header = {};
	header.header.packet_type = PACKET_TYPE_HARDWARE_UPDATE;
	header.header.packet_size = sizeof(header);
	header.hw_type = HARDWARE_TYPE_SOLENOID;
	header.hw_id = hw_id;
	header.solenoid_value = extended ? SOLENOID_STATE_EXTENDED : SOLENOID_STATE_RETRACTED;

	send(client_socket, &header, sizeof(header), 0);
}

void SendHardwareUpdatePacket_FloatSensor(int client_socket, int hw_id, float value)
{
	hardware_update_header header = {};
	header.header.packet_type = PACKET_TYPE_HARDWARE_UPDATE;
	header.header.packet_size = sizeof(header);
	header.hw_type = HARDWARE_TYPE_FLOAT_SENSOR;
	header.hw_id = hw_id;
	header.float_sensor_value = value;

	send(client_socket, &header, sizeof(header), 0);
}

void SendHardwareUpdatePacket_BoolSensor(int client_socket, int hw_id, bool value)
{
	hardware_update_header header = {};
	header.header.packet_type = PACKET_TYPE_HARDWARE_UPDATE;
	header.header.packet_size = sizeof(header);
	header.hw_type = HARDWARE_TYPE_BOOL_SENSOR;
	header.hw_id = hw_id;
	header.bool_sensor_value = value ? 1 : 0;

	send(client_socket, &header, sizeof(header), 0);
}

void SendHardwareUpdatePacket_Drive(int client_socket, int hw_id, float forward, float rotate)
{
	hardware_update_header header = {};
	header.header.packet_type = PACKET_TYPE_HARDWARE_UPDATE;
	header.header.packet_size = sizeof(header);
	header.hw_type = HARDWARE_TYPE_DRIVE;
	header.hw_id = hw_id;
	header.forward_value = forward;
	header.rotate_value = rotate;

	send(client_socket, &header, sizeof(header), 0);
}

void SendHardwareUpdatePacket_Camera(int client_socket, int hw_id, void *image_data, int image_data_size)
{
	int hardware_update_packet_size = sizeof(hardware_update_header) + image_data_size;
	void *hardware_update_packet = malloc(hardware_update_packet_size);

	hardware_update_header *header = (hardware_update_header *) hardware_update_packet;
	header->header.packet_type = PACKET_TYPE_HARDWARE_UPDATE;
	header->header.packet_size = sizeof(header);
	header->hw_type = HARDWARE_TYPE_CAMERA;
	header->hw_id = hw_id;

	void *packet_image_data = (void *)(header + 1);
	memcpy(packet_image_data, image_data, image_data_size);

	send(client_socket, hardware_update_packet, hardware_update_packet_size, 0);
	free(hardware_update_packet);
}


void SendWelcomePacket(int client_socket, char *name)
{
	welcome_header header = {};
	header.header.packet_type = PACKET_TYPE_WELCOME;
	header.header.packet_size = sizeof(header);
	StringCopy(name, header.name);

	send(client_socket, &header, sizeof(header), 0);
}

void SendDebugPacket(int client_socket, int type, char *text)
{
	debug_header header = {};
	header.header.packet_type = PACKET_TYPE_DEBUG;
	header.header.packet_size = sizeof(header);
	header.type = type;
	StringCopy(text, header.text);

	send(client_socket, &header, sizeof(header), 0);
}

void SendGetAutonomousStatePacket(int client_socket, bool has_autonomous)
{
	get_autonomous_state_header header = {};
	header.header.packet_type = PACKET_TYPE_GET_AUTONOMOUS_STATE;
	header.header.packet_size = sizeof(header);
	header.has_autonomous = has_autonomous;
	StringCopy((char *)autonomous_name, header.name);

	send(client_socket, &header, sizeof(header), 0);
}

int GetHWType(robot_hardware_type rht)
{
	switch(rht)
	{
		case RHT_Victor: return HARDWARE_TYPE_MOTOR_CONTROLLER;
		case RHT_Solenoid: return HARDWARE_TYPE_SOLENOID;
		case RHT_Microswitch: return HARDWARE_TYPE_BOOL_SENSOR;
		case RHT_Ultrasonic: return HARDWARE_TYPE_FLOAT_SENSOR;
		case RHT_Drive: return HARDWARE_TYPE_DRIVE;
		case RHT_Camera: return HARDWARE_TYPE_CAMERA;
	}

	return HARDWARE_TYPE_INVALID;
}

void SendHardwareDetails(int client_socket, robot_hardware_array *array)
{
	hardware_component *hardware = new hardware_component[array->count];

	for(int i = 0; i < array->count; i++)
	{
		hardware[i].hw_id = i;
		hardware[i].hw_type = GetHWType(array->hardware[i].type);
		StringCopy(array->hardware[i].name , hardware[i].name);
	}

	SendHardwareDetailsPacket(client_socket, hardware, array->count);
}

void SendHardwareUpdates(int client_socket, robot_hardware_array *array)
{
	for(int i = 0; i < array->count; i++)
	{
		robot_hardware_type type = array->hardware[i].type;
		switch(type)
		{
			case RHT_Victor:
			{
				if(array->hardware[i].last_state_float != array->hardware[i].state_float)
				{
					SendHardwareUpdatePacket_Motor(client_socket, i, array->hardware[i].state_float);
				}
				array->hardware[i].last_state_float = array->hardware[i].state_float;
			}
			break;

			case RHT_Solenoid:
			{
				if(array->hardware[i].last_state_bool != array->hardware[i].state_bool)
				{
					SendHardwareUpdatePacket_Solenoid(client_socket, i, array->hardware[i].state_bool);
				}
				array->hardware[i].last_state_bool = array->hardware[i].state_bool;
			}
			break;

			case RHT_Microswitch:
			{
				if(array->hardware[i].last_state_bool != array->hardware[i].state_bool)
				{
					SendHardwareUpdatePacket_BoolSensor(client_socket, i, array->hardware[i].state_bool);
				}
				array->hardware[i].last_state_bool = array->hardware[i].state_bool;
			}
			break;

			case RHT_Ultrasonic:
			{
				if(array->hardware[i].last_state_float != array->hardware[i].state_float)
				{
					SendHardwareUpdatePacket_FloatSensor(client_socket, i, array->hardware[i].state_float);
				}
				array->hardware[i].last_state_float = array->hardware[i].state_float;
			}
			break;

			case RHT_Drive:
			{
				bool forward_changed = (array->hardware[i].last_state_forward != array->hardware[i].state_forward);
				bool rotate_changed = (array->hardware[i].last_state_rotate != array->hardware[i].state_rotate);

				if(forward_changed || rotate_changed)
				{
					SendHardwareUpdatePacket_Drive(client_socket, i, array->hardware[i].state_forward, array->hardware[i].state_rotate);
				}

				array->hardware[i].last_state_forward = array->hardware[i].state_forward;
				array->hardware[i].last_state_rotate = array->hardware[i].state_rotate;
			}
			break;

			case RHT_Camera:
			{
#if defined(CAMERA_ENABLED)
				if(array->hardware[i].camera_active)
				{
					void *image_data = malloc(1024);
					array->hardware[i].camera->GetImageData(image_data, 1024);
					SendHardwareUpdatePacket_Camera(client_socket, i, image_data, 1024);
					free(image_data);
				}
#endif
			}
			break;
		}
	}
}

void UpdateHardware(robot_hardware_array *array)
{
	for(int i = 0; i < array->count; i++)
	{
		robot_hardware_type type = array->hardware[i].type;
		switch(type)
		{
			case RHT_Victor:
				array->hardware[i].state_float = array->hardware[i].victor->Get();
				break;

			case RHT_Solenoid:
				array->hardware[i].state_bool = (array->hardware[i].solenoid->Get() == DoubleSolenoid::kForward);
				break;

			case RHT_Microswitch:
				array->hardware[i].state_bool = !array->hardware[i].microswitch->Get();
				break;

			case RHT_Ultrasonic:
				array->hardware[i].state_float = !array->hardware[i].ultrasonic->GetRangeInches();
				break;

			case RHT_Drive:
			case RHT_Camera:
				break;
		}
	}
}

#pragma pack(push, 1)
struct auto_file_header
{
	uint32_t auto_op_count;
	char name[32];
};
#pragma pack(pop)

void SaveAutonomous(int client_socket)
{
	FILE *file = fopen("/home/lvuser/autonomous.rabin", "w");
	SendDebugPacket(client_socket, DEBUG_TYPE_MESSAGE, (char *)(file ? "FA Success" : "FA Denied"));

	if(file)
	{
		int file_size = sizeof(auto_file_header) + (sizeof(auto_operation) * autonomous_operation_count);
		void *auto_file_data = malloc(file_size);

		auto_file_header *header = (auto_file_header *) auto_file_data;
		header->auto_op_count = autonomous_operation_count;
		StringCopy((char *)autonomous_name, header->name);

		auto_operation *auto_ops = (auto_operation *)(header + 1);

		for(int i = 0; i < autonomous_operation_count; i++)
		{
			auto_ops[i] = ((auto_operation *) autonomous_operations)[i];
		}

		fwrite(auto_file_data, file_size, 1, file);
		free(auto_file_data);
		fclose(file);
	}
}

void LoadAutonomous()
{
	FILE *file = fopen("/home/lvuser/autonomous.rabin", "r");

	if(file != NULL)
	{
		int file_size = 2048;
		void *auto_file_data = malloc(file_size);
		fread(auto_file_data, file_size, 1, file);

		auto_file_header *header = (auto_file_header *) auto_file_data;
		auto_operation *auto_ops = (auto_operation *)(header + 1);

		StringCopy(header->name, (char *)autonomous_name);

		autonomous_operation_count = 0;
		for(int i = 0; i < (int)header->auto_op_count; i++)
		{
			((auto_operation *)autonomous_operations)[autonomous_operation_count] = auto_ops[i];
			autonomous_operation_count++;
		}

		free(auto_file_data);
		fclose(file);
	}
}

void HandlePacket(int client_socket, char *buffer, int size)
{
	generic_packet_header *packet_header = (generic_packet_header *)buffer;

	if(packet_header->packet_type == PACKET_TYPE_SEND_AUTONOMOUS)
	{
		send_autonomous_header *send_autonomous = (send_autonomous_header *)packet_header;
		auto_operation *operations = (auto_operation *)(send_autonomous  + 1);

		SendDebugPacket(client_socket, DEBUG_TYPE_MESSAGE, (char *)"Autononmous Recieved");
		StringCopy(send_autonomous->name, (char *)autonomous_name);
		SendGetAutonomousStatePacket(client_socket, (send_autonomous->auto_operation_count > 0));

		autonomous_operation_count = 0;
		for(int i = 0;
			(i < (int)send_autonomous->auto_operation_count);
			i++)
		{
			if((operations[i].hw_type == HARDWARE_TYPE_MOTOR_CONTROLLER) ||
			   (operations[i].hw_type == HARDWARE_TYPE_SOLENOID) ||
			   (operations[i].hw_type == HARDWARE_TYPE_DRIVE))
			{
				((auto_operation *)autonomous_operations)[autonomous_operation_count++] = operations[i];
			}
		}

		SaveAutonomous(client_socket);
	}
}

bool HandleNetwork(int client_socket)
{
	int flags = fcntl(client_socket, F_GETFL, 0);
	fcntl(client_socket, F_SETFL, flags|O_NONBLOCK);

	bool connected = false;
	bool had_packet = true;

	while(had_packet)
	{
		char packet_buffer[4000];
		int data_length = recv(client_socket, packet_buffer, 4000, 0);
		int error_code = errno;
		connected = ((error_code == EWOULDBLOCK) || (error_code == 0));
		had_packet = (data_length != -1);

		if(connected && (data_length != -1))
		{
			char *buffer = packet_buffer;
			int buffer_size = data_length;

			while(buffer_size > 0)
			{
				generic_packet_header *packet_header = (generic_packet_header *)buffer;

				if((int)packet_header->packet_size <= buffer_size)
					HandlePacket(client_socket, buffer, buffer_size);

				buffer += packet_header->packet_size;
				buffer_size -= packet_header->packet_size;
			}
		}
	}

	fcntl(client_socket, F_SETFL, flags);
	return connected;
}

void *NetworkMain(void *in_data)
{
	robot_hardware_array *hardware = (robot_hardware_array *)in_data;
	LoadAutonomous();

	while(net_running)
	{
		struct sockaddr_in client_info = {};
		unsigned int client_len = sizeof(client_info);
		bool connected = true;

		int client_socket = accept(server_socket, (struct sockaddr *) &client_info, &client_len);

		SendDebugPacket(client_socket, DEBUG_TYPE_MESSAGE, (char *)"Sup");
		SendWelcomePacket(client_socket, (char *)"Bob Ross");
		SendHardwareDetails(client_socket, hardware);
		SendGetAutonomousStatePacket(client_socket, (autonomous_operation_count > 0));

		do
		{
			SendHardwareUpdates(client_socket, hardware);

			if(__sync_bool_compare_and_swap(&autonomous_error, true, false))
			{
				SendDebugPacket(client_socket, DEBUG_TYPE_MESSAGE, (char *)"Autonomous Missing");
			}

			connected = HandleNetwork(client_socket);
			Wait(0.5f);
		}
		while(net_running && connected);
	}

	return NULL;
}

void ArcadeDrive(robot_hardware_array *hardware, int drive_id, float forwards, float rotate)
{
	RobotDrive *drive = GetDrive(drive_id, hardware);
	if(drive)
	{
		drive->ArcadeDrive(rotate, forwards);
		hardware->hardware[drive_id].state_forward = forwards;
		hardware->hardware[drive_id].state_rotate = rotate;
	}
}

int hopper_motor_id = 0;
int shooter_motor_id = 0;
int intake_motor_id = 0;
int arm_piston_id = 0;
int hopper_switch_id = 0;
int drive_id = 0;
#if defined(CAMERA_ENABLED)
int camera_id = 0;
#endif

BobRoss::BobRoss(void) :
		 drive_controller(0),
		 op_controller(1),
		 compressor(0),
		 hardware(),
		 light_relay(2, 3),
		 network_thread()
{
	hopper_motor_id = HWVictor(&hardware, (char *)"Hopper Motor", 9);
	shooter_motor_id = HWVictor(&hardware, (char *)"Shooter Motor", 8);
		GetMotorController(shooter_motor_id, &hardware)->SetInverted(true);
	intake_motor_id = HWVictor(&hardware, (char *)"Intake Motor", 0);
	arm_piston_id = HWSolenoid(&hardware, (char *)"Arm Piston", 0, 1);
	hopper_switch_id = HWMicroswitch(&hardware, (char *)"Hopper Switch", 0);
	drive_id = HW4VDrive(&hardware, (char *)"Drive", 5, 6, 3, 4);
		GetDrive(drive_id, &hardware)->SetInvertedMotor(RobotDrive::kFrontLeftMotor, true);
		GetDrive(drive_id, &hardware)->SetInvertedMotor(RobotDrive::kRearLeftMotor, true);
		GetDrive(drive_id, &hardware)->SetSafetyEnabled(false);
#if defined(CAMERA_ENABLED)
	camera_id = HWCamera(&hardware, (char *)"Camera", (char *)"cam0");
    	GetCamera(camera_id, &hardware)->OpenCamera();
#endif

	struct sockaddr_in server_info = {};
	server_info.sin_family = AF_INET;
	server_info.sin_addr.s_addr = htonl(INADDR_ANY);
	server_info.sin_port = htons(8089);

	server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(server_socket, (struct sockaddr *) &server_info, sizeof(server_info));
	listen(server_socket, MAXPENDING);

	autonomous_operations = (volatile auto_operation *)malloc(sizeof(auto_operation) * 32);
	autonomous_name = (volatile char *) malloc(sizeof(char) * 32);

	pthread_create(&network_thread, NULL, NetworkMain, &hardware);
}

void BobRoss::RobotInit(void)
{
	//CameraServer *camera_server = CameraServer::GetInstance();
	//camera_server->StartAutomaticCapture("cam0");
}

BobRoss::~BobRoss(void)
{
	net_running = false;
#if defined(CAMERA_ENABLED)
	GetCamera(camera_id, &hardware)->CloseCamera();
#endif
	pthread_join(network_thread, NULL);
	free((void *)autonomous_operations);
}

void BobRoss::Autonomous(void)
{
	if(autonomous_operation_count == 0)
	{
		__sync_bool_compare_and_swap(&autonomous_error, false, true);
	}

	for(int i = 0; (i < autonomous_operation_count) && (IsAutonomous() && IsEnabled()); i++)
	{
		volatile auto_operation *op = autonomous_operations + i;

		if(op->hw_type == HARDWARE_TYPE_MOTOR_CONTROLLER)
		{
			Victor *motor_controller = GetMotorController(op->hw_id, &hardware);
			if(motor_controller)
			{
				motor_controller->Set(op->motor_value);
				Wait(op->time);
				motor_controller->Set(0.0f);
			}
		}
		else if(op->hw_type == HARDWARE_TYPE_SOLENOID)
		{
			DoubleSolenoid *solenoid = GetSolenoid(op->hw_id, &hardware);
			if(solenoid)
			{
				if(op->solenoid_value == SOLENOID_STATE_EXTENDED)
				{
					solenoid->Set(DoubleSolenoid::kForward);
				}
				else if(op->solenoid_value == SOLENOID_STATE_RETRACTED)
				{
					solenoid->Set(DoubleSolenoid::kReverse);
				}
				else if(op->solenoid_value == SOLENOID_STATE_STOPPED)
				{
					solenoid->Set(DoubleSolenoid::kOff);
				}
			}
		}
		else if(op->hw_type == HARDWARE_TYPE_DRIVE)
		{
			RobotDrive *drive = GetDrive(op->hw_id, &hardware);
			drive->ArcadeDrive(op->rotate_value, op->forward_value);
			Wait(op->time);
			drive->ArcadeDrive(0.0f, 0.0f);
		}
	}

	while(IsAutonomous() && IsEnabled()) {}
}

#define DRIVE_AXIS 1
#define ROTATE_AXIS 4

#define A_BUTTON 1
#define B_BUTTON 2
#define X_BUTTON 3
#define Y_BUTTON 4

#define LEFT_BUMPER 5
#define RIGHT_BUMPER 6
#define LEFT_TRIGGER 2
#define RIGHT_TRIGGER 3

#define REV_LENGTH 20

void BobRoss::OperatorControl(void)
{
	DoubleSolenoid *arm_piston = GetSolenoid(arm_piston_id, &hardware);
	DigitalInput *intake_switch = GetSwitch(hopper_switch_id, &hardware);
	Victor *hopper_motor = GetMotorController(hopper_motor_id, &hardware);
	Victor *intake_motor = GetMotorController(intake_motor_id, &hardware);
	Victor *shooter_motor = GetMotorController(shooter_motor_id, &hardware);

	bool arm_piston_extended = (arm_piston->Get() == DoubleSolenoid::kForward) ? true : false;
	int time_since_launch = 0;
	bool intake_active = false;
	int time_since_rev = 0;

	bool op_button1_last_state = false;
	bool op_button3_last_state = false;

	bool left_bumper_last_state = false;
	bool right_bumper_last_state = false;
	bool left_trigger_last_state = false;
	bool right_trigger_last_state = false;
	bool y_button_last_state = false;

	bool shooter_light_enabled = false;

	while(IsOperatorControl() && IsEnabled())
	{
		if(!op_controller.GetRawButton(1) && op_button1_last_state)
		{
			intake_active = !intake_active;
		}
		op_button1_last_state = op_controller.GetRawButton(1);

		if(!op_controller.GetRawButton(3) && op_button3_last_state)
		{
			arm_piston_extended = !arm_piston_extended;
		}
		op_button3_last_state = op_controller.GetRawButton(3);

		int d_pad = drive_controller.GetPOV(0);
		if(d_pad == 0)
		{
			time_since_launch = 30;
		}
		else if(d_pad == 180)
		{
			time_since_launch = 0;
		}

		if(!intake_switch->Get())
		{
			intake_active = false;
		}

		float multiplier = drive_controller.GetRawButton(9) ? 0.5f : 1.0f;
		float forwards = drive_controller.GetRawAxis(DRIVE_AXIS) * multiplier;
		float rotation = drive_controller.GetRawAxis(ROTATE_AXIS) * multiplier * -1.0f;
		ArcadeDrive(&hardware, drive_id, forwards, rotation);

		if(drive_controller.GetRawButton(A_BUTTON))
		{
			hopper_motor->Set(1.0f);
		}
		else if(drive_controller.GetRawButton(B_BUTTON))
		{
			hopper_motor->Set(-1.0f);
		}
		else
		{
			hopper_motor->Set(0.0f);
		}

		if(drive_controller.GetRawButton(X_BUTTON))
		{
			intake_motor->Set(1.0f);
		}
		else if(drive_controller.GetRawButton(B_BUTTON))
		{
			intake_motor->Set(-1.0f);
		}
		else
		{
			intake_motor->Set(0.0f);
		}

		if(intake_active)
		{
			hopper_motor->Set(0.4f);
			intake_motor->Set(1.0f);
		}

		bool left_bumper_down = (drive_controller.GetRawButton(LEFT_BUMPER) && !left_bumper_last_state);
		bool right_bumper_down = (drive_controller.GetRawButton(RIGHT_BUMPER) && !right_bumper_last_state);
		bool left_trigger_down = ((drive_controller.GetRawAxis(LEFT_TRIGGER) > 0) && !left_trigger_last_state);
		bool right_trigger_down = ((drive_controller.GetRawAxis(RIGHT_TRIGGER) > 0) && !right_trigger_last_state);

		left_bumper_last_state = drive_controller.GetRawButton(LEFT_BUMPER);
		right_bumper_last_state = drive_controller.GetRawButton(RIGHT_BUMPER);
		left_trigger_last_state = (drive_controller.GetRawAxis(LEFT_TRIGGER) > 0);
		right_trigger_last_state = (drive_controller.GetRawAxis(RIGHT_TRIGGER) > 0);

		if(left_bumper_down || right_bumper_down || left_trigger_down || right_trigger_down)
		{
			time_since_rev = REV_LENGTH;
		}

		if(drive_controller.GetRawButton(LEFT_BUMPER))
		{
			if(compressor.Enabled())
				compressor.Stop();

			if(time_since_rev > 0)
			{
				shooter_motor->Set(1.0f);
				time_since_rev--;
			}
			else
			{
				shooter_motor->Set(0.75f);
			}
		}
		else if(drive_controller.GetRawButton(RIGHT_BUMPER))
		{
			if(compressor.Enabled())
				compressor.Stop();

			if(time_since_rev > 0)
				time_since_rev--;

			shooter_motor->Set(1.0f);
		}
		else if(drive_controller.GetRawAxis(LEFT_TRIGGER) > 0)
		{
			if(compressor.Enabled())
				compressor.Stop();

			if(time_since_rev > 0)
			{
				shooter_motor->Set(1.0f);
				time_since_rev--;
			}
			else
			{
				shooter_motor->Set(0.85f);
			}
		}
		else if(drive_controller.GetRawAxis(RIGHT_TRIGGER) > 0)
		{
			if(compressor.Enabled())
				compressor.Stop();

			if(time_since_rev > 0)
			{
				shooter_motor->Set(1.0f);
				time_since_rev--;
			}
			else
			{
				shooter_motor->Set(0.90f);
			}
		}
		else if(op_controller.GetRawButton(2))
		{
			if(compressor.Enabled())
				compressor.Stop();

			shooter_motor->Set(1.0f);
		}
		else if(op_controller.GetRawButton(8))
		{
			if(compressor.Enabled())
				compressor.Stop();

			shooter_motor->Set(op_controller.GetRawAxis(2));
		}
		else
		{
			time_since_rev = 0;
			shooter_motor->Set(0.0f);

			if(!compressor.Enabled())
				compressor.Start();
		}

		if(!drive_controller.GetRawButton(Y_BUTTON) && y_button_last_state)
		{
			shooter_light_enabled = !shooter_light_enabled;
#if defined(CAMERA_ENABLED)
			hardware.hardware[camera_id].camera_active = !hardware.hardware[camera_id].camera_active;

			if(hardware.hardware[camera_id].camera_active)
			{
				GetCamera(camera_id, &hardware)->StartCapture();
			}
			else
			{
				GetCamera(camera_id, &hardware)->StartCapture();
			}
#endif
		}
		y_button_last_state = drive_controller.GetRawButton(Y_BUTTON);

		if(shooter_light_enabled || (shooter_motor->Get() != 0.0f))
		{
			if(light_relay.Get() != DoubleSolenoid::kForward)
				light_relay.Set(DoubleSolenoid::kForward);
		}
		else
		{
			if(light_relay.Get() != DoubleSolenoid::kOff)
				light_relay.Set(DoubleSolenoid::kOff);
		}

		if(arm_piston_extended)
		{
			arm_piston->Set(DoubleSolenoid::kForward);
		}
		else
		{
			arm_piston->Set(DoubleSolenoid::kReverse);
		}

		if(time_since_launch > 0)
		{
			if((time_since_launch < 30) &&
			   (time_since_launch > 10))
			{
				hopper_motor->Set(0.0f);
				shooter_motor->Set(1.0f);
			}
			else if(time_since_launch <= 10)
			{
				hopper_motor->Set(1.0f);
				shooter_motor->Set(0.8f);
			}

			time_since_launch--;
		}

		UpdateHardware(&hardware);
		Wait(0.05f);
	}
}

START_ROBOT_CLASS(BobRoss);
