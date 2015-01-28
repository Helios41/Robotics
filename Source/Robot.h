#ifndef ROBOT_H_
#define ROBOT_H_

#include "WPILib.h"
#include "Gamepad.h"
#include "MechanumDrive.h"
#include "Globals.h"
#include "Pneumatics.h"

#define KIT_DRIVE

class Robot: public SimpleRobot
{
public:
	PneumaticSolenoid Solenoid;
	Gamepad Controller;
#ifdef KIT_DRIVE
	RobotDrive Drive;
#elif defined(MECHANUM_DRIVE)
	MechanumDrive Drive;
#endif

	Robot(void);
	~Robot(void);
	
	void Disabled(void);
	void Autonomous(void);
	void OperatorControl(void);
	void Test(void);
};

#endif
