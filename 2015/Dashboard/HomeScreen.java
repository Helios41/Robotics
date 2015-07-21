package Team4618.Dashboard;

import javafx.scene.Group;
import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.input.KeyCode;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.control.Button;

public class HomeScreen implements IDashboardPlugin{
	private Button reconnectButton;
	
	public HomeScreen()
	{
		reconnectButton = new Button();
		reconnectButton.setText("Reconnect");
		reconnectButton.setLayoutX(10);
		reconnectButton.setLayoutY(200);
		
		reconnectButton.setOnAction((event) -> 
		{
			NetworkHandler.Connect();
		});
	}
	
	@Override
	public String GetID() {
		return "HomeScreen";
	}

	@Override
	public void InitComponents(Group panel) {
		panel.getChildren().add(reconnectButton);
	}

	@Override
	public void Draw(GraphicsContext context, Canvas canvas) {
		context.setFill(Color.RED);
		context.fillRect(0, 0, canvas.getWidth(), canvas.getHeight());
		
		context.setFill(Color.WHITE);
		context.setFont(new Font(50));
		context.fillText("TEAM 4618", 50, 50);
		context.fillText("CN ROBOTICS", 30, 90);
		
		context.setFont(new Font(18));
		
		if(NetworkHandler.IsConnected())
		{
			context.fillText(NetworkHandler.GetNetworkName(), 10, 180);
		}
		else
		{
			context.fillText("NOT Connected", 10, 180);
		}
	}

	@Override
	public void OnKeyClick(KeyCode key) {
		
	}
}
