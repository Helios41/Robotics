package Team4618.Dashboard;

import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.Group;
import javafx.scene.input.KeyCode;

public interface IDashboardPlugin {
	String GetID();
	void InitComponents(Group panel);
	void Draw(GraphicsContext context, Canvas canvas);
	void OnKeyClick(KeyCode key);
}
