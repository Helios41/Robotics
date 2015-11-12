package Team4618.Dashboard;

public class NumberHandler 
{	
	public static double ToDouble(String string)
	{	
		try
		{
			return Double.valueOf(string);
		}
		catch(NumberFormatException e) { }
		
		return 0.0f;
	}
}
