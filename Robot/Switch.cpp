#include "Switch.h"

Switch::Switch(int port) :
   input(port)
{
   
}

Switch::~Switch(void)
{
   
}
      
bool Switch::IsOn(void)
{
   return !(this->input.Get());
}

bool Switch::IsOff(void)
{
   return this->input.Get();
}