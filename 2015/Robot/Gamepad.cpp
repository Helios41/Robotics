#include "Gamepad.h"

GamepadFloat::GamepadFloat(std::function<float (void)> Callback)
{
	this->Callback = Callback;
}

GamepadFloat::~GamepadFloat(void) { }

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

GamepadBool::~GamepadBool(void) { }

bool GamepadBool::operator()()
{
	return this->Callback();
}

GamepadDPad::GamepadDPad(std::function<DPadValue (void)> Callback)
{
	this->Callback = Callback;
}

GamepadDPad::~GamepadDPad(void) { }

DPadValue GamepadDPad::operator()()
{
	return this->Callback();
}

#define RB(PORT) [this](){ return this->Controller.GetRawButton(PORT); }
#define RA(PORT) [this](){ return this->Controller.GetRawAxis(PORT); }

Gamepad::Gamepad(int port):
	Controller(port),
	YButton(RB(GamepadGlobals::YButton)),
	XButton(RB(GamepadGlobals::XButton)),
	AButton(RB(GamepadGlobals::AButton)),
	BButton(RB(GamepadGlobals::BButton)),
	LBumper(RB(GamepadGlobals::LBButton)),
	RBumper(RB(GamepadGlobals::RBButton)),
	LTrigger([this]()
   {
		float v = this->Controller.GetRawAxis(GamepadGlobals::LTAxis);
		if(v > 0.05f)
		{
			return true;
		}
		return false;
	}),
	RTrigger([this]()
   {
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
	DPad([this]()
   {
      switch(this->Controller.GetPOV())
      {
         case 0:
            return DPadValue::DPadTop;
            
         case 45:
            return DPadValue::DPadTopRight;
         
         case 90:
            return DPadValue::DPadRight;
            
         case 135:
            return DPadValue::DPadBottomRight;
            
         case 180:
            return DPadValue::DPadBottom;
            
         case 225:
            return DPadValue::DPadBottomLeft;
            
         case 270:
            return DPadValue::DPadLeft;
            
         case 315:
            return DPadValue::DPadTopLeft;
            
         default:
            return DPadValue::DPadNone
      }
	})
{

}

#undef RB
#undef RA

Gamepad::~Gamepad(void) { }

void Gamepad::Rumble(float rumble)
{
	this->Controller.SetRumble(Joystick::RumbleType::kLeftRumble, rumble);
	this->Controller.SetRumble(Joystick::RumbleType::kRightRumble, rumble);
}
