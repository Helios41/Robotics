#include "Robot.h"

Robot::Robot(void) : Controller(1)
{
	SmartDashboard::PutBoolean("Active", true);
	SmartDashboard::PutString("State", "Unknown");
}

Robot::~Robot(void)
{
	SmartDashboard::PutBoolean("Active", false);
}

void Robot::DisabledInit(void)
{
	SmartDashboard::PutString("State", "Disabled");
	
}

void Robot::AutonomousInit(void)
{
	SmartDashboard::PutString("State", "Autonomous");
	
}

void Robot::TeleopInit(void)
{
	SmartDashboard::PutString("State", "Teleop");
	
}

void Robot::TestInit(void)
{
	SmartDashboard::PutString("State", "Test");
	
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

START_ROBOT_CLASS(Robot);