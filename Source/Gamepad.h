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
	
   static const int LTAxis = 2;
	static const int RTAxis = 3;
   
	static const int LXAxis = 0;
	static const int LYAxis = 1;
	static const int LButton = 9;
	
	static const int RXAxis = 4;
	static const int RYAxis = 5;
	static const int RButton = 10;
	
	static const int DXAxis = 6;
	static const int DYAxis = 7;
	
	Gamepad(int port);
	~Gamepad(void);
	
	bool GetYButton(void);
	bool GetXButton(void);
	bool GetAButton(void);
	bool GetBButton(void);
	
	bool GetLBumper(void);
	bool GetRBumper(void);
	
	bool GetLTrigger(void);
	bool GetRTrigger(void);

    bool GetLButton(void);
    bool GetRButton(void);
	
	float GetLAnalogX(void);
	float GetLAnalogY(void);
	
	float GetRAnalogX(void);
	float GetRAnalogY(void);
	
	DPadValue GetDPad(void);
   
   void Rumble(float rumble);
};

#endif
