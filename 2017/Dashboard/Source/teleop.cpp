//NOTE: currently only drawing a list of buttons, will eventually draw a controller graphic
void DrawController(layout *controller_panel, ControllerType *controller)
{
   Text(controller_panel, controller->name, 20, V2(0, 0), V2(0, 5));
	
   for(u32 i = 0;
	   i < controller->button_count;
	   i++)
   {
	   Button(controller_panel, NULL, controller->buttons[i], V2(60, 20), V2(0, 0), V2(5, 5));
   }
   
   for(u32 i = 0;
	   i < controller->axis_count;
	   i++)
   {
	   Button(controller_panel, NULL, controller->axes[i], V2(60, 20), V2(0, 0), V2(5, 5));
   }
}

//TODO: unify all the random magic numbers
//      theme system for the colors, way to keep track of text scales, etc...
void DrawTeleop(layout *teleop_ui, UIContext *context, DashboardState *dashstate)
{
   v2 panel_size = V2((GetSize(teleop_ui->bounds).x - 15) / 2,
					   GetSize(teleop_ui->bounds).y - 10);
   
   layout driver_panel = Panel(teleop_ui, panel_size, V2(0, 0), V2(5, 5)).lout;
   
   element driver_banner = Text(&driver_panel, Literal("Driver"), 40,
                                V2((panel_size.x - GetTextWidth(context->render_context, Literal("Driver"), 40)) / 2.0f, 0), V2(0, 5));
   
   //DrawController();
   //layout op_panel = Panel(teleop_ui, panel_size, V2(0, 0), V2(5, 5)).lout;
   
}