#include "Pneumatics.h"

PneumaticCompressor::PneumaticCompressor(int preasureSwitch, int compressorRelay) :
	compressor(preasureSwitch, compressorRelay)
{

}

PneumaticCompressor::~PneumaticCompressor(void)
{

}

void PneumaticCompressor::Start(void)
{
	this->compressor.Start();
}

void PneumaticCompressor::Stop(void)
{
	this->compressor.Stop();
}

int PneumaticCompressor::GetPreasure(void)
{
	return this->compressor.GetPreasureSwitchValue();	
}

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
