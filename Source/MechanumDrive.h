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
	  void Drive(float XAxis, float YAxis);
	  void Drive(float XAxis, float YAxis, float rotate);
	  void MechanumDrive:DriveForward(float speed);
	  void MechanumDrive:DriveBackward(float speed);
	  void MechanumDrive:DriveLeft(float speed);
	  void MechanumDrive:DriveRight(float speed);
};

#endif
