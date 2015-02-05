#ifndef CUSTOM_DASHBOARD_H_
#define CUSTOM_DASHBOARD_H_

#include "WPILib.h"
#include "NetworkTables/NetworkTable.h"
#include <string>

/* 
 * Idea is that when you set a variable in the
 * network table you append that variables name to the update
 * variable, so the client just looks through the update variable
 * and gets the variables, clears them then removes them 
 * from the update variable 
 */

class CDBGlobals
{
   public:
      static std::string Table = "custom_dashboard";
      static std::string Updated = "updated";
      static std::string WidgetPrefix = "widget_";
      
      static std::string RobotSuffix = "_robot";
      static std::string ClientSuffix = "_client";
};

class CustomDashboard
{
   private:
      NetworkTable *Table;
      
      void PushUpdate(std::string channel);
      void PushChannel(std::string channel, std::string data);
      void PopUpdate(std::string channel);
      void PopChannel(std::string channel);
      
   public:
      CustomDashboard(void);
      ~CustomDashboard(void);
      
      void Send(std::string widget, std::string data);
      std::string Recive(std::string widget);
};

#endif