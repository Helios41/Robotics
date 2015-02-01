#ifndef GAMEPAD_H_
#define GAMEPAD_H_

#include "WPILib.h"

enum DPadValue
{
	DPadUp,
	DPadDown,
	DPadLeft,
	DPadRight,
	DPadNone
};

class Gamepad
{
private:
	Joystick Controller;
	
public:
	static const int YButton = 4;
	static const int XButton = 3;
	static const int AButton = 1;
	static const int BButton = 2;

	static const int LBButton = 5;
	static const int RBButton = 6;
	
	static const int TAxis = 3;
	
	static const int LXAxis = 1;
	static const int LYAxis = 2;
	static const int LButton = 9;
	
	static const int RXAxis = 4;
	static const int RYAxis = 5;
	static const int RButton = 10;
	
	static const int DXAxis = 6;
	static const int DYAxis = 7;
	
	Gamepad(int port);
	~Gamepad();
	
	bool GetYButton();
	bool GetXButton();
	bool GetAButton();
	bool GetBButton();
	
	bool GetLBumper();
	bool GetRBumper();
	
	bool GetLTrigger();
	bool GetRTrigger();

    bool GetLButton();
    bool GetRButton();
	
	float GetLAnalogX();
	float GetLAnalogY();
	
	float GetRAnalogX();
	float GetRAnalogY();
	
	DPadValue GetDPad();
};

#endif
