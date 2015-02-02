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
      static const std::string Table = "custom_dashboard";
      static const std::string Updated = "updated";
      static const std::string WidgetPrefix = "widget_";
      
      static const std::string Log = "log";
};

class CustomDashboard
{
   private:
      NetworkTable *Table;
      
      void PushUpdate(std::string channel);
      void PopUpdate(std::string channel);
      
   public:
      CustomDashboard(void);
      ~CustomDashboard(void);
      
      void Log(std::string text);
      void Send(std::string widget, std::string data);
};

#endif