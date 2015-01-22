#include "MechanumDrive.h"
#include <math.h>

float Vector2::Length()
{
	return sqrt(pow(this->X, 2) + pow(this->Y, 2));
}

MechanumDrive::MechanumDrive(int frontLeft, int frontRight, int backLeft, int backRight) :
   Drive(frontLeft, frontRight, backLeft, backRight);
{

}

MechanumDrive::~MechanumDrive()
{

}

void MechanumDrive::SetDirection(float XAxis, float YAxis)
{
	this->Direction.X = XAxis;
	this->Direction.Y = YAxis;
}

void MechanumDrive::UpdateDrive()
{
	
}