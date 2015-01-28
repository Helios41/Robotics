#ifndef PNEUMATICS_H_
#define PNEUMATICS_H_

#include "WPILib.h"

class PneumaticCompressor
{
	private:
		Compressor compressor;

	public:
		PneumaticCompressor(int pressureSwitch, int compressorRelay);
		~PneumaticCompressor(void);

		void Start(void);
		void Stop(void);
		int GetPreasure(void);
};

class PneumaticSolenoid
{
	private:
		DoubleSolenoid solenoid;
	
	public:
		PneumaticSolenoid(int forwardPort, int reversePort);
		~PneumaticSolenoid(void);

		void Off(void);
		void Forward(void);
		void Reverse(void);
}

#endif
