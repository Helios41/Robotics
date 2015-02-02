#include "CustomDashboard.h"
#include <sstream>

CustomDashboard::CustomDashboard(void)
{
   this->Table = NetworkTable::GetTable(CDBGlobals::Table);
   this->Table->PutString(CDBGlobals::Updated, "");
}

CustomDashboard::~CustomDashboard(void)
{
   
}

void CustomDashboard::PushUpdate(std::string channel)
{
   std::string newUpdated = this->Table->GetString(CDBGlobals::Updated) + ":" + channel;
   this->Table->PutString(CDBGlobals::Updated, newUploaded);
}

void CustomDashboard::PushChannel(std::string channel, std::string data)
{
   this->Table->PutString(channel, data);
   this->PushUpdate(channel);
}

void CustomDashboard::PopUpdate(std::string channel)
{
   std::string oldUpdated = this->Table->GetString(CDBGlobals::Updated);
   std::string newUpdated = "";
   
   for (int i=0; i<str.length(); i++)
   {
      if (oldUpdated[i] == ':')
         oldUpdated[i] = ' ';
   }
   
   std::stringstream ss(oldUpdated);
   std::string currString;
   
   while(ss.good())
   {
      ss >> currString;
      
      if(!(currString == channel))
      {
         newUpdated = newUpdated + ":" + currString;
      }
   }
   
   this->Table->PutString(CDBGlobals::Updated, newUpdated);
}

void CustomDashboard::PopChannel(std::string channel)
{
   this->PopUpdate(channel);
   this->Table->PutString(channel, "");
}

void CustomDashboard::Send(std::string widget, std::string data)
{
   this->PushChannel(CDBGlobals::WidgetPrefix + widget + CDBGlobals::RobotSuffix, data);
}

std::string CustomDashboard::Recive(std::string widget)
{
   std::string result = this->Table->GetString(CDBGlobals::WidgetPrefix + widget + CDBGlobals::ClientSuffix);
   this->PopChannel(CDBGlobals::WidgetPrefix + widget + CDBGlobals::ClientSuffix);
   return result;
}