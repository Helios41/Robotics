#ifndef MECANUM_DRIVE_H_
#define MECANUM_DRIVE_H_

#include "WPILib.h"
#include "CustomDashboard.h"

class MecanumDrive
{
   private:
      RobotDrive DriveTrain;
      float RotateLimit;
      float DriveLimit;
      float Multiple;

   public:
      MecanumDrive(int frontLeft, int frontRight, int backLeft, int backRight);
      ~MecanumDrive();
      void SetRotateLimit(float limit);
      void ResetRotateLimit(void);
      void SetDriveLimit(float limit);
      void ResetDriveLimit(void);
      void SetMultiple(float multiple);
      void ResetMultiple(void);
      void Drive(float XAxis, float YAxis, float rotate);
      void Drive(float XAxis, float YAxis);
      void DriveForward(float speed);
      void DriveBackward(float speed);
      void DriveLeft(float speed);
      void DriveRight(float speed);
};

#endif
