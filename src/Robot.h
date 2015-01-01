#ifndef ROBOT_H_
#define ROBOT_H_

#include "BuiltIns.h"
#include "Gamepad.h"

//we need a better name for the robot class
//why is everything public? change?

//change to SimpleRobot?

class Robot: public IterativeRobot
{
public:
	Gamepad Controller;
	
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