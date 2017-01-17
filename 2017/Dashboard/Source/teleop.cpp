void DrawDisplayConfig(layout *display_rect, TeleopDisplay *display_config)
{
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

enum ControllerType
{
   Controller_LogitechF310,
   Controller_Joystick_____
};

//NOTE: currently only drawing a list of buttons, will eventually draw a controller graphic
void DrawController(layout *controller_panel)
{
   
}

//TODO: unify all the random magic numbers
//      theme system for the colors, way to keep track of text scales, etc...
void DrawTeleop(layout *teleop_ui, UIContext *context, DashboardState *dashstate)
{
   element top_banner = Rectangle(teleop_ui, V4(1.0f, 0.0f, 0.0f, 1.0f),
                                  V2(GetSize(teleop_ui->bounds).x - 10, 60), V2(0, 0), V2(5, 5));
   
   r32 width = (GetSize(teleop_ui->bounds).x - 15) / 2;
   layout driver_panel = Panel(teleop_ui, V2(width, (9.0f / 16.0f) * width), V2(0, 0), V2(5, 5)).lout;
   Rectangle(context->render_context, driver_panel.bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   Text(context->render_context,
        V2((GetSize(driver_panel.bounds).x - GetTextWidth(context->render_context, Literal("Driver"), 40)) / 2.0f,
           (GetSize(driver_panel.bounds).y - 40) / 2.0f) + driver_panel.bounds.min,
        Literal("Driver"), 40);
   DrawDisplayConfig(&driver_panel, &dashstate->driver_display);
   
   layout op_panel = Panel(teleop_ui, V2(width, (9.0f / 16.0f) * width), V2(0, 0), V2(5, 5)).lout;
   Rectangle(context->render_context, op_panel.bounds, V4(1.0f, 0.0f, 0.0f, 1.0f));
   Text(context->render_context,
        V2((GetSize(op_panel.bounds).x - GetTextWidth(context->render_context, Literal("Operator"), 40)) / 2.0f,
           (GetSize(op_panel.bounds).y - 40) / 2.0f) + op_panel.bounds.min,
        Literal("Operator"), 40);
   DrawDisplayConfig(&op_panel, &dashstate->op_display);
}