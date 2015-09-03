#include "cnfrc.h"
#include "HAL.hpp"
#include <iostream>
#include "LiveWindow/LiveWindow.h"
#include "networktables/NetworkTable.h"
#include <stdio.h>

//TODO: extern C

b32 cnfrcInit(cnfrcState *state)
{
   if(!HALInitialize())
   {
      std::cerr << "FATAL ERROR: HAL Initialization Failed" << std::endl;
      return false;
   }
   
   HALReport(HALUsageReporting::kResourceType_Language,
             HALUsageReporting::kLanguage_CPlusPlus);
             
   /*
   RobotState::SetImplementation(DriverStation::GetInstance());
   HLUsageReporting::SetImplementation(new HardwareHLReporting());

   RobotBase::setInstance(this);
   */
   
   FILE *file = NULL;
   file = fopen("/tmp/frc_versions/FRC_Lib_Version.ini", "w");

   fputs("2015 C++ 1.2.0", file);
   
   if(file != NULL) 
      fclose(file);
             
   HALNetworkCommunicationObserveUserProgramStarting();
   
   LiveWindow &lw = LiveWindow::GetInstance();

   HALReport(HALUsageReporting::kResourceType_Framework,
             HALUsageReporting::kFramework_Simple);

   SmartDashboard::init();
   NetworkTable::GetTable("LiveWindow")
                 ->GetSubTable("~STATUS~")
                 ->PutBoolean("LW Enabled", false);
                 
   lw.SetEnabled(false);
}

void cnfrcStartLoop(cnfrcState *state, cnfrcCallbacks callbacks)
{
   while(true)
   {
      /*
      if(IsDisabled()) 
      {
         m_ds.InDisabled(true);
         callbacks.disabled();
         m_ds.InDisabled(false);
         while (IsDisabled()) m_ds.WaitForData();
      }
      else if(IsAutonomous())
      {
         m_ds.InAutonomous(true);
         callbacks.autonomous();
         m_ds.InAutonomous(false);
         while (IsAutonomous() && IsEnabled()) m_ds.WaitForData();
      }
      else if(IsTest())
      {
         lw.SetEnabled(true);
         m_ds.InTest(true);
         callbacks.test();
         m_ds.InTest(false);
         while (IsTest() && IsEnabled()) m_ds.WaitForData();
         lw.SetEnabled(false);
      }
      else
      {
         m_ds.InOperatorControl(true);
         callbacks.operatorControl();
         m_ds.InOperatorControl(false);
         while (IsOperatorControl() && IsEnabled()) m_ds.WaitForData();
      }
      */
   }
}