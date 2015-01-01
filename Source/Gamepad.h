#ifndef GAMEPAD_H_
#define GAMEPAD_H_

#include "BuiltIns.h"

/* 
	TODO:
	-Map D-Pad
	-Fix not being able to check for both triggers
*/

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
	
	bool GetLBumper();
	bool GetRBumper();
	
	bool GetLTrigger();
	bool GetRTrigger();
	
	float GetLeftAnalogX();
	float GetLeftAnalogY();
	
	float GetRightAnalogX();
	float GetRightAnalogY();
};

#endif