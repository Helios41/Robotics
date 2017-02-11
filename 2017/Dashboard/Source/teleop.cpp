void DrawDisplayConfig(layout *display_panel, TeleopDisplay *display_config)
{
   UIContext *context = display_panel->context;
   Rectangle(context->render_context, display_panel->bounds, V4(0.5f, 0.0f, 0.0f, 0.5f));
   
   for(u32 i = 0;
       i < display_config->overlay_count;
       i++)
   {
      TeleopDisplayOverlay *curr_overlay = display_config->overlays + i;
      
      if(curr_overlay->widget)
      {
         //NOTE: info blocks go here
      }
      else
      {
         //NOTE: guides & scopes go here
      }
   }
}

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
   
   layout driver_overlays = Panel(&driver_panel, V2((panel_size.x - 15) / 2.0f, (panel_size.y - 10) - GetSize(driver_banner.margin_bounds).y), V2(0, 0), V2(5, 5)).lout;
   layout driver_controls = Panel(&driver_panel, V2((panel_size.x - 15) / 2.0f, (panel_size.y - 10) - GetSize(driver_banner.margin_bounds).y), V2(0, 0), V2(5, 5)).lout;
   
   DrawDisplayConfig(&driver_overlays, &dashstate->driver_display);
   
   //layout op_panel = Panel(teleop_ui, panel_size, V2(0, 0), V2(5, 5)).lout;
   
}