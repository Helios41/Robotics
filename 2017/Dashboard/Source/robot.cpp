void DrawRobot(layout *robot_ui, UIContext *context, DashboardState *dashstate)
{
   v2 robot_ui_size = GetSize(robot_ui->bounds);
   layout hardware_list = Panel(robot_ui, V2(robot_ui_size.x * 0.2, robot_ui_size.y) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   
   Rectangle(context->render_context, hardware_list.bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, hardware_list.bounds, V4(0, 0, 0, 1));
   
   v2 button_size = V2(GetSize(hardware_list.bounds).x * 0.8, 40);
   v2 button_margin = V2(GetSize(hardware_list.bounds).x * 0.1, 10);
    
   for(u32 i = 0;
       i < 4;
       i++)
   {
      element hardware_element = Rectangle(&hardware_list, V4(0.5f, 0.0f, 0.0f, 1.0f), button_size, V2(0, 0), button_margin);
      TextWrapRect(context->render_context, hardware_element.bounds, Literal("Hardware Test"));
   }
}