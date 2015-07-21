#include "CustomDashboard.h"

std::string CustomDashboard::TableID = "custom_dashboard";
NetworkTable *CustomDashboard::Table = nullptr;

const std::string CustomDashboard::InvalidReturn = "__INVALID_RETURN_CUSTOM_DASHBOARD__";

CustomDashboard::CustomDashboard(void) { }

CustomDashboard::~CustomDashboard(void) { }

void CustomDashboard::Init(std::string name)
{
	CustomDashboard::Table = NetworkTable::GetTable(CustomDashboard::TableID);
	CustomDashboard::Table->PutString("networkName", name);
}

std::string CustomDashboard::GetNetworkName(void)
{
	if(CustomDashboard::Table != nullptr)
	{
		if(CustomDashboard::Table->ContainsKey("networkName"))
			return CustomDashboard::Table->GetString("networkName");
	}

	return nullptr;
}

void CustomDashboard::Set(std::string widget, std::string varname, std::string data)
{
	if(CustomDashboard::Table != nullptr)
		CustomDashboard::Table->PutString(widget + "_" + varname, data);
}

void CustomDashboard::SetArray(std::string widget, std::string varname, int index, std::string data)
{
	if(CustomDashboard::Table != nullptr)
		CustomDashboard::Table->PutString(widget + "_" + varname + "[" + std::to_string(index) + "]", data);
}

std::string CustomDashboard::Get(std::string widget, std::string varname)
{
	if(CustomDashboard::Table != nullptr)
	{
		if(CustomDashboard::Table->ContainsKey(widget + "_" + varname))
			return CustomDashboard::Table->GetString(widget + "_" + varname);
	}

	return CustomDashboard::InvalidReturn;
}

std::string CustomDashboard::GetArray(std::string widget, std::string varname, int index)
{
	if(CustomDashboard::Table != nullptr)
	{
		if(CustomDashboard::Table->ContainsKey(widget + "_" + varname + "[" + std::to_string(index) + "]"))
			return CustomDashboard::Table->GetString(widget + "_" + varname + "[" + std::to_string(index) + "]");
	}

	return CustomDashboard::InvalidReturn;
}
