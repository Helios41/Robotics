#ifndef ROBOT_H_
#define ROBOT_H_

#include "BuiltIns.h"
#include "Gamepad.h"

//we need a better name for the robot class

#define CONTROLLER_PORT 1

#define LEFT_WHEEL_PORT 1
#define RIGHT_WHEEL_PORT 2

class Robot: public IterativeRobot
{
public:
	Gamepad Controller;
	RobotDrive Drive;
	
	Robot(void);
	~Robot(void);
	
	void DisabledInit(void);
	void AutonomousInit(void);
	void TeleopInit(void);
	void TestInit(void);
	
	void DisabledPeriodic(void);
	void AutonomousPeriodic(void);
	void TeleopPeriodic(void);
	void TestPeriodic(void);
};

#endif