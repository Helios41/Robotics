#include "Robot.h"
#include "math.h"

Robot::Robot(void): 
	Controller(Globals::Controller),
	Solenoid(Globals::SolenoidForward, Globals::SolenoidReverse),
   Drive(Globals::LeftFrontWheel, Globals::RightFrontWheel, Globals::LeftBackWheel, Globals::RightBackWheel),
   Elevator(Globals::Elevator),
   TopSwitch(Globals::TopSwitch),
   BottomSwitch(Globals::BottomSwitch),
   LeftIntake(Globals::LeftIntake),
   RightIntake(Globals::RightIntake)
{
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

      this->Drive.DriveForward(leftSpeed);
		Wait(0.005);
	}
}

void Robot::OperatorControl(void)
{
	SmartDashboard::PutString("State", "Teleop");
	
	while(IsOperatorControl() && IsEnabled())
	{	
		float controllerY = this->Controller.GetLAnalogY();
		float controllerX = this->Controller.GetLAnalogX();
      float rotate = 0.0f;
   
		if(this->Controller.GetXButton())
		{
			this->Solenoid.Forward();
		}
		else if(this->Controller.GetYButton())
		{
			this->Solenoid.Reverse();
		}
      else
      {
         this->Solenoid.Off();
      }
	
      //intake motors
   
      if((this->Controller.GetDPad() == DPadValue::DPadUp) && TopSwitch.IsOff())
      {
         this->Elevator.Set(1.0f);
      }
      else if((this->Controller.GetDPad() == DPadValue::DPadDown) && BottomSwitch.IsOff())
      {
         this->Elevator.Set(-1.0f);
      }
      else
      {
         this->Elevator.Set(0.0f);
      }
   
		if(this->Controller.GetLBumper())
		{
			rotate = -0.5f;
		}
      
		if(this->Controller.GetRBumper())
		{
			rotate = 0.5f;
		}
	
		this->Drive.Drive(controllerX, controllerY, rotate);
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
