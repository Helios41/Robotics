package Team4618.Dashboard;

import javafx.scene.Group;
import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.input.KeyCode;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;

public class HolderState implements IDashboardPlugin{

	public HolderState()
	{
		
	}
	
	@Override
	public String GetID() {
		return "HolderState";
	}

	@Override
	public void InitComponents(Group panel) {
		
	}

	@Override
	public void Draw(GraphicsContext context, Canvas canvas) {
		context.setFill(Color.DARKBLUE);
		context.fillRect(0, 0, canvas.getWidth(), canvas.getHeight());
		
		context.setFill(Color.WHITESMOKE);
		context.setFont(new Font(50));
		
		if(NetworkHandler.IsConnected() && (NetworkHandler.Get(GetID(), "holder").equals("open")))
		{
			context.fillText("Holder OPEN", 5, 90);
		}
		else if(NetworkHandler.IsConnected() && !(NetworkHandler.Get(GetID(), "holder").equals("open")))
		{
			context.fillText("Holder CLOSED", 5, 90);
		}
		else
		{
			context.setFont(new Font(45));
			context.fillText("No Robot Detected", 5, 90);
		}
	}

	@Override
	public void OnKeyClick(KeyCode key) {
		
	}
}
