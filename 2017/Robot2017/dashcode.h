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
									ErrorFunction e_function, char *name)
{
	RobotHardware result = {};
	result.multiplier = 1.0f;
	result.encoder_motor.motor = new Victor(motor_port);
	result.encoder_motor.encoder = new Encoder(encoder_a, encoder_b);
	result.encoder_motor.error_function = e_function;
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
		hardware->encoder_motor.error_function.target = hardware->multiplier * value;
	}
}

void ArcadeDrive(TestRobot *robot, r32 power, r32 rotate)
{
	r32 right_speed = power + rotate;
	r32 left_speed = power - rotate;

	right_speed = Clamp(-1, robot->drive_multiplier * right_speed, 1);
	left_speed = Clamp(-1, robot->drive_multiplier * left_speed, 1);

	robot->right_front.Set(right_speed);
	robot->right_back.Set(right_speed);

	robot->left_front.Set(-left_speed);
	robot->left_back.Set(-left_speed);
}

void ExecuteBlocklangFunction(FunctionBlock function, TestRobot *robot)
{
	RobotHardware *hardware_array = robot->hardware;
	Joystick *driver_controller = &robot->driver_controller;
	Joystick *op_controller = &robot->op_controller;

	switch(function.type)
	{
		case FunctionBlock_Wait:
			Wait(function.wait.duration);
			break;
		
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
			ArcadeDrive(robot, function.arcade_drive_const.power, function.arcade_drive_const.rotate);
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

		/*
		case FunctionBlock_DriveDistance:
		case FunctionBlock_GotoPosition:
		*/
	}
}
