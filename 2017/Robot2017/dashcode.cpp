RobotHardware HWMotor(u32 port, char *name)
{
	RobotHardware result = {};
	result.multiplier = 1.0f;
	result.motor = new Victor(port);
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

void ExecuteBlocklangFunction(FunctionBlock *function, TestRobot *robot)
{
	RobotHardware *robot_hardware = robot->hardware;
	switch(function->type)
	{
		case FunctionBlock_Wait:
			Wait(function->wait.duration);
			break;
		
		case FunctionBlock_SetFloat:
		{
			RobotHardware *hardware = hardware_array + function->set_float.hardware_index;
			
			if(hardware->type == Hardware_Motor)
			{
				hardware->motor->Set(hardware->multiplier * function->set_float.value);
			}
			else if(hardware->type == Hardware_EncoderMotor)
			{
				hardware->encoder_motor.error_function.target = hardware->multiplier * function->set_float.value;
			}
		}
		break;
		
		case FunctionBlock_SetBool:
		{
			RobotHardware *hardware = hardware_array + function->set_bool.hardware_index;
			if(hardware->type == Hardware_Solenoid)
			{
				switch(function->set_bool.op)
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
		
		case FunctionBlock_SetMultiplier:
		{
			RobotHardware *hardware = hardware_array + function->set_bool.hardware_index;
			hardware->multiplier = function->set_multiplier.value;
		}
		break;
		
		case FunctionBlock_Vision:
		{
			
		}
		break;
		
		case FunctionBlock_ArcadeDrive:
		{
			robot->drive.ArcadeDrive(robot->drive_multiplier * function->arcade_drive.power,
									 robot->drive_multiplier * function->arcade_drive.rotate);
		}
		break;
		
		case FunctionBlock_SetDriveMultiplier:
		{
			robot->drive_multiplier = function->set_drive_multipler.value;
		}
		break;
	}
}