package Team4618.Dashboard;

import edu.wpi.first.wpilibj.networktables.NetworkTable;
//MOM GET THE CAMERA
import javafx.application.Application;

public class Startup {
	
	public static void main(String[] args) throws Exception
	{
		NetworkTable.setClientMode();
		NetworkTable.setTeam(4618);
		
		NetworkHandler.Connect();
		Application.launch(Dashboard.class, args);
		
		Dashboard.DrawLoopRunning.set(false);

		System.exit(0);
	}
	
}
