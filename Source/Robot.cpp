#include "Robot.h"
#include "math.h"

Robot::Robot(void): 
	Controller(Globals::Controller),
	Solenoid(Globals::SolenoidForward, Globals::SolenoidReverse)
{
#ifdef KIT_DRIVE
	this->Drive(Globals::LeftFrontWheel, Globals::RightFrontWheel);
#endif

#ifdef MECHANUM_DRIVE
	this->Drive(Globals::LeftFrontWheel, Globals::RightFrontWheel, Globals::LeftBackWheel, Globals::RightBackWheel);
#endif

	this->Drive.SetExpiration(0.1);
	SmartDashboard::PutString("State", "Unknown");
}

Robot::~Robot(void)
{

}

void Robot::Disabled(void)
{
	SmartDashboard::PutString("State", "Disabled");
	
	while(IsDisabled())
	{
		
	}
}

void Robot::Autonomous(void)
{
	this->Drive.SetSafetyEnabled(false);
	SmartDashboard::PutString("State", "Autonomous");
	
	float leftSpeed = 0.0f;
	float rightSpeed = 0.0f;
	
	while(IsAutonomous() && IsEnabled())
	{
		if((leftSpeed < 1.0f) && (rightSpeed < 1.0f))
		{
			leftSpeed += 0.01f;
			rightSpeed += 0.01f;
		}
		else
		{
			leftSpeed = -0.3f;
			rightSpeed = -0.3f;
		}
		
		this->Drive.TankDrive(leftSpeed, rightSpeed);
		Wait(0.005);
	}
}

void Robot::OperatorControl(void)
{
	this->Drive.SetSafetyEnabled(true);
	SmartDashboard::PutString("State", "Teleop");
	
	while(IsOperatorControl() && IsEnabled())
	{	
		float controllerY = this->Controller.GetLAnalogY();
		float controllerX = this->Controller.GetLAnalogX();
	
		if(this->Controller.GetXButton())
		{
			this->Solenoid.Forward();
		}
		else
		{
			this->Solenoid.Off();
		}

#ifdef MECHANUM_DRIVE
		float rotate = 0.0f;
	
		if(this->Controller.GetLBumper())
		{
			rotate = -0.5f;
		}
		
		if(this->Controller.GetRBumper())
		{
			rotate = 0.5f;
		}
	
		this->Drive.Drive(controllerX, controllerY, rotate);
#endif

#ifdef KIT_DRIVE	
		float leftSpeed = controllerY;
		float rightSpeed = controllerY;
	
		if(controllerX > 0.0)
		{
			rightSpeed *= 1.0 / controllerX;
		}
		else if(controllerX < 0.0)
		{
			leftSpeed *= 1.0 / fabs(controllerX);
		}
	
		this->Drive.TankDrive(leftSpeed, rightSpeed);
#endif
		
		Wait(0.005);
	}
}

void Robot::Test(void)
{
	SmartDashboard::PutString("State", "Test");
	
	while(IsTest() && IsEnabled())
	{
		
	}
}

START_ROBOT_CLASS(Robot);
