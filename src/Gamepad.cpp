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
	
bool Gamepad::GetLBTrigger()
{
	return this->Controller.GetRawButton(5);
}

bool Gamepad::GetRBTrigger()
{
	return this->Controller.GetRawButton(6);
}

bool Gamepad::GetLTTrigger()
{
	float v = this->Controller.GetThrottle();

	if((v > 0.0f) && (v < 1.0f))
	{
		return true;
	}
	
	return false;
}

bool Gamepad::GetRTTrigger()
{
	float v = this->Controller.GetThrottle();

	if((v < 0.0f) && (v > -1.0f))
	{
		return true;
	}
	
	return false;
}