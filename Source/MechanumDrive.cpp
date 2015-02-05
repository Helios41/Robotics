#include "MechanumDrive.h"

MechanumDrive::MechanumDrive(int frontLeft, int frontRight, int backLeft, int backRight) :
   DriveTrain(frontLeft, backLeft, frontRight, backRight)
{
   this->DriveTrain.SetExpiration(0.1);
   this->DriveTrain.SetSafetyEnabled(false);
}

MechanumDrive::~MechanumDrive()
{

}

void MechanumDrive::Drive(float XAxis, float YAxis)
{
	this->DriveTrain.MecanumDrive_Cartesian(XAxis, YAxis, 0.0f);
}

void MechanumDrive::Drive(float XAxis, float YAxis, float rotate)
{
	this->DriveTrain.MecanumDrive_Cartesian(XAxis, YAxis, rotate);
}

void MechanumDrive::DriveForward(float speed)
{	
	this->Drive(0.0f, speed);
}

void MechanumDrive::DriveBackward(float speed)
{
	this->Drive(0.0f, speed - (speed * 2));
}

void MechanumDrive::DriveLeft(float speed)
{
	this->Drive(speed - (speed * 2), 0.0f);
}

void MechanumDrive::DriveRight(float speed)
{
	this->Drive(speed, 0.0f);
}