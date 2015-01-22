#ifndef MECHANUM_DRIVE_H_
#define MECHANUM_DRIVE_H_

#include "BuiltIns.h"

class MechanumDrive
{
   private:
      RobotDrive Drive;
   public:
      MechanumDrive(int frontLeft, int frontRight, int backLeft, int backRight);
      ~MechanumDrive();
	  void Drive(float XAxis, float YAxis);
	  void Drive(float XAxis, float YAxis, float rotate);
};

#endif
