#ifndef ROBOT_H_
#define ROBOT_H_

#include "WPILib.h"
#include "Gamepad.h"
#include "MechanumDrive.h"
#include "Globals.h"
#include "PneumaticSolenoid.h"
#include "Switch.h"

class Robot: public SampleRobot
{
public:
	PneumaticSolenoid Solenoid;
	Gamepad Controller;
	MechanumDrive Drive;
   Victor Elevator;
   Switch TopSwitch;
   Switch BottomSwitch;
   Victor LeftIntake;
   Victor RightIntake;
   
	Robot(void);
	~Robot(void);
	
	void Disabled(void);
	void Autonomous(void);
	void OperatorControl(void);
	void Test(void);
};

#endif
