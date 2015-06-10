#include "Gamepad.h"

GamepadFloat::GamepadFloat(std::function<float (void)> Callback)
{
	this->Callback = Callback;
}

GamepadFloat::~GamepadFloat(void)
{

}

float GamepadFloat::operator()()
{
	return this->Callback();
}

float GamepadFloat::Invert(void)
{
	return this->Callback() * -1;
}

GamepadBool::GamepadBool(std::function<bool (void)> Callback)
{
	this->Callback = Callback;
}

GamepadBool::~GamepadBool(void)
{

}

bool GamepadBool::operator()()
{
	return this->Callback();
}

GamepadDPad::GamepadDPad(std::function<DPadValue (void)> Callback)
{
	this->Callback = Callback;
}

GamepadDPad::~GamepadDPad(void)
{

}

DPadValue GamepadDPad::operator()()
{
	return this->Callback();
}

#define RB(PORT) [this](){return this->Controller.GetRawButton(PORT);}
#define RA(PORT) [this](){return this->Controller.GetRawAxis(PORT);}

Gamepad::Gamepad(int port):
	Controller(port),
	YButton(RB(GamepadGlobals::YButton)),
	XButton(RB(GamepadGlobals::XButton)),
	AButton(RB(GamepadGlobals::AButton)),
	BButton(RB(GamepadGlobals::BButton)),
	LBumper(RB(GamepadGlobals::LBButton)),
	RBumper(RB(GamepadGlobals::RBButton)),
	LTrigger([this](){
						float v = this->Controller.GetRawAxis(GamepadGlobals::LTAxis);
						if(v > 0.05f)
						{
							return true;
						}
						return false;
					}),
	RTrigger([this](){
						float v = this->Controller.GetRawAxis(GamepadGlobals::RTAxis);
						if(v > 0.05f)
						{
							return true;
						}
						return false;
					}),
	LButton(RB(GamepadGlobals::LButton)),
	RButton(RB(GamepadGlobals::RButton)),
	LAnalogX(RA(GamepadGlobals::LXAxis)),
	LAnalogY(RA(GamepadGlobals::LYAxis)),
	RAnalogX(RA(GamepadGlobals::RXAxis)),
	RAnalogY(RA(GamepadGlobals::RYAxis)),
	DPad([this](){
					int dpad = this->Controller.GetPOV();

					if(dpad == 0)
						return DPadValue::DPadTop;

					if(dpad == 45)
						return DPadValue::DPadTopRight;

					if(dpad == 90)
						return DPadValue::DPadRight;

					if(dpad == 135)
						return DPadValue::DPadBottomRight;

					if(dpad == 180)
						return DPadValue::DPadBottom;

					if(dpad == 225)
						return DPadValue::DPadBottomLeft;

					if(dpad == 270)
						return DPadValue::DPadLeft;

					if(dpad == 315)
						return DPadValue::DPadTopLeft;

					return DPadValue::DPadNone;
				})
{

}

Gamepad::~Gamepad(void)
{

}

bool Gamepad::GetYButton(void)
{
	return this->Controller.GetRawButton(GamepadGlobals::YButton);
}

bool Gamepad::GetXButton(void)
{
	return this->Controller.GetRawButton(GamepadGlobals::XButton);
}

bool Gamepad::GetAButton(void)
{
	return this->Controller.GetRawButton(GamepadGlobals::AButton);
}

bool Gamepad::GetBButton(void)
{
	return this->Controller.GetRawButton(GamepadGlobals::BButton);
}
	
bool Gamepad::GetLBumper(void)
{
	return this->Controller.GetRawButton(GamepadGlobals::LBButton);
}

bool Gamepad::GetRBumper(void)
{
	return this->Controller.GetRawButton(GamepadGlobals::RBButton);
}

bool Gamepad::GetLTrigger(void)
{
	float v = this->Controller.GetRawAxis(GamepadGlobals::LTAxis);

	if(v > 0.05f)
	{
		return true;
	}

	return false;
}

bool Gamepad::GetRTrigger(void)
{
	float v = this->Controller.GetRawAxis(GamepadGlobals::RTAxis);

	if(v > 0.05f)
	{
		return true;
	}
	
	return false;
}

float Gamepad::GetLAnalogX(void)
{
	return this->Controller.GetRawAxis(GamepadGlobals::LXAxis);
}

float Gamepad::GetLAnalogY(void)
{
	return this->Controller.GetRawAxis(GamepadGlobals::LYAxis);
}

float Gamepad::GetRAnalogX(void)
{
	return this->Controller.GetRawAxis(GamepadGlobals::RXAxis);
}

float Gamepad::GetRAnalogY(void)
{
	return this->Controller.GetRawAxis(GamepadGlobals::RYAxis);
}

DPadValue Gamepad::GetDPad(void)
{
	int dpad = this->Controller.GetPOV();

	if(dpad == 0)
		return DPadValue::DPadTop;

	if(dpad == 45)
		return DPadValue::DPadTopRight;

	if(dpad == 90)
		return DPadValue::DPadRight;

	if(dpad == 135)
		return DPadValue::DPadBottomRight;

	if(dpad == 180)
		return DPadValue::DPadBottom;

	if(dpad == 225)
		return DPadValue::DPadBottomLeft;

	if(dpad == 270)
		return DPadValue::DPadLeft;

	if(dpad == 315)
		return DPadValue::DPadTopLeft;

	return DPadValue::DPadNone;
}

bool Gamepad::GetLButton(void)
{
	return this->Controller.GetRawButton(GamepadGlobals::LButton);
}

bool Gamepad::GetRButton(void)
{
	return this->Controller.GetRawButton(GamepadGlobals::RButton);
}

void Gamepad::Rumble(float rumble)
{
	this->Controller.SetRumble(Joystick::RumbleType::kLeftRumble, rumble);
	this->Controller.SetRumble(Joystick::RumbleType::kRightRumble, rumble);
}
