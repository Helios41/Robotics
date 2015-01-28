#include "Gamepad.h"

Gamepad::Gamepad(int port):
	Controller(port)
{

}

Gamepad::~Gamepad()
{

}

bool Gamepad::GetYButton();
{
	return this->Controller.GetRawButton(Gamepad::YButton);
}

bool Gamepad::GetXButton()
{
	return this->Controller.GetRawButton(Gamepad::XButton);
}

bool Gamepad::GetAButton()
{
	return this->Controller.GetRawButton(Gamepad::AButton);
}

bool Gamepad::GetBButton()
{
	return this->Controller.GetRawButton(Gamepad::BButton);
}
	
bool Gamepad::GetLBumper()
{
	return this->Controller.GetRawButton(Gamepad::LBButton);
}

bool Gamepad::GetRBumper()
{
	return this->Controller.GetRawButton(Gamepad::LBButton);
}

bool Gamepad::GetLTrigger()
{
	float v = this->Controller.GetRawAxis(Gamepad::TAxis);

	if((v > 0.0f) && (v < 1.0f))
	{
		return true;
	}
	
	return false;
}

bool Gamepad::GetRTrigger()
{
	float v = this->Controller.GetRawAxis(Gamepad::TAxis);

	if((v < 0.0f) && (v > -1.0f))
	{
		return true;
	}
	
	return false;
}

float Gamepad::GetLAnalogX()
{
	return this->Controller.GetRawAxis(Gamepad::LXAxis);
}

float Gamepad::GetLAnalogY()
{
	return this->Controller.GetRawAxis(Gamepad::LYAxis);
}

float Gamepad::GetRAnalogX()
{
	return this->Controller.GetRawAxis(Gamepad::RXAxis);
}

float Gamepad::GetRAnalogY()
{
	return this->Controller.GetRawAxis(Gamepad::RYAxis);
}

DPadValue Gamepad::GetDPad()
{
	float x = this->Controller.GetRawAxis(Gamepad::DXAxis);
	float y = this->Controller.GetRawAxis(Gamepad::DYAxis);
	
	if(x > 0.0f)
		return DPadValue::DPadRight;
		
	if(x < 0.0f)
		return DPadValue::DPadLeft;
		
	if(y < 0.0f)
		return DPadValue::DPadDown;
		
	if(y > 0.0f)
		return DPadValue::DPadUp;
		
	return DPadValue::DPadNone;
}

bool Gamepad::GetLButton()
{
	return this->Controller.GetRawButton(Gamepad::LButton);
}

bool Gamepad::GetRButton()
{
	return this->Controller.GetRawButton(Gamepad::RButton);
}
