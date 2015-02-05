#ifndef PNEUMATICS_H_
#define PNEUMATICS_H_

#include "WPILib.h"

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
};

#endif
