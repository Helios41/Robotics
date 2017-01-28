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

void DrawSelectedHardwarePage(layout *selected_hardware_page, UIContext *context,
                              Robot *robot, MemoryArena *generic_arena)
{
   Rectangle(context->render_context, selected_hardware_page->bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, selected_hardware_page->bounds, V4(0, 0, 0, 1));
   
   RobotHardware *selected_hardware = robot->selected_hardware;
   Text(selected_hardware_page, selected_hardware->name, 40,
        V2((GetSize(selected_hardware_page->bounds).x - GetTextWidth(context->render_context, selected_hardware->name, 40)) / 2.0f, 0), V2(0, 5)); 
   
   layout time_graph = Panel(selected_hardware_page, V2(GetSize(selected_hardware_page->bounds).x - 10, GetSize(selected_hardware_page->bounds).y * 0.45), V2(0, 0), V2(5, 5)).lout;
   NextLine(selected_hardware_page);
   
   u64 earliest_timestamp = MAX_U64;
   u64 latest_timestamp = MIN_U64;
      
   for(u32 i = 0;
       i < ArrayCount(selected_hardware->samples);
       i++)
   {
      RobotHardwareSample *curr = selected_hardware->samples + i;
      earliest_timestamp = Min(earliest_timestamp, curr->timestamp);
      latest_timestamp = Max(latest_timestamp, curr->timestamp);
   }
   
   RobotHardwareState *latest_sample = NULL;
   
   for(u32 i = 0;
       i < ArrayCount(selected_hardware->samples);
       i++)
   {
      RobotHardwareSample *curr = selected_hardware->samples + i;
      if(curr->timestamp == latest_timestamp)
      {
         latest_sample = &curr->state;
         break;
      }
   }
   
   Rectangle(context->render_context, time_graph.bounds, V4(0.3, 0.3, 0.3, 0.6));
   RectangleOutline(context->render_context, time_graph.bounds, V4(0, 0, 0, 1));
   
   if((latest_timestamp - earliest_timestamp) != 0)
   {
      r32 graph_height = GetSize(time_graph.bounds).y;
      r32 x_axis_interval = GetSize(time_graph.bounds).x / (latest_timestamp - earliest_timestamp);
      r32 y_axis_scale = GetSize(time_graph.bounds).y / 2.0f;
   
      for(u32 i = 0;
          i < (ArrayCount(selected_hardware->samples) - 1);
          i++)
      {
         //TODO: finish this once we have the server so we can test
#if 0
         if(selected_hardware->type == Hardware_Motor)
         {
            v2 a = V2(i * x_axis_interval, graph_height - (selected_hardware->samples[i].state.motor + 1) * y_axis_scale);
            v2 b = V2((i + 1) * x_axis_interval, graph_height - (selected_hardware->samples[i + 1].state.motor + 1) * y_axis_scale);
            Line(context->render_context, time_graph.bounds.min + a, time_graph.bounds.min + b, V4(0, 0, 0, 1), 2);
         }
         else if(selected_hardware->type == Hardware_Solenoid)
         {
            
         }
         else if(selected_hardware->type == Hardware_Drive)
         {
            v2 a_rotate = V2(i * x_axis_interval,
                             graph_height - (selected_hardware->samples[i].state.rotate + 1) * y_axis_scale);
            v2 b_rotate = V2((i + 1) * x_axis_interval,
                             graph_height - (selected_hardware->samples[i + 1].state.rotate + 1) * y_axis_scale);
            Line(context->render_context, time_graph.bounds.min + a_rotate, time_graph.bounds.min + b_rotate, V4(0, 0, 0, 1), 2);
            
            v2 a_forward = V2(i * x_axis_interval,
                              graph_height - (selected_hardware->samples[i].state.forward + 1) * y_axis_scale);
            v2 b_forward = V2((i + 1) * x_axis_interval,
                              graph_height - (selected_hardware->samples[i + 1].state.forward + 1) * y_axis_scale);
            Line(context->render_context, time_graph.bounds.min + a_forward, time_graph.bounds.min + b_forward, V4(1, 1, 1, 1), 2);
         }
#endif
      }
      
      if(selected_hardware->type == Hardware_Motor)
      {
         TemporaryMemoryArena temp_memory = BeginTemporaryMemory(generic_arena);
         
         Text(selected_hardware_page, 
              Concat(Literal("State: "), ToString(latest_sample->motor, &temp_memory), &temp_memory),
              20, V2(0, 0), V2(0, 5));  
              
         EndTemporaryMemory(temp_memory);
      }
      else if(selected_hardware->type == Hardware_Solenoid)
      {
         string solenoid_state_text = latest_sample->solenoid ? Literal("State: Extended") : Literal("State: Retracted");
         Text(selected_hardware_page, solenoid_state_text, 20, V2(0, 0), V2(0, 5)); 
      }
      else if(selected_hardware->type == Hardware_Drive)
      {
         TemporaryMemoryArena temp_memory = BeginTemporaryMemory(generic_arena);
         
         Text(selected_hardware_page,
              Concat(Literal("Forward: "), ToString(latest_sample->forward, &temp_memory), &temp_memory),
              20, V2(0, 0), V2(0, 5)); 
         NextLine(selected_hardware_page);
         Text(selected_hardware_page,
              Concat(Literal("Rotate: "), ToString(latest_sample->rotate, &temp_memory), &temp_memory),
              20, V2(0, 0), V2(0, 5)); 
         
         EndTemporaryMemory(temp_memory);
      }
   }
   else
   {
      Text(&time_graph, Literal("No Data"), 40,
           V2((GetSize(time_graph.bounds).x - GetTextWidth(context->render_context, Literal("No Data"), 40)) / 2.0f,
              (GetSize(time_graph.bounds).y - 40) / 2.0f), V2(0, 0)); 
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
      DrawSelectedHardwarePage(&selected_hardware_page, context, &dashstate->robot, dashstate->generic_arena);
   }
   else
   {
      //TODO: draw a default robot info page (eg. name, game name, etc...)
      //DrawRobotPage();
   }
}