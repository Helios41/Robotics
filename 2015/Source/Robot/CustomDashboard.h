#ifndef CUSTOM_DASHBOARD_H_
#define CUSTOM_DASHBOARD_H_

#include "WPILib.h"
#include "NetworkTables/NetworkTable.h"
#include <string>

class CustomDashboard
{
   private:
	  static std::string TableID;
     static NetworkTable *Table;
      
   public:
     CustomDashboard(void);
     ~CustomDashboard(void);
      
     static void Init(std::string name);
     static std::string GetNetworkName(void);
     static void Set(std::string widget, std::string varname, std::string data);
     static void SetArray(std::string widget, std::string varname, int index, std::string data);
     static std::string Get(std::string widget, std::string varname);
     static std::string GetArray(std::string widget, std::string varname, int index);
     
     static const std::string InvalidReturn;
};

#endif
