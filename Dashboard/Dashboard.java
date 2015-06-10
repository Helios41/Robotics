package Team4618.Dashboard;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.stage.Stage;
import javafx.stage.StageStyle;
import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.Group;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.event.EventHandler;

public class Dashboard extends Application{
	public static Canvas DashboardCanvas;
	public static GraphicsContext DashboardContext;
	public static Group DashboardPanel;
	
	public static List<Button> PluginButtons = new ArrayList<Button>();
	
	public static List<IDashboardPlugin> DashboardPlugins = new ArrayList<IDashboardPlugin>();
	public static IDashboardPlugin CurrentPlugin = null;
	
	public static Thread DrawLoop;
	public static AtomicReference<Boolean> DrawLoopRunning = new AtomicReference<Boolean>();
	
	@Override
	public void start(Stage stage) throws Exception {
		stage.initStyle(StageStyle.UNDECORATED);
		stage.setX(0);
		stage.setY(0);
		stage.setTitle("Custom Dashboard");
		stage.setMaxHeight(400);
		stage.setMaxWidth(1030);
		
		DashboardPlugins.add(new HomeScreen());
		DashboardPlugins.add(new AutonomousEditor(stage));
		DashboardPlugins.add(new Logger(stage));
		DashboardPlugins.add(new HolderState());
		
		DrawLoopRunning.set(true);
		
		DrawLoop = new Thread(() ->
		{
			while(DrawLoopRunning.get())
			{
				try {
					Platform.runLater(() -> 
					{
						DashboardContext.setFill(Color.DARKGREY);
						DashboardContext.fillRect(0, 0, DashboardCanvas.getHeight(), DashboardCanvas.getWidth());
					});
					
					if(CurrentPlugin != null)
					{
						Platform.runLater(() -> CurrentPlugin.Draw(DashboardContext, DashboardCanvas));
						Platform.runLater(() -> DashboardContext.setFont(new Font(12)));
					}
					
					Thread.sleep(120);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		});
		
		DashboardCanvas = new Canvas(stage.getMaxHeight(), stage.getMaxWidth());
		DashboardCanvas.setLayoutX(120);
		
		DashboardContext = DashboardCanvas.getGraphicsContext2D();
		
		DashboardPanel = new Group();
		DashboardPanel.setLayoutX(120);
		
		if(!DashboardPlugins.isEmpty())
		{
			DashboardPanel.getChildren().clear();
			CurrentPlugin = DashboardPlugins.get(0);
			CurrentPlugin.InitComponents(DashboardPanel);
			CurrentPlugin.Draw(DashboardContext, DashboardCanvas);
		}
		
		int buttonY = 0;
		
		for(IDashboardPlugin plugin : DashboardPlugins)
		{
			Button pluginButton = new Button();
			
			pluginButton.setText(plugin.GetID());
			pluginButton.setLayoutY(buttonY);
			
			pluginButton.setOnAction((event) -> 
			{
				DashboardPanel.getChildren().clear();
				CurrentPlugin = plugin;
				CurrentPlugin.InitComponents(DashboardPanel);
				CurrentPlugin.Draw(DashboardContext, DashboardCanvas);
			});
			
			buttonY += 30;
			
			PluginButtons.add(pluginButton);
		}
		
		Group stageGroup = new Group();
		
		stageGroup.setOnKeyPressed((event) -> 
		{
			if(CurrentPlugin != null)
				CurrentPlugin.OnKeyClick(event.getCode());
		});
		
		stageGroup.getChildren().add(DashboardCanvas);
		stageGroup.getChildren().addAll(PluginButtons);
		stageGroup.getChildren().add(DashboardPanel);
		
		stage.addEventFilter(KeyEvent.KEY_PRESSED, new EventHandler<KeyEvent>()
		{
			@Override
			public void handle(KeyEvent evt)
			{
				if(evt.getCode() == KeyCode.ESCAPE)
				{
					DrawLoopRunning.set(false);
				}
			}
		});
		
		DashboardContext.setFill(Color.DARKGREY);
		DashboardContext.fillRect(0, 0, DashboardCanvas.getHeight(), DashboardCanvas.getWidth());
		
		stage.setScene(new Scene(stageGroup));
		
		stage.setHeight(stage.getMaxHeight());
		stage.setWidth(stage.getMaxWidth());
		
		stage.show();
		
		DrawLoop.start();
	}
}