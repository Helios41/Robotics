#ifndef MECHANUM_DRIVE_H_
#define MECHANUM_DRIVE_H_

#include "WPILib.h"

class MechanumDrive
{
   private:
      RobotDrive DriveTrain;
   public:
      MechanumDrive(int frontLeft, int frontRight, int backLeft, int backRight);
      ~MechanumDrive();
	  void Drive(float XAxis, float YAxis, float rotate);
     void Drive(float XAxis, float YAxis);
	  void DriveForward(float speed);
	  void DriveBackward(float speed);
	  void DriveLeft(float speed);
	  void DriveRight(float speed);
};

#endif
