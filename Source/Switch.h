#ifndef SWITCH_H_
#define SWITCH_H_

#include "WPILib.h"

class Switch
{
   private:
      DigitalInput input;

   public:
      Switch(int port);
      ~Switch(void);
      
      bool IsOn(void);
      bool IsOff(void);
};

#endif