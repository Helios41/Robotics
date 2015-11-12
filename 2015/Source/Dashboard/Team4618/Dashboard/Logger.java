package Team4618.Dashboard;

import java.io.File;
import java.io.FileOutputStream;

import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.input.KeyCode;
import javafx.scene.paint.Color;
import javafx.scene.Group;
import javafx.stage.FileChooser;
import javafx.scene.control.TextArea;
import javafx.scene.control.ScrollPane;
import javafx.scene.control.ToggleButton;
import javafx.scene.control.Button;
import javafx.stage.Stage;

public class Logger implements IDashboardPlugin
{
	private ToggleButton logging;
	private ScrollPane logPane;
	private TextArea log;
	private FileChooser fileChooser;
	private File file = null;
	private Button fileChooserButton;
	private Button saveButton;
	private Button clearButton;
	
	private double X = 0.0;
	private double Y = 0.0;
	private double rotate = 0.0;
	
	//TODO: make text area scrollable
	
	public Logger(Stage stage)
	{
		logging = new ToggleButton();
		logging.setText("Logging");
		logging.setLayoutX(10);
		logging.setLayoutY(10);
		logging.setOnAction((event) -> NetworkHandler.Set(GetID(), "active", logging.isSelected() ? "true" : "false"));
		
		log = new TextArea();
		log.setEditable(false);
		log.setMaxWidth(200);
		
		logPane = new ScrollPane();
		logPane.setLayoutX(200);
		logPane.setMaxWidth(log.getMaxWidth());
		logPane.setContent(log);
		
		fileChooser = new FileChooser();
		fileChooser.setTitle("Log File");
		
		fileChooserButton = new Button();
		fileChooserButton.setText("Open File");
		fileChooserButton.setLayoutX(200);
		fileChooserButton.setLayoutY(200);
		fileChooserButton.setOnAction((event) -> {
			file = fileChooser.showOpenDialog(stage);
		});
		
		saveButton = new Button();
		saveButton.setText("Save");
		saveButton.setLayoutX(200);
		saveButton.setLayoutY(230);
		saveButton.setOnAction((event) -> {
			if(file != null)
			{
				try
				{
					FileOutputStream writer = new FileOutputStream(file);
					
					writer.write(log.getText().getBytes());
					
					writer.close();
				}
				catch(Exception e) { e.printStackTrace(); }
			}
		});
		
		clearButton = new Button();
		clearButton.setText("Clear");
		clearButton.setLayoutX(200);
		clearButton.setLayoutY(260);
		clearButton.setOnAction((event) -> log.setText(""));
	}
	
	@Override
	public String GetID() {
		return "Logger";
	}

	@Override
	public void Draw(GraphicsContext context, Canvas canvas) {
		context.setFill(Color.LIGHTGRAY);
		context.fillRect(0, 0, canvas.getWidth(), canvas.getHeight());
		
		context.setFill(Color.BLACK);
		
		if(NetworkHandler.IsConnected() && NetworkHandler.Get(this.GetID(), "active").equals("true"))
		{
			double newX = NumberHandler.ToDouble(NetworkHandler.Get(this.GetID(), "X"));
			double newY = NumberHandler.ToDouble(NetworkHandler.Get(this.GetID(), "X"));
			double newRotate = NumberHandler.ToDouble(NetworkHandler.Get(this.GetID(), "X"));
			boolean valueChanged = false;
			
			context.fillText("Motor X: " + newX, 10, 60);
			context.fillText("Motor Y: " + newY, 10, 90);
			context.fillText("Motor Rotate: " + newRotate, 10, 120);
			
			if(Double.compare(newX, X) == 0)
			{
				log.setText(log.getText() + "X: " + newX);
				valueChanged = true;
			}
			
			if(Double.compare(newY, Y) == 0)
			{
				log.setText(log.getText() + "Y: " + newY);
				valueChanged = true;
			}
			
			if(Double.compare(newRotate, rotate) == 0)
			{
				log.setText(log.getText() + "Rotate: " + newRotate);
				valueChanged = true;
			}
			
			if(valueChanged)
			{
				log.setText(log.getText() + "\n");
			}
			
			X = newX;
			Y = newY;
			rotate = newRotate;
		}
		else if(!NetworkHandler.IsConnected())
		{
			context.fillText("No Robot Detected", 10, 60);
		}
		else if(!NetworkHandler.Get(this.GetID(), "active").equals("true"))
		{
			context.fillText("Logging not enabled", 10, 60);
		}
		
		if(file == null)
		{
			saveButton.setDisable(true);
		}
		else
		{
			saveButton.setDisable(false);
		}
	}
	
	@Override
	public void InitComponents(Group panel) {
		panel.getChildren().add(logging);
		panel.getChildren().add(logPane);
		panel.getChildren().add(fileChooserButton);
		panel.getChildren().add(saveButton);
		panel.getChildren().add(clearButton);
	}

	@Override
	public void OnKeyClick(KeyCode key) {
		
	}
}
