void DrawHardwareList(layout *hardware_list, UIContext *context, Robot *robot)
{
   Rectangle(context->render_context, hardware_list->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, hardware_list->bounds, V4(0, 0, 0, 1));
   
   Text(hardware_list, Literal("Hardware"), 40,
        V2((GetSize(hardware_list->bounds).x - GetTextWidth(context->render_context, Literal("Hardware"), 40)) / 2.0f, 0), V2(0, 5)); 
   
   ui_id list_id = GEN_UI_ID;
   interaction_state hardware_list_interact =
      ClickInteraction(context, Interaction(list_id, hardware_list), context->input_state.left_up,
                       context->input_state.left_down, Contains(hardware_list->bounds, context->input_state.pos));
   
   if(hardware_list_interact.became_selected)
   {
      robot->selected_hardware = NULL;
   }
   
   v2 button_size = V2(GetSize(hardware_list->bounds).x * 0.8, 40);
   v2 button_margin = V2(GetSize(hardware_list->bounds).x * 0.1, 10);
    
   for(u32 i = 0;
       i < robot->hardware_count;
       i++)
   {
      RobotHardware *hardware = robot->hardware + i;
      button hardware_button = _Button(POINTER_UI_ID(hardware), hardware_list, NULL,
                                       hardware->name, robot->selected_hardware == hardware,
                                       button_size, V2(0, 0), button_margin);
      
      if(hardware_button.state)
      {
         robot->selected_hardware = hardware;
      }
   }
}

void DrawSelectedHardwarePage(layout *selected_hardware_page, UIContext *context, Robot *robot)
{
   Rectangle(context->render_context, selected_hardware_page->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, selected_hardware_page->bounds, V4(0, 0, 0, 1));
   
   RobotHardware *selected_hardware = robot->selected_hardware;
   Text(selected_hardware_page, selected_hardware->name, 40,
        V2((GetSize(selected_hardware_page->bounds).x - GetTextWidth(context->render_context, Literal("Hardware"), 40)) / 2.0f, 0), V2(0, 5)); 
   
   layout time_graph = Panel(selected_hardware_page, V2(GetSize(selected_hardware_page->bounds).x - 10, GetSize(selected_hardware_page->bounds).y * 0.45), V2(0, 0), V2(5, 5)).lout;
   NextLine(selected_hardware_page);
   
   Rectangle(context->render_context, time_graph.bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, time_graph.bounds, V4(0, 0, 0, 1));
   
   if(selected_hardware->type == Hardware_Motor)
   {
      Text(selected_hardware_page, Literal("State: "), 20, V2(0, 0), V2(0, 5)); 
   }
   else if(selected_hardware->type == Hardware_Solenoid)
   {
      Text(selected_hardware_page, Literal("State: "), 20, V2(0, 0), V2(0, 5)); 
   }
   else if(selected_hardware->type == Hardware_Drive)
   {
      Text(selected_hardware_page, Literal("Forward: "), 20, V2(0, 0), V2(0, 5)); 
      NextLine(selected_hardware_page);
      Text(selected_hardware_page, Literal("Rotate: "), 20, V2(0, 0), V2(0, 5)); 
   }
}

void DrawRobot(layout *robot_ui, UIContext *context, DashboardState *dashstate)
{
   v2 robot_ui_size = GetSize(robot_ui->bounds);
   layout hardware_list = Panel(robot_ui, V2(robot_ui_size.x * 0.2, robot_ui_size.y) - V2(10, 10), V2(0, 0), V2(5, 5)).lout;
   layout selected_hardware_page = Panel(robot_ui, V2(robot_ui_size.x * 0.8, robot_ui_size.y) - V2(5, 10), V2(0, 0), V2(5, 5)).lout;
   
   DrawHardwareList(&hardware_list, context, &dashstate->robot);
   
   if(dashstate->robot.selected_hardware)
   {
      DrawSelectedHardwarePage(&selected_hardware_page, context, &dashstate->robot);
   }
}