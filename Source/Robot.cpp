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
	
}

Robot::~Robot(void)
{

}

void Robot::Disabled(void)
{
	while(IsDisabled())
	{
		
	}
}

void Robot::Autonomous(void)
{
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
	while(IsOperatorControl() && IsEnabled())
	{	
		float controllerY = this->Controller.GetLAnalogY();
		float controllerX = this->Controller.GetLAnalogX();
      float rotate = this->Controller.GetRAnalogX();
   
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
	
      if(this->Controller.GetAButton())
      {
         this->LeftIntake.Set(1.0f);
         this->RightIntake.Set(-1.0f);
      }
      else if(this->Controller.GetBButton())
      {
         this->LeftIntake.Set(-1.0f);
         this->RightIntake.Set(1.0f);
      }
      else
      {
         this->LeftIntake.Set(0.0f);
         this->RightIntake.Set(0.0f);
      }
   
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
	
		this->Drive.Drive(controllerX, controllerY, rotate);
		Wait(0.005);
	}
}

START_ROBOT_CLASS(Robot);
