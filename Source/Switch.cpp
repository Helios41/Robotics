#include "Switch.h"

Switch::Switch(int port) :
   Switch(port)
{
   
}

Switch::~Switch(void)
{
   
}
      
bool Switch::IsOn(void)
{
   return !(this->Switch.Get());
}

bool Switch::IsOff(void)
{
   return this->Switch.Get();
}