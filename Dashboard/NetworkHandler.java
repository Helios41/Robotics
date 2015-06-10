package Team4618.Dashboard;

import edu.wpi.first.wpilibj.networktables.NetworkTable;

public class NetworkHandler {
	public static final String TableID = "custom_dashboard";
	private static NetworkTable Table = null;
	
	public static void Connect()
	{
		Table = NetworkTable.getTable(TableID);
	}
	
	public static String GetNetworkName()
	{
		if(Table != null)
		{
			if(Table.containsKey("networkName"))
				return Table.getString("networkName");
		}
		
		return "NOPE";
	}
	
	public static String Get(String widget, String varname)
	{
		if(Table != null)
		{
			if(Table.containsKey(widget + "_" + varname))
				return Table.getString(widget + "_" + varname);
		}
		
		return "NOPE";
	}
	
	public static String GetArray(String widget, String varname, int index)
	{
		if(Table != null)
		{
			if(Table.containsKey(widget + "_" + varname + "[" + index + "]"))
				return Table.getString(widget + "_" + varname + "[" + index + "]");
		}
		
		return "NOPE";
	}
	
	public static void Set(String widget, String varname, String data)
	{
		if(Table != null)
			Table.putString(widget + "_" + varname, data);
	}
	
	public static void SetArray(String widget, String varname, int index, String data)
	{
		if(Table != null)
			Table.putString(widget + "_" + varname + "[" + index + "]", data);
	}
	
	public static boolean IsConnected()
	{
		if(Table != null)
			return Table.isConnected();
		
		return false;
	}
}
