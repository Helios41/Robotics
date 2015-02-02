#include "Gamepad.h"

Gamepad::Gamepad(int port):
	Controller(port)
{

}

Gamepad::~Gamepad(void)
{

}

bool Gamepad::GetYButton(void);
{
	return this->Controller.GetRawButton(Gamepad::YButton);
}

bool Gamepad::GetXButton(void)
{
	return this->Controller.GetRawButton(Gamepad::XButton);
}

bool Gamepad::GetAButton(void)
{
	return this->Controller.GetRawButton(Gamepad::AButton);
}

bool Gamepad::GetBButton(void)
{
	return this->Controller.GetRawButton(Gamepad::BButton);
}
	
bool Gamepad::GetLBumper(void)
{
	return this->Controller.GetRawButton(Gamepad::LBButton);
}

bool Gamepad::GetRBumper(void)
{
	return this->Controller.GetRawButton(Gamepad::LBButton);
}

bool Gamepad::GetLTrigger(void)
{
	float v = this->Controller.GetRawAxis(Gamepad::TAxis);

	if((v > 0.0f) && (v < 1.0f))
	{
		return true;
	}
	
	return false;
}

bool Gamepad::GetRTrigger(void)
{
	float v = this->Controller.GetRawAxis(Gamepad::TAxis);

	if((v < 0.0f) && (v > -1.0f))
	{
		return true;
	}
	
	return false;
}

float Gamepad::GetLAnalogX(void)
{
	return this->Controller.GetRawAxis(Gamepad::LXAxis);
}

float Gamepad::GetLAnalogY(void)
{
	return this->Controller.GetRawAxis(Gamepad::LYAxis);
}

float Gamepad::GetRAnalogX(void)
{
	return this->Controller.GetRawAxis(Gamepad::RXAxis);
}

float Gamepad::GetRAnalogY(void)
{
	return this->Controller.GetRawAxis(Gamepad::RYAxis);
}

DPadValue Gamepad::GetDPad(void)
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

bool Gamepad::GetLButton(void)
{
	return this->Controller.GetRawButton(Gamepad::LButton);
}

bool Gamepad::GetRButton(void)
{
	return this->Controller.GetRawButton(Gamepad::RButton);
}
