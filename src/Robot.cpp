#include "Robot.h"

using namespace std; //do we need this?

Robot::Robot(void)
{
	printf("Creating robot object\n");
	SmartDashboard::PutBoolean("Active", true);
	
}

Robot::~Robot(void)
{
	printf("Destroying robot object\n");
	SmartDashboard::PutBoolean("Active", false);
	
}
	
void Robot::StartCompetition(void)
{

}
	
void Robot::RobotInit(void)
{

}

void Robot::DisabledInit(void)
{

}

void Robot::AutonomousInit(void)
{

}

void Robot::TeleopInit(void)
{

}

void Robot::TestInit(void)
{

}

void Robot::DisabledPeriodic(void)
{

}

void Robot::AutonomousPeriodic(void)
{

}

void Robot::TeleopPeriodic(void);
{

}

void Robot::TestPeriodic(void)
{

}