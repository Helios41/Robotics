#include "cnfrc.h"
#include "HAL.hpp"
#include "Power.hpp"
#include <iostream>
#include "LiveWindow/LiveWindow.h"
#include "networktables/NetworkTable.h"
#include <stdio.h>
#include <string.h>

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
   
   FILE *file = NULL;
   file = fopen("/tmp/frc_versions/FRC_Lib_Version.ini", "w");

   fputs("2015 C++ 1.2.0", file);
   
   if(file != NULL) 
      fclose(file);
             
   HALNetworkCommunicationObserveUserProgramStarting();
   
   LiveWindow *lw = (LiveWindow *) LiveWindow::GetInstance();
   state->live_window = lw;
   
   HALReport(HALUsageReporting::kResourceType_Framework,
             HALUsageReporting::kFramework_Simple);

   SmartDashboard::init();
   NetworkTable::GetTable("LiveWindow")
                 ->GetSubTable("~STATUS~")
                 ->PutBoolean("LW Enabled", false);
                 
   lw->SetEnabled(false);
   InitDriverStation();
}

static void InitDriverStation()
{
   
}

static r32 GetBatteryVoltage()
{
   s32 return_code = 0;
   r32 voltage = getVinVoltage(&return_code);
   return voltage;
}

static b32 IsRobotEnabled()
{
   HALControlWord control_word;
   memset(&control_word, 0, sizeof(control_word));
   HALGetControlWord(&control_word);
   
   return control_word.enabled && control_word.dsAttached;
}

static b32 IsAutonomousMode()
{
   HALControlWord control_word;
   memset(&control_word, 0, sizeof(control_word));
   HALGetControlWord(&control_word);
   
   return control_word.autonomous;
}

static b32 IsTestMode()
{
   HALControlWord control_word;
   memset(&control_word, 0, sizeof(control_word));
   HALGetControlWord(&control_word);
   
   return control_word.test;
}

static b32 IsOperatorControlMode()
{
   HALControlWord control_word;
   memset(&control_word, 0, sizeof(control_word));
   HALGetControlWord(&control_word);
   
   return !(control_word.autonomous || control_word.test);
}

cnfrcAlliance cnfrcGetAlliance()
{
   HALAllianceStationID alliance_station;
   HALGetAllianceStation(&alliance_station);
   
   switch(allianceStationID)
   {
      case kHALAllianceStationID_red1:
      case kHALAllianceStationID_red2:
      case kHALAllianceStationID_red3:
         return cnfrcAlliance_Red;
         
      case kHALAllianceStationID_blue1:
      case kHALAllianceStationID_blue2:
      case kHALAllianceStationID_blue3:
         return cnfrcAlliance_Blue;
         
      default:
         return cnfrcAlliance_Unknown;
   }
}

void cnfrcStartLoop(cnfrcState *state, cnfrcCallbacks callbacks)
{
   while(true)
   {
      if(!IsRobotEnabled()) 
      {
         /*
         m_ds.InDisabled(true);
         callbacks.disabled();
         m_ds.InDisabled(false);
         while (IsDisabled()) m_ds.WaitForData();
         */
      }
      else if(IsAutonomousMode())
      {  
         /*
         m_ds.InAutonomous(true);
         callbacks.autonomous();
         m_ds.InAutonomous(false);
         while (IsAutonomous() && IsEnabled()) m_ds.WaitForData();
         */
      }
      else if(IsTestMode())
      {
         /*
         lw.SetEnabled(true);
         m_ds.InTest(true);
         callbacks.test();
         m_ds.InTest(false);
         while (IsTest() && IsEnabled()) m_ds.WaitForData();
         lw.SetEnabled(false);
         */
      }
      else if(IsOperatorControlMode())
      {
         /*
         m_ds.InOperatorControl(true);
         callbacks.operatorControl();
         m_ds.InOperatorControl(false);
         while (IsOperatorControl() && IsEnabled()) m_ds.WaitForData();
         */
      }
      else
      {
         //TODO: error!
      }
   }
}