package Team4618.Dashboard;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.List;

import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.paint.Color;
import javafx.scene.control.Button;
import javafx.scene.Group;
import javafx.scene.control.TextField;
import javafx.stage.FileChooser;
import javafx.stage.Stage;
import javafx.scene.input.KeyCode;

public class AutonomousEditor implements IDashboardPlugin
{
	public static enum AutonomousCommands
	{
		DriveForwards(0),
		DriveBackwards(1),
		DriveLeft(2),
		DriveRight(3),
		ElevatorUp(4),
		ElevtorDown(5);
		
		private int opcode = 0;
		AutonomousCommands(int opcode) { this.opcode = opcode; }
		int get() { return this.opcode; }
	}
	
	public static class CommandInstance
	{
		public AutonomousCommands command;
		public double time;
		public double power;
		
		public CommandInstance(AutonomousCommands command, double time, double power)
		{
			this.command = command;
			this.time = time;
			this.power = power;
		}
	}
	
	private List<CommandInstance> commands = new ArrayList<CommandInstance>();
	private List<Button> buttons = new ArrayList<Button>();
	private Button uploadButton;
	private TextField powerField;
	private TextField timeField;
	private CommandInstance currCommand = null;
	private Button removeCommand;
	private int commandIndex = 0;
	private FileChooser fileChooser;
	private File configFile = null;
	private Button fileChooserButton;
	private Button saveButton;
	private Button loadButton;
	
	public AutonomousEditor(Stage stage)
	{
		int buttonY = 10;
		
		for(AutonomousCommands command : AutonomousCommands.values())
		{
			Button button = new Button();
			
			button.setText(command.toString());
			button.setLayoutX(5);
			button.setLayoutY(buttonY);
			
			button.setOnAction((event) -> 
			{
				commands.add(new CommandInstance(command, 0.0, 0.0));
			});
			
			buttonY += 25;
			
			buttons.add(button);
		}
		
		uploadButton = new Button();
		uploadButton.setText("Upload");
		uploadButton.setLayoutY(buttonY + 30);
		uploadButton.setLayoutX(5);
		uploadButton.setOnAction((event) -> UploadToRobot());
		
		powerField = new TextField();
		powerField.setLayoutY(30);
		powerField.setLayoutX(300);
		
		powerField.setOnAction((event) -> 
		{
			if(currCommand != null)
			{
				double power =  Double.valueOf(powerField.getText());
				
				if(power > 100.0)
					power = 100.0;
					
				if(power < -100.0)
					power = -100.0;
					
				currCommand.power = power;
			}
		});
		
		timeField = new TextField();
		timeField.setLayoutY(80);
		timeField.setLayoutX(300);
		
		timeField.setOnAction((event) -> 
		{
			if(currCommand != null)
			{
				double time = Double.valueOf(timeField.getText());
				
				if(time < 0.0)
					time = 0.0;
				
				currCommand.time = time;
			}
		});
		
		removeCommand = new Button();
		removeCommand.setText("Remove");
		removeCommand.setLayoutY(120);
		removeCommand.setLayoutX(300);
		
		removeCommand.setOnAction((event) -> 
		{
			commands.remove(currCommand);
			currCommand = null;
			
			if(commandIndex > 0)
				--commandIndex;
		});
		
		fileChooser = new FileChooser();
		fileChooser.setTitle("Autonomous Config");
		
		fileChooserButton = new Button();
		fileChooserButton.setText("Open File");
		fileChooserButton.setLayoutX(5);
		fileChooserButton.setLayoutY(buttonY + 90);
		
		fileChooserButton.setOnAction((event) -> 
		{
			configFile = fileChooser.showOpenDialog(stage);
		});
		
		saveButton = new Button();
		saveButton.setText("Save");
		saveButton.setLayoutX(5);
		saveButton.setLayoutY(buttonY + 115);
		
		saveButton.setOnAction((event) -> 
		{
			if((configFile != null) && !commands.isEmpty())
			{
				try
				{
					FileOutputStream writer = new FileOutputStream(configFile);
					
					writer.write(Serialize().getBytes());
					
					writer.close();
				}
				catch(Exception e) { e.printStackTrace(); }
			}
		});
		
		loadButton = new Button();
		loadButton.setText("Load");
		loadButton.setLayoutX(5);
		loadButton.setLayoutY(buttonY + 140);
		
		loadButton.setOnAction((event) -> 
		{
			if(configFile != null)
			{
				try
				{
					FileInputStream reader = new FileInputStream(configFile);
					
					byte[] buffer = new byte[1024];
					reader.read(buffer);
					
					commands.clear();
					Deserialize(new String(buffer));
					
					reader.close();
				}
				catch(Exception e) { e.printStackTrace(); }
			}
		});
	}
	
	@Override
	public String GetID() {
		return "AutonomousEditor";
	}

	@Override
	public void Draw(GraphicsContext context, Canvas canvas) {
		context.setFill(Color.AQUAMARINE);
		context.fillRect(0, 0, canvas.getWidth(), canvas.getHeight());
		
		if(commandIndex < commands.size())
			currCommand = commands.get(commandIndex);
		
		int textY = 35;
		int i = 0;
		
		context.setFill(Color.BLACK);
		
		context.fillText("Start", 150, 15);
		
		double time = GetTotalTime();
			
		if((time > 15.0) || !NetworkHandler.IsConnected())
		{
			uploadButton.setDisable(true);
		}
		else
		{
			uploadButton.setDisable(false);
		}
		
		if(time > 15.0)
			context.setFill(Color.RED);
		
		context.fillText("Time: " + GetTotalTime(), 10, 240);
		context.setFill(Color.BLACK);
		
		for(CommandInstance command : commands)
		{
			if(i == commandIndex)
				context.setFill(Color.DARKGOLDENROD);
				
			context.fillText(command.command.toString() + " (" + command.power + ", " + command.time + ")", 150, textY);
			context.setFill(Color.BLACK);
			
			textY += 15;
			++i;
		}
		
		if(currCommand != null)
		{
			context.fillText("Power(Percent):", 300, 20);
			context.fillText("Time(Seconds):", 300, 70);
			
			timeField.setOpacity(100);
			powerField.setOpacity(100);
			removeCommand.setOpacity(100);
		}
		else
		{
			timeField.setOpacity(0);
			powerField.setOpacity(0);
			removeCommand.setOpacity(0);
		}
		
		if(configFile == null)
		{
			saveButton.setDisable(true);
			loadButton.setDisable(true);
		}
		else
		{
			saveButton.setDisable(false);
			loadButton.setDisable(false);
		}
	}
	
	private double GetTotalTime()
	{
		double time = 0.0;
		
		for(CommandInstance command : commands)
		{
			time += command.time;
		}
		
		return time;
	}
	
	private void UploadToRobot()
	{
		int index = 0;
		
		for(CommandInstance command : commands)
		{
			NetworkHandler.SetArray(GetID(), "command", index, String.valueOf(command.command.get()));
			NetworkHandler.SetArray(GetID(), "time", index, String.valueOf(command.time));
			NetworkHandler.SetArray(GetID(), "power", index, String.valueOf(command.power));
			
			++index;
		}
		
		NetworkHandler.Set(GetID(), "size", String.valueOf(commands.size()));
	}

	private String Serialize()
	{
		String output = "";
		
		for(CommandInstance command : commands)
		{
			output = output + command.command.get() + ":" + command.time + ":" + command.power + ";";
		}
		
		return output;
	}
	
	private void Deserialize(String string)
	{
		String[] commandStrings = string.split(";");
		
		for(String commandString : commandStrings)
		{
			String[] componentStrings = commandString.split(":");
			
			if(componentStrings.length >= 3)
			{
				int commandIndex = Integer.valueOf(componentStrings[0]);
				double time = Double.valueOf(componentStrings[1]);
				double power = Double.valueOf(componentStrings[2]);
				
				CommandInstance commandInstance = new CommandInstance(AutonomousCommands.values()[commandIndex], time, power);
				commands.add(commandInstance);
			}
		}
	}
	
	@Override
	public void InitComponents(Group panel) {
		panel.getChildren().addAll(buttons);
		panel.getChildren().add(uploadButton);
		panel.getChildren().add(powerField);
		panel.getChildren().add(timeField);
		panel.getChildren().add(removeCommand);
		panel.getChildren().add(fileChooserButton);
		panel.getChildren().add(saveButton);
		panel.getChildren().add(loadButton);
	}

	@Override
	public void OnKeyClick(KeyCode key) {
		if(key == KeyCode.S)
		{
			if(commandIndex + 1 < commands.size())
				++commandIndex;
		}
		else if(key == KeyCode.W)
		{
			if(commandIndex > 0)
				--commandIndex;
		}	
	}
}
