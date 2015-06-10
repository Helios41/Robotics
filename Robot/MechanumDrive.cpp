#include "MechanumDrive.h"

MechanumDrive::MechanumDrive(int frontLeft, int frontRight, int backLeft, int backRight) :
   DriveTrain(frontLeft, backLeft, frontRight, backRight),
   RotateLimit(1.0f),
   DriveLimit(1.0f),
   Multiple(1.0f)
{
   this->DriveTrain.SetInvertedMotor(RobotDrive::kFrontLeftMotor, true);
   this->DriveTrain.SetInvertedMotor(RobotDrive::kRearLeftMotor, true);
   this->DriveTrain.SetExpiration(0.1);
   this->DriveTrain.SetSafetyEnabled(false);
}

MechanumDrive::~MechanumDrive()
{

}

void MechanumDrive::SetRotateLimit(float limit)
{
	this->RotateLimit = limit;
}

void MechanumDrive::ResetRotateLimit(void)
{
	this->RotateLimit = 1.0f;
}

void MechanumDrive::SetDriveLimit(float limit)
{
	this->DriveLimit = limit;
}

void MechanumDrive::ResetDriveLimit(void)
{
	this->DriveLimit = 1.0f;
}

void MechanumDrive::SetMultiple(float multiple)
{
	this->Multiple = multiple;
}

void MechanumDrive::ResetMultiple()
{
	this->Multiple = 1.0f;
}

void MechanumDrive::Drive(float XAxis, float YAxis, float rotate)
{
	XAxis *= this->Multiple;
	YAxis *= this->Multiple;
	rotate *= this->Multiple;

	if(rotate > this->RotateLimit)
	{
		rotate = this->RotateLimit;
	}
	else if(rotate < -(this->RotateLimit))
	{
		rotate = -(this->RotateLimit);
	}

	if(XAxis > this->DriveLimit)
	{
		XAxis = this->DriveLimit;
	}
	else if(XAxis < -(this->DriveLimit))
	{
		XAxis = -(this->DriveLimit);
	}

	if(YAxis > this->DriveLimit)
	{
		YAxis = this->DriveLimit;
	}
	else if(YAxis < -(this->DriveLimit))
	{
		YAxis = -(this->DriveLimit);
	}

	if(CustomDashboard::Get("Logger", "active") == "true")
	{
		CustomDashboard::Set("Logger", "X", std::to_string(XAxis));
		CustomDashboard::Set("Logger", "Y", std::to_string(YAxis));
		CustomDashboard::Set("Logger", "rotate", std::to_string(rotate));
	}

	this->DriveTrain.MecanumDrive_Cartesian(XAxis, YAxis, rotate);
}

void MechanumDrive::Drive(float XAxis, float YAxis)
{
	this->Drive(XAxis, YAxis, 0.0f);
}

void MechanumDrive::DriveForward(float speed)
{	
	this->Drive(0.0f, speed);
}

void MechanumDrive::DriveBackward(float speed)
{
	this->Drive(0.0f, -speed);
}

void MechanumDrive::DriveLeft(float speed)
{
	this->Drive(speed, 0.0f);
}

void MechanumDrive::DriveRight(float speed)
{
	this->Drive(-speed, 0.0f);
}
