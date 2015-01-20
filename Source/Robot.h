#ifndef ROBOT_H_
#define ROBOT_H_

#include "WPILib.h"
#include "Gamepad.h"
#include "MechanumDrive.h"
#include "Globals.h"

class Robot: public SimpleRobot
{
public:
	Gamepad Controller;
	//MechanumDrive Drive;
	RobotDrive Drive;	

	Robot(void);
	~Robot(void);
	
	void Disabled(void);
	void Autonomous(void);
	void OperatorControl(void);
	void Test(void);
};

#endif
