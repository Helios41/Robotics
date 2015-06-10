#include "Robot.h"

Robot::Robot(void): 
	DriverController(Globals::Driver),
	OperatorController(Globals::Operator),
	Solenoid(Globals::SolenoidForward, Globals::SolenoidReverse),
	Drive(Globals::LeftFrontWheel, Globals::RightFrontWheel, Globals::LeftBackWheel, Globals::RightBackWheel),
	Elevator(Globals::Elevator),
	TopSwitch(Globals::TopSwitch),
	BottomSwitch(Globals::BottomSwitch),
	LeftIntake(Globals::LeftIntake),
	RightIntake(Globals::RightIntake),
	Holder(Globals::HolderForward, Globals::HolderReverse)
{

}

Robot::~Robot(void)
{

}

void Robot::RobotInit(void)
{
	CustomDashboard::Init("Rattles-Net");
}

void Robot::Disabled(void)
{
	this->Drive.Drive(0.0f, 0.0f);
	this->Solenoid.Forward();

	while(IsDisabled())
	{
		
	}
}

void Robot::Autonomous(void)
{
	this->Solenoid.Reverse();
	this->Holder.Forward();
	CustomDashboard::Set("HolderState", "holder", "open");

	std::string autoSizeStr = CustomDashboard::Get("AutonomousEditor", "size");

	if(autoSizeStr == "NOPE")
	{
		/*if(IsAutonomous() && IsEnabled())
		{
			this->Elevator.Set(0.40f);
			Wait(2.0);
			this->Elevator.Set(0.20f);

			this->Drive.DriveBackward(0.30f);
			Wait(4.2);
			this->Drive.Drive(0.0f, 0.0f);
		}*/
	}
	else
	{
		int autoSize = atoi(autoSizeStr.c_str());

		int *commands = new int[autoSize];
		double *times = new double[autoSize];
		double *powers = new double[autoSize];

		for(int i = 0; i < autoSize; ++i)
		{
			commands[i] = atoi(CustomDashboard::GetArray("AutonomousEditor", "command", i).c_str());
			times[i] = atof(CustomDashboard::GetArray("AutonomousEditor", "time", i).c_str());
			powers[i] = atof(CustomDashboard::GetArray("AutonomousEditor", "power", i).c_str());
		}

		if(IsAutonomous() && IsEnabled())
		{
			for(int i = 0; (i < autoSize) && (i < 10); ++i)
			{
				switch(commands[i])
				{
				case Autonomous::DriveBackwards:
					this->Drive.DriveBackward(powers[i] / 100);
					break;

				case Autonomous::DriveForwards:
					this->Drive.DriveForward(powers[i] / 100);
					break;

				case Autonomous::DriveLeft:
					this->Drive.DriveLeft(powers[i] / 100);
					break;

				case Autonomous::DriveRight:
					this->Drive.DriveRight(powers[i] / 100);
					break;

				case Autonomous::ElevatorUp:
					this->Elevator.Set(powers[i] / 100);
					break;

				case Autonomous::ElevtorDown:
					this->Elevator.Set((-powers[i]) / 100);
					break;
				}

				Wait(times[i]);
				this->Drive.Drive(0.0f, 0.0f);
			}

			delete commands;
			delete powers;
			delete times;
		}
	}
}

void Robot::OperatorControl(void)
{
	bool SolenoidExtended = false;
	bool HolderExtended = true;

	SensitiveListener SolenoidToggle(
	[this]()
	{
		return this->DriverController.YButton();
	}, 30);

	SensitiveListener HolderToggle(
	[this]()
	{
		return this->OperatorController.GetRawButton(1);
	}, 30);

	this->Drive.SetRotateLimit(0.5f);

	while(IsOperatorControl() && IsEnabled())
	{	
		float controllerY = this->DriverController.LAnalogY.Invert();
		float controllerX = this->DriverController.LAnalogX.Invert();
		float rotate = this->DriverController.RAnalogX.Invert();
		float ElevatorSpeed = this->OperatorController.GetY();

		if(SolenoidToggle.GetValue())
		{
			SolenoidExtended = !SolenoidExtended;
		}

		if(SolenoidExtended)
		{
			this->Solenoid.Forward();
		}
		else
		{
			this->Solenoid.Reverse();
		}

		if(HolderToggle.GetValue())
		{
			HolderExtended = !HolderExtended;
		}

		if(HolderExtended)
		{
			this->Holder.Forward();
			CustomDashboard::Set("HolderState", "holder", "open");
		}
		else
		{
			this->Holder.Reverse();
			CustomDashboard::Set("HolderState", "holder", "closed");
		}

		if(this->DriverController.AButton())
		{
			this->LeftIntake.Set(-1.0f);
			this->RightIntake.Set(1.0f);
		}
		else if(this->DriverController.BButton())
		{
			this->LeftIntake.Set(1.0f);
			this->RightIntake.Set(-1.0f);
		}
		else if(this->DriverController.XButton())
		{
			this->LeftIntake.Set(1.0f);
			this->RightIntake.Set(1.0f);
		}
		else
		{
			this->LeftIntake.Set(0.0f);
			this->RightIntake.Set(0.0f);
		}

      	if(this->OperatorController.GetRawButton(11) && this->TopSwitch.IsOff())
      	{
      		this->Elevator.Set(0.25f);
      	}
      	else if(this->OperatorController.GetRawButton(12) && this->TopSwitch.IsOff())
      	{
      		this->Elevator.Set(0.40f);
      	}
		else if((ElevatorSpeed > 0.0f) && this->TopSwitch.IsOff())
		{
			this->Elevator.Set(ElevatorSpeed);
		}
		else if((ElevatorSpeed < 0.0f) && this->BottomSwitch.IsOff())
		{
			this->Elevator.Set(ElevatorSpeed + 0.1f);
		}
		else
		{
			this->Elevator.Set(0.0f);
		}

		if(!this->DriverController.LBumper())
		{
			this->Drive.SetMultiple(0.5f);
		}

		this->Drive.Drive(controllerX, controllerY, rotate);
		this->Drive.ResetMultiple();
		Wait(0.005);
	}
}

START_ROBOT_CLASS(Robot);
