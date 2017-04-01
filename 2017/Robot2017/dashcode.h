RobotHardware HWMotor_Victor(u32 port, char *name)
{
	RobotHardware result = {};
	result.multiplier = 1.0f;
	result.motor.victor = new Victor(port);
	result.motor.is_victor = true;
	result.type = Hardware_Motor;
	stpcpy(result.name, name);
	return result;
}

RobotHardware HWMotor_EncoderVictor(u32 motor_port, u32 encoder_a,
									u32 encoder_b,
									r32 Kp, r32 Ki, r32 Kd,
									u32 average_span, r32 encoder_coeff, char *name)
{
	RobotHardware result = {};
	result.multiplier = 1.0f;
	result.encoder_motor.motor = new Victor(motor_port);

	result.encoder_motor.encoder = new Encoder(encoder_a, encoder_b);
	result.encoder_motor.encoder->SetPIDSourceType(PIDSourceType::kRate);
	result.encoder_motor.encoder->SetSamplesToAverage(average_span);
	result.encoder_motor.encoder->SetDistancePerPulse(encoder_coeff);

	result.encoder_motor.pid = new PIDController(Kp, Ki, Kd, result.encoder_motor.encoder, result.encoder_motor.motor);
	result.encoder_motor.pid->Enable();
	result.type = Hardware_EncoderMotor;
	stpcpy(result.name, name);
	return result;
}

RobotHardware HWMotor_Spark(u32 port, char *name)
{
	RobotHardware result = {};
	result.multiplier = 1.0f;
	result.motor.spark = new Spark(port);
	result.motor.is_victor = false;
	result.type = Hardware_Motor;
	stpcpy(result.name, name);
	return result;
}

RobotHardware HWSolenoid(u32 fport, u32 rport, char *name)
{
	RobotHardware result = {};
	result.solenoid = new DoubleSolenoid(fport, rport);
	result.type = Hardware_Solenoid;
	stpcpy(result.name, name);
	return result;
}

RobotHardware HWPotentiometer(u32 port, char *name)
{
	RobotHardware result = {};
	result.potentiometer = new AnalogInput(port);
	result.type = Hardware_Potentiometer;
	stpcpy(result.name, name);
	return result;
}

void SetMotor(RobotHardware *hardware, r32 value)
{
	if(hardware->type == Hardware_Motor)
	{
		if(hardware->motor.is_victor)
		{
			hardware->motor.victor->Set(hardware->multiplier * value);
		}
		else
		{
			hardware->motor.spark->Set(hardware->multiplier * value);
		}
	}
	else if(hardware->type == Hardware_EncoderMotor)
	{
#ifdef COMP_BOT
		//if(i == HW_SHOOTER)
		{
			hardware->encoder_motor.error_function.target = hardware->multiplier * -value;
		}
		/*
		else
		{
			hardware->encoder_motor.error_function.target = hardware->multiplier * value;
		}
		*/
#else
		r32 rmp_value = value / 60.0f;

		if(hardware->encoder_motor.pid->GetSetpoint() != rmp_value)
		{
			SmartDashboard::PutNumber("Setpoint Changed", SmartDashboard::GetNumber("Setpoint Changed", 0) + 1);
			hardware->encoder_motor.pid->SetSetpoint(rmp_value);
		}
#endif
	}
}

void TankDrive(TestRobot *robot, r32 left, r32 right)
{
	robot->right_front.Set(right);
	robot->right_back.Set(right);

	robot->left_front.Set(-left);
	robot->left_back.Set(-left);
}

void ArcadeDrive(TestRobot *robot, r32 power, r32 rotate)
{
	r32 right_speed = power + rotate;
	r32 left_speed = power - rotate;

	right_speed = Clamp(-1, robot->drive_multiplier * right_speed, 1);
	left_speed = Clamp(-1, robot->drive_multiplier * left_speed, 1);

	TankDrive(robot, left_speed, right_speed);
}

void ExecuteBlocklangFunction(FunctionBlock function, TestRobot *robot, b32 flip_turns = false)
{
	RobotHardware *hardware_array = robot->hardware;
	Joystick *driver_controller = &robot->driver_controller;
	Joystick *op_controller = &robot->op_controller;

	switch(function.type)
	{
		case FunctionBlock_SetFloatConst:
		{
			RobotHardware *motor_hardware = hardware_array + function.set_float_const.hardware_index;

			SetMotor(motor_hardware, function.set_float_const.value);
		}
		break;
		
		case FunctionBlock_SetFloatController:
		{
			RobotHardware *motor_hardware = hardware_array + function.set_float_controller.hardware_index;
			r32 value = (function.set_float_controller.is_op ? op_controller : driver_controller)->GetRawAxis(function.set_float_controller.axis_index);

			SetMotor(motor_hardware, value);
		}
		break;

		case FunctionBlock_SetMultiplier:
		{
			RobotHardware *hardware = hardware_array + function.set_bool.hardware_index;
			if((hardware->type == Hardware_Motor) ||
			   (hardware->type == Hardware_EncoderMotor))
			{
				hardware->multiplier = function.set_multiplier.value;
			}
		}
		break;

		case FunctionBlock_SetBool:
		{
			RobotHardware *hardware = hardware_array + function.set_bool.hardware_index;
			if(hardware->type == Hardware_Solenoid)
			{
				switch(function.set_bool.op)
				{
					case BooleanOp_True:
						hardware->solenoid->Set(DoubleSolenoid::Value::kForward);
						break;
						
					case BooleanOp_False:
						hardware->solenoid->Set(DoubleSolenoid::Value::kReverse);
						break;
						
					case BooleanOp_Not:
					{
						DoubleSolenoid::Value value = hardware->solenoid->Get();
						hardware->solenoid->Set((value == DoubleSolenoid::Value::kForward) ? DoubleSolenoid::Value::kReverse : DoubleSolenoid::Value::kForward);
					}
					break; 
				}
			}
		}
		break;
		
		case FunctionBlock_ArcadeDriveConst:
		{
			ArcadeDrive(robot, function.arcade_drive_const.power, flip_turns ? -function.arcade_drive_const.rotate : function.arcade_drive_const.rotate);
		}
		break;
		
		case FunctionBlock_ArcadeDriveController:
		{
			ArcadeDrive(robot,
						driver_controller->GetRawAxis(function.arcade_drive_controller.power_axis_index),
						driver_controller->GetRawAxis(function.arcade_drive_controller.rotate_axis_index));
		}
		break;

		case FunctionBlock_SetDriveMultiplier:
		{
			robot->drive_multiplier = function.set_drive_multiplier.value;
		}
		break;
	}
}

struct blocklang_runtime
{
	FunctionBlock *functions;
	u32 function_count;
	r32 wait_time;
	u32 curr_function;
	u64 timer;

	b32 left_remaining;
	b32 right_remaining;

	r32 left_distance;
	r32 right_distance;
};

b32 IsRunning(blocklang_runtime *runtime)
{
	return (runtime->curr_function != runtime->function_count) || (runtime->wait_time != 0.0) || runtime->left_remaining || runtime->right_remaining;
}

blocklang_runtime SetupRuntime(FunctionBlock *functions, u32 function_count)
{
	blocklang_runtime result = {};

	result.timer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	result.function_count = function_count;
	result.functions = functions;

	return result;
}

void ExecuteBlocklangRuntime(blocklang_runtime *runtime, TestRobot *robot, b32 flip_turns)
{
	if(IsRunning(runtime))
	{
		u64 curr_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() - runtime->timer;
		runtime->timer = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		if((runtime->wait_time == 0.0) &&
		   !runtime->left_remaining && !runtime->right_remaining)
		{
			while(runtime->curr_function < runtime->function_count)
			{
				FunctionBlock function = runtime->functions[runtime->curr_function++];
				if(function.type == FunctionBlock_Wait)
				{
					runtime->wait_time = function.wait.duration * 1000;
					break;
				}
				else if(function.type == FunctionBlock_DriveDistance)
				{
					robot->left_encoder.Reset();
					robot->right_encoder.Reset();

					runtime->left_remaining = true;
					runtime->right_remaining = true;

					b32 is_turn = Sign(function.drive_distance.left_distance) != Sign(function.drive_distance.right_distance);

					runtime->left_distance = function.drive_distance.left_distance;
					runtime->right_distance = function.drive_distance.right_distance;

					if(is_turn && flip_turns)
					{
						runtime->left_distance *= -1;
						runtime->right_distance *= -1;
					}

					break;
				}
				else if(function.type == FunctionBlock_GotoPosition)
				{

				}
				else
				{
					ExecuteBlocklangFunction(function, robot, flip_turns);
				}
			}
		}
		else if(runtime->left_remaining || runtime->right_remaining)
		{
			b32 is_turn = Sign(runtime->left_distance) != Sign(runtime->right_distance);
			r32 speed = is_turn ? 0.80 : (robot->current_auto_slot == 3 ? 0.60 : 0.30);

			r32 left_distance = robot->left_encoder.Get() / DRIVE_ENCODER_COEFF;
			r32 right_distance = robot->right_encoder.Get() / DRIVE_ENCODER_COEFF;

#ifdef COMP_BOT
			left_distance *= -1;
			right_distance *= -1;
#endif

			b32 prev_left_remaining = runtime->left_remaining;
			runtime->left_remaining = (runtime->left_distance > 0) ? ((left_distance + runtime->left_distance) > 0.01) : ((left_distance + runtime->left_distance) < -0.01);
			b32 left_moving = Absolute(robot->left_encoder.GetRate()) > 1;
			b32 left_drove_half = Absolute(left_distance) > (Absolute(runtime->left_distance) / 2);

			b32 prev_right_remaining = runtime->right_remaining;
			runtime->right_remaining = (runtime->right_distance > 0) ? ((right_distance + runtime->right_distance) > 0.01) : ((right_distance + runtime->right_distance) < -0.01);
			b32 right_moving = Absolute(robot->right_encoder.GetRate()) > 1;
			b32 right_drove_half = Absolute(right_distance) > (Absolute(runtime->right_distance) / 2);

			b32 not_moving = (!left_moving || !right_moving) && (left_drove_half && right_drove_half);

			if(not_moving)
			{
				runtime->left_remaining = false;
				runtime->right_remaining = false;
			}

			r32 left_speed = (prev_left_remaining && !runtime->left_remaining) ? 0 : (runtime->left_distance > 0 ? speed : -speed);
			r32 right_speed = (prev_right_remaining && !runtime->right_remaining) ? 0 : (runtime->right_distance > 0 ? speed : -speed);

			SmartDashboard::PutNumber("Left Remaining", (runtime->left_distance > 0) ? (left_distance + runtime->left_distance) : (left_distance + runtime->left_distance));
			SmartDashboard::PutNumber("Right Remaining", (runtime->right_distance > 0) ? (right_distance + runtime->right_distance) : (right_distance + runtime->right_distance));

			TankDrive(robot, left_speed, right_speed);
		}

		runtime->wait_time -= curr_time;
		runtime->wait_time = (runtime->wait_time < 0.0) ? 0.0 : runtime->wait_time;
	}
}

struct auto_save_file_header
{
	char name[32];
	u32 block_count;
};

void SaveAutoFile(TestRobot *robot, const char *file_name, u8 slot)
{
	FILE *auto_save_file = fopen(file_name, "wb");

	if(auto_save_file)
	{
		auto_save_file_header save_file_header = {};
		strcpy(save_file_header.name, robot->auto_name[slot]);
		save_file_header.block_count = robot->auto_length[slot];

		fwrite(&save_file_header, sizeof(save_file_header), 1, auto_save_file);

		fwrite(robot->auto_program[slot], sizeof(FunctionBlock) * save_file_header.block_count, 1, auto_save_file);
		fclose(auto_save_file);
	}
	else
	{
		SendDebugMessagePacket(&robot->net_state, "Save Error", true);
	}
}

void LoadAutoFile(TestRobot *robot, const char *file_name, u8 slot)
{
	FILE *auto_save_file = fopen(file_name, "rb");

	if(auto_save_file)
	{
		auto_save_file_header save_file_header = {};
		fread(&save_file_header, sizeof(save_file_header), 1, auto_save_file);

		robot->auto_length[slot] = save_file_header.block_count;
		strcpy(robot->auto_name[slot], save_file_header.name);

		robot->auto_program[slot] = (FunctionBlock *) malloc(sizeof(FunctionBlock) * save_file_header.block_count);
		fread(robot->auto_program[slot], sizeof(FunctionBlock) * save_file_header.block_count, 1, auto_save_file);

		SendDebugMessagePacket(&robot->net_state, (save_file_header.block_count > 0) ? "Load Success" : "Empty", true);

		fclose(auto_save_file);
	}
	else
	{
		SendDebugMessagePacket(&robot->net_state, "Load Error", true);
	}
}
