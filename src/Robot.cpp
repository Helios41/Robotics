#include "Robot.h"

Robot::Robot(void): 
	Controller(CONTROLLER_PORT), 
	Drive(LEFT_WHEEL_PORT, RIGHT_WHEEL_PORT)
{
	SmartDashboard::PutString("State", "Unknown");
}

Robot::~Robot(void)
{

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
	float leftSpeed = 0.0f;
	float rightSpeed = 0.0f;
	
	if(this->Controller.GetLBTrigger())
		leftSpeed = 5.0f;
		
	if(this->Controller.GetRBTrigger())
		rightSpeed = 5.0f;
	
	this->Drive.TankDrive(leftSpeed, rightSpeed);
	
	if(!this->Controller.GetXButton())
		this->GetWatchdog().Feed();
}

void Robot::TestPeriodic(void)
{

}

START_ROBOT_CLASS(Robot);