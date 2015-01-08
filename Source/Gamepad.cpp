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

float Gamepad::GetLAnalogX()
{
	return this->Controller.GetRawAxis(1);
}

float Gamepad::GetLAnalogY()
{
	return this->Controller.GetRawAxis(2);
}

float Gamepad::GetRAnalogX()
{
	return this->Controller.GetRawAxis(4);
}

float Gamepad::GetRAnalogY()
{
	return this->Controller.GetRawAxis(5);
}

int Gamepad::GetDPad()
{
	float x = this->Controller.GetRawAxis(6);
	float y = this->Controller.GetRawAxis(7);
	
	if(x > 0.0f)
		return DPAD_RIGHT;
		
	if(x < 0.0f)
		return DPAD_LEFT;
		
	if(y < 0.0f)
		return DPAD_DOWN;
		
	if(y > 0.0f)
		return DPAD_UP;
		
	return DPAD_NONE;
}

bool Gamepad::GetLButton()
{
	return this->Controller.GetRawButton(9);
}

bool Gamepad::GetRButton()
{
	return this->Controller.GetRawButton(10);
}
