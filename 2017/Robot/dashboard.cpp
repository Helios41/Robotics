enum robot_hardware_type
{
	RHT_Victor,
	RHT_Solenoid,
	RHT_Ultrasonic,
	RHT_Microswitch,
	RHT_Drive,
	RHT_Camera
};

struct robot_hardware
{
	robot_hardware_type type;
	char *name;

	union
	{
		struct
		{
			float last_state_float;
			float state_float;
		};

		struct
		{
			bool last_state_bool;
			bool state_bool;
		};

		struct
		{
			float last_state_forward;
			float state_forward;
			float last_state_rotate;
			float state_rotate;
		};

		bool camera_active;
	};

	union
	{
		Victor *victor;
		DoubleSolenoid *solenoid;
		Ultrasonic *ultrasonic;
		DigitalInput *microswitch;
		RobotDrive *drive;
		USBCamera *camera;
	};
};

struct robot_hardware_array
{
	robot_hardware hardware[32];
	int count;
};

int HWVictor(robot_hardware_array *array, char *name, int pwm_port)
{
	int index = array->count++;
	array->hardware[index].name = name;
	array->hardware[index].type = RHT_Victor;
	array->hardware[index].victor = new Victor(pwm_port);
	return index;
}

int HWSolenoid(robot_hardware_array *array, char *name, int f_port, int r_port)
{
	int index = array->count++;
	array->hardware[index].name = name;
	array->hardware[index].type = RHT_Solenoid;
	array->hardware[index].solenoid = new DoubleSolenoid(f_port, r_port);
	return index;
}

int HWUltrasonic(robot_hardware_array *array, char *name, int ping_port, int echo_port)
{
	int index = array->count++;
	array->hardware[index].name = name;
	array->hardware[index].type = RHT_Ultrasonic;
	array->hardware[index].ultrasonic = new Ultrasonic(ping_port, echo_port);
	return index;
}

int HWMicroswitch(robot_hardware_array *array, char *name, int dio_port)
{
	int index = array->count++;
	array->hardware[index].name = name;
	array->hardware[index].type = RHT_Microswitch;
	array->hardware[index].microswitch = new DigitalInput(dio_port);
	return index;
}

int HW4VDrive(robot_hardware_array *array, char *name, int front_left, int back_left, int front_right, int back_right)
{
	int index = array->count++;
	array->hardware[index].name = name;
	array->hardware[index].type = RHT_Drive;
	array->hardware[index].drive = new RobotDrive(new Victor(front_left), new Victor(back_left), new Victor(front_right), new Victor(back_right));
	return index;
}

int HWCamera(robot_hardware_array *array, char *name, char *camera_id)
{
	int index = array->count++;
	array->hardware[index].name = name;
	array->hardware[index].type = RHT_Camera;
	array->hardware[index].camera = new USBCamera(camera_id, true);
	return index;
}

Victor *GetMotorController(int id, robot_hardware_array *array)
{
	if(id < array->count)
	{
		if(array->hardware[id].type == RHT_Victor)
		{
			return array->hardware[id].victor;
		}
	}

	return NULL;
}

DoubleSolenoid *GetSolenoid(int id, robot_hardware_array *array)
{
	if(id < array->count)
	{
		if(array->hardware[id].type == RHT_Solenoid)
		{
			return array->hardware[id].solenoid;
		}
	}

	return NULL;
}

DigitalInput *GetSwitch(int id, robot_hardware_array *array)
{
	if(id < array->count)
	{
		if(array->hardware[id].type == RHT_Microswitch)
		{
			return array->hardware[id].microswitch;
		}
	}

	return NULL;
}

Ultrasonic *GetUltrasonic(int id, robot_hardware_array *array)
{
	if(id < array->count)
	{
		if(array->hardware[id].type == RHT_Ultrasonic)
		{
			return array->hardware[id].ultrasonic;
		}
	}

	return NULL;
}

RobotDrive *GetDrive(int id, robot_hardware_array *array)
{
	if(id < array->count)
	{
		if(array->hardware[id].type == RHT_Drive)
		{
			return array->hardware[id].drive;
		}
	}

	return NULL;
}

USBCamera *GetCamera(int id, robot_hardware_array *array)
{
	if(id < array->count)
	{
		if(array->hardware[id].type == RHT_Camera)
		{
			return array->hardware[id].camera;
		}
	}

	return NULL;
}