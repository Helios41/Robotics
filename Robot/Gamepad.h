#ifndef GAMEPAD_H_
#define GAMEPAD_H_

#include "WPILib.h"
#include <functional>

enum DPadValue
{
	DPadTop,
	DPadBottom,
	DPadLeft,
	DPadRight,
	DPadTopLeft,
	DPadTopRight,
	DPadBottomLeft,
	DPadBottomRight,
	DPadNone
};

class GamepadFloat
{
private:
	std::function<float (void)> Callback;

public:
	GamepadFloat(std::function<float (void)> Callback);
	~GamepadFloat(void);

	float operator()();
	float Invert(void);
};

class GamepadBool
{
private:
	std::function<bool (void)> Callback;

public:
	GamepadBool(std::function<bool (void)> Callback);
	~GamepadBool(void);

	bool operator()();
};

class GamepadDPad
{
private:
	std::function<DPadValue (void)> Callback;

public:
	GamepadDPad(std::function<DPadValue (void)> Callback);
	~GamepadDPad(void);

	DPadValue operator()();
};

class GamepadGlobals
{
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
};

class Gamepad
{
private:
	Joystick Controller;
	
public:
	Gamepad(int port);
	~Gamepad(void);

	GamepadBool YButton;
	GamepadBool XButton;
	GamepadBool AButton;
	GamepadBool BButton;

	GamepadBool LBumper;
	GamepadBool RBumper;

	GamepadBool LTrigger;
	GamepadBool RTrigger;

	GamepadBool LButton;
	GamepadBool RButton;

	GamepadFloat LAnalogX;
	GamepadFloat LAnalogY;

	GamepadFloat RAnalogX;
	GamepadFloat RAnalogY;

	GamepadDPad DPad;

	void Rumble(float rumble);
};

#endif
