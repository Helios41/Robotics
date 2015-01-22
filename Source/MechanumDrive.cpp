#include "MechanumDrive.h"

MechanumDrive::MechanumDrive(int frontLeft, int frontRight, int backLeft, int backRight) :
   Drive(frontLeft, backLeft, frontRight, backRight);
{

}

MechanumDrive::~MechanumDrive()
{

}

void MechanumDrive::Drive(float XAxis, float YAxis)
{
	this->Drive.MecanumDrive_Cartesian(XAxis, YAxis, 0.0f);
}

void MechanumDrive::Drive(float XAxis, float YAxis, float rotate)
{
	this->Drive.MecanumDrive_Cartesian(XAxis, YAxis, rotate);
}