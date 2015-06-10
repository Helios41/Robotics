#ifndef ROBOT_H_
#define ROBOT_H_

#include "WPILib.h"
#include "Gamepad.h"
#include "MechanumDrive.h"
#include "Globals.h"
#include "PneumaticSolenoid.h"
#include "Switch.h"
#include "CustomDashboard.h"
#include "SensitiveListener.h"
#include "Autonomous.h"

class Robot: public SampleRobot
{
public:
	Gamepad DriverController;
	Joystick OperatorController;
	PneumaticSolenoid Solenoid;
	MechanumDrive Drive;
	Victor Elevator;
	Switch TopSwitch;
	Switch BottomSwitch;
	Victor LeftIntake;
	Victor RightIntake;
	PneumaticSolenoid Holder;
   
	Robot(void);
	~Robot(void);
	
	void RobotInit(void);
	void Disabled(void);
	void Autonomous(void);
	void OperatorControl(void);
};

#endif
