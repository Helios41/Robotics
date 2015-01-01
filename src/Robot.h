#include <stdio.h>
#include <string> 
#include "BuiltIns.h"

//do we need to include "string"
//we need a better name for the robot class

class Robot: public IterativeRobot
{
public:
	Robot(void);
	~Robot(void);
	
	void StartCompetition(void);
	
	void RobotInit(void);
	void DisabledInit(void);
	void AutonomousInit(void);
	void TeleopInit(void);
	void TestInit(void);
	
	void DisabledPeriodic(void);
	void AutonomousPeriodic(void);
	void TeleopPeriodic(void);
	void TestPeriodic(void);
};