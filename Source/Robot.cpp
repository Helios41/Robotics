#include "Robot.h"

Robot::Robot(void): 
	Controller(CONTROLLER_PORT),
	Drive(LEFT_FRONT_WHEEL_PORT, RIGHT_FRONT_WHEEL_PORT) 
	//Drive(LEFT_FRONT_WHEEL_PORT, RIGHT_FRONT_WHEEL_PORT, LEFT_BACK_WHEEL_PORT, RIGHT_BACK_WHEEL_PORT)
{
	this->Drive.SetExpiration(0.1);
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
	this->Drive.SetSafetyEnabled(false);
	SmartDashboard::PutString("State", "Autonomous");
}

void Robot::TeleopInit(void)
{
	this->Drive.SetSafetyEnabled(true);
	SmartDashboard::PutString("State", "Teleop");
}

void Robot::TestInit(void)
{
	SmartDashboard::PutString("State", "Test");
	
}

void Robot::DisabledPeriodic(void)
{
	FEED;
}

void Robot::AutonomousPeriodic(void)
{
	
	FEED;
}

void Robot::TeleopPeriodic(void);
{
	float leftSpeed = 0.0f;
	float rightSpeed = 0.0f;
	
	if(this->Controller.GetLAnalogY() > 0.0f)
		leftSpeed = this->Controller.GetLAnalogY();
		
	if(this->Controller.GetRAnalogY() > 0.0f)
		rightSpeed = this->Controller.GetRAnalogY();
	
	this->Drive.TankDrive(leftSpeed, rightSpeed);
	
	if(!this->Controller.GetXButton())
		FEED;
}

void Robot::TestPeriodic(void)
{
	
}

START_ROBOT_CLASS(Robot);
