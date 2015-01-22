#ifndef MECHANUM_DRIVE_H_
#define MECHANUM_DRIVE_H_

#include "BuiltIns.h"

class Vector2
{
	float X;
	float Y;
	
	float Length();
};

class MechanumDrive
{
   private:
      RobotDrive Drive;
	  Vector2 Direction;
   public:
      MechanumDrive(int frontLeft, int frontRight, int backLeft, int backRight);
      ~MechanumDrive();
	  void SetDirection(float XAxis, float YAxis);
	  void UpdateDrive();
};

#endif
