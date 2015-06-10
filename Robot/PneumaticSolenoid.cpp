#include "PneumaticSolenoid.h"

PneumaticSolenoid::PneumaticSolenoid(int forwardPort, int reversePort) : 
	solenoid(forwardPort, reversePort)
{
	
}

PneumaticSolenoid::~PneumaticSolenoid(void)
{

}

void PneumaticSolenoid::Off(void)
{
	this->solenoid.Set(DoubleSolenoid::Value::kOff);
}

void PneumaticSolenoid::Reverse(void)
{
	this->solenoid.Set(DoubleSolenoid::Value::kReverse);
}

void PneumaticSolenoid::Forward(void)
{
	this->solenoid.Set(DoubleSolenoid::Value::kForward);
}
