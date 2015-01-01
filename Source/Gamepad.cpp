#include "Gamepad.h"

bool Gamepad::GetYButton();
{
	return this->Controller.GetRawButton(4);
}

bool Gamepad::GetXButton()
{
	return this->Controller.GetRawButton(3);
}

bool Gamepad::GetAButton()
{
	return this->Controller.GetRawButton(1);
}

bool Gamepad::GetBButton()
{
	return this->Controller.GetRawButton(2);
}
	
bool Gamepad::GetLBumper()
{
	return this->Controller.GetRawButton(5);
}

bool Gamepad::GetRBumper()
{
	return this->Controller.GetRawButton(6);
}

bool Gamepad::GetLTrigger()
{
	float v = this->Controller.GetRawAxis(3);

	if((v > 0.0f) && (v < 1.0f))
	{
		return true;
	}
	
	return false;
}

bool Gamepad::GetRTrigger()
{
	float v = this->Controller.GetRawAxis(3);

	if((v < 0.0f) && (v > -1.0f))
	{
		return true;
	}
	
	return false;
}

float GetLeftAnalogX()
{
	return this->Controller.GetRawAxis(1);
}

float GetLeftAnalogY()
{
	return this->Controller.GetRawAxis(2);
}

float GetRightAnalogX()
{
	return this->Controller.GetRawAxis(4);
}

float GetRightAnalogY()
{
	return this->Controller.GetRawAxis(5);
}