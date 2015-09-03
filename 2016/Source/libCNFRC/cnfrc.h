#ifndef LIBCNFRC_H_
#define LIBCNFRC_H_

#include "Definitions.h"

typedef struct 
{
   //TODO: state data
}cnfrcState;

typedef struct
{
   void (* disabled)(void *data);
   void (* autonomous)(void *data);
   void (* test)(void *data);
   void (* operatorControl)(void *data);
}cnfrcCallbacks;

b32 cnfrcInit(cnfrcState *state);
void cnfrcStartLoop(cnfrcState *state, cnfrcCallbacks callbacks);

#endif