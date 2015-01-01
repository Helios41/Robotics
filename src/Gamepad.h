#ifndef GAMEPAD_H_
#define GAMEPAD_H_

#include "BuiltIns.h"

class Gamepad
{
private:
	Joystick Controller;
	
public:
	Gamepad(int port) : Controller(port) {};
	~Gamepad() {};
	
	bool GetYButton();
	bool GetXButton();
	bool GetAButton();
	bool GetBButton();
	
	bool GetLBTrigger();
	bool GetRBTrigger();
	bool GetLTTrigger();
	bool GetRTTrigger();
};

#endif